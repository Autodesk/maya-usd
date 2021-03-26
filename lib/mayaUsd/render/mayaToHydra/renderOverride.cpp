//
// Copyright 2019 Luma Pictures
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
#include "renderOverride.h"

#include "pluginDebugCodes.h"
#include "renderOverrideUtils.h"
#include "tokens.h"
#include "utils.h"

#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/delegates/sceneDelegate.h>
#include <hdMaya/utils.h>
#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/utils/hash.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hdx/colorizeSelectionTask.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgi/tokens.h>
#include <pxr/pxr.h>

#include <maya/M3dView.h>
#include <maya/MConditionMessage.h>
#include <maya/MDrawContext.h>
#include <maya/MEventMessage.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MTimerMessage.h>
#include <maya/MUiMessage.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <limits>

#if WANT_UFE_BUILD
#include <mayaUsd/ufe/Global.h>

#include <maya/MFileIO.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif
#include <ufe/selectionNotification.h>
#endif // WANT_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Not sure if we actually need a mutex guarding _allInstances, but
// everywhere that uses it isn't a "frequent" operation, so the
// extra speed loss should be fine, and I'd rather be safe.
std::mutex                       _allInstancesMutex;
std::vector<MtohRenderOverride*> _allInstances;

#if WANT_UFE_BUILD

// Observe UFE scene items for transformation changed only when they are
// selected.
class UfeSelectionObserver : public UFE_NS::Observer
{
public:
    UfeSelectionObserver(MtohRenderOverride& mtohRenderOverride)
        : UFE_NS::Observer()
        , _mtohRenderOverride(mtohRenderOverride)
    {
    }

    //~UfeSelectionObserver() override {}

    void operator()(const UFE_NS::Notification& notification) override
    {
        // During Maya file read, each node will be selected in turn, so we get
        // notified for each node in the scene.  Prune this out.
        if (MFileIO::isOpeningFile()) {
            return;
        }

        auto selectionChanged = dynamic_cast<const UFE_NS::SelectionChanged*>(&notification);
        if (selectionChanged == nullptr) {
            return;
        }

        TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
            .Msg("UfeSelectionObserver triggered (ufe selection change "
                 "triggered)\n");
        _mtohRenderOverride.SelectionChanged();
    }

private:
    MtohRenderOverride& _mtohRenderOverride;
};

#endif // WANT_UFE_BUILD

#if MAYA_API_VERSION >= 20210000

//! \brief  Get the index of the hit nearest to a given cursor point.
int GetNearestHitIndex(
    const MHWRender::MFrameContext& frameContext,
    const HdxPickHitVector&         hits,
    int                             cursor_x,
    int                             cursor_y)
{
    int nearestHitIndex = -1;

    double dist2_min = std::numeric_limits<double>::max();
    float  depth_min = std::numeric_limits<float>::max();

    for (unsigned int i = 0; i < hits.size(); i++) {
        const HdxPickHit& hit = hits[i];
        const MPoint      worldSpaceHitPoint(
            hit.worldSpaceHitPoint[0], hit.worldSpaceHitPoint[1], hit.worldSpaceHitPoint[2]);

        // Calculate the (x, y) coordinate relative to the lower left corner of the viewport.
        double hit_x, hit_y;
        frameContext.worldToViewport(worldSpaceHitPoint, hit_x, hit_y);

        // Calculate the 2D distance between the hit and the cursor
        double dist_x = hit_x - (double)cursor_x;
        double dist_y = hit_y - (double)cursor_y;
        double dist2 = dist_x * dist_x + dist_y * dist_y;

        // Find the hit nearest to the cursor.
        if ((dist2 < dist2_min) || (dist2 == dist2_min && hit.normalizedDepth < depth_min)) {
            dist2_min = dist2;
            depth_min = hit.normalizedDepth;
            nearestHitIndex = (int)i;
        }
    }

    return nearestHitIndex;
}

//! \brief  workaround to remove duplicate hits and improve selection performance.
void ResolveUniqueHits_Workaround(const HdxPickHitVector& inHits, HdxPickHitVector& outHits)
{
    outHits.clear();

    // hash -> hitIndex
    std::unordered_map<size_t, size_t> hitIndices;

    size_t previousHash = 0;

    for (size_t i = 0; i < inHits.size(); i++) {
        const HdxPickHit& hit = inHits[i];

        size_t hash = 0;
        MayaUsd::hash_combine(hash, hit.delegateId.GetHash());
        MayaUsd::hash_combine(hash, hit.objectId.GetHash());
        MayaUsd::hash_combine(hash, hit.instancerId.GetHash());
        MayaUsd::hash_combine(hash, hit.instanceIndex);

        // As an optimization, keep track of the previous hash value and
        // reject indices that match it without performing a map lookup.
        // Adjacent indices are likely enough to have the same prim,
        // instance and element ids that this can be a significant
        // improvement.
        if (hitIndices.empty() || hash != previousHash) {
            if (hitIndices.insert(std::make_pair(hash, i)).second) {
                outHits.push_back(inHits[i]);
            }
            previousHash = hash;
        }
    }
}

