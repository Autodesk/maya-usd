#include <hdmaya/adapters/materialAdapter.h>

#include <pxr/base/tf/fileUtils.h>

#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hd/instanceRegistry.h>

#include <pxr/imaging/glf/glslfx.h>
#include <pxr/imaging/glf/textureRegistry.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <hdmaya/adapters/adapterRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue _emptyValue;
const TfToken _emptyToken;
const TfTokenVector _stSamplerCoords = {TfToken("st")};
// const TfTokenVector _stSamplerCoords;
const MString _fileTextureName("fileTextureName");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (roughness)
    (clearcoat)
    (clearcoatRoughness)
    (emissiveColor)
    (specularColor)
    (metallic)
    (useSpecularWorkflow)
    (occlusion)
    (ior)
    (normal)
    (opacity)
    (diffuseColor)
    (displacement)
    // Supported material tokens.
    (UsdPreviewSurface)
    (lambert)
    // Other tokens
    (fileTextureName)
);

struct _PreviewParam {
    HdMaterialParam _param;
    SdfValueTypeName _type;

    _PreviewParam(const TfToken& name, const VtValue& value, const SdfValueTypeName& type)
    : _param(HdMaterialParam::ParamTypeFallback, name, value), _type(type) {

    }
};

using _PreviewParams = std::vector<_PreviewParam>;

