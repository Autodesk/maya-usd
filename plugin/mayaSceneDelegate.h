#ifndef __MAYA_SCENE_DELEGATE_H__
#define __MAYA_SCENE_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/base/tf/declarePtrs.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaSceneDelegate : public HdSceneDelegate {
public:
    MayaSceneDelegate(
        HdRenderIndex* parentIndex,
        const SdfPath& delegateID);

    virtual ~MayaSceneDelegate();
};

typedef std::shared_ptr<MayaSceneDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MAYA_SCENE_DELEGATE_H__
