//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "materialNetworkConverter.h"

#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/materialAdapter.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/adapters/tokens.h>
#include <hdMaya/utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr float defaultTextureMemoryLimit = 1e8f;

// Lists of preferred shader output names, from SdfValueTypeName to list of
// preferred output names for that type.  The list that has an empty token for
// SdfValueTypeName is used as a default.
const std::vector<std::pair<SdfValueTypeName, std::vector<TfToken>>> preferredOutputNamesByType {
    { SdfValueTypeNames->Float3,
      { HdMayaAdapterTokens->result,
        HdMayaAdapterTokens->out,
        HdMayaAdapterTokens->output,
        HdMayaAdapterTokens->rgb,
        HdMayaAdapterTokens->xyz } },
    { SdfValueTypeNames->Float2,
      { HdMayaAdapterTokens->result,
        HdMayaAdapterTokens->out,
        HdMayaAdapterTokens->output,
        HdMayaAdapterTokens->st,
        HdMayaAdapterTokens->uv } },
    { SdfValueTypeNames->Float,
      { HdMayaAdapterTokens->result,
        HdMayaAdapterTokens->out,
        HdMayaAdapterTokens->output,
        HdMayaAdapterTokens->r,
        HdMayaAdapterTokens->x } }
};

// Default set of preferred output names, if type not in
// preferredOutputNamesByType
const std::vector<TfToken> defaultPreferredOutputNames { HdMayaAdapterTokens->result,
                                                         HdMayaAdapterTokens->out,
                                                         HdMayaAdapterTokens->output };

SdfValueTypeName GetStandardTypeName(SdfValueTypeName type)
{
    // Will map, ie, Vector3f to Float3, TexCoord2f to Float2
    return SdfGetValueTypeNameForValue(type.GetDefaultValue());
}

const std::vector<TfToken>&
GetPreferredOutputNames(SdfValueTypeName type, bool useStandardType = true)
{
    for (const auto& typeAndNames : preferredOutputNamesByType) {
        if (typeAndNames.first == type) {
            return typeAndNames.second;
        }
    }

    if (useStandardType) {
        // If we were given, ie, Vector3f, check to see if there's an entry for
        // Float3
        auto standardType = GetStandardTypeName(type);
        if (type != standardType) {
            return GetPreferredOutputNames(standardType, false);
        }
    }
    return defaultPreferredOutputNames;
}

TfToken GetOutputName(const HdMaterialNode& material, SdfValueTypeName type)
{
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg(
            "GetOutputName(%s - %s, %s)\n",
            material.path.GetText(),
            material.identifier.GetText(),
            type.GetAsToken().GetText());
    auto& shaderReg = SdrRegistry::GetInstance();
    if (SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifier(material.identifier)) {
        // First, get the list off all outputs of the correct type.
        std::vector<TfToken> validOutputs;
        auto                 outputNames = sdrNode->GetOutputNames();

        auto addMatchingOutputs = [&](SdfValueTypeName matchingType) {
            for (const auto& outName : outputNames) {
                auto* sdrInfo = sdrNode->GetShaderOutput(outName);
                if (sdrInfo && sdrInfo->GetTypeAsSdfType().first == matchingType) {
                    validOutputs.push_back(outName);
                }
            }
        };

        addMatchingOutputs(type);
        if (validOutputs.empty()) {
            auto standardType = GetStandardTypeName(type);
            if (standardType != type) {
                addMatchingOutputs(standardType);
            }
        }

        // If there's only one, use that
        if (validOutputs.size() == 1) {
            TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                .Msg(
                    "  found exactly one output of correct type in "
                    "registry: "
                    "%s\n",
                    validOutputs[0].GetText());
            return validOutputs[0];
        }

        // Then see if any preferred names are found
        if (!validOutputs.empty()) {
            const auto& preferredNames = GetPreferredOutputNames(type);
            for (const auto& preferredName : preferredNames) {
                if (std::find(validOutputs.begin(), validOutputs.end(), preferredName)
                    != validOutputs.end()) {
                    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                        .Msg(
                            "  found preferred name of correct type in "
                            "registry: %s\n",
                            preferredName.GetText());
                    return preferredName;
                }
            }
            // No preferred names were found, use the first valid name
            TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                .Msg(
                    "  found no preferred names of correct type in "
                    "registry, returning first valid name: %s\n",
                    validOutputs[0].GetText());
            return validOutputs[0];
        }
    }

    // We either couldn't find the entry in the SdrRegistry, or there were
    // no outputs of the right type - make a guess, use the first preferred
    // name
    const auto& preferredNames = GetPreferredOutputNames(type);
    if (TF_VERIFY(!preferredNames.empty())) {
        TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
            .Msg(
                "  found no valid entries in registry, returning guess: "
                "%s\n",
                preferredNames[0].GetText());
        return preferredNames[0];
    }

    // We should never get here - preferredNames should never be empty!
    return HdMayaAdapterTokens->result;
}

