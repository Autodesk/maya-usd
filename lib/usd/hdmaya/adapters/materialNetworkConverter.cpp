//
// Copyright 2019 Luma Pictures
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
#include "materialNetworkConverter.h"

#include "adapterDebugCodes.h"
#include "materialAdapter.h"
#include "mayaAttrs.h"
#include "tokens.h"

#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

#include "../utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr float defaultTextureMemoryLimit = 1e8f;

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

class HdMayaGenericMaterialAttrConverter : public HdMayaMaterialAttrConverter {
public:
    /// Generic attr converter has no fixed type
    SdfValueTypeName GetType() override { return SdfValueTypeName(); }

    TfToken GetPlugName(const TfToken& usdName) override { return usdName; }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, fallback, outPlug);
    }
};

class HdMayaNewDefaultMaterialAttrConverter
    : public HdMayaMaterialAttrConverter {
public:
    template <typename T>
    HdMayaNewDefaultMaterialAttrConverter(const T& defaultValue)
        : _defaultValue(defaultValue) {}

    SdfValueTypeName GetType() override {
        return SdfGetValueTypeNameForValue(_defaultValue);
    }

    TfToken GetPlugName(const TfToken& usdName) override { return usdName; }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, &_defaultValue, outPlug);
    }

    const VtValue _defaultValue;
};

class HdMayaRemappingMaterialAttrConverter
    : public HdMayaMaterialAttrConverter {
public:
    HdMayaRemappingMaterialAttrConverter(
        const TfToken& remappedName, const SdfValueTypeName& type)
        : _remappedName(remappedName), _type(type) {}

    SdfValueTypeName GetType() override { return _type; }

    TfToken GetPlugName(const TfToken& usdName) override {
        return _remappedName;
    }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, _remappedName.GetText(), type, fallback, outPlug);
    }

private:
    const TfToken& _remappedName;
    const SdfValueTypeName& _type;
};

class HdMayaComputedMaterialAttrConverter : public HdMayaMaterialAttrConverter {
public:
    /// Classes which derive from this use some sort of calculation to get
    /// the right value for the node, and so don't have a single plug that
    /// can be hooked into a node network.
    TfToken GetPlugName(const TfToken& usdName) override { return TfToken(); }
};

class HdMayaFixedMaterialAttrConverter
    : public HdMayaComputedMaterialAttrConverter {
public:
    template <typename T>
    HdMayaFixedMaterialAttrConverter(const T& value) : _value(value) {}

    SdfValueTypeName GetType() override {
        return SdfGetValueTypeNameForValue(_value);
    }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        return _value;
    }

private:
    const VtValue _value;
};

class HdMayaCosinePowerMaterialAttrConverter
    : public HdMayaComputedMaterialAttrConverter {
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Float; }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        VtValue cosinePower =
            HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
                node, "cosinePower", type, nullptr);
        if (!cosinePower.IsHolding<float>()) {
            if (fallback) { return *fallback; }
            TF_DEBUG(HDMAYA_ADAPTER_GET)
                .Msg(
                    "HdMayaCosinePowerMaterialAttrConverter::GetValue(): "
                    "No float plug found with name: cosinePower and no "
                    "fallback given");
            return VtValue();
        } else {
            // In the maya UI, cosinePower goes from 2.0 to 100.0 ...
            // so for now, we just do a dumb linear mapping from that onto
            // 1 to 0 for roughness
            float roughnessFloat =
                1.0f - (cosinePower.UncheckedGet<float>() - 2.0f) / 98.0f;
            return VtValue(roughnessFloat);
        }
    }
};

class HdMayaFilenameMaterialAttrConverter
    : public HdMayaComputedMaterialAttrConverter {
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Asset; }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        auto path = GetTextureFilePath(node);
        return VtValue(SdfAssetPath(path.asChar(), path.asChar()));
    }
};