const _PreviewParams _previewShaderParams = [] () -> _PreviewParams {
    _PreviewParams ret = {
        {
            _tokens->roughness,
            VtValue(1.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->clearcoat,
            VtValue(0.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->clearcoatRoughness,
            VtValue(0.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->emissiveColor,
            VtValue(GfVec3f(0.0f, 0.0f, 0.0f)),
            SdfValueTypeNames->Vector3f
        }, {
            _tokens->specularColor,
            VtValue(GfVec3f(0.0f, 0.0f, 0.0f)),
            SdfValueTypeNames->Vector3f
        }, {
            _tokens->metallic,
            VtValue(0.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->useSpecularWorkflow,
            VtValue(1),
            SdfValueTypeNames->Int
        }, {
            _tokens->occlusion,
            VtValue(1.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->ior,
            VtValue(1.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->normal,
            VtValue(GfVec3f(1.0f, 1.0f, 1.0f)),
            SdfValueTypeNames->Vector3f,
        }, {
            _tokens->opacity,
            VtValue(1.0f),
            SdfValueTypeNames->Float
        }, {
            _tokens->diffuseColor,
            VtValue(GfVec3f(1.0, 1.0, 1.0)),
            SdfValueTypeNames->Vector3f
        }, {
            _tokens->displacement,
            VtValue(0.0f),
            SdfValueTypeNames->Float
        },
    };
    std::sort(ret.begin(), ret.end(), [](const _PreviewParam& a, const _PreviewParam& b) -> bool {
        return a._param.GetName() < b._param.GetName();
    });
    return ret;
} ();

// This is required quite often, so we precalc.
const HdMaterialParamVector _previewShaderParamVector = [] () -> HdMaterialParamVector {
    HdMaterialParamVector ret;
    for (const auto& it: _previewShaderParams) {
        ret.emplace_back(it._param);
    }
    return ret;
} ();

// Specialized version of : https://en.cppreference.com/w/cpp/algorithm/lower_bound
auto _FindPreviewParam = [] (const TfToken& id) -> _PreviewParams::const_iterator {
    _PreviewParams::const_iterator it;
    typename std::iterator_traits<_PreviewParams::const_iterator>::difference_type count, step;
    auto first = _previewShaderParams.cbegin();
    count = std::distance(first, _previewShaderParams.cend());

    while (count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if (it->_param.GetName() < id) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first != _previewShaderParams.cend() ?
       (first->_param.GetName() == id ? first : _previewShaderParams.cend()) :
       first;
};

static const std::pair<std::string, std::string> _previewShaderSource = []() -> std::pair<std::string, std::string> {
    GlfGLSLFX gfx(UsdImagingGLPackagePreviewSurfaceShader());
    return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
} ();

}

HdMayaMaterialAdapter::HdMayaMaterialAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& node)
    : HdMayaAdapter(node, id, delegate) {
}

bool
HdMayaMaterialAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool
HdMayaMaterialAdapter::HasType(const TfToken& typeId) {
    return typeId == HdPrimTypeTokens->material;
}

void
HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void
HdMayaMaterialAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
}


void
HdMayaMaterialAdapter::Populate() {
    GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
}

std::string
HdMayaMaterialAdapter::GetSurfaceShaderSource() {
    return GetPreviewSurfaceSource();
}

std::string
HdMayaMaterialAdapter::GetDisplacementShaderSource() {
    return GetPreviewDisplacementSource();
}

VtValue
HdMayaMaterialAdapter::GetMaterialParamValue(const TfToken& paramName) {
    return GetPreviewMaterialParamValue(paramName);
}

HdMaterialParamVector
HdMayaMaterialAdapter::GetMaterialParams() {
    return GetPreviewMaterialParams();
}

HdTextureResource::ID
HdMayaMaterialAdapter::GetTextureResourceID(const TfToken& paramName) {
    return {};
}

HdTextureResourceSharedPtr
HdMayaMaterialAdapter::GetTextureResource(const TfToken& paramName) {
    return {};
}

const HdMaterialParamVector&
HdMayaMaterialAdapter::GetPreviewMaterialParams() {
    return _previewShaderParamVector;
}

const std::string&
HdMayaMaterialAdapter::GetPreviewSurfaceSource() {
    return _previewShaderSource.first;
}

const std::string&
HdMayaMaterialAdapter::GetPreviewDisplacementSource() {
    return _previewShaderSource.second;
}

const VtValue&
HdMayaMaterialAdapter::GetPreviewMaterialParamValue(const TfToken& paramName) {
    const auto it = _FindPreviewParam(paramName);
    if (ARCH_UNLIKELY(it == _previewShaderParams.cend())) {
        TF_CODING_ERROR("Incorrect name passed to GetMaterialParamValue: %s", paramName.GetText());
        return _emptyValue;
    }
    return it->_param.GetFallbackValue();
}

class HdMayaShadingEngineAdapter : public HdMayaMaterialAdapter {
public:
    HdMayaShadingEngineAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj)
        : HdMayaMaterialAdapter(id, delegate, obj),
          _surfaceShaderCallback(0) {
        _CacheNodeAndTypes();
    }

    ~HdMayaShadingEngineAdapter() override {
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
        }
    }

    void
    CreateCallbacks() override {
        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) { AddCallback(id); }
        _CreateSurfaceMaterialCallback();
        HdMayaAdapter::CreateCallbacks();
    }
private:
    static void
    _DirtyMaterialParams(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->_CreateSurfaceMaterialCallback();
        adapter->MarkDirty(HdMaterial::DirtyParams | HdMaterial::DirtySurfaceShader);
    }

    static void
    _DirtyShaderParams(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->MarkDirty(HdMaterial::DirtyParams | HdMaterial::DirtySurfaceShader);
    }

    void
    _CacheNodeAndTypes() {
        _surfaceShader = MObject::kNullObj;
        _surfaceShaderType = _emptyToken;
        MStatus status;
        MFnDependencyNode node(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) { return; }

        auto p = node.findPlug("surfaceShader");
        MPlugArray conns;
        p.connectedTo(conns, true, false);
        if (conns.length() > 0) {
            _surfaceShader = conns[0].node();
            MFnDependencyNode surfaceNode(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) { return; }
            _surfaceShaderType = TfToken(surfaceNode.typeName().asChar());
        }
    }

    HdMaterialParamVector GetMaterialParams() override {
        if (_surfaceShaderType != _tokens->UsdPreviewSurface) {
            return GetPreviewMaterialParams();
        }

        MStatus status;
        MFnDependencyNode node(_surfaceShader, &status);
        if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParams(); }
        HdMaterialParamVector ret;
        ret.reserve(_previewShaderParamVector.size());
        for (const auto& it: _previewShaderParamVector) {
            const auto connectedFileObj = GetConnectedFileNode(node, it.GetName());
            if (connectedFileObj != MObject::kNullObj) {
                const auto filePath = _GetFilePath(MFnDependencyNode(connectedFileObj));
                auto textureId = _GetTextureResourceID(filePath);
                if (textureId != HdTextureResource::ID(-1)) {
                    const auto& resourceRegistry = GetDelegate()->GetRenderIndex().GetResourceRegistry();
                    HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> textureInstance;
                    auto regLock = resourceRegistry->RegisterTextureResource(textureId, &textureInstance);
                    if (textureInstance.IsFirstInstance()) {
                        auto textureResource = _GetTextureResource(filePath);
                        _textureResources[it.GetName()] = textureResource;
                        textureInstance.SetValue(textureResource);
                    } else {
                        _textureResources[it.GetName()] = textureInstance.GetValue();
                    }
                    ret.emplace_back(
                        HdMaterialParam::ParamTypeTexture,
                        it.GetName(),
                        it.GetFallbackValue(),
                        GetID().AppendProperty(it.GetName()),
                        _stSamplerCoords);
                    continue;
                } else {
                    _textureResources[it.GetName()].reset();
                }
            }
            ret.emplace_back(it);
        }

        return ret;
    }

    VtValue
    GetMaterialParamValue(const TfToken& paramName) override {
        if (ARCH_UNLIKELY(_surfaceShaderType.IsEmpty())) {
            return GetPreviewMaterialParamValue(paramName);
        }

        auto convertPlugToValue = [] (const MPlug& plug, const SdfValueTypeName& type) -> VtValue {
            if (type == SdfValueTypeNames->Vector3f) {
                return VtValue(GfVec3f(
                    plug.child(0).asFloat(),
                    plug.child(1).asFloat(),
                    plug.child(2).asFloat()));
            } else if (type == SdfValueTypeNames->Float) {
                return VtValue(plug.asFloat());
            } else if (type == SdfValueTypeNames->Int) {
                return VtValue(plug.asInt());
            }
            return {};
        };

        if (_surfaceShaderType == _tokens->UsdPreviewSurface) {
            MStatus status;
            MFnDependencyNode node(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParamValue(paramName); }
            auto p = node.findPlug(paramName.GetText());
            if (ARCH_UNLIKELY(p.isNull())) { return GetPreviewMaterialParamValue(paramName); }
            auto it = _FindPreviewParam(paramName);
            if (ARCH_UNLIKELY(it == _previewShaderParams.cend())) { return GetPreviewMaterialParamValue(paramName); }
            auto ret = convertPlugToValue(p, it->_type);
            if (ARCH_UNLIKELY(ret.IsEmpty())) { return GetPreviewMaterialParamValue(paramName); }
            return ret;
        } else if (_surfaceShaderType == _tokens->lambert) {
            MStatus status;
            MFnDependencyNode node(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParamValue(paramName); }
            if (paramName == _tokens->diffuseColor) {
                return convertPlugToValue(node.findPlug("color"), SdfValueTypeNames->Vector3f);
            } else if (paramName == _tokens->emissiveColor) {
                return convertPlugToValue(node.findPlug("incandescence"), SdfValueTypeNames->Vector3f);
            }
        }

        return GetPreviewMaterialParamValue(paramName);
    }

    void _CreateSurfaceMaterialCallback() {
        _CacheNodeAndTypes();
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
            _surfaceShaderCallback = 0;
        }

        if (_surfaceShader != MObject::kNullObj) {
            _surfaceShaderCallback = MNodeMessage::addNodeDirtyCallback(_surfaceShader, _DirtyShaderParams, this);
        }
    }

    HdTextureResource::ID
    GetTextureResourceID(const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return HdTextureResource::ID(-1); }
        return _GetTextureResourceID(
            _GetFilePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResource::ID
    _GetTextureResourceID(const TfToken& filePath) {
        size_t hash = filePath.Hash();
        boost::hash_combine(hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        return HdTextureResource::ID(hash);
    }

    inline TfToken
    _GetFilePath(const MFnDependencyNode& fileNode) {
        return TfToken(fileNode.findPlug(_fileTextureName).asString().asChar());
    }

    HdTextureResourceSharedPtr
    GetTextureResource(const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return {}; }
        return _GetTextureResource(_GetFilePath(MFnDependencyNode(fileObj)));

    }

    inline HdTextureResourceSharedPtr
    _GetTextureResource(const TfToken& filePath){
        if (filePath.IsEmpty() || !TfPathExists(filePath)) { return {}; }
        // TODO: handle origin
        auto texture = GlfTextureRegistry::GetInstance().GetTextureHandle(filePath);
        // We can't really mimic texture wrapping and mirroring settings from
        // the uv placement node, so we don't touch those for now.
        return HdTextureResourceSharedPtr(
            new HdStSimpleTextureResource(
                texture, false, false, HdWrapClamp, HdWrapClamp,
                HdMinFilterLinearMipmapLinear, HdMagFilterLinear,
                GetDelegate()->GetParams().textureMemoryPerTexture)
        );
    }

    MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName) {
        MStatus status;
        MFnDependencyNode node(obj, &status);
        if (ARCH_UNLIKELY(!status)) { return MObject::kNullObj; }
        return GetConnectedFileNode(node, paramName);
    }

    MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName) {
        MPlugArray conns;
        node.findPlug(paramName.GetText()).connectedTo(conns, true, false);
        if (conns.length() == 0) { return MObject::kNullObj; }
        const auto ret = conns[0].node();
        if (ret.apiType() == MFn::kFileTexture) {
            return ret;
        }
        return MObject::kNullObj;
    }

    MObject _surfaceShader;
    TfToken _surfaceShaderType;
    MCallbackId _surfaceShaderCallback;
    // So they live long enough
    std::unordered_map<TfToken, HdTextureResourceSharedPtr, TfToken::HashFunctor> _textureResources;
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaMaterialAdapter, TfType::Bases<HdMayaAdapter> >();
    TfType::Define<HdMayaShadingEngineAdapter, TfType::Bases<HdMayaMaterialAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, shadingEngine) {
    HdMayaAdapterRegistry::RegisterMaterialAdapter(
        TfToken("shadingEngine"),
        [](const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj) -> HdMayaMaterialAdapterPtr {
            return HdMayaMaterialAdapterPtr(new HdMayaShadingEngineAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