class HdMayaGenericMaterialAttrConverter : public HdMayaMaterialAttrConverter
{
public:
    /// Generic attr converter has no fixed type
    SdfValueTypeName GetType() override { return SdfValueTypeName(); }

    TfToken GetPlugName(const TfToken& usdName) override { return usdName; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, fallback, outPlug);
    }
};

class HdMayaNewDefaultMaterialAttrConverter : public HdMayaMaterialAttrConverter
{
public:
    template <typename T>
    HdMayaNewDefaultMaterialAttrConverter(const T& defaultValue)
        : _defaultValue(defaultValue)
    {
    }

    SdfValueTypeName GetType() override { return SdfGetValueTypeNameForValue(_defaultValue); }

    TfToken GetPlugName(const TfToken& usdName) override { return usdName; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, &_defaultValue, outPlug);
    }

    const VtValue _defaultValue;
};

class HdMayaRemappingMaterialAttrConverter : public HdMayaMaterialAttrConverter
{
public:
    HdMayaRemappingMaterialAttrConverter(const TfToken& remappedName, const SdfValueTypeName& type)
        : _remappedName(remappedName)
        , _type(type)
    {
    }

    SdfValueTypeName GetType() override { return _type; }

    TfToken GetPlugName(const TfToken& usdName) override { return _remappedName; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, _remappedName.GetText(), type, fallback, outPlug);
    }

protected:
    const TfToken&          _remappedName;
    const SdfValueTypeName& _type;
};

class HdMayaScaledRemappingMaterialAttrConverter : public HdMayaRemappingMaterialAttrConverter
{
public:
    HdMayaScaledRemappingMaterialAttrConverter(
        const TfToken&          remappedName,
        const TfToken&          scaleName,
        const SdfValueTypeName& type)
        : HdMayaRemappingMaterialAttrConverter(remappedName, type)
        , _scaleName(scaleName)
    {
    }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        return HdMayaMaterialNetworkConverter::ConvertMayaAttrToScaledValue(
            node, _remappedName.GetText(), _scaleName.GetText(), type, fallback, outPlug);
    }

private:
    const TfToken& _scaleName;
};

class HdMayaComputedMaterialAttrConverter : public HdMayaMaterialAttrConverter
{
public:
    /// Classes which derive from this use some sort of calculation to get
    /// the right value for the node, and so don't have a single plug that
    /// can be hooked into a node network.
    TfToken GetPlugName(const TfToken& usdName) override { return TfToken(); }
};

class HdMayaFixedMaterialAttrConverter : public HdMayaComputedMaterialAttrConverter
{
public:
    template <typename T>
    HdMayaFixedMaterialAttrConverter(const T& value)
        : _value(value)
    {
    }

    SdfValueTypeName GetType() override { return SdfGetValueTypeNameForValue(_value); }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        return _value;
    }

private:
    const VtValue _value;
};

class HdMayaUvAttrConverter : public HdMayaMaterialAttrConverter
{
public:
    HdMayaUvAttrConverter()
        : _value(GfVec2f(0.0f, 0.0f))
    {
    }

    SdfValueTypeName GetType() override { return SdfValueTypeNames->TexCoord2f; }

