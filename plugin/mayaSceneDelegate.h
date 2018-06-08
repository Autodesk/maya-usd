#ifndef __MAYA_SCENE_DELEGATE_H__
#define __MAYA_SCENE_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaSceneDelegate : public HdSceneDelegate {
public:
    MayaSceneDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    virtual ~MayaSceneDelegate();

    VtValue Get(SdfPath const& id, TfToken const& key) override;

    void SetCameraState(
        const MMatrix& worldToView,
        const MMatrix& projection,
        const GfVec4d& _viewport);

    HdTaskSharedPtrVector GetRenderTasks();
private:
    SdfPath cameraId;
    SdfPath rootId;
    GfVec4d viewport;

    using ValueCache = TfHashMap<TfToken, VtValue, TfToken::HashFunctor>;
    using ValueCacheMap = TfHashMap<SdfPath, ValueCache, SdfPath::Hash>;
    ValueCacheMap valueCacheMap;

    using RenderTaskIdMap = std::unordered_map<size_t, SdfPath>;
    RenderTaskIdMap renderSetupTaskIdMap;
    RenderTaskIdMap renderTaskIdMap;
};

typedef std::shared_ptr<MayaSceneDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MAYA_SCENE_DELEGATE_H__