class HdMayaWrapMaterialAttrConverter
    : public HdMayaComputedMaterialAttrConverter {
public:
    HdMayaWrapMaterialAttrConverter(MObject& wrapAttr, MObject& mirrorAttr)
        : _wrapAttr(wrapAttr), _mirrorAttr(mirrorAttr) {}

    SdfValueTypeName GetType() override { return SdfValueTypeNames->Token; }

    VtValue GetValue(
        MFnDependencyNode& node, const TfToken& paramName,
        const SdfValueTypeName& type, const VtValue* fallback = nullptr,
        MPlug* outPlug = nullptr) override {
        if (node.findPlug(_wrapAttr, true).asBool()) {
            if (node.findPlug(_mirrorAttr, true).asBool()) {
                return VtValue(UsdHydraTokens->mirror);
            } else {
                return VtValue(UsdHydraTokens->repeat);
            }
        } else {
            return VtValue(UsdHydraTokens->clamp);
        }
    }

private:
    MObject _wrapAttr;
    MObject _mirrorAttr;
};

auto _genericAttrConverter =
    std::make_shared<HdMayaGenericMaterialAttrConverter>();

typedef std::unordered_map<
    TfToken, HdMayaMaterialNodeConverter, TfToken::HashFunctor>
    NameToNodeConverterMap;

NameToNodeConverterMap _nodeConverters;

} // namespace

/*static*/
void HdMayaMaterialNetworkConverter::initialize() {
    auto colorConverter =
        std::make_shared<HdMayaRemappingMaterialAttrConverter>(
            HdMayaAdapterTokens->color, SdfValueTypeNames->Vector3f);
    auto incandescenceConverter =
        std::make_shared<HdMayaRemappingMaterialAttrConverter>(
            HdMayaAdapterTokens->incandescence, SdfValueTypeNames->Vector3f);
    auto eccentricityConverter =
        std::make_shared<HdMayaRemappingMaterialAttrConverter>(
            HdMayaAdapterTokens->eccentricity, SdfValueTypeNames->Float);
    auto uvConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->uvCoord, SdfValueTypeNames->TexCoord2f);

    auto fixedZeroFloat =
        std::make_shared<HdMayaFixedMaterialAttrConverter>(0.0f);
    auto fixedOneFloat =
        std::make_shared<HdMayaFixedMaterialAttrConverter>(1.0f);
    auto fixedZeroInt = std::make_shared<HdMayaFixedMaterialAttrConverter>(0);
    auto fixedOneInt = std::make_shared<HdMayaFixedMaterialAttrConverter>(1);
    auto fixedStToken = std::make_shared<HdMayaFixedMaterialAttrConverter>(
        HdMayaAdapterTokens->st);

    auto cosinePowerToRoughness =
        std::make_shared<HdMayaCosinePowerMaterialAttrConverter>();
    auto filenameConverter =
        std::make_shared<HdMayaFilenameMaterialAttrConverter>();

    auto wrapUConverter = std::make_shared<HdMayaWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapU, MayaAttrs::file::mirrorU);
    auto wrapVConverter = std::make_shared<HdMayaWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapV, MayaAttrs::file::mirrorV);

    auto textureMemoryConverter =
        std::make_shared<HdMayaNewDefaultMaterialAttrConverter>(
            defaultTextureMemoryLimit);

    _nodeConverters = {
        {UsdImagingTokens->UsdPreviewSurface,
         {UsdImagingTokens->UsdPreviewSurface, {}}},
        {HdMayaAdapterTokens->pxrUsdPreviewSurface,
         {UsdImagingTokens->UsdPreviewSurface, {}}},
        {HdMayaAdapterTokens->lambert,
         {UsdImagingTokens->UsdPreviewSurface,
          {
              {HdMayaAdapterTokens->diffuseColor, colorConverter},
              {HdMayaAdapterTokens->emissiveColor, incandescenceConverter},
              {HdMayaAdapterTokens->roughness, fixedOneFloat},
              {HdMayaAdapterTokens->metallic, fixedZeroFloat},
              {HdMayaAdapterTokens->useSpecularWorkflow, fixedZeroInt},
          }}},
        {HdMayaAdapterTokens->blinn,
         {UsdImagingTokens->UsdPreviewSurface,
          {
              {HdMayaAdapterTokens->diffuseColor, colorConverter},
              {HdMayaAdapterTokens->emissiveColor, incandescenceConverter},
              {HdMayaAdapterTokens->roughness, eccentricityConverter},
              {HdMayaAdapterTokens->metallic, fixedZeroFloat},
              {HdMayaAdapterTokens->useSpecularWorkflow, fixedOneInt},
          }}},
        {HdMayaAdapterTokens->phong,
         {UsdImagingTokens->UsdPreviewSurface,
          {
              {HdMayaAdapterTokens->diffuseColor, colorConverter},
              {HdMayaAdapterTokens->emissiveColor, incandescenceConverter},
              {HdMayaAdapterTokens->roughness, cosinePowerToRoughness},
              {HdMayaAdapterTokens->metallic, fixedZeroFloat},
              {HdMayaAdapterTokens->useSpecularWorkflow, fixedOneInt},
          }}},
        {HdMayaAdapterTokens->file,
         {UsdImagingTokens->UsdUVTexture,
          {
              {HdMayaAdapterTokens->file, filenameConverter},
              {HdMayaAdapterTokens->st, uvConverter},
              {UsdHydraTokens->wrapS, wrapUConverter},
              {UsdHydraTokens->wrapT, wrapVConverter},
              {UsdHydraTokens->textureMemory, textureMemoryConverter},
          }}},
        {HdMayaAdapterTokens->place2dTexture,
         {UsdImagingTokens->UsdPrimvarReader_float2,
          {
              {HdMayaAdapterTokens->varname, fixedStToken},
          }}},
    };
}