    TfToken GetPlugName(const TfToken& usdName) override { return HdMayaAdapterTokens->uvCoord; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        if (outPlug) {
            // TODO: create a UsdPrimvarReader_float2 even if there's no
            // connected maya place2dTexture node

            // Find a connected place2dTexture node, and set that as the
            // outPlug, so that the place2dTexture node will trigger
            // creation of a UsdPrimvarReader_float2
            MStatus    status;
            MPlugArray connections;
            status = node.getConnections(connections);
            if (status) {
                for (size_t i = 0, len = connections.length(); i < len; ++i) {
                    MPlug source = connections[i].source();
                    if (source.isNull()) {
                        continue;
                    }
                    if (source.node().hasFn(MFn::kPlace2dTexture)) {
                        *outPlug = connections[i];
                        break;
                    }
                }
            }
        }
        return _value;
    }

private:
    const VtValue _value;
}; // namespace

class HdMayaCosinePowerMaterialAttrConverter : public HdMayaComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Float; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        VtValue cosinePower = HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "cosinePower", type, nullptr);
        if (!cosinePower.IsHolding<float>()) {
            if (fallback) {
                return *fallback;
            }
            TF_DEBUG(HDMAYA_ADAPTER_GET)
                .Msg("HdMayaCosinePowerMaterialAttrConverter::GetValue(): "
                     "No float plug found with name: cosinePower and no "
                     "fallback given");
            return VtValue();
        } else {
            // In the maya UI, cosinePower goes from 2.0 to 100.0 ...
            // so for now, we just do a dumb linear mapping from that onto
            // 1 to 0 for roughness
            float roughnessFloat = 1.0f - (cosinePower.UncheckedGet<float>() - 2.0f) / 98.0f;
            return VtValue(roughnessFloat);
        }
    }
};

class HdMayaTransmissionMaterialAttrConverter : public HdMayaComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Float; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        VtValue transmission = HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "transmission", type, nullptr);
        if (!transmission.IsHolding<float>()) {
            if (fallback) {
                return *fallback;
            }
            TF_DEBUG(HDMAYA_ADAPTER_GET)
                .Msg("HdMayaTransmissionMaterialAttrConverter::GetValue(): "
                     "No float plug found with name: transmission and no "
                     "fallback given");
            return VtValue();
        } else {
            return VtValue(1.0f - transmission.UncheckedGet<float>());
        }
    }
};

class HdMayaFilenameMaterialAttrConverter : public HdMayaComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Asset; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
        auto path = GetFileTexturePath(node);
        return VtValue(SdfAssetPath(path.GetText(), path.GetText()));
    }
};

class HdMayaWrapMaterialAttrConverter : public HdMayaComputedMaterialAttrConverter
{
public:
    HdMayaWrapMaterialAttrConverter(MObject& wrapAttr, MObject& mirrorAttr)
        : _wrapAttr(wrapAttr)
        , _mirrorAttr(mirrorAttr)
    {
    }

