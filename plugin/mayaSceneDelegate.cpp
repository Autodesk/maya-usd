#include "mayaSceneDelegate.h"

#include <pxr/base/gf/matrix4d.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

MayaSceneDelegate::MayaSceneDelegate(
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

MayaSceneDelegate::~MayaSceneDelegate() {

}

VtValue
MayaSceneDelegate::Get(SdfPath const& id, const TfToken& key) {
    auto* cache = TfMapLookupPtr(valueCacheMap, id);
    VtValue ret;
    if (cache && TfMapLookup(*cache, key, &ret)) {
        return ret;
    }

    std::cerr << "[MayaSceneDelegate] Error accessing " << key << " from the valueCacheMap!" << std::endl;

    return VtValue();

}

void
MayaSceneDelegate::SetCameraState(const MMatrix& worldToView, const MMatrix& projection,
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

HdTaskSharedPtrVector MayaSceneDelegate::GetRenderTasks() {
    return {};
}
