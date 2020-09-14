//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "sceneDelegate.h"

#include <vector>
#include <boost/functional/hash.hpp>

#include <maya/MDrawContext.h>

#include <pxr/pxr.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/cameraUtil/conformWindow.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/glf/simpleLightingContext.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdSt/light.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/selectionTask.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/imaging/hdx/shadowTask.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/usd/sdf/path.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/shapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (selectionTask)
);


namespace {
    class PxrMayaHdShadowMatrix : public HdxShadowMatrixComputation
    {
        public:
#if HDX_API_VERSION >= 6
            PxrMayaHdShadowMatrix(const GlfSimpleLight& light)
            {
                // We use the shadow matrix as provided by Maya directly.
                _shadowMatrices = light.GetShadowMatrices();
            }

            std::vector<GfMatrix4d> Compute(
                    const GfVec4f& viewport,
                    CameraUtilConformWindowPolicy policy) override
            {
                return _shadowMatrices;
            }
#else
            PxrMayaHdShadowMatrix(const GlfSimpleLight& light)
            {
                // We use the shadow matrix as provided by Maya directly.
                _shadowMatrices.push_back(light.GetShadowMatrix());
            }

            GfMatrix4d Compute(
                    const GfVec4f& viewport,
                    CameraUtilConformWindowPolicy policy) override
            {
                return _shadowMatrices.back();
            }
#endif

        private:
            std::vector<GfMatrix4d> _shadowMatrices;
    };
}


PxrMayaHdSceneDelegate::PxrMayaHdSceneDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID)
{
    _lightingContext = GlfSimpleLightingContext::New();

    // populate tasks in renderindex

    // create an unique namespace
    _rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_UsdImaging_%p", this)));

    _simpleLightTaskId = _rootId.AppendChild(HdxPrimitiveTokens->simpleLightTask);
    _shadowTaskId = _rootId.AppendChild(HdxPrimitiveTokens->shadowTask);
    _pickingTaskId = _rootId.AppendChild(HdxPrimitiveTokens->pickTask);
    _selectionTaskId = _rootId.AppendChild(_tokens->selectionTask);
    _cameraId = _rootId.AppendChild(HdPrimTypeTokens->camera);

    // camera
    {
        // Since the batch renderer is hardcoded to use HdStRenderDelegate, we
        // expect to have camera Sprims.
        TF_VERIFY(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera));

        renderIndex->InsertSprim(HdPrimTypeTokens->camera, this, _cameraId);
        _ValueCache& cache = _valueCacheMap[_cameraId];
        cache[HdCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdCameraTokens->windowPolicy] = VtValue(CameraUtilFit);
    }

    // Simple lighting task.
    {
        renderIndex->InsertTask<HdxSimpleLightTask>(this, _simpleLightTaskId);
        _ValueCache& cache = _valueCacheMap[_simpleLightTaskId];
        HdxSimpleLightTaskParams taskParams;
        taskParams.cameraPath = _cameraId;
        taskParams.viewport = GfVec4f(_viewport);
        taskParams.enableShadows = _lightingContext->GetUseShadows();
        cache[HdTokens->params] = VtValue(taskParams);
    }

    // Shadow task.
    {
        // By default we only want geometry in the shadow pass
        const TfTokenVector defaultShadowRenderTags = {
           HdRenderTagTokens->geometry,
        };

        renderIndex->InsertTask<HdxShadowTask>(this, _shadowTaskId);
        _ValueCache& cache = _valueCacheMap[_shadowTaskId];
        HdxShadowTaskParams taskParams;
        taskParams.camera = _cameraId;
        taskParams.viewport = _viewport;
        cache[HdTokens->params] = VtValue(taskParams);
        cache[HdTokens->renderTags] = VtValue(defaultShadowRenderTags);
    }

    // Picking task.
    {
        renderIndex->InsertTask<HdxPickTask>(this, _pickingTaskId);
        _ValueCache& cache = _valueCacheMap[_pickingTaskId];
        HdxPickTaskParams taskParams;
        taskParams.enableSceneMaterials = true;
        cache[HdTokens->params] = VtValue(taskParams);

        // Initialize empty render tags, they will be set
        // on first use, but this ensures we don't need
        // to special case first time vs others for comparing
        // to current render tags
        cache[HdTokens->renderTags] = VtValue(TfTokenVector());
    }

    // Selection task.
    {
        renderIndex->InsertTask<HdxSelectionTask>(this, _selectionTaskId);
        _ValueCache& cache = _valueCacheMap[_selectionTaskId];
        HdxSelectionTaskParams taskParams;
        taskParams.enableSelection = true;

        // Note that the selection color is a constant zero value. This is to
        // mimic selection behavior in Maya where the wireframe color is what
        // changes to indicate selection and not the object color.
        taskParams.selectionColor = GfVec4f(0.0f);

        cache[HdTokens->params] = VtValue(taskParams);
        cache[HdTokens->collection] = VtValue();
    }
}