    SdfValueTypeName GetType() override { return SdfValueTypeNames->Token; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlug*                  outPlug = nullptr) override
    {
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

auto _genericAttrConverter = std::make_shared<HdMayaGenericMaterialAttrConverter>();

typedef std::unordered_map<TfToken, HdMayaMaterialNodeConverter, TfToken::HashFunctor>
    NameToNodeConverterMap;

NameToNodeConverterMap _nodeConverters;

} // namespace

/*static*/
void HdMayaMaterialNetworkConverter::initialize()
{
    auto colorConverter = std::make_shared<HdMayaScaledRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->color, HdMayaAdapterTokens->diffuse, SdfValueTypeNames->Vector3f);
    auto incandescenceConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->incandescence, SdfValueTypeNames->Vector3f);
    auto eccentricityConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->eccentricity, SdfValueTypeNames->Float);
    auto uvConverter = std::make_shared<HdMayaUvAttrConverter>();

    // Standard surface:
    auto baseColorConverter = std::make_shared<HdMayaScaledRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->baseColor, HdMayaAdapterTokens->base, SdfValueTypeNames->Vector3f);
    auto emissionColorConverter = std::make_shared<HdMayaScaledRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->emissionColor,
        HdMayaAdapterTokens->emission,
        SdfValueTypeNames->Vector3f);
    auto specularColorConverter = std::make_shared<HdMayaScaledRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->specularColor,
        HdMayaAdapterTokens->specular,
        SdfValueTypeNames->Vector3f);
    auto specularIORConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->specularIOR, SdfValueTypeNames->Float);
    auto specularRoughnessConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->specularRoughness, SdfValueTypeNames->Float);
    auto coatConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->coat, SdfValueTypeNames->Float);
    auto coatRoughnessConverter = std::make_shared<HdMayaRemappingMaterialAttrConverter>(
        HdMayaAdapterTokens->coatRoughness, SdfValueTypeNames->Float);
    auto transmissionToOpacity = std::make_shared<HdMayaTransmissionMaterialAttrConverter>();

    auto fixedZeroFloat = std::make_shared<HdMayaFixedMaterialAttrConverter>(0.0f);
    auto fixedOneFloat = std::make_shared<HdMayaFixedMaterialAttrConverter>(1.0f);
    auto fixedZeroInt = std::make_shared<HdMayaFixedMaterialAttrConverter>(0);
    auto fixedOneInt = std::make_shared<HdMayaFixedMaterialAttrConverter>(1);
    auto fixedStToken = std::make_shared<HdMayaFixedMaterialAttrConverter>(HdMayaAdapterTokens->st);

    auto cosinePowerToRoughness = std::make_shared<HdMayaCosinePowerMaterialAttrConverter>();
    auto filenameConverter = std::make_shared<HdMayaFilenameMaterialAttrConverter>();

    auto wrapUConverter = std::make_shared<HdMayaWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapU, MayaAttrs::file::mirrorU);
    auto wrapVConverter = std::make_shared<HdMayaWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapV, MayaAttrs::file::mirrorV);

    auto textureMemoryConverter
        = std::make_shared<HdMayaNewDefaultMaterialAttrConverter>(defaultTextureMemoryLimit);

    _nodeConverters = {
        { HdMayaAdapterTokens->usdPreviewSurface, { UsdImagingTokens->UsdPreviewSurface, {} } },
        { HdMayaAdapterTokens->pxrUsdPreviewSurface, { UsdImagingTokens->UsdPreviewSurface, {} } },
        { HdMayaAdapterTokens->lambert,
          { UsdImagingTokens->UsdPreviewSurface,
            {
                { HdMayaAdapterTokens->diffuseColor, colorConverter },
                { HdMayaAdapterTokens->emissiveColor, incandescenceConverter },
                { HdMayaAdapterTokens->roughness, fixedOneFloat },
                { HdMayaAdapterTokens->metallic, fixedZeroFloat },
                { HdMayaAdapterTokens->useSpecularWorkflow, fixedZeroInt },
            } } },
        { HdMayaAdapterTokens->blinn,
          { UsdImagingTokens->UsdPreviewSurface,
            {
                { HdMayaAdapterTokens->diffuseColor, colorConverter },
                { HdMayaAdapterTokens->emissiveColor, incandescenceConverter },
                { HdMayaAdapterTokens->roughness, eccentricityConverter },
                { HdMayaAdapterTokens->metallic, fixedZeroFloat },
                { HdMayaAdapterTokens->useSpecularWorkflow, fixedOneInt },
            } } },
        { HdMayaAdapterTokens->phong,
          { UsdImagingTokens->UsdPreviewSurface,
            {
                { HdMayaAdapterTokens->diffuseColor, colorConverter },
                { HdMayaAdapterTokens->emissiveColor, incandescenceConverter },
                { HdMayaAdapterTokens->roughness, cosinePowerToRoughness },
                { HdMayaAdapterTokens->metallic, fixedZeroFloat },
                { HdMayaAdapterTokens->useSpecularWorkflow, fixedOneInt },
            } } },
        { HdMayaAdapterTokens->standardSurface,
          { UsdImagingTokens->UsdPreviewSurface,
            {
                { HdMayaAdapterTokens->diffuseColor, baseColorConverter },
                { HdMayaAdapterTokens->emissiveColor, emissionColorConverter },
                { HdMayaAdapterTokens->specularColor, specularColorConverter },
                { HdMayaAdapterTokens->ior, specularIORConverter },
                { HdMayaAdapterTokens->roughness, specularRoughnessConverter },
                { HdMayaAdapterTokens->clearcoat, coatConverter },
                { HdMayaAdapterTokens->clearcoatRoughness, coatRoughnessConverter },
                { HdMayaAdapterTokens->opacity, transmissionToOpacity },
                { HdMayaAdapterTokens->metallic, fixedZeroFloat },
            } } },
        { HdMayaAdapterTokens->file,
          { UsdImagingTokens->UsdUVTexture,
            {
                { HdMayaAdapterTokens->file, filenameConverter },
                { HdMayaAdapterTokens->st, uvConverter },
                { UsdHydraTokens->wrapS, wrapUConverter },
                { UsdHydraTokens->wrapT, wrapVConverter },
                { UsdHydraTokens->textureMemory, textureMemoryConverter },
            } } },
        { HdMayaAdapterTokens->place2dTexture,
          { UsdImagingTokens->UsdPrimvarReader_float2,
            {
                { HdMayaAdapterTokens->varname, fixedStToken },
            } } },
    };
}