HdMayaMaterialNodeConverter::HdMayaMaterialNodeConverter(
    const TfToken& identifier, const NameToAttrConverterMap& attrConverters)
    : _attrConverters(attrConverters), _identifier(identifier) {}

HdMayaMaterialAttrConverter::RefPtr
HdMayaMaterialNodeConverter::GetAttrConverter(const TfToken& paramName) {
    auto it = _attrConverters.find(paramName);
    if (it == _attrConverters.end()) { return _genericAttrConverter; }
    return it->second;
}

HdMayaMaterialNodeConverter* HdMayaMaterialNodeConverter::GetNodeConverter(
    const TfToken& nodeType) {
    auto it = _nodeConverters.find(nodeType);
    if (it == _nodeConverters.end()) { return nullptr; }
    return &(it->second);
}

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
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("HdMayaMaterialNetworkConverter::GetMaterial(node=%s)\n", chr);
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

    auto* nodeConverter = HdMayaMaterialNodeConverter::GetNodeConverter(
        TfToken(node.typeName().asChar()));
    if (!nodeConverter) { return SdfPath(); }
    HdMaterialNode material{};
    material.path = materialPath;
    material.identifier = nodeConverter->GetIdentifier();
    if (material.identifier == UsdImagingTokens->UsdPreviewSurface) {
        for (const auto& param : _previewShaderParams) {
            this->ConvertParameter(
                node, *nodeConverter, material, param.param.GetName(),
                param.type, &param.param.GetFallbackValue());
        }
    } else {
        for (auto& nameAttrConverterPair : nodeConverter->GetAttrConverters()) {
            auto& name = nameAttrConverterPair.first;
            auto& attrConverter = nameAttrConverterPair.second;
            this->ConvertParameter(
                node, *nodeConverter, material, name, attrConverter->GetType());

            if (name == HdMayaAdapterTokens->varname &&
                (material.identifier ==
                     UsdImagingTokens->UsdPrimvarReader_float ||
                 material.identifier ==
                     UsdImagingTokens->UsdPrimvarReader_float2 ||
                 material.identifier ==
                     UsdImagingTokens->UsdPrimvarReader_float3 ||
                 material.identifier ==
                     UsdImagingTokens->UsdPrimvarReader_float4)) {
                VtValue& primVarName = material.parameters[name];
                if (TF_VERIFY(primVarName.IsHolding<TfToken>())) {
                    AddPrimvar(primVarName.UncheckedGet<TfToken>());
                } else {
                    TF_WARN(
                        "Converter identified as a UsdPrimvarReader*, but "
                        "it's "
                        "varname did not hold a TfToken");
                }
            }
        }
    }
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
    MFnDependencyNode& node, HdMayaMaterialNodeConverter& nodeConverter,
    HdMaterialNode& material, const TfToken& paramName,
    const SdfValueTypeName& type, const VtValue* fallback) {
    MPlug plug;
    VtValue val;
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("ConvertParameter(%s)\n", paramName.GetText());

    auto attrConverter = nodeConverter.GetAttrConverter(paramName);
    if (attrConverter) {
        val = attrConverter->GetValue(node, paramName, type, fallback, &plug);
    } else if (fallback) {
        val = *fallback;
    } else {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "HdMayaMaterialNetworkConverter::ConvertParameter(): "
                "No attrConverter found with name: %s and no fallback "
                "given",
                paramName.GetText());
        val = VtValue();
    }

    material.parameters[paramName] = val;
    if (plug.isNull()) { return; }
    MPlugArray conns;
    plug.connectedTo(conns, true, false);
    if (conns.length() > 0) {
        const auto sourceNodePath = GetMaterial(conns[0].node());
        if (sourceNodePath.IsEmpty()) { return; }
        HdMaterialRelationship rel;
        rel.inputId = sourceNodePath;
        if (type == SdfValueTypeNames->Vector3f) {
            rel.inputName = HdMayaAdapterTokens->rgb;
        } else {
            rel.inputName = HdMayaAdapterTokens->result;
        }
        rel.outputId = material.path;
        rel.outputName = paramName;
        _network.relationships.push_back(rel);
    }
}

