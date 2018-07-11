#ifndef __HDMAYA_DELEGATE_BASE_H__
#define __HDMAYA_DELEGATE_BASE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>

#include <hdmaya/delegates/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateCtx : public HdSceneDelegate, public HdMayaDelegate {
protected:
    HDMAYA_API
    HdMayaDelegateCtx(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);
public:
    using HdSceneDelegate::GetRenderIndex;
    HdChangeTracker& GetChangeTracker() { return GetRenderIndex().GetChangeTracker(); }

    HDMAYA_API
    void InsertRprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    HDMAYA_API
    void InsertSprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    HDMAYA_API
    void RemoveRprim(const SdfPath& id);
    HDMAYA_API
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);
    virtual void RemoveAdapter(const SdfPath& id) = 0;
    const HdRprimCollection& GetRprimCollection() { return _rprimCollection; }
    HDMAYA_API
    SdfPath GetPrimPath(const MDagPath& dg);
    HDMAYA_API
    SdfPath GetMaterialPath(const MObject& obj);

    /// Fit the frustum's near/far value to contain all
    /// the rprims inside the render index;
    HDMAYA_API
    void FitFrustumToRprims(GfFrustum& frustum);
private:
    HdRprimCollection _rprimCollection;
    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_BASE_H__