HdMayaMaterialNodeConverter::HdMayaMaterialNodeConverter(
    const TfToken&                identifier,
    const NameToAttrConverterMap& attrConverters)
    : _attrConverters(attrConverters)
    , _identifier(identifier)
{
}

HdMayaMaterialAttrConverter::RefPtr
HdMayaMaterialNodeConverter::GetAttrConverter(const TfToken& paramName)
{
    auto it = _attrConverters.find(paramName);
    if (it == _attrConverters.end()) {
        return _genericAttrConverter;
    }
    return it->second;
}

HdMayaMaterialNodeConverter* HdMayaMaterialNodeConverter::GetNodeConverter(const TfToken& nodeType)
{
    auto it = _nodeConverters.find(nodeType);
    if (it == _nodeConverters.end()) {
        return nullptr;
    }
    return &(it->second);
}

HdMayaShaderParam::HdMayaShaderParam(
    const TfToken&          name,
    const VtValue&          value,
    const SdfValueTypeName& type)
    : name(name)
    , fallbackValue(value)
    , type(type)
{
}

HdMayaMaterialNetworkConverter::HdMayaMaterialNetworkConverter(
    HdMaterialNetwork& network,
    const SdfPath&     prefix,
    PathToMobjMap*     pathToMobj)
    : _network(network)
    , _prefix(prefix)
    , _pathToMobj(pathToMobj)
{
}

HdMaterialNode* HdMayaMaterialNetworkConverter::GetMaterial(const MObject& mayaNode)
{
    MStatus           status;
    MFnDependencyNode node(mayaNode, &status);
    if (ARCH_UNLIKELY(!status)) {
        return nullptr;
    }
    const auto* chr = node.name().asChar();
    if (chr == nullptr || chr[0] == '\0') {
        return nullptr;
    }
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("HdMayaMaterialNetworkConverter::GetMaterial(node=%s)\n", chr);
    std::string usdNameStr = UsdMayaUtil::SanitizeName(chr);
    const auto  materialPath = _prefix.AppendChild(TfToken(usdNameStr));

    auto findResult = std::find_if(
        _network.nodes.begin(),
        _network.nodes.end(),
        [&materialPath](const HdMaterialNode& m) -> bool { return m.path == materialPath; });
    if (findResult != _network.nodes.end()) {
        return &(*findResult);
    }

    auto* nodeConverter
        = HdMayaMaterialNodeConverter::GetNodeConverter(TfToken(node.typeName().asChar()));
    if (!nodeConverter) {
        return nullptr;
    }
    HdMaterialNode material {};
    material.path = materialPath;
    material.identifier = nodeConverter->GetIdentifier();
    if (material.identifier == UsdImagingTokens->UsdPreviewSurface) {
        for (const auto& param : HdMayaMaterialNetworkConverter::GetPreviewShaderParams()) {
            this->ConvertParameter(
                node, *nodeConverter, material, param.name, param.type, &param.fallbackValue);
        }
    } else {
        for (auto& nameAttrConverterPair : nodeConverter->GetAttrConverters()) {
            auto& name = nameAttrConverterPair.first;
            auto& attrConverter = nameAttrConverterPair.second;
            this->ConvertParameter(node, *nodeConverter, material, name, attrConverter->GetType());

            if (name == HdMayaAdapterTokens->varname
                && (material.identifier == UsdImagingTokens->UsdPrimvarReader_float
                    || material.identifier == UsdImagingTokens->UsdPrimvarReader_float2
                    || material.identifier == UsdImagingTokens->UsdPrimvarReader_float3
                    || material.identifier == UsdImagingTokens->UsdPrimvarReader_float4)) {
                VtValue& primVarName = material.parameters[name];
                if (TF_VERIFY(primVarName.IsHolding<TfToken>())) {
                    AddPrimvar(primVarName.UncheckedGet<TfToken>());
                } else {
                    TF_WARN("Converter identified as a UsdPrimvarReader*, but "
                            "it's "
                            "varname did not hold a TfToken");
                }
            }
        }
    }
    if (_pathToMobj) {
        (*_pathToMobj)[materialPath] = mayaNode;
    }
    _network.nodes.push_back(material);
    return &_network.nodes.back();
}

