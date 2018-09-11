//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <hdmaya/adapters/materialNetworkConverter.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/materialAdapter.h>
#include <hdmaya/adapters/mayaAttrs.h>

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
TF_DEFINE_PRIVATE_TOKENS(
    _tokens, (roughness)(clearcoat)(clearcoatRoughness)(emissiveColor)(
                 specularColor)(metallic)(useSpecularWorkflow)(occlusion)(ior)(
                 normal)(opacity)(diffuseColor)(displacement)
    // Supported material tokens.
    (lambert)(blinn)(file)(place2dTexture)
    // Other tokens
    (fileTextureName)(color)(incandescence)(out)(st)(uvCoord)(rgb)(r)(varname)(
        result)(eccentricity));

const auto _previewShaderParams = []() -> HdMayaShaderParams {
    // TODO: Use SdrRegistry, but it seems PreviewSurface is not there yet?
    HdMayaShaderParams ret = {
        {_tokens->roughness, VtValue(0.01f), SdfValueTypeNames->Float},
        {_tokens->clearcoat, VtValue(0.0f), SdfValueTypeNames->Float},
        {_tokens->clearcoatRoughness, VtValue(0.01f), SdfValueTypeNames->Float},
        {_tokens->emissiveColor, VtValue(GfVec3f(0.0f, 0.0f, 0.0f)),
         SdfValueTypeNames->Vector3f},
        {_tokens->specularColor, VtValue(GfVec3f(1.0f, 1.0f, 1.0f)),
         SdfValueTypeNames->Vector3f},
        {_tokens->metallic, VtValue(1.0f), SdfValueTypeNames->Float},
        {_tokens->useSpecularWorkflow, VtValue(0), SdfValueTypeNames->Int},
        {_tokens->occlusion, VtValue(1.0f), SdfValueTypeNames->Float},
        {_tokens->ior, VtValue(1.5f), SdfValueTypeNames->Float},
        {_tokens->normal, VtValue(GfVec3f(0.0f, 0.0f, 1.0f)),
         SdfValueTypeNames->Vector3f},
        {_tokens->opacity, VtValue(1.0f), SdfValueTypeNames->Float},
        {_tokens->diffuseColor, VtValue(GfVec3f(0.18f, 0.18f, 0.18f)),
         SdfValueTypeNames->Vector3f},
        {_tokens->displacement, VtValue(0.0f), SdfValueTypeNames->Float},
    };
    std::sort(
        ret.begin(), ret.end(),
        [](const HdMayaShaderParam& a, const HdMayaShaderParam& b) -> bool {
            return a._param.GetName() < b._param.GetName();
        });
    return ret;
}();

// This is required quite often, so we precalc.
const auto _previewMaterialParamVector = []() -> HdMaterialParamVector {
    HdMaterialParamVector ret;
    for (const auto& it : _previewShaderParams) { ret.emplace_back(it._param); }
    return ret;
}();

const MString _fileTextureName("fileTextureName");

void ConvertUsdPreviewSurface(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param._param.GetFallbackValue();
        converter.ConvertParameter(
            node, material, param._param.GetName(), param._param.GetName(),
            param._type, fallback);
    }
    material.identifier = UsdImagingTokens->UsdPreviewSurface;
}

void ConvertLambert(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param._param.GetFallbackValue();
        if (param._param.GetName() == _tokens->diffuseColor) {
            converter.ConvertParameter(
                node, material, _tokens->color, _tokens->diffuseColor,
                param._type, fallback);
        } else if (param._param.GetName() == _tokens->emissiveColor) {
            converter.ConvertParameter(
                node, material, _tokens->incandescence, _tokens->emissiveColor,
                param._type, fallback);
        } else {
            converter.ConvertParameter(
                node, material, param._param.GetName(), param._param.GetName(),
                param._type, fallback);
        }
    }
    material.identifier = UsdImagingTokens->UsdPreviewSurface;
}

void ConvertBlinn(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param._param.GetFallbackValue();
        if (param._param.GetName() == _tokens->diffuseColor) {
            converter.ConvertParameter(
                node, material, _tokens->color, _tokens->diffuseColor,
                param._type, fallback);
        } else if (param._param.GetName() == _tokens->emissiveColor) {
            converter.ConvertParameter(
                node, material, _tokens->incandescence, _tokens->emissiveColor,
                param._type, fallback);
        } else if (param._param.GetName() == _tokens->roughness) {
            converter.ConvertParameter(
                node, material, _tokens->eccentricity, _tokens->roughness,
                param._type, fallback);
        } else {
            converter.ConvertParameter(
                node, material, param._param.GetName(), param._param.GetName(),
                param._type, fallback);
        }
    }
    material.identifier = UsdImagingTokens->UsdPreviewSurface;
}

