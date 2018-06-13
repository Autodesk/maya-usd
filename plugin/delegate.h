#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/meshTopology.h>

#include <maya/MDagPath.h>

#include "params.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate : public HdSceneDelegate {
public:
    HdMayaDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    virtual ~HdMayaDelegate();

    HdMeshTopology GetMeshTopology(const SdfPath& id) override;
    GfRange3d GetExtent(const SdfPath& id) override;
    GfMatrix4d GetTransform(const SdfPath& id) override;
    bool GetVisible(const SdfPath& id) override;
    bool IsEnabled(const TfToken& option) const override;
    VtValue Get(SdfPath const& id, TfToken const& key) override;

    void Populate();
private:
    SdfPath getPrimPath(const MDagPath& dg);
    using PathToDgMap = TfHashMap<SdfPath, MDagPath, SdfPath::Hash>;
    PathToDgMap pathToDgMap;
};

typedef std::shared_ptr<HdMayaDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