void HdMayaMaterialNetworkConverter::AddPrimvar(const TfToken& primvar)
{
    if (std::find(_network.primvars.begin(), _network.primvars.end(), primvar)
        == _network.primvars.end()) {
        _network.primvars.push_back(primvar);
    }
}

void HdMayaMaterialNetworkConverter::ConvertParameter(
    MFnDependencyNode&           node,
    HdMayaMaterialNodeConverter& nodeConverter,
    HdMaterialNode&              material,
    const TfToken&               paramName,
    const SdfValueTypeName&      type,
    const VtValue*               fallback)
{
    MPlug   plug;
    VtValue val;
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS).Msg("ConvertParameter(%s)\n", paramName.GetText());

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
    if (plug.isNull()) {
        return;
    }
    MPlug source = plug.source();
    if (!source.isNull()) {
        auto* sourceMat = GetMaterial(source.node());
        if (!sourceMat) {
            return;
        }
        const auto& sourceMatPath = sourceMat->path;
        if (sourceMatPath.IsEmpty()) {
            return;
        }
        HdMaterialRelationship rel;
        rel.inputId = sourceMatPath;
        rel.inputName = GetOutputName(*sourceMat, type);
        rel.outputId = material.path;
        rel.outputName = paramName;
        _network.relationships.push_back(rel);
    }
}

VtValue HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
    MFnDependencyNode&      node,
    const MString&          plugName,
    const SdfValueTypeName& type,
    const VtValue*          fallback,
    MPlug*                  outPlug)
{
    MStatus status;
    auto    p = node.findPlug(plugName, true, &status);
    VtValue val;
    if (status) {
        if (outPlug) {
            *outPlug = p;
        }
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

VtValue HdMayaMaterialNetworkConverter::ConvertMayaAttrToScaledValue(
    MFnDependencyNode&      node,
    const MString&          plugName,
    const MString&          scaleName,
    const SdfValueTypeName& type,
    const VtValue*          fallback,
    MPlug*                  outPlug)
{
    VtValue val = ConvertMayaAttrToValue(node, plugName, type, fallback, outPlug);
    MStatus status;
    auto    p = node.findPlug(scaleName, true, &status);
    if (status) {
        if (type.GetType() == SdfValueTypeNames->Vector3f.GetType()) {
            val = val.UncheckedGet<GfVec3f>() * p.asFloat();
        } else if (type == SdfValueTypeNames->Float) {
            val = val.UncheckedGet<float>() * p.asFloat();
        } else if (type.GetType() == SdfValueTypeNames->Float2.GetType()) {
            val = val.UncheckedGet<GfVec2f>() * p.asFloat();
        }
    } else {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "HdMayaMaterialNetworkConverter::ConvertMayaAttrToScaledValue(): "
                "No scaling plug found with name: %s",
                scaleName.asChar());
    }
    return val;
}

VtValue HdMayaMaterialNetworkConverter::ConvertPlugToValue(
    const MPlug&            plug,
    const SdfValueTypeName& type,
    const VtValue*          fallback)
{
    if (type.GetType() == SdfValueTypeNames->Vector3f.GetType()) {
        return VtValue(
            GfVec3f(plug.child(0).asFloat(), plug.child(1).asFloat(), plug.child(2).asFloat()));
    } else if (type == SdfValueTypeNames->Float) {
        return VtValue(plug.asFloat());
    } else if (type.GetType() == SdfValueTypeNames->Float2.GetType()) {
        return VtValue(GfVec2f(plug.child(0).asFloat(), plug.child(1).asFloat()));
    } else if (type == SdfValueTypeNames->Int) {
        return VtValue(plug.asInt());
    }
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "HdMayaMaterialNetworkConverter::ConvertPlugToValue(): do not "
            "know how to handle type: %s (cpp type: %s)\n",
            type.GetAsToken().GetText(),
            type.GetCPPTypeName().c_str());
    if (fallback) {
        return *fallback;
    }
    return {};
};