void ConvertFile(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    std::string fileTextureName{};
    if (node.findPlug(MayaAttrs::file::uvTilingMode).asShort() != 0) {
        fileTextureName = node.findPlug(MayaAttrs::file::fileTextureNamePattern)
                              .asString()
                              .asChar();
        if (fileTextureName.empty()) {
            fileTextureName =
                node.findPlug(MayaAttrs::file::computedFileTextureNamePattern)
                    .asString()
                    .asChar();
        }
    } else {
        fileTextureName = node.findPlug(_fileTextureName).asString().asChar();
    }
    material.parameters[_tokens->file] =
        VtValue(SdfAssetPath(fileTextureName, fileTextureName));
    converter.ConvertParameter(
        node, material, _tokens->uvCoord, _tokens->st,
        SdfValueTypeNames->Float2);
    material.identifier = UsdImagingTokens->UsdUVTexture;
}

void ConvertPlace2dTexture(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    converter.AddPrimvar(_tokens->st);
    material.parameters[_tokens->varname] = VtValue(_tokens->st);
    material.identifier = UsdImagingTokens->UsdPrimvarReader_float2;
}

std::unordered_map<
    TfToken,
    std::function<void(
        HdMayaMaterialNetworkConverter&, HdMaterialNode&, MFnDependencyNode&)>,
    TfToken::HashFunctor>
    _converters{
        {UsdImagingTokens->UsdPreviewSurface, ConvertUsdPreviewSurface},
        {_tokens->lambert, ConvertLambert},
        {_tokens->blinn, ConvertBlinn},
        {_tokens->file, ConvertFile},
        {_tokens->place2dTexture, ConvertPlace2dTexture},
    };

} // namespace

HdMayaMaterialNetworkConverter::HdMayaMaterialNetworkConverter(
    HdMaterialNetwork& network, const SdfPath& prefix)
    : _network(network), _prefix(prefix) {}

SdfPath HdMayaMaterialNetworkConverter::GetMaterial(const MObject& mayaNode) {
    MStatus status;
    MFnDependencyNode node(mayaNode, &status);
    if (ARCH_UNLIKELY(!status)) { return {}; }
    const auto* chr = node.name().asChar();
    if (chr == nullptr || chr[0] == '\0') { return {}; }
    std::string usdPathStr(chr);
    // replace namespace ":" with "_"
    std::replace(usdPathStr.begin(), usdPathStr.end(), ':', '_');
    const auto materialPath = _prefix.AppendPath(SdfPath(usdPathStr));

    if (std::find_if(
            _network.nodes.begin(), _network.nodes.end(),
            [&materialPath](const HdMaterialNode& m) -> bool {
                return m.path == materialPath;
            }) != _network.nodes.end()) {
        return materialPath;
    }

    const auto converterIt =
        _converters.find(TfToken(node.typeName().asChar()));
    if (converterIt == _converters.end()) { return SdfPath(); }
    HdMaterialNode material{};
    material.path = materialPath;
    converterIt->second(*this, material, node);
    _network.nodes.push_back(material);
    return materialPath;
}

void HdMayaMaterialNetworkConverter::AddPrimvar(const TfToken& primvar) {
    if (std::find(
            _network.primvars.begin(), _network.primvars.end(), primvar) ==
        _network.primvars.end()) {
        _network.primvars.push_back(primvar);
    }
}

void HdMayaMaterialNetworkConverter::ConvertParameter(
    MFnDependencyNode& node, HdMaterialNode& material, const TfToken& mayaName,
    const TfToken& name, const SdfValueTypeName& type,
    const VtValue* fallback) {
    MStatus status;
    auto p = node.findPlug(mayaName.GetText(), &status);
    VtValue val;
    if (status) {
        val = ConvertPlugToValue(p, type);
    } else if (fallback) {
        val = *fallback;
    } else {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "HdMayaMaterialNetworkConverter::ConvertParameter(): "
                "No plug found with name: %s and no fallback given",
                mayaName.GetText());
        val = VtValue();
    }
    material.parameters[name] = val;
    MPlugArray conns;
    p.connectedTo(conns, true, false);
    if (conns.length() > 0) {
        const auto connectedNodePath = GetMaterial(conns[0].node());
        if (connectedNodePath.IsEmpty()) { return; }
        HdMaterialRelationship rel;
        rel.inputId = connectedNodePath;
        if (type == SdfValueTypeNames->Vector3f) {
            rel.inputName = _tokens->rgb;
        } else {
            rel.inputName = _tokens->result;
        }
        rel.outputId = material.path;
        rel.outputName = name;
        _network.relationships.push_back(rel);
    }
}

VtValue HdMayaMaterialNetworkConverter::ConvertPlugToValue(
    const MPlug& plug, const SdfValueTypeName& type) {
    if (type == SdfValueTypeNames->Vector3f) {
        return VtValue(GfVec3f(
            plug.child(0).asFloat(), plug.child(1).asFloat(),
            plug.child(2).asFloat()));
    } else if (type == SdfValueTypeNames->Float) {
        return VtValue(plug.asFloat());
    } else if (type == SdfValueTypeNames->Int) {
        return VtValue(plug.asInt());
    }
    return {};
};

const HdMayaShaderParams&
HdMayaMaterialNetworkConverter::GetPreviewShaderParams() {
    return _previewShaderParams;
}

const HdMaterialParamVector&
HdMayaMaterialNetworkConverter::GetPreviewMaterialParamVector() {
    return _previewMaterialParamVector;
}

PXR_NAMESPACE_CLOSE_SCOPE
