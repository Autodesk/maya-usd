#include "delegate.h"

#include <pxr/base/gf/matrix4d.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

PXR_NAMESPACE_USING_DIRECTIVE

HdMayaDelegate::HdMayaDelegate(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID) {
    // Unique ID
    rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_HdMaya_%p", this)));
    cameraId = rootId.AppendChild(HdPrimTypeTokens->camera);

    TF_VERIFY(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera));
    renderIndex->InsertSprim(HdPrimTypeTokens->camera, this, cameraId);
    {
        auto& cache = valueCacheMap[cameraId];
        cache[HdCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdCameraTokens->windowPolicy] = VtValue();
    }
}

HdMayaDelegate::~HdMayaDelegate() {

}

VtValue
HdMayaDelegate::Get(SdfPath const& id, const TfToken& key) {
    auto* cache = TfMapLookupPtr(valueCacheMap, id);
    VtValue ret;
    if (cache && TfMapLookup(*cache, key, &ret)) {
        return ret;
    }

    std::cerr << "[HdMayaDelegate] Error accessing " << key << " from the valueCacheMap!" << std::endl;

    return VtValue();

}

void
HdMayaDelegate::SetCameraState(const MMatrix& worldToView, const MMatrix& projection,
                                  const GfVec4d& _viewport) {
    auto& cache = valueCacheMap[cameraId];
    GfMatrix4d mat;
    memcpy(mat.GetArray(), worldToView[0], sizeof(double) * 16);
    cache[HdCameraTokens->worldToViewMatrix] = mat;
    memcpy(mat.GetArray(), projection[0], sizeof(double) * 16);
    cache[HdCameraTokens->projectionMatrix] = mat;
    cache[HdCameraTokens->windowPolicy] = VtValue();

    GetRenderIndex().GetChangeTracker().MarkSprimDirty(cameraId, HdCamera::AllDirty);

    viewport = _viewport;
}

HdTaskSharedPtrVector HdMayaDelegate::GetRenderTasks(
    const MayaRenderParams& params,
    const HdRprimCollection& rprimCollection) {
    const auto hash = params.hash();

    SdfPath renderSetupTaskId;
    if (!TfMapLookup(renderSetupTaskIdMap, hash, &renderSetupTaskId)) {
        // Create a new render setup task if one does not exist for this hash.
        renderSetupTaskId = rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   HdxPrimitiveTokens->renderSetupTask.GetText(),
                                   hash)));

        GetRenderIndex().InsertTask<HdxRenderSetupTask>(this, renderSetupTaskId);

        HdxRenderTaskParams renderSetupTaskParams;
        renderSetupTaskParams.camera = cameraId;
        renderSetupTaskParams.viewport = viewport;

        auto& cache = valueCacheMap[renderSetupTaskId];
        cache[HdTokens->params] = VtValue(renderSetupTaskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
        cache[HdTokens->collection] = VtValue();

        renderSetupTaskIdMap[hash] = renderSetupTaskId;
    }

    SdfPath renderTaskId;
    if (!TfMapLookup(renderTaskIdMap, hash, &renderTaskId)) {
        renderTaskId = rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   HdxPrimitiveTokens->renderTask.GetText(),
                                   hash)));

        GetRenderIndex().InsertTask<HdxRenderTask>(this, renderTaskId);

        auto& cache = valueCacheMap[renderTaskId];
        cache[HdTokens->params] = VtValue();
        cache[HdTokens->children] = VtValue(SdfPathVector());
        cache[HdTokens->collection] = VtValue();

        renderTaskIdMap[hash] = renderTaskId;
    }

    // Get the render setup task params from the value cache.
    HdxRenderTaskParams renderSetupTaskParams =
        GetValue<HdxRenderTaskParams>(renderSetupTaskId, HdTokens->params);

    // Update the render setup task params.
    renderSetupTaskParams.overrideColor = GfVec4f(0.5f);
    renderSetupTaskParams.wireframeColor = GfVec4f(0.1f, 0.8f, 0.1f, 1.0f);
    renderSetupTaskParams.enableLighting = params.enableLighting;
    renderSetupTaskParams.enableIdRender = false;
    renderSetupTaskParams.alphaThreshold = 0.1f;
    renderSetupTaskParams.tessLevel = 1.0f;
    constexpr auto tinyThreshold = 0.9f;
    renderSetupTaskParams.drawingRange = GfVec2f(tinyThreshold, -1.0f);
    renderSetupTaskParams.enableHardwareShading = true;
    renderSetupTaskParams.depthBiasUseDefault = true;
    renderSetupTaskParams.depthFunc = HdCmpFuncLess;
    renderSetupTaskParams.geomStyle = HdGeomStylePolygons;

    SetValue(renderSetupTaskId, HdTokens->params, renderSetupTaskParams);
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderSetupTaskId,
        HdChangeTracker::DirtyParams);

    // The render setup task both supports rprimCollection and rprimCollectionVector
    SetValue(renderTaskId, HdTokens->collection, rprimCollection);
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,
        HdChangeTracker::DirtyCollection);

    return {
        GetRenderIndex().GetTask(renderSetupTaskId),
        GetRenderIndex().GetTask(renderTaskId)
    };
}