std::mutex         _previewShaderParams_mutex;
bool               _previewShaderParams_initialized = false;
HdMayaShaderParams _previewShaderParams;
static std::map<TfToken, HdMayaShaderParams> _defaultShaderParams;

const HdMayaShaderParams& HdMayaMaterialNetworkConverter::GetPreviewShaderParams()
{
    if (!_previewShaderParams_initialized) {
        std::lock_guard<std::mutex> lock(_previewShaderParams_mutex);
        // Once we have the lock, recheck to make sure it's still
        // uninitialized...
        if (!_previewShaderParams_initialized) {
            auto&                 shaderReg = SdrRegistry::GetInstance();
            SdrShaderNodeConstPtr sdrNode
                = shaderReg.GetShaderNodeByIdentifier(UsdImagingTokens->UsdPreviewSurface);
            if (TF_VERIFY(sdrNode)) {
                auto inputNames = sdrNode->GetInputNames();
                _previewShaderParams.reserve(inputNames.size());

                for (auto& inputName : inputNames) {
                    auto property = sdrNode->GetInput(inputName);
                    if (!TF_VERIFY(property)) {
                        continue;
                    }
                    _previewShaderParams.emplace_back(
                        inputName, property->GetDefaultValue(), property->GetTypeAsSdfType().first);
                }
                std::sort(
                    _previewShaderParams.begin(),
                    _previewShaderParams.end(),
                    [](const HdMayaShaderParam& a, const HdMayaShaderParam& b) -> bool {
                        return a.name < b.name;
                    });
                _previewShaderParams_initialized = true;
            }
        }
    }
    return _previewShaderParams;
}

const HdMayaShaderParams& HdMayaMaterialNetworkConverter::GetShaderParams(const TfToken& shaderNodeIdentifier)
{
	if(shaderNodeIdentifier == UsdImagingTokens->UsdPreviewSurface)
        return GetPreviewShaderParams();

	auto& it = _defaultShaderParams.find(shaderNodeIdentifier);
	if (it == _defaultShaderParams.end()) 
	{
		HdMayaShaderParams params;

		// TODO: Handle mutual exclusion
		// Once we have the lock, recheck to make sure it's still
		// uninitialized...
		auto& shaderReg = SdrRegistry::GetInstance();
		SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifier(shaderNodeIdentifier);
		assert(TF_VERIFY(sdrNode));

		auto inputNames = sdrNode->GetInputNames();
		params.reserve(inputNames.size());

		std::string inputNameStr;
		std::vector<std::string> inputNameStrs;
		for (auto& inputName : inputNames)
		{
			inputNameStr = inputName.GetString();
			inputNameStrs.push_back(inputNameStr);
			auto property = sdrNode->GetInput(inputName);
			if (!TF_VERIFY(property)) {
				continue;
			}

			params.emplace_back(
				inputName,
				property->GetDefaultValue(),
				property->GetTypeAsSdfType().first);
		}

		std::sort(
			params.begin(),
			params.end(),
			[](const HdMayaShaderParam& a, const HdMayaShaderParam& b) -> bool {
			return a.name < b.name;
		});

		return _defaultShaderParams.insert({ shaderNodeIdentifier, params }).first->second;			
	}

	return it->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