#endif

} // namespace

MtohRenderOverride::MtohRenderOverride(const MtohRendererDescription& desc)
    : MHWRender::MRenderOverride(desc.overrideName.GetText())
    , _rendererDesc(desc)
    , _globals(MtohRenderGlobals::GetInstance())
#if PXR_VERSION > 2005
    , _hgi(Hgi::CreatePlatformDefaultHgi())
#else
    , _hgi(Hgi::GetPlatformDefaultHgi())
#endif
    , _hgiDriver { HgiTokens->renderDriver, VtValue(_hgi.get()) }
    , _selectionTracker(new HdxSelectionTracker)
    , _isUsingHdSt(desc.rendererName == MtohTokens->HdStormRendererPlugin)
{
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride created (%s - %s - %s)\n",
            _rendererDesc.rendererName.GetText(),
            _rendererDesc.overrideName.GetText(),
            _rendererDesc.displayName.GetText());
    HdMayaDelegateRegistry::InstallDelegatesChangedSignal([this]() { _needsClear.store(true); });
    _ID = SdfPath("/HdMayaViewportRenderer")
              .AppendChild(
                  TfToken(TfStringPrintf("_HdMaya_%s_%p", desc.rendererName.GetText(), this)));

    MStatus status;
    auto    id
        = MSceneMessage::addCallback(MSceneMessage::kBeforeNew, _ClearHydraCallback, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, _ClearHydraCallback, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MEventMessage::addEventCallback(
        MString("SelectionChanged"), _SelectionChangedCallback, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }

    // Setup the playblast watch.
    // _playBlasting is forced to true here so we can just use _PlayblastingChanged below
    //
    _playBlasting = true;
    MConditionMessage::addConditionCallback(
        "playblasting", &MtohRenderOverride::_PlayblastingChanged, this, &status);
    MtohRenderOverride::_PlayblastingChanged(false, this);

    _defaultLight.SetSpecular(GfVec4f(0.0f));
    _defaultLight.SetAmbient(GfVec4f(0.0f));

    {
        std::lock_guard<std::mutex> lock(_allInstancesMutex);
        _allInstances.push_back(this);
    }

#if WANT_UFE_BUILD
    const UFE_NS::GlobalSelection::Ptr& ufeSelection = UFE_NS::GlobalSelection::get();
    if (ufeSelection) {
        _ufeSelectionObserver = std::make_shared<UfeSelectionObserver>(*this);
        ufeSelection->addObserver(_ufeSelectionObserver);
    }
#endif // WANT_UFE_BUILD
}

MtohRenderOverride::~MtohRenderOverride()
{
#if WANT_UFE_BUILD
    const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();
    if (ufeSelection) {
        ufeSelection->removeObserver(_ufeSelectionObserver);
        _ufeSelectionObserver = nullptr;
    }
#endif // WANT_UFE_BUILD

    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride destroyed (%s - %s - %s)\n",
            _rendererDesc.rendererName.GetText(),
            _rendererDesc.overrideName.GetText(),
            _rendererDesc.displayName.GetText());

    if (_timerCallback)
        MMessage::removeCallback(_timerCallback);

    ClearHydraResources();

    for (auto operation : _operations) {
        delete operation;
    }

    for (auto callback : _callbacks) {
        MMessage::removeCallback(callback);
    }
    for (auto& panelAndCallbacks : _renderPanelCallbacks) {
        MMessage::removeCallbacks(panelAndCallbacks.second);
    }

    if (!_allInstances.empty()) {
        std::lock_guard<std::mutex> lock(_allInstancesMutex);
        _allInstances.erase(
            std::remove(_allInstances.begin(), _allInstances.end(), this), _allInstances.end());
    }
}

HdRenderDelegate* MtohRenderOverride::_GetRenderDelegate()
{
    return _renderIndex ? _renderIndex->GetRenderDelegate() : nullptr;
}

void MtohRenderOverride::UpdateRenderGlobals(
    const MtohRenderGlobals& globals,
    const TfToken&           attrName)
{
    // If no attribute or attribute starts with 'mtoh', these setting wil be applied on the next
    // call to MtohRenderOverride::Render, so just force an invalidation
    // XXX: This will need to change if mtoh settings should ever make it to the delegate itself.
    if (attrName.GetString().find("mtoh") != 0) {
        std::lock_guard<std::mutex> lock(_allInstancesMutex);
        for (auto* instance : _allInstances) {
            const auto& rendererName = instance->_rendererDesc.rendererName;

            // If no attrName or the attrName is the renderer, then update everything
            const size_t attrFilter = (attrName.IsEmpty() || attrName == rendererName) ? 0 : 1;
            if (attrFilter && !instance->_globals.AffectsRenderer(attrName, rendererName)) {
                continue;
            }

            // Will be applied in _InitHydraResources later anyway
            if (auto* renderDelegate = instance->_GetRenderDelegate()) {
                instance->_globals.ApplySettings(
                    renderDelegate,
                    instance->_rendererDesc.rendererName,
                    TfTokenVector(attrFilter, attrName));
                if (attrFilter) {
                    break;
                }
            }
        }
    }

    // Less than ideal still
    MGlobal::executeCommandOnIdle("refresh -f");
}

