#ifndef __HDMAYA_SCENE_DELEGATE_H__
#define __HDMAYA_SCENE_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/meshTopology.h>

#include <maya/MDagPath.h>
#include <maya/MObject.h>

#include <memory>

#include <hdmaya/adapters/shapeAdapter.h>
#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/materialAdapter.h>
#include <hdmaya/delegates/delegateCtx.h>

/*
 * Notes.
 *
 * To remove the need of casting between different adapter types or
 * making the base adapter class too heavy I decided to use 3 different set
 * or map types. This adds a bit of extra code to the RemoveAdapter function
 * but simplifies the rest of the functions significantly (and no downcasting!).
 *
 * All this would be probably way nicer / easier with C++14 and the polymorphic
 * lambdas.
 *
 * This also optimizes other things, like it's easier to separate functionality
 * that only affects shapes, lights or materials.
 */

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaSceneDelegate : public HdMayaDelegateCtx {
public:
    HdMayaSceneDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    virtual ~HdMayaSceneDelegate();

    void Populate() override;
    void RemoveAdapter(const SdfPath& id) override;
    void InsertDag(const MDagPath& dag);
    void SetParams(const HdMayaParams& params) override;
protected:
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
    SdfPath GetMaterialId(const SdfPath& id) override;
    std::string GetSurfaceShaderSource(const SdfPath& id) override;
    std::string GetDisplacementShaderSource(const SdfPath& id) override;
    VtValue GetMaterialParamValue(const SdfPath& id, const TfToken& paramName) override;
    HdMaterialParamVector GetMaterialParams(const SdfPath& id) override;
    VtValue GetMaterialResource(const SdfPath& id) override;
    TfTokenVector GetMaterialPrimvars(const SdfPath& id) override;

private:
    std::unordered_map<SdfPath, HdMayaShapeAdapterPtr, SdfPath::Hash> _shapeAdapters;
    std::unordered_map<SdfPath, HdMayaLightAdapterPtr, SdfPath::Hash> _lightAdapters;
    std::unordered_map<SdfPath, HdMayaMaterialAdapterPtr, SdfPath::Hash> _materialAdapters;
    std::vector<MCallbackId> _callbacks;

    SdfPath _fallbackMaterial;
};

typedef std::shared_ptr<HdMayaSceneDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SCENE_DELEGATE_H__
