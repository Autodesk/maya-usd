#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/meshTopology.h>

#include <maya/MDagPath.h>

#include <memory>

#include "params.h"
#include "dagAdapter.h"

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
    bool GetDoubleSided(const SdfPath& id) override;
    HdCullStyle GetCullStyle(const SdfPath& id) override;
    // VtValue GetShadingStyle(const SdfPath& id) override;
    HdDisplayStyle GetDisplayStyle(const SdfPath& id) override;
    // TfToken GetReprName(const SdfPath& id) override;
    VtValue Get(SdfPath const& id, TfToken const& key) override;
    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation) override;
    VtValue GetLightParamValue(const SdfPath& id, const TfToken& paramName) override;

    // SdfPath GetMaterialId(const SdfPath& id) override;
    // std::string GetSurfaceShaderSource(const SdfPath& id) override;
    // std::string GetDisplacementShaderSource(const SdfPath& id) override;
    // VtValue GetMaterialParamValue(const SdfPath& id, const TfToken& paramName) override;
    // HdMaterialParamVector GetMaterialParams(const SdfPath& id) override;
    // VtValue GetMaterialResource(const SdfPath& id) override;
    // TfTokenVector GetMaterialPrimvars(const SdfPath& id) override;

    void Populate();
private:
    SdfPath GetPrimPath(const MDagPath& dg);
    TfHashMap<SdfPath, HdMayaDagAdapterPtr, SdfPath::Hash> _pathToAdapterMap;
};

typedef std::shared_ptr<HdMayaDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