std::vector<MString> MtohRenderOverride::AllActiveRendererNames()
{
    std::vector<MString> renderers;

    std::lock_guard<std::mutex> lock(_allInstancesMutex);
    for (auto* instance : _allInstances) {
        if (instance->_initializationSucceeded) {
            renderers.push_back(instance->_rendererDesc.rendererName.GetText());
        }
    }
    return renderers;
}

SdfPathVector MtohRenderOverride::RendererRprims(TfToken rendererName, bool visibleOnly)
{
    MtohRenderOverride* instance = _GetByName(rendererName);
    if (!instance) {
        return SdfPathVector();
    }

    auto* renderIndex = instance->_renderIndex;
    if (!renderIndex) {
        return SdfPathVector();
    }
    auto primIds = renderIndex->GetRprimIds();
    if (visibleOnly) {
        primIds.erase(
            std::remove_if(
                primIds.begin(),
                primIds.end(),
                [renderIndex](const SdfPath& primId) {
                    auto* rprim = renderIndex->GetRprim(primId);
                    if (!rprim)
                        return true;
                    return !rprim->IsVisible();
                }),
            primIds.end());
    }
    return primIds;
}

SdfPath MtohRenderOverride::RendererSceneDelegateId(TfToken rendererName, TfToken sceneDelegateName)
{
    MtohRenderOverride* instance = _GetByName(rendererName);
    if (!instance) {
        return SdfPath();
    }

    for (auto& delegate : instance->_delegates) {
        if (delegate->GetName() == sceneDelegateName) {
            return delegate->GetMayaDelegateID();
        }
    }
    return SdfPath();
}

void MtohRenderOverride::_DetectMayaDefaultLighting(const MHWRender::MDrawContext& drawContext)
{
    constexpr auto considerAllSceneLights = MHWRender::MDrawContext::kFilteredIgnoreLightLimit;

    const auto numLights = drawContext.numberOfActiveLights(considerAllSceneLights);
    auto       foundMayaDefaultLight = false;
    if (numLights == 1) {
        auto* lightParam = drawContext.getLightParameterInformation(0, considerAllSceneLights);
        if (lightParam != nullptr && !lightParam->lightPath().isValid()) {
            // This light does not exist so it must be the
            // default maya light
            MFloatPointArray positions;
            MFloatVector     direction;
            auto             intensity = 0.0f;
            MColor           color;
            auto             hasDirection = false;
            auto             hasPosition = false;

            // Maya default light has no position, only direction
            drawContext.getLightInformation(
                0,
                positions,
                direction,
                intensity,
                color,
                hasDirection,
                hasPosition,
                considerAllSceneLights);

            if (hasDirection && !hasPosition) {
                _defaultLight.SetPosition({ -direction.x, -direction.y, -direction.z, 0.0f });
                _defaultLight.SetDiffuse(
                    { intensity * color.r, intensity * color.g, intensity * color.b, 1.0f });
                foundMayaDefaultLight = true;
            }
        }
    }

    TF_DEBUG(HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING)
        .Msg(
            "MtohRenderOverride::"
            "_DetectMayaDefaultLighting() "
            "foundMayaDefaultLight=%i\n",
            foundMayaDefaultLight);

    if (foundMayaDefaultLight != _hasDefaultLighting) {
        _hasDefaultLighting = foundMayaDefaultLight;
        _needsClear.store(true);
        TF_DEBUG(HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING)
            .Msg(
                "MtohRenderOverride::"
                "_DetectMayaDefaultLighting() clearing! "
                "_hasDefaultLighting=%i\n",
                _hasDefaultLighting);
    }
}

