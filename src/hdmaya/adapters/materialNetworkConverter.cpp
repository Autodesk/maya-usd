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
#include <hdmaya/adapters/tokens.h>

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const auto _previewShaderParams = []() -> HdMayaShaderParams {
    // TODO: Use SdrRegistry, but it seems PreviewSurface is not there yet?
    HdMayaShaderParams ret = {
        {HdMayaAdapterTokens->roughness, VtValue(0.01f),
         SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->clearcoat, VtValue(0.0f),
         SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->clearcoatRoughness, VtValue(0.01f),
         SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->emissiveColor, VtValue(GfVec3f(0.0f, 0.0f, 0.0f)),
         SdfValueTypeNames->Vector3f},
        {HdMayaAdapterTokens->specularColor, VtValue(GfVec3f(1.0f, 1.0f, 1.0f)),
         SdfValueTypeNames->Vector3f},
        {HdMayaAdapterTokens->metallic, VtValue(0.0f),
         SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->useSpecularWorkflow, VtValue(0),
         SdfValueTypeNames->Int},
        {HdMayaAdapterTokens->occlusion, VtValue(1.0f),
         SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->ior, VtValue(1.5f), SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->normal, VtValue(GfVec3f(0.0f, 0.0f, 1.0f)),
         SdfValueTypeNames->Vector3f},
        {HdMayaAdapterTokens->opacity, VtValue(1.0f), SdfValueTypeNames->Float},
        {HdMayaAdapterTokens->diffuseColor,
         VtValue(GfVec3f(0.18f, 0.18f, 0.18f)), SdfValueTypeNames->Vector3f},
        {HdMayaAdapterTokens->displacement, VtValue(0.0f),
         SdfValueTypeNames->Float},
    };
    std::sort(
        ret.begin(), ret.end(),
        [](const HdMayaShaderParam& a, const HdMayaShaderParam& b) -> bool {
            return a.param.GetName() < b.param.GetName();
        });
    return ret;
}();

// This is required quite often, so we precalc.
const auto _previewMaterialParamVector = []() -> HdMaterialParamVector {
    HdMaterialParamVector ret;
    for (const auto& it : _previewShaderParams) { ret.emplace_back(it.param); }
    return ret;
}();

const MString _fileTextureName("fileTextureName");

void ConvertUsdPreviewSurface(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param.param.GetFallbackValue();
        converter.ConvertParameter(
            node, material, param.param.GetName(), param.param.GetName(),
            param.type, fallback);
    }
    material.identifier = UsdImagingTokens->UsdPreviewSurface;
}

void ConvertLambert(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param.param.GetFallbackValue();
        if (param.param.GetName() == HdMayaAdapterTokens->diffuseColor) {
            converter.ConvertParameter(
                node, material, HdMayaAdapterTokens->color,
                HdMayaAdapterTokens->diffuseColor, param.type, fallback);
        } else if (
            param.param.GetName() == HdMayaAdapterTokens->emissiveColor) {
            converter.ConvertParameter(
                node, material, HdMayaAdapterTokens->incandescence,
                HdMayaAdapterTokens->emissiveColor, param.type, fallback);
        } else {
            converter.ConvertParameter(
                node, material, param.param.GetName(), param.param.GetName(),
                param.type, fallback);
        }
    }
    material.identifier = UsdImagingTokens->UsdPreviewSurface;
}

void ConvertBlinn(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    for (const auto& param : _previewShaderParams) {
        const VtValue* fallback = &param.param.GetFallbackValue();
        if (param.param.GetName() == HdMayaAdapterTokens->diffuseColor) {
            converter.ConvertParameter(
                node, material, HdMayaAdapterTokens->color,
                HdMayaAdapterTokens->diffuseColor, param.type, fallback);
        } else if (
            param.param.GetName() == HdMayaAdapterTokens->emissiveColor) {
            converter.ConvertParameter(
                node, material, HdMayaAdapterTokens->incandescence,
                HdMayaAdapterTokens->emissiveColor, param.type, fallback);
        } else if (param.param.GetName() == HdMayaAdapterTokens->roughness) {
            converter.ConvertParameter(
                node, material, HdMayaAdapterTokens->eccentricity,
                HdMayaAdapterTokens->roughness, param.type, fallback);
        } else {
            converter.ConvertParameter(
                node, material, param.param.GetName(), param.param.GetName(),
                param.type, fallback);
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
    material.parameters[HdMayaAdapterTokens->file] =
        VtValue(SdfAssetPath(fileTextureName, fileTextureName));
    converter.ConvertParameter(
        node, material, HdMayaAdapterTokens->uvCoord, HdMayaAdapterTokens->st,
        SdfValueTypeNames->Float2);
    material.identifier = UsdImagingTokens->UsdUVTexture;
}

void ConvertPlace2dTexture(
    HdMayaMaterialNetworkConverter& converter, HdMaterialNode& material,
    MFnDependencyNode& node) {
    converter.AddPrimvar(HdMayaAdapterTokens->st);
    material.parameters[HdMayaAdapterTokens->varname] =
        VtValue(HdMayaAdapterTokens->st);
    material.identifier = UsdImagingTokens->UsdPrimvarReader_float2;
}

std::unordered_map<
    TfToken,
    std::function<void(
        HdMayaMaterialNetworkConverter&, HdMaterialNode&, MFnDependencyNode&)>,
    TfToken::HashFunctor>
    _converters{
        {UsdImagingTokens->UsdPreviewSurface, ConvertUsdPreviewSurface},
        {HdMayaAdapterTokens->lambert, ConvertLambert},
        {HdMayaAdapterTokens->blinn, ConvertBlinn},
        {HdMayaAdapterTokens->file, ConvertFile},
        {HdMayaAdapterTokens->place2dTexture, ConvertPlace2dTexture},
    };

} // namespace

HdMayaShaderParam::HdMayaShaderParam(
    const TfToken& name, const VtValue& value, const SdfValueTypeName& type)
    : param(HdMaterialParam::ParamTypeFallback, name, value), type(type) {}

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
            rel.inputName = HdMayaAdapterTokens->rgb;
        } else {
            rel.inputName = HdMayaAdapterTokens->result;
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