VtValue HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
    MFnDependencyNode& node, const MString& plugName,
    const SdfValueTypeName& type, const VtValue* fallback, MPlug* outPlug) {
    MStatus status;
    auto p = node.findPlug(plugName, true, &status);
    VtValue val;
    if (status) {
        if (outPlug) { *outPlug = p; }
        val = ConvertPlugToValue(p, type, fallback);
    } else if (fallback) {
        val = *fallback;
    } else {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(): "
                "No plug found with name: %s and no fallback given",
                plugName.asChar());
        val = VtValue();
    }
    return val;
}

VtValue HdMayaMaterialNetworkConverter::ConvertPlugToValue(
    const MPlug& plug, const SdfValueTypeName& type, const VtValue* fallback) {
    if (type == SdfValueTypeNames->Vector3f) {
        return VtValue(GfVec3f(
            plug.child(0).asFloat(), plug.child(1).asFloat(),
            plug.child(2).asFloat()));
    } else if (type == SdfValueTypeNames->Float) {
        return VtValue(plug.asFloat());
    } else if (
        type == SdfValueTypeNames->Float2 ||
        type == SdfValueTypeNames->TexCoord2f) {
        return VtValue(
            GfVec2f(plug.child(0).asFloat(), plug.child(1).asFloat()));
    } else if (type == SdfValueTypeNames->Int) {
        return VtValue(plug.asInt());
    }
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "HdMayaMaterialNetworkConverter::ConvertPlugToValue(): do not "
            "know "
            "how to handle type: %s (cpp type: %s)\n",
            type.GetAsToken().GetText(), type.GetCPPTypeName().c_str());
    if (fallback) { return *fallback; }
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