MStatus MtohRenderOverride::Render(const MHWRender::MDrawContext& drawContext, const MHWRender::MViewportScene& scene)
{
    // It would be good to clear the resources of the overrides that are
    // not in active use, but I'm not sure if we have a better way than
    // the idle time we use currently. The approach below would break if
    // two render overrides were used at the same time.
    // for (auto* override: _allInstances) {
    //     if (override != this) {
    //         override->ClearHydraResources();
    //     }
    // }
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RENDER).Msg("MtohRenderOverride::Render()\n");
    auto renderFrame = [&](bool markTime = false) {
        HdTaskSharedPtrVector tasks = _taskController->GetRenderingTasks();

        // For playblasting, a glReadPixels is going to occur sometime after we return.
        // But if we call Execute on all of the tasks, then z-buffer fighting may occur
        // because every colorize/present task is going to be drawing a full-screen quad
        // with 'unconverged' depth.
        //
        // To work arround this (for not Storm) we pull the first task, (render/synch)
        // and continually execute it until the renderer signals converged, at which point
        // we break and call HdEngine::Execute once more to copy the aovs into OpenGL
        //
        if (_playBlasting && !_isUsingHdSt && !tasks.empty()) {
            // XXX: Is this better as user-configurable ?
            constexpr auto msWait = std::chrono::duration<float, std::milli>(100);
#if PXR_VERSION >= 2005
            std::shared_ptr<HdxRenderTask> renderTask
                = std::dynamic_pointer_cast<HdxRenderTask>(tasks.front());
#else
            boost::shared_ptr<HdxRenderTask> renderTask
                = boost::dynamic_pointer_cast<HdxRenderTask>(tasks.front());
#endif
            if (renderTask) {
                HdTaskSharedPtrVector renderOnly = { renderTask };
                _engine.Execute(_renderIndex, &renderOnly);

                while (_playBlasting && !renderTask->IsConverged()) {
                    std::this_thread::sleep_for(msWait);
                    _engine.Execute(_renderIndex, &renderOnly);
                }
            } else {
                TF_WARN("HdxProgressiveTask not found");
            }
        }

        // MAYA-114630
        // https://github.com/PixarAnimationStudios/USD/commit/fc63eaef29
        // removed backing, and restoring of GL_FRAMEBUFFER state.
        // At the same time HdxColorizeSelectionTask modifies the frame buffer state
        // Manually backup and restore the state of the frame buffer for now.
        HdMayaGLBackup backup;
        if (_backupFrameBufferWorkaround) {
            HdTaskSharedPtr backupTask(new HdMayaBackupGLStateTask(backup));
            HdTaskSharedPtr restoreTask(new HdMayaRestoreGLStateTask(backup));
            tasks.reserve(tasks.size() + 2);
            for (auto it = tasks.begin(); it != tasks.end(); it++) {
                if (std::dynamic_pointer_cast<HdxColorizeSelectionTask>(*it)) {
                    tasks.insert(it, backupTask);
                    tasks.insert(it + 2, restoreTask);
                    break;
                }
            }
        }
        _engine.Execute(_renderIndex, &tasks);

        // HdTaskController will query all of the tasks it can for IsConverged.
        // This includes HdRenderPass::IsConverged and HdRenderBuffer::IsConverged (via colorizer).
        //
        _isConverged = _taskController->IsConverged();
        if (markTime) {
            std::lock_guard<std::mutex> lock(_lastRenderTimeMutex);
            _lastRenderTime = std::chrono::system_clock::now();
        }
    };
    if (_initializationAttempted && !_initializationSucceeded) {
        // Initialization must have failed already, stop trying.
        return MStatus::kFailure;
    }

    _DetectMayaDefaultLighting(drawContext);
    if (_needsClear.exchange(false)) {
        ClearHydraResources();
    }

    if (!_initializationAttempted) {
		// TODO: First time viewport scene update here
        // Anything special ? Dag items populate called here before
       _InitHydraResources();

        if (!_initializationSucceeded) {
            return MStatus::kFailure;
        }
    }

	// TODO: Change management
	// Every frame update everything
	if (scene.mCount > 0)
	{
		for (auto& it : _delegates) {
			auto sceneDelegate = std::dynamic_pointer_cast<HdMayaSceneDelegate>(it);
			if (sceneDelegate)
			{
				sceneDelegate->HandleCompleteViewportScene(scene);
			}
		}
	}

    GLUniformBufferBindingsSaver bindingsSaver;

    _SelectionChanged();

    const auto   displayStyle = drawContext.getDisplayStyle();
    HdMayaParams delegateParams = _globals.delegateParams;
    delegateParams.displaySmoothMeshes = !(displayStyle & MHWRender::MFrameContext::kFlatShaded);

    if (_defaultLightDelegate != nullptr) {
        _defaultLightDelegate->SetDefaultLight(_defaultLight);
    }
    for (auto& it : _delegates) {
        it->SetParams(delegateParams);
        it->PreFrame(drawContext);
    }

    HdxRenderTaskParams params;
    params.enableLighting = true;
    params.enableSceneMaterials
        = !(drawContext.getDisplayStyle() & MHWRender::MFrameContext::kDefaultMaterial);

    /* TODO: Find replacement
     * if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
        params.complexity = HdComplexityBoundingBox;
    } else {
        params.complexity = HdComplexityVeryHigh;
    }

    if (displayStyle & MHWRender::MFrameContext::kWireFrame) {
        params.geomStyle = HdGeomStyleLines;
    } else {
        params.geomStyle = HdGeomStylePolygons;
    }*/

    // TODO: separate color for normal wireframe / selected
    MColor colour = M3dView::leadColor();
    params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);

    params.cullStyle = HdCullStyleBackUnlessDoubleSided;

    int width = 0;
    int height = 0;
    drawContext.getRenderTargetSize(width, height);

    bool vpDirty;
    if ((vpDirty = (width != _viewport[2] || height != _viewport[3]))) {
        _viewport = GfVec4d(0, 0, width, height);
        _taskController->SetRenderViewport(_viewport);
    }

    _taskController->SetFreeCameraMatrices(
        GetGfMatrixFromMaya(drawContext.getMatrix(MHWRender::MFrameContext::kViewMtx)),
        GetGfMatrixFromMaya(drawContext.getMatrix(MHWRender::MFrameContext::kProjectionMtx)));

    if (delegateParams.motionSamplesEnabled()) {
        MStatus  status;
        MDagPath camPath = getFrameContext()->getCurrentCameraPath(&status);
        if (status == MStatus::kSuccess) {
#ifdef MAYA_CURRENT_UFE_CAMERA_SUPPORT
            MString   ufeCameraPathString = getFrameContext()->getCurrentUfeCameraPath(&status);
            Ufe::Path ufeCameraPath = Ufe::PathString::path(ufeCameraPathString.c_str());
            bool      isUsdCamera = ufeCameraPath.runTimeId() == MayaUsd::ufe::getUsdRunTimeId();
#else
            static const MString defaultUfeProxyCameraShape(
                "|defaultUfeProxyCameraTransformParent|defaultUfeProxyCameraTransform|"
                "defaultUfeProxyCameraShape");
            bool isUsdCamera = defaultUfeProxyCameraShape == camPath.fullPathName();
#endif
            if (!isUsdCamera) {
                for (auto& delegate : _delegates) {
                    if (HdMayaSceneDelegate* mayaScene
                        = dynamic_cast<HdMayaSceneDelegate*>(delegate.get())) {
                        params.camera = mayaScene->SetCameraViewport(camPath, _viewport);
                        if (vpDirty)
#if HD_API_VERSION >= 43
                            mayaScene->GetChangeTracker().MarkSprimDirty(
                                params.camera, HdCamera::DirtyParams);
#else
                            mayaScene->GetChangeTracker().MarkSprimDirty(
                                params.camera, HdCamera::DirtyParams | HdCamera::DirtyProjMatrix);
#endif
                        break;
                    }
                }
            }
        } else {
            TF_WARN(
                "MFrameContext::getCurrentCameraPath failure (%d): '%s'"
                "\nUsing viewport matrices.",
                int(status.statusCode()),
                status.errorString().asChar());
        }
    }

    _taskController->SetRenderParams(params);
    if (!params.camera.IsEmpty())
        _taskController->SetCameraPath(params.camera);

    // Default color in usdview.
    _taskController->SetSelectionColor(_globals.colorSelectionHighlightColor);
    _taskController->SetEnableSelection(_globals.colorSelectionHighlight);