/*virtual*/
VtValue
PxrMayaHdSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    _ValueCache* vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache && TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }

    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
                    id.GetText(),
                    key.GetText());

    return VtValue();
}

/*virtual*/
VtValue
PxrMayaHdSceneDelegate::GetCameraParamValue(
        SdfPath const& cameraId,
        TfToken const& paramName)
{
    return Get(cameraId, paramName);
}

TfTokenVector
PxrMayaHdSceneDelegate::GetTaskRenderTags(SdfPath const& taskId)
{
    VtValue value = Get(taskId, HdTokens->renderTags);

    return value.Get<TfTokenVector>();
}

void
PxrMayaHdSceneDelegate::SetCameraState(
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix,
        const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache& cache = _valueCacheMap[_cameraId];
    cache[HdCameraTokens->worldToViewMatrix] = VtValue(worldToViewMatrix);
    cache[HdCameraTokens->projectionMatrix] = VtValue(projectionMatrix);
    cache[HdCameraTokens->windowPolicy] = VtValue(CameraUtilFit);
    cache[HdCameraTokens->clipPlanes] = VtValue(std::vector<GfVec4d>());

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkSprimDirty(_cameraId,
                                                       HdCamera::AllDirty);

    if (_viewport != viewport) {
        _viewport = viewport;

        // Update the simple light task.
        HdxSimpleLightTaskParams simpleLightTaskParams =
            _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                                HdTokens->params);

        simpleLightTaskParams.viewport = GfVec4f(_viewport);
        _SetValue(_simpleLightTaskId, HdTokens->params, simpleLightTaskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId,
            HdChangeTracker::DirtyParams);

        // Update the shadow task.
        HdxShadowTaskParams shadowTaskParams =
            _GetValue<HdxShadowTaskParams>(_shadowTaskId,
                                           HdTokens->params);

        shadowTaskParams.viewport = _viewport;
        _SetValue(_shadowTaskId, HdTokens->params, shadowTaskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _shadowTaskId,
            HdChangeTracker::DirtyParams);

        // Update all render setup tasks.
        for (const auto& it : _renderSetupTaskIdMap) {
            const SdfPath& renderSetupTaskId = it.second;

            HdxRenderTaskParams renderSetupTaskParams =
                _GetValue<HdxRenderTaskParams>(renderSetupTaskId,
                                               HdTokens->params);

            renderSetupTaskParams.viewport = _viewport;
            _SetValue(renderSetupTaskId,
                      HdTokens->params,
                      renderSetupTaskParams);

            GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                renderSetupTaskId,
                HdChangeTracker::DirtyParams);
        }
    }
}

void
PxrMayaHdSceneDelegate::SetLightingStateFromVP1(
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix)
{
    // This function should only be called in a VP1.0 context. In VP2.0, we can
    // translate the lighting state from the MDrawContext directly into Glf,
    // but there is no draw context in VP1.0, so we have to transfer the state
    // through OpenGL.

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(worldToViewMatrix.GetArray());
    _lightingContext->SetStateFromOpenGL();
    glPopMatrix();

    _lightingContext->SetCamera(worldToViewMatrix, projectionMatrix);

    _SetLightingStateFromLightingContext();
}

