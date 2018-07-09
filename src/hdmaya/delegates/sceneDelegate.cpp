#include <hdmaya/delegates/sceneDelegate.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MString.h>

#include <hdmaya/utils.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/delegates/delegateRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

void
_nodeAdded(MObject& obj, void* clientData) {
    auto* delegate = reinterpret_cast<HdMayaSceneDelegate*>(clientData);
    MDagPath dag;
    MStatus status = MDagPath::getAPathTo(obj, dag);
    if (status) {
        delegate->InsertDag(dag);
    }
}

}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdMayaSceneDelegate)
    ((FallbackMaterial, "__fallback_material__"))
);

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaSceneDelegate) {
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaSceneDelegate,
        [](HdRenderIndex* parentIndex, const SdfPath& id) -> HdMayaDelegatePtr {
            return std::static_pointer_cast<HdMayaDelegate>(std::make_shared<HdMayaSceneDelegate>(parentIndex, id));
        });
}

HdMayaSceneDelegate::HdMayaSceneDelegate(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) :
    HdMayaDelegateCtx(renderIndex, delegateID),
    _fallbackMaterial(delegateID.AppendChild(_tokens->FallbackMaterial)) {

}

HdMayaSceneDelegate::~HdMayaSceneDelegate() {
    for (auto callback : _callbacks) {
        MMessage::removeCallback(callback);
    }
}

void
HdMayaSceneDelegate::Populate() {
    auto& renderIndex = GetRenderIndex();
    auto& changeTracker = renderIndex.GetChangeTracker();
    for (MItDag dagIt(MItDag::kDepthFirst, MFn::kInvalid); !dagIt.isDone(); dagIt.next()) {
        MDagPath path;
        dagIt.getPath(path);
        InsertDag(path);
    }
    MStatus status;
    auto id = MDGMessage::addNodeAddedCallback(_nodeAdded, "dagNode", this, &status);
    if (status) { _callbacks.push_back(id); }

    // Adding fallback material sprim to the render index.
    renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _fallbackMaterial);
}

void
HdMayaSceneDelegate::RemoveAdapter(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (TfMapLookup(_pathToAdapterMap, id, &adapter) && adapter != nullptr) {
        adapter->RemovePrim();
        _pathToAdapterMap.erase(id);
    }
}

void
HdMayaSceneDelegate::InsertDag(const MDagPath& dag) {
    // We don't care about transforms.
    if (dag.hasFn(MFn::kTransform)) { return; }

    auto adapterCreator = HdMayaAdapterRegistry::GetAdapterCreator(dag);
    if (adapterCreator == nullptr) { return; }
    const auto id = GetPrimPath(dag);
    if (TfMapLookupPtr(_pathToAdapterMap, id) != nullptr) {
        return;
    }
    auto adapter = adapterCreator(this, dag);
    if (adapter == nullptr) { return; }
    if (!adapter->IsSupported()) { return; }
    adapter->Populate();
    adapter->CreateCallbacks();
    _pathToAdapterMap.insert({id, adapter});
}

void
HdMayaSceneDelegate::SetParams(const HdMayaParams& params) {
    const auto& oldParams = GetParams();
    if (oldParams.displaySmoothMeshes != params.displaySmoothMeshes) {
        // I couldn't find any other way to turn this on / off.
        // I can't convert HdRprim to HdMesh easily and no simple way
        // to get the type of the HdRprim from the render index.
        // If we want to allow creating multiple rprims and returning an id
        // to a subtree, we need to use the HasType function and the mark dirty
        // from each adapter.
        for (auto& it: _pathToAdapterMap) {
            if (it.second->HasType(HdPrimTypeTokens->mesh)) {
                it.second->MarkDirty(HdChangeTracker::DirtyTopology);
            }
        }
    }
    HdMayaDelegate::SetParams(params);
}

HdMeshTopology
HdMayaSceneDelegate::GetMeshTopology(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return {};
    }
    return adapter->GetMeshTopology();
}

GfRange3d
HdMayaSceneDelegate::GetExtent(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        return {};
    }
    return adapter->GetExtent();
}

GfMatrix4d
HdMayaSceneDelegate::GetTransform(const SdfPath& id) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        std::cerr << "[HdMayaSceneDelegate::GetTransform] No adapter for " << id << std::endl;
        return GfMatrix4d(1.0);
    }
    return adapter->GetTransform();
}