#if PXR_VERSION >= 2005
    if (_globals.outlineSelectionWidth != 0.f) {
        _taskController->SetSelectionOutlineRadius(_globals.outlineSelectionWidth);
        _taskController->SetSelectionEnableOutline(true);
    } else
        _taskController->SetSelectionEnableOutline(false);
#endif
#if PXR_VERSION <= 2005
    _taskController->SetColorizeQuantizationEnabled(_globals.enableColorQuantization);
#endif

    _taskController->SetCollection(_renderCollection);
    if (_isUsingHdSt) {
        // TODO: Is there a way to improve this? Quite silly.
        auto  enableShadows = true;
        auto* lightParam = drawContext.getLightParameterInformation(
            0, MHWRender::MDrawContext::kFilteredIgnoreLightLimit);
        if (lightParam != nullptr) {
            MIntArray intVals;
            if (lightParam->getParameter(
                    MHWRender::MLightParameterInformation::kGlobalShadowOn, intVals)
                && intVals.length() > 0) {
                enableShadows = intVals[0] != 0;
            }
        }
        HdxShadowTaskParams shadowParams;
        shadowParams.cullStyle = HdCullStyleNothing;

        // The light & shadow parameters currently (19.11-20.08) are only used for tasks specific to
        // Storm
        _taskController->SetEnableShadows(enableShadows);
        _taskController->SetShadowParams(shadowParams);

#ifndef HDMAYA_OIT_ENABLED
        // This is required for HdStorm to display transparency.
        // We should fix this upstream, so HdStorm can setup
        // all the required states.
        HdMayaSetRenderGLState state;
#endif
        renderFrame(true);

        // This causes issues with the embree delegate and potentially others.
        // (i.e. rendering a wireframe via collections isn't supported by other delegates)
        if (_globals.wireframeSelectionHighlight && !_selectionCollection.GetRootPaths().empty()) {
            _taskController->SetCollection(_selectionCollection);
            renderFrame();
            // XXX: This call isn't 'free' and will be done again on the next
            // MtohRenderOverride::Render call anyway
            _taskController->SetCollection(_renderCollection);
        }
    } else {
        renderFrame(true);
    }

    for (auto& it : _delegates) {
        it->PostFrame();
    }

    return MStatus::kSuccess;
}

MtohRenderOverride* MtohRenderOverride::_GetByName(TfToken rendererName)
{
    std::lock_guard<std::mutex> lock(_allInstancesMutex);
    for (auto* instance : _allInstances) {
        if (instance->_rendererDesc.rendererName == rendererName) {
            return instance;
        }
    }
    return nullptr;
}