void
PxrMayaHdSceneDelegate::SetLightingStateFromMayaDrawContext(
        const MHWRender::MDrawContext& context)
{
    _lightingContext = px_vp20Utils::GetLightingContextFromDrawContext(context);

    _SetLightingStateFromLightingContext();
}

void
PxrMayaHdSceneDelegate::_SetLightingStateFromLightingContext()
{
    const GlfSimpleLightVector& lights = _lightingContext->GetLights();

    bool hasLightingChanged = false;

    // Insert light Ids into the render index for those that do not yet exist.
    while (_lightIds.size() < lights.size()) {
        const SdfPath lightId = _rootId.AppendChild(TfToken(
            TfStringPrintf("light%zu", _lightIds.size())));
        _lightIds.push_back(lightId);

        // Since we're hardcoded to use HdStRenderDelegate, we expect to have
        // light Sprims.
        TF_VERIFY(GetRenderIndex().
            IsSprimTypeSupported(HdPrimTypeTokens->simpleLight));

        GetRenderIndex().
            InsertSprim(HdPrimTypeTokens->simpleLight, this, lightId);
        hasLightingChanged = true;
    }

    // Remove unused light Ids from HdRenderIndex
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex().
            RemoveSprim(HdPrimTypeTokens->simpleLight, _lightIds.back());
        _lightIds.pop_back();
        hasLightingChanged = true;
    }

    // Check for any changes in lighting.
    for (size_t i = 0; i < lights.size(); ++i) {
        _ValueCache& cache = _valueCacheMap[_lightIds[i]];

        const VtValue& currLightValue = cache[HdLightTokens->params];
        if (currLightValue.IsHolding<GlfSimpleLight>()) {
            const GlfSimpleLight& currLight =
                currLightValue.Get<GlfSimpleLight>();

            if (lights[i] == currLight) {
                // This light hasn't changed since the last time it was stored
                // in the cache, so skip it.
                continue;
            }
        }

        hasLightingChanged = true;

        // Store GlfSimpleLight directly.
        cache[HdLightTokens->params] = VtValue(lights[i]);
        cache[HdTokens->transform] = VtValue();

        HdxShadowParams shadowParams;
        shadowParams.enabled = lights[i].HasShadow();
        shadowParams.resolution = lights[i].GetShadowResolution();
        // XXX: The Hydra lighting shader currently adds the bias value to the
        // depth of the position being tested for shadowing whereas the Maya
        // behavior appears to be that it is subtracted. To handle this for now,
        // we simply negate the bias value from Maya before passing it to Hydra.
        shadowParams.bias = -1.0f * lights[i].GetShadowBias();
        shadowParams.blur = lights[i].GetShadowBlur();

        if (lights[i].HasShadow()) {
            shadowParams.shadowMatrix =
                HdxShadowMatrixComputationSharedPtr(
                    new PxrMayaHdShadowMatrix(lights[i]));
        }

        cache[HdLightTokens->shadowParams] = VtValue(shadowParams);
        cache[HdLightTokens->shadowCollection] =
            VtValue(HdRprimCollection(
                HdTokens->geometry,
                HdReprSelector(HdReprTokens->refined)));

        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightIds[i],
            HdStLight::AllDirty);
    }

    HdxSimpleLightTaskParams taskParams =
        _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                            HdTokens->params);

    if (taskParams.enableShadows != _lightingContext->GetUseShadows()) {
        taskParams.enableShadows = _lightingContext->GetUseShadows();
        hasLightingChanged = true;
    }

    // Sadly the material also comes from the lighting context right now...
    bool hasSceneAmbientChanged = false;
    if (taskParams.sceneAmbient != _lightingContext->GetSceneAmbient()) {
        taskParams.sceneAmbient = _lightingContext->GetSceneAmbient();
        hasSceneAmbientChanged = true;
    }
    bool hasMaterialChanged = false;
    if (taskParams.material != _lightingContext->GetMaterial()) {
        taskParams.material = _lightingContext->GetMaterial();
        hasMaterialChanged = true;
    }

    if (hasLightingChanged || hasSceneAmbientChanged || hasMaterialChanged) {
        _SetValue(_simpleLightTaskId, HdTokens->params, taskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId,
            HdChangeTracker::DirtyParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _shadowTaskId,
            HdChangeTracker::DirtyParams);
    }
}

