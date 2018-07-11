#include <hdmaya/delegates/sceneDelegate.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/tf/type.h>

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/sdf/assetPath.h>

#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

#include <pxr/imaging/glf/glslfx.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

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

const HdMaterialParamVector _defaultShaderParams = {
    {
        HdMaterialParam::ParamTypeFallback,
        TfToken("roughness"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("clearcoat"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("clearcoatRoughness"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("emissiveColor"),
        VtValue(GfVec3f(0.0f, 0.0f, 0.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("specularColor"),
        VtValue(GfVec3f(0.0f, 0.0f, 0.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("metallic"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("useSpecularWorkflow"),
        VtValue(1)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("occlusion"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("ior"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("normal"),
        VtValue(GfVec3f(1.0f, 1.0f, 1.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("opacity"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("diffuseColor"),
        VtValue(GfVec3f(1.0, 1.0, 1.0))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("displacement"),
        VtValue(0.0f)
    },
};

template <typename T> inline
void _FindAdapter(const SdfPath&, const std::function<void(T*)>&) {
    // Do nothing.
}

template <typename T, typename M0, typename... M> inline
void _FindAdapter(
    const SdfPath& id,
    const std::function<void(T*)>& f,
    const M0& m0,
    const M& ...m) {
    auto* adapterPtr = TfMapLookupPtr(m0, id);
    if (adapterPtr == nullptr) {
        _FindAdapter<T>(id, f, m...);
    } else {
        f(static_cast<T*>(adapterPtr->get()));
    }
}

// This will be nicer to use with automatic parameter deduction for lambdas in C++14.
template <typename T, typename R> inline
R _GetValue(const SdfPath&, const std::function<R(T*)>&) {
    return {};
}

template <typename T, typename R, typename M0, typename... M> inline
R _GetValue(
    const SdfPath& id,
    const std::function<R(T*)>& f,
    const M0& m0,
    const M& ...m) {
    auto* adapterPtr = TfMapLookupPtr(m0, id);
    if (adapterPtr == nullptr) {
        return _GetValue<T, R>(id, f, m...);
    } else {
        return f(static_cast<T*>(adapterPtr->get()));
    }
}

template <typename T> inline
void _MapAdapter(const std::function<void(T*)>&) {
    // Do nothing.
}

template <typename T, typename M0, typename... M> inline
void _MapAdapter(
    const std::function<void(T*)>& f,
    const M0& m0,
    const M& ...m) {
    for (auto& it : m0) {
        f(static_cast<T*>(it.second->get()));
    }
    _MapAdapter<T>(f, m...);
}

}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdMayaSceneDelegate)
    ((FallbackMaterial, "__fallback_material__"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaSceneDelegate, TfType::Bases<HdMayaDelegate> >();
}

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
    HdMayaAdapterRegistry::LoadAllPlugin();
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
    // FIXME: Improve this function!
    HdMayaShapeAdapterPtr adapter;
    if (TfMapLookup(_shapeAdapters, id, &adapter) && adapter != nullptr) {
        adapter->RemovePrim();
        _shapeAdapters.erase(id);
        return;
    }

    HdMayaLightAdapterPtr lightAdapter;
    if (TfMapLookup(_lightAdapters, id, &lightAdapter) && lightAdapter != nullptr) {
        lightAdapter->RemovePrim();
        _lightAdapters.erase(id);
        return;
    }

    HdMayaMaterialAdapterPtr materialAdapter;
    if (TfMapLookup(_materialAdapters, id, &materialAdapter) && materialAdapter != nullptr) {
        materialAdapter->RemovePrim();
        _materialAdapters.erase(id);
    }
}

void
HdMayaSceneDelegate::InsertDag(const MDagPath& dag) {
    // We don't care about transforms.
    if (dag.hasFn(MFn::kTransform)) { return; }

    // FIXME: put this into a function!
    if (dag.hasFn(MFn::kLight)) {
        auto adapterCreator = HdMayaAdapterRegistry::GetLightAdapterCreator(dag);
        if (adapterCreator == nullptr) { return; }
        const auto id = GetPrimPath(dag);
        if (TfMapLookupPtr(_lightAdapters, id) != nullptr) {
            return;
        }
        auto adapter = adapterCreator(this, dag);
        if (adapter == nullptr) { return; }
        if (!adapter->IsSupported()) { return; }
        adapter->Populate();
        adapter->CreateCallbacks();
        _lightAdapters.insert({id, adapter});
    } else {
        auto adapterCreator = HdMayaAdapterRegistry::GetShapeAdapterCreator(dag);
        if (adapterCreator == nullptr) { return; }
        const auto id = GetPrimPath(dag);
        if (TfMapLookupPtr(_shapeAdapters, id) != nullptr) {
            return;
        }
        auto adapter = adapterCreator(this, dag);
        if (adapter == nullptr) { return; }
        if (!adapter->IsSupported()) { return; }
        adapter->Populate();
        adapter->CreateCallbacks();
        _shapeAdapters.insert({id, adapter});
    }
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
        _MapAdapter<HdMayaDagAdapter>(
            [] (HdMayaDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            }
        );
    }
    HdMayaDelegate::SetParams(params);
}

HdMeshTopology
HdMayaSceneDelegate::GetMeshTopology(const SdfPath& id) {
    return _GetValue<HdMayaShapeAdapter, HdMeshTopology>(
        id,
        [](HdMayaShapeAdapter* a) -> HdMeshTopology { return a->GetMeshTopology(); },
        _shapeAdapters);
}

GfRange3d
HdMayaSceneDelegate::GetExtent(const SdfPath& id) {
    return _GetValue<HdMayaShapeAdapter, GfRange3d>(
        id,
        [](HdMayaShapeAdapter* a) -> GfRange3d { return a->GetExtent(); },
        _shapeAdapters);
}

GfMatrix4d
HdMayaSceneDelegate::GetTransform(const SdfPath& id) {
    return _GetValue<HdMayaDagAdapter, GfMatrix4d>(
        id,
        [](HdMayaDagAdapter* a) -> GfMatrix4d { return a->GetTransform(); },
        _shapeAdapters, _lightAdapters);
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
    return _GetValue<HdMayaAdapter, VtValue>(
        id,
        [&key](HdMayaAdapter* a) -> VtValue { return a->Get(key); },
        _shapeAdapters, _lightAdapters, _materialAdapters);
}

HdPrimvarDescriptorVector
HdMayaSceneDelegate::GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation) {
    return _GetValue<HdMayaShapeAdapter, HdPrimvarDescriptorVector>(
        id,
        [&interpolation](HdMayaShapeAdapter* a) -> HdPrimvarDescriptorVector { return a->GetPrimvarDescriptors(interpolation); },
        _shapeAdapters);
}

VtValue
HdMayaSceneDelegate::GetLightParamValue(const SdfPath& id, const TfToken& paramName) {
    return _GetValue<HdMayaLightAdapter, VtValue>(
        id,
        [&paramName](HdMayaLightAdapter* a) -> VtValue { return a->GetLightParamValue(paramName); },
        _lightAdapters);
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
    return _fallbackMaterial;
}

std::string
HdMayaSceneDelegate::GetSurfaceShaderSource(const SdfPath& id) {
    if (id == _fallbackMaterial) {
        GlfGLSLFX gfx(UsdImagingGLPackagePreviewSurfaceShader());
        return gfx.GetSurfaceSource();
    }
    std::cerr << "[HdMayaSceneDelegate] Getting surface shader source of " << id << std::endl;
    return {};
}

std::string
HdMayaSceneDelegate::GetDisplacementShaderSource(const SdfPath& id) {
    if (id == _fallbackMaterial) {
        GlfGLSLFX gfx(UsdImagingGLPackagePreviewSurfaceShader());
        return gfx.GetDisplacementSource();
    }
    std::cerr << "[HdMayaSceneDelegate] Getting displacement shader source of " << id << std::endl;
    return {};
}

VtValue
HdMayaSceneDelegate::GetMaterialParamValue(const SdfPath& id, const TfToken& paramName) {
    if (id == _fallbackMaterial) {
        auto it = std::find_if(
            _defaultShaderParams.begin(), _defaultShaderParams.end(),
            [paramName] (const HdMaterialParam& param) -> bool
            {
                return param.GetName() == paramName;
            });
        if (ARCH_UNLIKELY(it == _defaultShaderParams.end())) {
            TF_CODING_ERROR("Incorrect name passed to GetMaterialParamValue: %s", paramName.GetText());
            return {};
        }
        return it->GetFallbackValue();
    }
    std::cerr << "[HdMayaSceneDelegate] Getting material param value of " << id << std::endl;
    return {};
}

HdMaterialParamVector
HdMayaSceneDelegate::GetMaterialParams(const SdfPath& id) {
    if (id == _fallbackMaterial) {
        return _defaultShaderParams;
    }
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