// TODO: Pass MViewportScene
void MtohRenderOverride::_InitHydraResources()
{
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg("MtohRenderOverride::_InitHydraResources(%s)\n", _rendererDesc.rendererName.GetText());

    _initializationAttempted = true;

#if PXR_VERSION < 2102
    GlfGlewInit();
#endif
    GlfContextCaps::InitInstance();
    _rendererPlugin
        = HdRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererDesc.rendererName);
    if (!_rendererPlugin)
        return;

    auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
    if (!renderDelegate)
        return;

    _renderIndex = HdRenderIndex::New(renderDelegate, { &_hgiDriver });
    if (!_renderIndex)
        return;

    _taskController = new HdxTaskController(
        _renderIndex,
        _ID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p",
            TfMakeValidIdentifier(_rendererDesc.rendererName.GetText()).c_str(),
            this))));
    _taskController->SetEnableShadows(true);

    HdMayaDelegate::InitData delegateInitData(
        TfToken(),
        _engine,
        _renderIndex,
        _rendererPlugin,
        _taskController,
        SdfPath(),
        _isUsingHdSt);

    auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
    auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
    TF_VERIFY(delegateNames.size() == creators.size());
    for (size_t i = 0, n = creators.size(); i < n; ++i) {
        const auto& creator = creators[i];
        if (creator == nullptr) {
            continue;
        }
        delegateInitData.name = delegateNames[i];
        delegateInitData.delegateID = _ID.AppendChild(
            TfToken(TfStringPrintf("_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
        auto newDelegate = creator(delegateInitData);
        if (newDelegate) {
            // Call SetLightsEnabled before the delegate is populated
            newDelegate->SetLightsEnabled(!_hasDefaultLighting);
            _delegates.emplace_back(std::move(newDelegate));
        }
    }
    if (_hasDefaultLighting) {
        delegateInitData.delegateID
            = _ID.AppendChild(TfToken(TfStringPrintf("_DefaultLightDelegate_%p", this)));
        _defaultLightDelegate.reset(new MtohDefaultLightDelegate(delegateInitData));
    }
    VtValue selectionTrackerValue(_selectionTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selectionTrackerValue);
    for (auto& it : _delegates) {
        it->Populate();
    }
    if (_defaultLightDelegate) {
        _defaultLightDelegate->Populate();
    }

    _renderIndex->GetChangeTracker().AddCollection(_selectionCollection.GetName());
    _SelectionChanged();

    if (auto* renderDelegate = _GetRenderDelegate()) {
        // Pull in any options that may have changed due file-open.
        // If the currentScene has defaultRenderGlobals we'll absorb those new settings,
        // but if not, fallback to user-defaults (current state) .
        const bool filterRenderer = true;
        const bool fallbackToUserDefaults = true;
        _globals.GlobalChanged(
            { _rendererDesc.rendererName, filterRenderer, fallbackToUserDefaults });
        _globals.ApplySettings(renderDelegate, _rendererDesc.rendererName);
    }
    auto tasks = _taskController->GetRenderingTasks();
    for (auto task : tasks) {
        if (std::dynamic_pointer_cast<HdxColorizeSelectionTask>(task)) {
            _backupFrameBufferWorkaround = true;
            break;
        }
    }
    _initializationSucceeded = true;
}

void MtohRenderOverride::ClearHydraResources()
{
    if (!_initializationAttempted) {
        return;
    }

    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg("MtohRenderOverride::ClearHydraResources(%s)\n", _rendererDesc.rendererName.GetText());

    _delegates.clear();
    _defaultLightDelegate.reset();

    // Cleanup internal context data that keep references to data that is now
    // invalid.
#if PXR_VERSION >= 2108
    _engine.ClearTaskContextData();
#else
    for (const auto& token : HdxTokens->allTokens)
        _engine.RemoveTaskContextData(token);
#endif

    if (_taskController != nullptr) {
        delete _taskController;
        _taskController = nullptr;
    }

    HdRenderDelegate* renderDelegate = nullptr;
    if (_renderIndex != nullptr) {
        renderDelegate = _renderIndex->GetRenderDelegate();
        delete _renderIndex;
        _renderIndex = nullptr;
    }

    if (_rendererPlugin != nullptr) {
        if (renderDelegate != nullptr) {
            _rendererPlugin->DeleteRenderDelegate(renderDelegate);
        }
        HdRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
        _rendererPlugin = nullptr;
    }

    _viewport = GfVec4d(0, 0, 0, 0);
    _initializationSucceeded = false;
    _initializationAttempted = false;
    SelectionChanged();
}

void MtohRenderOverride::_RemovePanel(MString panelName)
{
    auto foundPanelCallbacks = _FindPanelCallbacks(panelName);
    if (foundPanelCallbacks != _renderPanelCallbacks.end()) {
        MMessage::removeCallbacks(foundPanelCallbacks->second);
        _renderPanelCallbacks.erase(foundPanelCallbacks);
    }

    if (_renderPanelCallbacks.empty()) {
        ClearHydraResources();
    }
}

void MtohRenderOverride::SelectionChanged() { _selectionChanged = true; }

void MtohRenderOverride::_SelectionChanged()
{
    if (!_selectionChanged) {
        return;
    }
    _selectionChanged = false;
    MSelectionList sel;
    if (!TF_VERIFY(MGlobal::getActiveSelectionList(sel))) {
        return;
    }
    SdfPathVector selectedPaths;
    auto          selection = std::make_shared<HdSelection>();

#if WANT_UFE_BUILD
    const UFE_NS::GlobalSelection::Ptr& ufeSelection = UFE_NS::GlobalSelection::get();
#endif // WANT_UFE_BUILD

    for (auto& it : _delegates) {
#if WANT_UFE_BUILD
        if (it->SupportsUfeSelection()) {
            if (ufeSelection) {
                it->PopulateSelectedPaths(*ufeSelection, selectedPaths, selection);
            }
            // skip non-ufe PopulateSelectedPaths call
            continue;
        }
#endif // WANT_UFE_BUILD
        it->PopulateSelectedPaths(sel, selectedPaths, selection);
    }
    _selectionCollection.SetRootPaths(selectedPaths);
    _selectionTracker->SetSelection(HdSelectionSharedPtr(selection));
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
        .Msg("MtohRenderOverride::_SelectionChanged - num selected: %lu\n", selectedPaths.size());
}

MHWRender::DrawAPI MtohRenderOverride::supportedDrawAPIs() const
{
    return MHWRender::kOpenGLCoreProfile | MHWRender::kOpenGL;
}

MStatus MtohRenderOverride::setup(const MString& destination)
{
    MStatus status;

    auto panelNameAndCallbacks = _FindPanelCallbacks(destination);
    if (panelNameAndCallbacks == _renderPanelCallbacks.end()) {
        // Install the panel callbacks
        MCallbackIdArray newCallbacks;

        auto id = MUiMessage::add3dViewDestroyMsgCallback(
            destination, _PanelDeletedCallback, this, &status);
        if (status) {
            newCallbacks.append(id);
        }

        id = MUiMessage::add3dViewRendererChangedCallback(
            destination, _RendererChangedCallback, this, &status);
        if (status) {
            newCallbacks.append(id);
        }

        id = MUiMessage::add3dViewRenderOverrideChangedCallback(
            destination, _RenderOverrideChangedCallback, this, &status);
        if (status) {
            newCallbacks.append(id);
        }

        _renderPanelCallbacks.emplace_back(destination, newCallbacks);
    }

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer == nullptr) {
        return MStatus::kFailure;
    }

    if (_operations.empty()) {
        // Clear and draw the grid
        _operations.push_back(new HdMayaPreRender("HydraRenderOverride_PreScene"));

        // The main hydra render
        _operations.push_back(new HdMayaRender("HydraRenderOverride_Hydra", this));

        // Draw scene elements (cameras, CVs, grid, shapes not pushed into hydra)
        _operations.push_back(new HdMayaPostRender("HydraRenderOverride_PostScene"));

        // Draw HUD elements
        _operations.push_back(new MHWRender::MHUDRender());

        // Set final buffer options
        auto* presentTarget = new MHWRender::MPresentTarget("HydraRenderOverride_Present");
        presentTarget->setPresentDepth(true);
        presentTarget->setTargetBackBuffer(MHWRender::MPresentTarget::kCenterBuffer);
        _operations.push_back(presentTarget);
    }

    return MS::kSuccess;
}

MStatus MtohRenderOverride::cleanup()
{
    _currentOperation = -1;
    return MS::kSuccess;
}

bool MtohRenderOverride::startOperationIterator()
{
    _currentOperation = 0;
    return true;
}

MHWRender::MRenderOperation* MtohRenderOverride::renderOperation()
{
    if (_currentOperation >= 0 && _currentOperation < static_cast<int>(_operations.size())) {
        return _operations[_currentOperation];
    }
    return nullptr;
}

bool MtohRenderOverride::nextRenderOperation()
{
    return ++_currentOperation < static_cast<int>(_operations.size());
}

#if MAYA_API_VERSION >= 20210000
bool MtohRenderOverride::select(
    const MHWRender::MFrameContext&  frameContext,
    const MHWRender::MSelectionInfo& selectInfo,
    bool /*useDepth*/,
    MSelectionList& selectionList,
    MPointArray&    worldSpaceHitPts)
{
    MStatus status = MStatus::kFailure;

    MMatrix viewMatrix = frameContext.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    if (status != MStatus::kSuccess)
        return false;

    MMatrix projMatrix = frameContext.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    if (status != MStatus::kSuccess)
        return false;

    int view_x, view_y, view_w, view_h;
    status = frameContext.getViewportDimensions(view_x, view_y, view_w, view_h);
    if (status != MStatus::kSuccess)
        return false;

    unsigned int sel_x, sel_y, sel_w, sel_h;
    status = selectInfo.selectRect(sel_x, sel_y, sel_w, sel_h);
    if (status != MStatus::kSuccess)
        return false;

    // Compute a pick matrix that, when it is post-multiplied with the projection matrix, will
    // cause the picking region to fill the entire/ viewport for OpenGL selection.
    {
        MMatrix pickMatrix;
        pickMatrix[0][0] = view_w / double(sel_w);
        pickMatrix[1][1] = view_h / double(sel_h);
        pickMatrix[3][0] = (view_w - (double)(sel_x * 2 + sel_w)) / double(sel_w);
        pickMatrix[3][1] = (view_h - (double)(sel_y * 2 + sel_h)) / double(sel_h);

        projMatrix *= pickMatrix;
    }

    const bool pointSnappingActive = selectInfo.pointSnapping();

    // Set up picking params.
    HdxPickTaskContextParams pickParams;
    pickParams.resolution.Set(view_w, view_h);
    pickParams.viewMatrix.Set(viewMatrix.matrix);
    pickParams.projectionMatrix.Set(projMatrix.matrix);
    pickParams.resolveMode = HdxPickTokens->resolveUnique;

    if (pointSnappingActive) {
        pickParams.pickTarget = HdxPickTokens->pickPoints;

        // Exclude selected Rprims to avoid self-snapping issue.
        pickParams.collection = _pointSnappingCollection;
        pickParams.collection.SetExcludePaths(_selectionCollection.GetRootPaths());
    } else {
        pickParams.collection = _renderCollection;
    }

    HdxPickHitVector outHits;
    pickParams.outHits = &outHits;

    // Execute picking tasks.
    HdTaskSharedPtrVector pickingTasks = _taskController->GetPickingTasks();
    VtValue               pickParamsValue(pickParams);
    _engine.SetTaskContextData(HdxPickTokens->pickParams, pickParamsValue);
    _engine.Execute(_taskController->GetRenderIndex(), &pickingTasks);

    if (pointSnappingActive) {
        // Find the hit nearest to the cursor point and use it for point snapping.
        int nearestHitIndex = -1;
        int cursor_x, cursor_y;
        if (selectInfo.cursorPoint(cursor_x, cursor_y)) {
            nearestHitIndex = GetNearestHitIndex(frameContext, outHits, cursor_x, cursor_y);
        }

        if (nearestHitIndex >= 0) {
            const HdxPickHit hit = outHits[nearestHitIndex];
            outHits.clear();
            outHits.push_back(hit);
        } else {
            outHits.clear();
        }
    } else {
        // Multiple hits can be produced for a single object on marquee selection even pickTarget
        // is the default "pickPrimsAndInstances" mode, and each hit is created for an "element"
        // which I guess means a face id and should only be required when pickTarget is "pickFaces".
        // I would expect only one hit to be created for object-level selection. Having duplicated
        // hits for the same object would slow down selection performance, esp. for dense mesh.
        // Some more details can be found: https://groups.google.com/g/usd-interest/c/Cgosy3r7Vv4
        HdxPickHitVector uniqueHits;
        ResolveUniqueHits_Workaround(outHits, uniqueHits);
        outHits.swap(uniqueHits);
    }

    if (!outHits.empty()) {
        for (auto& it : _delegates) {
            it->PopulateSelectionList(outHits, selectInfo, selectionList, worldSpaceHitPts);
        }
    }

    return true;
}
#endif

void MtohRenderOverride::_ClearHydraCallback(void* data)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }
    instance->ClearHydraResources();
}