bool
HdMayaSceneDelegate::IsEnabled(const TfToken& option) const {
    // Maya scene can't be accessed on multiple threads,
    // so I don't think this is safe to enable.
    if (option == HdOptionTokens->parallelRprimSync) {
        return false;
    }

    std::cerr << "[HdMayaSceneDelegate::IsEnabled] Unsupported option " << option << std::endl;
    return false;
}

VtValue
HdMayaSceneDelegate::Get(SdfPath const& id, const TfToken& key) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        std::cerr << "[HdMayaSceneDelegate::Get] No adapter for " << key << " on " << id << std::endl;
        return {};
    }
    auto ret = adapter->Get(key);
    if (ret.IsEmpty()) {
        std::cerr << "[HdMayaSceneDelegate::Get] Failed for " << key << " on " << id << std::endl;
    }
    return ret;
}

HdPrimvarDescriptorVector
HdMayaSceneDelegate::GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        std::cerr << "[HdMayaSceneDelegate::GetPrimvarDescriptors] No adapter for " << interpolation << " on " << id << std::endl;
        return {};
    }
    return adapter->GetPrimvarDescriptors(interpolation);
}

VtValue
HdMayaSceneDelegate::GetLightParamValue(const SdfPath& id, const TfToken& paramName) {
    HdMayaDagAdapterPtr adapter;
    if (!TfMapLookup(_pathToAdapterMap, id, &adapter) || adapter == nullptr) {
        std::cerr << "[HdMayaSceneDelegate::GetLightParamValue] No adapter for " << paramName << " on " << id << std::endl;
        return {};
    }
    auto ret = adapter->GetLightParamValue(paramName);
    if (ret.IsEmpty()) {
        std::cerr << "[HdMayaSceneDelegate::GetLightParamValue] Failed for " << paramName << " on " << id << std::endl;
    }
    return ret;
}

bool
HdMayaSceneDelegate::GetVisible(const SdfPath& id) {
    return true;
}

bool
HdMayaSceneDelegate::GetDoubleSided(const SdfPath& id) {
    return true;
}

HdCullStyle
HdMayaSceneDelegate::GetCullStyle(const SdfPath& id) {
    // std::cerr << "[HdMayaSceneDelegate] Getting cull style of " << id << std::endl;
    return HdCullStyleDontCare;
}

// VtValue
// HdMayaSceneDelegate::GetShadingStyle(const SdfPath& id) {
//     std::cerr << "[HdMayaSceneDelegate] Getting shading style of " << id << std::endl;
//     return HdMayaSceneDelegate::GetShadingStyle(id);
// }

HdDisplayStyle
HdMayaSceneDelegate::GetDisplayStyle(const SdfPath& id) {
    // std::cerr << "[HdMayaSceneDelegate] Getting display style of " << id << std::endl;
    HdDisplayStyle style;
    style.flatShadingEnabled = false;
    style.displacementEnabled = false;
    return style;
}

// TfToken
// HdMayaSceneDelegate::GetReprName(const SdfPath& id) {
//     std::cerr << "[HdMayaSceneDelegate] Getting repr name of " << id << std::endl;
//     return HdTokens->hull;
// }

SdfPath
HdMayaSceneDelegate::GetMaterialId(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting material id of " << id << std::endl;
    return _fallbackMaterial;
}

std::string
HdMayaSceneDelegate::GetSurfaceShaderSource(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting surface shader source of " << id << std::endl;
    return {};
}

std::string
HdMayaSceneDelegate::GetDisplacementShaderSource(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting displacement shader source of " << id << std::endl;
    return {};
}

VtValue
HdMayaSceneDelegate::GetMaterialParamValue(const SdfPath& id, const TfToken& paramName) {
    std::cerr << "[HdMayaSceneDelegate] Getting material param value of " << id << std::endl;
    return {};
}

HdMaterialParamVector
HdMayaSceneDelegate::GetMaterialParams(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting material params of " << id << std::endl;
    return {};
}

VtValue
HdMayaSceneDelegate::GetMaterialResource(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting material resource of " << id << std::endl;
    return {};
}

TfTokenVector
HdMayaSceneDelegate::GetMaterialPrimvars(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting material primvars of " << id << std::endl;
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