HdTaskSharedPtrVector
PxrMayaHdSceneDelegate::GetSetupTasks()
{
    HdTaskSharedPtrVector tasks;

    tasks.push_back(GetRenderIndex().GetTask(_simpleLightTaskId));
    tasks.push_back(GetRenderIndex().GetTask(_shadowTaskId));

    return tasks;
}

HdTaskSharedPtrVector
PxrMayaHdSceneDelegate::GetRenderTasks(
        const size_t hash,
        const PxrMayaHdRenderParams& renderParams,
        unsigned int displayStyle,
        const PxrMayaHdPrimFilterVector& primFilters)
{
    HdTaskSharedPtrVector taskList;
    HdRenderIndex& renderIndex = GetRenderIndex();

    // Task List Consist of:
    //  Render Setup Task
    //  Render Task Per Shape Adapter/Collection
    //  Selection Task
    taskList.reserve(2 + primFilters.size());

    SdfPath renderSetupTaskId;
    if (!TfMapLookup(_renderSetupTaskIdMap, hash, &renderSetupTaskId)) {
        // Create a new render setup task if one does not exist for this hash.
        renderSetupTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   HdxPrimitiveTokens->renderSetupTask.GetText(),
                                   hash)));

        renderIndex.InsertTask<HdxRenderSetupTask>(this, renderSetupTaskId);

        HdxRenderTaskParams renderSetupTaskParams;
        renderSetupTaskParams.camera = _cameraId;
        // Initialize viewport to the latest value since render setup tasks can
        // be lazily instantiated, potentially even after SetCameraState().
        renderSetupTaskParams.viewport = _viewport;

        // Set the parameters that are constant for all draws.
        renderSetupTaskParams.enableIdRender = false;
        renderSetupTaskParams.alphaThreshold = 0.1f;
        renderSetupTaskParams.enableSceneMaterials = true;
        renderSetupTaskParams.depthBiasUseDefault = true;
        renderSetupTaskParams.depthFunc = HdCmpFuncLess;

        _ValueCache& cache = _valueCacheMap[renderSetupTaskId];
        cache[HdTokens->params] = VtValue(renderSetupTaskParams);
        cache[HdTokens->collection] = VtValue();

        _renderSetupTaskIdMap[hash] = renderSetupTaskId;
    }
    taskList.emplace_back(renderIndex.GetTask(renderSetupTaskId));

    for (const PxrMayaHdPrimFilter& primFilter : primFilters) {
        SdfPath renderTaskId;
        HdRprimCollection rprimCollection;
        TfTokenVector renderTags;

        if (primFilter.shapeAdapter != nullptr) {
            const HdReprSelector repr =
                primFilter.shapeAdapter->GetReprSelectorForDisplayStyle(
                    displayStyle);

            if (!repr.AnyActiveRepr()) {
                continue;
            }

            renderTaskId = primFilter.shapeAdapter->GetRenderTaskId(repr);
            rprimCollection = primFilter.shapeAdapter->GetRprimCollection(repr);
            renderTags = primFilter.shapeAdapter->GetRenderTags();
        } else {
            rprimCollection = primFilter.collection;
            renderTags = primFilter.renderTags;

            // The batch renderer manages the render task ID for this
            // collection, so look up its ID by name.
            const TfToken& collectionName = rprimCollection.GetName();

            if (!TfMapLookup(_renderTaskIdMap, collectionName, &renderTaskId)) {
                // Create a new render task ID if one does not exist for this
                // collection.
                // Note that we expect the collection name to have already been
                // sanitized for use in SdfPaths.
                TF_VERIFY(TfIsValidIdentifier(collectionName.GetString()));
                renderTaskId = _rootId.AppendChild(
                    TfToken(
                        TfStringPrintf(
                            "%s_%s",
                            HdxPrimitiveTokens->renderTask.GetText(),
                            collectionName.GetText())));

                _renderTaskIdMap[collectionName] = renderTaskId;
            }
        }

        HdTaskSharedPtr renderTask = renderIndex.GetTask(renderTaskId);
        if (!renderTask) {
            renderIndex.InsertTask<HdxRenderTask>(this, renderTaskId);
            renderTask = renderIndex.GetTask(renderTaskId);

            _ValueCache& cache = _valueCacheMap[renderTaskId];

            // Note that the render task has no params of its own. All of the
            // render params are on the render setup task instead.
            cache[HdTokens->params] = VtValue();

            // Once the task is created, the batch renderer itself will not
            // change the task's collection.
            cache[HdTokens->collection] = VtValue(rprimCollection);

            cache[HdTokens->renderTags] = VtValue(renderTags);
        } else {
            // Update task's render tags
            const TfTokenVector& currentRenderTags =
                _GetValue<TfTokenVector>(renderTaskId, HdTokens->renderTags);

            if (currentRenderTags != renderTags) {
                _SetValue(renderTaskId, HdTokens->renderTags, renderTags);
                renderIndex.GetChangeTracker().MarkTaskDirty(
                    renderTaskId,
                    HdChangeTracker::DirtyRenderTags);
            }
        }

        if (renderTask) {
            taskList.emplace_back(renderTask);
        }
    }

    taskList.emplace_back(renderIndex.GetTask(_selectionTaskId));

    //
    // (meta-XXX): The notes below are actively being addressed with an
    // HdRprimCollection now being created and managed by the shape adapter of
    // each shape being drawn. I'm leaving the full notes intact while work
    // continues, as they've been immensely helpful in optimizing our usage of
    // the Hydra API.
    //
    // ------------------------------------------------------------------------
    //
    // XXX: The Maya-Hydra plugin needs refactoring such that the plugin is
    // creating a different collection name for each collection it is trying to
    // manage. (i.e. Each collection within a frame that has different content
    // should have a different collection name)
    //
    // With Hydra, changing the contents of a collection can be
    // an expensive operation as it causes draw batches to be rebuilt.
    //
    // The plugin should also track deltas to the contents of a collection
    // and set Hydra's dirty state when prims get added and removed from
    // the collection.
    //
    // However, a further improvement to the code could be made using
    // UsdDelegate's fallback repr feature instead of using multiple
    // collections as it would avoid modifying the collection as a Maya shape
    // object display state changes.  This would result in a much cheaper state
    // transition within Hydra itself.
    //

    // Get the render setup task params from the value cache.
    HdxRenderTaskParams renderSetupTaskParams =
        _GetValue<HdxRenderTaskParams>(renderSetupTaskId, HdTokens->params);

    if (renderSetupTaskParams.enableLighting != renderParams.enableLighting ||
        renderSetupTaskParams.wireframeColor != renderParams.wireframeColor) {
        // Update the render setup task params.
        renderSetupTaskParams.enableLighting = renderParams.enableLighting;
        renderSetupTaskParams.wireframeColor = renderParams.wireframeColor;

        // Store the updated render setup task params back in the cache and
        // mark them dirty.
        _SetValue(renderSetupTaskId, HdTokens->params, renderSetupTaskParams);
        renderIndex.GetChangeTracker().MarkTaskDirty(
            renderSetupTaskId,
            HdChangeTracker::DirtyParams);
    }

    return taskList;
}

HdTaskSharedPtrVector
PxrMayaHdSceneDelegate::GetPickingTasks(
        const TfTokenVector& renderTags)
{
    // Update tasks render tags to match those specified in the parameter.
    const TfTokenVector &currentRenderTags =
        _GetValue<TfTokenVector>(_pickingTaskId, HdTokens->renderTags);

    if (currentRenderTags != renderTags) {
        _SetValue(_pickingTaskId, HdTokens->renderTags, renderTags);
        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _pickingTaskId,
            HdChangeTracker::DirtyRenderTags);
    }

    HdTaskSharedPtrVector tasks;

    tasks.push_back(GetRenderIndex().GetTask(_pickingTaskId));

    return tasks;
}


PXR_NAMESPACE_CLOSE_SCOPE