void MtohRenderOverride::_PlayblastingChanged(bool playBlasting, void* userData)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(userData);
    if (std::atomic_exchange(&instance->_playBlasting, playBlasting) == playBlasting)
        return;

    MStatus status;
    if (!playBlasting) {
        assert(instance->_timerCallback == 0 && "Callback exists");
        instance->_timerCallback
            = MTimerMessage::addTimerCallback(1.0f / 10.0f, _TimerCallback, instance, &status);
    } else {
        status = MMessage::removeCallback(instance->_timerCallback);
        instance->_timerCallback = 0;
    }
    CHECK_MSTATUS(status);
}

void MtohRenderOverride::_TimerCallback(float, float, void* data)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (instance->_playBlasting || instance->_isConverged) {
        return;
    }

    std::lock_guard<std::mutex> lock(instance->_lastRenderTimeMutex);
    if ((std::chrono::system_clock::now() - instance->_lastRenderTime) < std::chrono::seconds(5)) {
        MGlobal::executeCommandOnIdle("refresh -f");
    }
}

void MtohRenderOverride::_PanelDeletedCallback(const MString& panelName, void* data)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }

    instance->_RemovePanel(panelName);
}

void MtohRenderOverride::_RendererChangedCallback(
    const MString& panelName,
    const MString& oldRenderer,
    const MString& newRenderer,
    void*          data)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }

    if (newRenderer != oldRenderer) {
        instance->_RemovePanel(panelName);
    }
}

void MtohRenderOverride::_RenderOverrideChangedCallback(
    const MString& panelName,
    const MString& oldOverride,
    const MString& newOverride,
    void*          data)
{
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }

    if (newOverride != instance->name()) {
        instance->_RemovePanel(panelName);
    }
}

void MtohRenderOverride::_SelectionChangedCallback(void* data)
{
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
        .Msg("MtohRenderOverride::_SelectionChangedCallback() (normal maya "
             "selection triggered)\n");
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }
    instance->SelectionChanged();
}

PXR_NAMESPACE_CLOSE_SCOPE
