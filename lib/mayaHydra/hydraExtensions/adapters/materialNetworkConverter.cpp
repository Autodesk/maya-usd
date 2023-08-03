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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#include "materialNetworkConverter.h"

#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/materialAdapter.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/adapters/tokens.h>
#include <mayaHydraLib/hydraUtils.h>
#include <mayaHydraLib/mixedUtils.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

#include <mutex>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE
// Bring the MayaHydra namespace into scope.
// The following code currently lives inside the pxr namespace, but it would make more sense to 
// have it inside the MayaHydra namespace. This using statement allows us to use MayaHydra symbols
// from within the pxr namespace as if we were in the MayaHydra namespace.
// Remove this once the code has been moved to the MayaHydra namespace.
using namespace MayaHydra;

/* This file contains how we translate the Maya shaders (Blinn, Lambert, Standard Surface etc.) to
   hydra and how we do the parameters mapping. See MayaHydraMaterialNetworkConverter::initialize()
   for that purpose.
*/

namespace {

// Print to the output window the type and value of each parameter from the std::map
void DebugPrintParameters(const std::map<TfToken, VtValue>& params)
{
    cout << "\n"; // Add a line
    // Print all parameters types and values
    for (auto param : params) {
        std::string valueAsString = ConvertVtValueToString(param.second);
        cout << "Material parameters : (" << param.first.GetText() << " - " << valueAsString.c_str()
             << ")\n";
    }
}

const TfToken _useSpecularWorkflowToken("useSpecularWorkflow");
const TfToken _specularColorToken("specularColor");
const TfToken _opacityToken("opacity");

constexpr float defaultTextureMemoryLimit = 1e8f;
constexpr float xRayOpacityValue
    = 0.3f; // Hardcoded value taken from OGSMayaRenderItem::UpdateExtraOpacityParam

// Lists of preferred shader output names, from SdfValueTypeName to list of
// preferred output names for that type.  The list that has an empty token for
// SdfValueTypeName is used as a default.
const std::vector<std::pair<SdfValueTypeName, std::vector<TfToken>>> preferredOutputNamesByType {
    { SdfValueTypeNames->Float3,
      { MayaHydraAdapterTokens->result,
        MayaHydraAdapterTokens->out,
        MayaHydraAdapterTokens->output,
        MayaHydraAdapterTokens->rgb,
        MayaHydraAdapterTokens->xyz } },
    { SdfValueTypeNames->Float2,
      { MayaHydraAdapterTokens->result,
        MayaHydraAdapterTokens->out,
        MayaHydraAdapterTokens->output,
        MayaHydraAdapterTokens->st,
        MayaHydraAdapterTokens->uv } },
    { SdfValueTypeNames->Float,
      { MayaHydraAdapterTokens->result,
        MayaHydraAdapterTokens->out,
        MayaHydraAdapterTokens->output,
        MayaHydraAdapterTokens->r,
        MayaHydraAdapterTokens->x } }
};

// Default set of preferred output names, if type not in
// preferredOutputNamesByType
const std::vector<TfToken> defaultPreferredOutputNames { MayaHydraAdapterTokens->result,
                                                         MayaHydraAdapterTokens->out,
                                                         MayaHydraAdapterTokens->output };

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
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
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
            TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
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
                    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
                        .Msg(
                            "  found preferred name of correct type in "
                            "registry: %s\n",
                            preferredName.GetText());
                    return preferredName;
                }
            }
            // No preferred names were found, use the first valid name
            TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
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
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
            .Msg(
                "  found no valid entries in registry, returning guess: "
                "%s\n",
                preferredNames[0].GetText());
        return preferredNames[0];
    }

    // We should never get here - preferredNames should never be empty!
    return MayaHydraAdapterTokens->result;
}

class MayaHydraGenericMaterialAttrConverter : public MayaHydraMaterialAttrConverter
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
        MPlugArray*             outPlug = nullptr) override
    {
        return MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, fallback, outPlug);
    }
};

class MayaHydraNewDefaultMaterialAttrConverter : public MayaHydraMaterialAttrConverter
{
public:
    template <typename T>
    MayaHydraNewDefaultMaterialAttrConverter(const T& defaultValue)
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
        MPlugArray*             outPlug = nullptr) override
    {
        return MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, paramName.GetText(), type, &_defaultValue, outPlug);
    }

    const VtValue _defaultValue;
};

class MayaHydraRemappingMaterialAttrConverter : public MayaHydraMaterialAttrConverter
{
public:
    MayaHydraRemappingMaterialAttrConverter(
        const TfToken&          remappedName,
        const SdfValueTypeName& type)
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
        MPlugArray*             outPlug = nullptr) override
    {
        return MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, _remappedName.GetText(), type, fallback, outPlug);
    }

protected:
    const TfToken&          _remappedName;
    const SdfValueTypeName& _type;
};

class MayaHydraScaledRemappingMaterialAttrConverter : public MayaHydraRemappingMaterialAttrConverter
{
public:
    MayaHydraScaledRemappingMaterialAttrConverter(
        const TfToken&          remappedName,
        const TfToken&          scaleName,
        const SdfValueTypeName& type)
        : MayaHydraRemappingMaterialAttrConverter(remappedName, type)
        , _scaleName(scaleName)
    {
    }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        return MayaHydraMaterialNetworkConverter::ConvertMayaAttrToScaledValue(
            node, _remappedName.GetText(), _scaleName.GetText(), type, fallback, outPlug);
    }

private:
    const TfToken& _scaleName;
};

class MayaHydraComputedMaterialAttrConverter : public MayaHydraMaterialAttrConverter
{
public:
    /// Classes which derive from this use some sort of calculation to get
    /// the right value for the node, and so don't have a single plug that
    /// can be hooked into a node network.
    TfToken GetPlugName(const TfToken& usdName) override { return TfToken(); }
};

class MayaHydraFixedMaterialAttrConverter : public MayaHydraComputedMaterialAttrConverter
{
public:
    template <typename T>
    MayaHydraFixedMaterialAttrConverter(const T& value)
        : _value(value)
    {
    }

    SdfValueTypeName GetType() override { return SdfGetValueTypeNameForValue(_value); }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        return _value;
    }

private:
    const VtValue _value;
};

class MayaHydraUvAttrConverter : public MayaHydraMaterialAttrConverter
{
public:
    MayaHydraUvAttrConverter()
        : _value(GfVec2f(0.0f, 0.0f))
    {
    }

    SdfValueTypeName GetType() override { return SdfValueTypeNames->TexCoord2f; }

    TfToken GetPlugName(const TfToken& usdName) override { return MayaHydraAdapterTokens->uvCoord; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        if (outPlug) {
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
                        outPlug->append(connections[i]);
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

class MayaHydraCosinePowerMaterialAttrConverter : public MayaHydraComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Float; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        VtValue cosinePower = MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "cosinePower", type, nullptr, outPlug);
        if (!cosinePower.IsHolding<float>()) {
            if (fallback) {
                return *fallback;
            }
            TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
                .Msg("MayaHydraCosinePowerMaterialAttrConverter::GetValue(): "
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

class MayaHydraTransmissionMaterialAttrConverter : public MayaHydraComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Float; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        VtValue transmission = MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "transmission", type, nullptr, outPlug);
        // Combine transmission and Geometry-->Opacity R,G and B attributes
        VtValue geometryOpacityR = MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "opacityR", type, nullptr, outPlug);
        VtValue geometryOpacityG = MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "opacityG", type, nullptr, outPlug);
        VtValue geometryOpacityB = MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, "opacityB", type, nullptr, outPlug);

        if (!transmission.IsHolding<float>()) {
            if (fallback) {
                return *fallback;
            }
            TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
                .Msg("MayaHydraTransmissionMaterialAttrConverter::GetValue(): "
                     "No float plug found with name: transmission and no "
                     "fallback given");
            return VtValue();
        }

        float val = 1.0f - transmission.UncheckedGet<float>();
        if (val < 1.0e-4f) {
            // Clamp lower value as an opacity of 0.0 in hydra makes the object fully transparent,
            // but in VP2 we still see the specular highlight if any, avoiding 0.0 leads to the same
            // effect in hydra.
            val = 1.0e-4f;
        }

        float fGeometryOpacity = 1.0f;
        if (geometryOpacityR.IsHolding<float>() && geometryOpacityG.IsHolding<float>()
            && geometryOpacityB.IsHolding<float>()) {
            // Take the average as there is only 1 parameter in hydra
            fGeometryOpacity = (1.0f / 3.0f)
                * (geometryOpacityR.UncheckedGet<float>() + geometryOpacityG.UncheckedGet<float>()
                   + geometryOpacityB.UncheckedGet<float>());
        }

        val *= fGeometryOpacity;

        return VtValue(val);
    }
};

class MayaHydraFilenameMaterialAttrConverter : public MayaHydraComputedMaterialAttrConverter
{
public:
    SdfValueTypeName GetType() override { return SdfValueTypeNames->Asset; }

    VtValue GetValue(
        MFnDependencyNode&      node,
        const TfToken&          paramName,
        const SdfValueTypeName& type,
        const VtValue*          fallback = nullptr,
        MPlugArray*             outPlug = nullptr) override
    {
        auto path = GetFileTexturePath(node);
        return VtValue(SdfAssetPath(path.GetText(), path.GetText()));
    }
};

class MayaHydraWrapMaterialAttrConverter : public MayaHydraComputedMaterialAttrConverter
{
public:
    MayaHydraWrapMaterialAttrConverter(MObject& wrapAttr, MObject& mirrorAttr)
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
        MPlugArray*             outPlug = nullptr) override
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

auto _genericAttrConverter = std::make_shared<MayaHydraGenericMaterialAttrConverter>();

typedef std::unordered_map<TfToken, MayaHydraMaterialNodeConverter, TfToken::HashFunctor>
    NameToNodeConverterMap;

/// _nodeConverters contains how we translate from a Maya shader to hydra and the parameters mapping
/// we use.
NameToNodeConverterMap _nodeConverters;

} // namespace

/*static*/
void MayaHydraMaterialNetworkConverter::initialize()
{
    // Define different converters for translating from specific Maya attributes types to hydra
    auto colorConverter = std::make_shared<MayaHydraScaledRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->color,
        MayaHydraAdapterTokens->diffuse,
        SdfValueTypeNames->Vector3f);
    auto incandescenceConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->incandescence, SdfValueTypeNames->Vector3f);
    auto eccentricityConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->eccentricity, SdfValueTypeNames->Float);
    auto uvConverter = std::make_shared<MayaHydraUvAttrConverter>();

    // Standard surface:
    auto baseColorConverter = std::make_shared<MayaHydraScaledRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->baseColor,
        MayaHydraAdapterTokens->base,
        SdfValueTypeNames->Vector3f);
    auto emissionColorConverter = std::make_shared<MayaHydraScaledRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->emissionColor,
        MayaHydraAdapterTokens->emission,
        SdfValueTypeNames->Vector3f);
    auto specularColorConverter = std::make_shared<MayaHydraScaledRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->specularColor,
        MayaHydraAdapterTokens->specular,
        SdfValueTypeNames->Vector3f);
    auto specularIORConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->specularIOR, SdfValueTypeNames->Float);
    auto specularRoughnessConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->specularRoughness, SdfValueTypeNames->Float);
    auto metallicConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->metalness, SdfValueTypeNames->Float);
    auto coatConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->coat, SdfValueTypeNames->Float);
    auto coatRoughnessConverter = std::make_shared<MayaHydraRemappingMaterialAttrConverter>(
        MayaHydraAdapterTokens->coatRoughness, SdfValueTypeNames->Float);
    auto transmissionToOpacity = std::make_shared<MayaHydraTransmissionMaterialAttrConverter>();

    auto fixedZeroFloat = std::make_shared<MayaHydraFixedMaterialAttrConverter>(0.0f);
    auto fixedOneFloat = std::make_shared<MayaHydraFixedMaterialAttrConverter>(1.0f);
    auto fixedZeroInt = std::make_shared<MayaHydraFixedMaterialAttrConverter>(0);
    auto fixedOneInt = std::make_shared<MayaHydraFixedMaterialAttrConverter>(1);
    auto fixedStToken
        = std::make_shared<MayaHydraFixedMaterialAttrConverter>(MayaHydraAdapterTokens->st);

    auto cosinePowerToRoughness = std::make_shared<MayaHydraCosinePowerMaterialAttrConverter>();
    auto filenameConverter = std::make_shared<MayaHydraFilenameMaterialAttrConverter>();

    auto wrapUConverter = std::make_shared<MayaHydraWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapU, MayaAttrs::file::mirrorU);
    auto wrapVConverter = std::make_shared<MayaHydraWrapMaterialAttrConverter>(
        MayaAttrs::file::wrapV, MayaAttrs::file::mirrorV);

    auto textureMemoryConverter
        = std::make_shared<MayaHydraNewDefaultMaterialAttrConverter>(defaultTextureMemoryLimit);

    // In the following code we define how we translate from a Maya shader to hydra and how we do
    // the parameters mapping
    _nodeConverters = {
        { MayaHydraAdapterTokens->usdPreviewSurface, { UsdImagingTokens->UsdPreviewSurface, {} } },
        { MayaHydraAdapterTokens->pxrUsdPreviewSurface,
          { UsdImagingTokens->UsdPreviewSurface, {} } },
        { MayaHydraAdapterTokens->lambert,
          { UsdImagingTokens
                ->UsdPreviewSurface, // Maya Lambert shader translated to a UsdPreviewSurface with
                                     // the following UsdPreviewSurface parameters mapped
            {
                { MayaHydraAdapterTokens->diffuseColor, colorConverter },
                { MayaHydraAdapterTokens->emissiveColor, incandescenceConverter },
                { MayaHydraAdapterTokens->roughness, fixedOneFloat },
                { MayaHydraAdapterTokens->metallic, fixedZeroFloat },
                { MayaHydraAdapterTokens->useSpecularWorkflow, fixedZeroInt },
            } } },
        { MayaHydraAdapterTokens->blinn,
          { UsdImagingTokens
                ->UsdPreviewSurface, // Maya Blinn shader translated to a UsdPreviewSurface with the
                                     // following UsdPreviewSurface parameters mapped
            {
                { MayaHydraAdapterTokens->diffuseColor, colorConverter },
                { MayaHydraAdapterTokens->emissiveColor, incandescenceConverter },
                { MayaHydraAdapterTokens->roughness, eccentricityConverter },
                { MayaHydraAdapterTokens->metallic, fixedZeroFloat },
                { MayaHydraAdapterTokens->useSpecularWorkflow, fixedOneInt },
            } } },
        { MayaHydraAdapterTokens->phong,
          { UsdImagingTokens
                ->UsdPreviewSurface, // Maya Phong shader translated to a UsdPreviewSurface with the
                                     // following UsdPreviewSurface parameters mapped
            {
                { MayaHydraAdapterTokens->diffuseColor, colorConverter },
                { MayaHydraAdapterTokens->emissiveColor, incandescenceConverter },
                { MayaHydraAdapterTokens->roughness, cosinePowerToRoughness },
                { MayaHydraAdapterTokens->metallic, fixedZeroFloat },
                { MayaHydraAdapterTokens->useSpecularWorkflow, fixedOneInt },
            } } },
        { MayaHydraAdapterTokens->standardSurface,
          { UsdImagingTokens->UsdPreviewSurface, // Maya Standard surface shader translated to a
                                                 // UsdPreviewSurface with the following
                                                 // UsdPreviewSurface parameters mapped
            {
                { MayaHydraAdapterTokens->diffuseColor, baseColorConverter },
                { MayaHydraAdapterTokens->emissiveColor, emissionColorConverter },
                { MayaHydraAdapterTokens->specularColor, specularColorConverter },
                { MayaHydraAdapterTokens->ior, specularIORConverter },
                { MayaHydraAdapterTokens->roughness, specularRoughnessConverter },
                { MayaHydraAdapterTokens->clearcoat, coatConverter },
                { MayaHydraAdapterTokens->clearcoatRoughness, coatRoughnessConverter },
                { MayaHydraAdapterTokens->opacity, transmissionToOpacity },
                { MayaHydraAdapterTokens->metallic, metallicConverter },
            } } },
        { MayaHydraAdapterTokens->file,
          { UsdImagingTokens->UsdUVTexture, // Maya file translated to a UsdUVTexture with the
                                            // following UsdUVTexture parameters mapped
            {
                { MayaHydraAdapterTokens->file, filenameConverter },
                { MayaHydraAdapterTokens->st, uvConverter },
                { UsdHydraTokens->wrapS, wrapUConverter },
                { UsdHydraTokens->wrapT, wrapVConverter },
                { UsdHydraTokens->textureMemory, textureMemoryConverter },
            } } },
        { MayaHydraAdapterTokens
              ->place2dTexture, // Maya place2dTexture translated to a UsdPrimvarReader_float2 with
                                // the following UsdPrimvarReader_float2 parameters mapped
          { UsdImagingTokens->UsdPrimvarReader_float2,
            {
                { MayaHydraAdapterTokens->varname, fixedStToken },
            } } },
    };
}

MayaHydraMaterialNodeConverter::MayaHydraMaterialNodeConverter(
    const TfToken&                identifier,
    const NameToAttrConverterMap& attrConverters)
    : _attrConverters(attrConverters)
    , _identifier(identifier)
{
}

MayaHydraMaterialAttrConverter::RefPtr
MayaHydraMaterialNodeConverter::GetAttrConverter(const TfToken& paramName)
{
    auto it = _attrConverters.find(paramName);
    if (it == _attrConverters.end()) {
        return _genericAttrConverter;
    }
    return it->second;
}

MayaHydraMaterialNodeConverter*
MayaHydraMaterialNodeConverter::GetNodeConverter(const TfToken& nodeType)
{
    auto it = _nodeConverters.find(nodeType);
    if (it == _nodeConverters.end()) {
        return nullptr;
    }
    return &(it->second);
}

MayaHydraShaderParam::MayaHydraShaderParam(
    const TfToken&          name,
    const VtValue&          value,
    const SdfValueTypeName& type)
    : name(name)
    , fallbackValue(value)
    , type(type)
{
}

MayaHydraMaterialNetworkConverter::MayaHydraMaterialNetworkConverter(
    MayaHydraMaterialNetworkConverterInit& init)
    : _network(init._materialNetwork)
    , _prefix(init._prefix)
    , _pathToMobj(init._pathToMobj)
    , _enableXRayShadingMode(init._enableXRayShadingMode)
{
}

HdMaterialNode* MayaHydraMaterialNetworkConverter::GetMaterial(const MObject& mayaNode)
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
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
        .Msg("MayaHydraMaterialNetworkConverter::GetMaterial(node=%s)\n", chr);
    std::string nodeNameString(chr);
    SanitizeNameForSdfPath(nodeNameString);
    const auto  materialPath = _prefix.AppendChild(TfToken(nodeNameString));

    auto findResult = std::find_if(
        _network.nodes.begin(),
        _network.nodes.end(),
        [&materialPath](const HdMaterialNode& m) -> bool { return m.path == materialPath; });
    if (findResult != _network.nodes.end()) {
        return &(*findResult);
    }

    auto* nodeConverter
        = MayaHydraMaterialNodeConverter::GetNodeConverter(TfToken(node.typeName().asChar()));
    if (!nodeConverter) {
        return nullptr;
    }
    HdMaterialNode material {};
    material.path = materialPath;
    material.identifier = nodeConverter->GetIdentifier();
    if (material.identifier == UsdImagingTokens->UsdPreviewSurface) {
        for (const auto& param : MayaHydraMaterialNetworkConverter::GetPreviewShaderParams()) {
            this->ConvertParameter(
                node, *nodeConverter, material, param.name, param.type, &param.fallbackValue);
        }

        // If we are using a specular color which is not white, the UsdPreviewsurface specular
        // workflow must be enabled to use the specular color which is done by setting the
        // UsdPreviewSurface param "useSpecularWork" to 1
        {
            const auto it = material.parameters.find(_specularColorToken);
            if (it != material.parameters.cend()) {
                const VtValue& specColorVal = it->second;
                if (!specColorVal.IsEmpty()
                    && specColorVal.UncheckedGet<GfVec3f>() != GfVec3f(1, 1, 1)) {
                    material.parameters[_useSpecularWorkflowToken] = VtValue(1);
                }
            }
        }

        if (_enableXRayShadingMode) {
            // Multiply current opacity by hardcoded xRayOpacityValue
            const auto it = material.parameters.find(_opacityToken);
            if (it != material.parameters.cend()) {
                const VtValue& opacityVal = it->second;
                if (!opacityVal.IsEmpty()) {
                    material.parameters[_opacityToken]
                        = VtValue(opacityVal.UncheckedGet<float>() * xRayOpacityValue);
                }
            }
        }

        if (TfDebug::IsEnabled(MAYAHYDRALIB_ADAPTER_MATERIALS_PRINT_PARAMETERS_VALUES)) {
            // DEBUG to print material parameters type and value to the output window
            DebugPrintParameters(material.parameters);
        }

    } else {
        for (auto& nameAttrConverterPair : nodeConverter->GetAttrConverters()) {
            auto& name = nameAttrConverterPair.first;
            auto& attrConverter = nameAttrConverterPair.second;
            this->ConvertParameter(node, *nodeConverter, material, name, attrConverter->GetType());

            if (name == MayaHydraAdapterTokens->varname
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

void MayaHydraMaterialNetworkConverter::AddPrimvar(const TfToken& primvar)
{
    if (std::find(_network.primvars.begin(), _network.primvars.end(), primvar)
        == _network.primvars.end()) {
        _network.primvars.push_back(primvar);
    }
}

void MayaHydraMaterialNetworkConverter::ConvertParameter(
    MFnDependencyNode&              node,
    MayaHydraMaterialNodeConverter& nodeConverter,
    HdMaterialNode&                 material,
    const TfToken&                  paramName,
    const SdfValueTypeName&         type,
    const VtValue*                  fallback)
{
    MPlugArray plugArray;
    VtValue    val;
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS).Msg("ConvertParameter(%s)\n", paramName.GetText());

    auto attrConverter = nodeConverter.GetAttrConverter(paramName);
    if (attrConverter) {
        // Using an array of MPlug in plugArray, as some settings may have 2 or more attributes that
        // should be taken into consideration for connections. For example : specular has a specular
        // color and specular weight attributes, both should be considered. So after calling
        // attrConverter->GetValue, the plugArray will contain all dependents MPlug for connections.
        val = attrConverter->GetValue(node, paramName, type, fallback, &plugArray);
    } else if (fallback) {
        val = *fallback;
    } else {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
            .Msg(
                "MayaHydraMaterialNetworkConverter::ConvertParameter(): "
                "No attrConverter found with name: %s and no fallback "
                "given",
                paramName.GetText());
        val = VtValue();
    }

    material.parameters[paramName] = val;

    /*plugArray contains all dependents MPlug we should consider for connections.
    Usually it contains 1 or 2 MPlug (2 is when dealing with a weighted attribute),
    it can have more than 2 when dealing with the transmission which is combined with opacityR,
    opacityG and opacityB attributes. But a limitation we have at this time is that if both the
    color and the weight attributes have a connection, one of both connections will be ignored by
    hydra as we have only one parameter in the UsdPreviewSurface which will have both connections
    and hydra only considers the last connection added. There is no blending node we could use with
    the UsdPreviewSurface. We would need the StandardSurface to be in hydra or use MaterialX to
    build a shading network to handle this case with a multiply node for example.
    */
    for (auto plug : plugArray) {
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
}

VtValue MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(
    MFnDependencyNode&      node,
    const MString&          plugName,
    const SdfValueTypeName& type,
    const VtValue*          fallback,
    MPlugArray*             outPlug)
{
    MStatus status;
    auto    p = node.findPlug(plugName, true, &status);
    VtValue val;
    if (status) {
        if (outPlug) {
            outPlug->append(p);
        }
        val = ConvertPlugToValue(p, type, fallback);
    } else if (fallback) {
        val = *fallback;
    } else {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
            .Msg(
                "MayaHydraMaterialNetworkConverter::ConvertMayaAttrToValue(): "
                "No plug found with name: %s and no fallback given",
                plugName.asChar());
        val = VtValue();
    }
    return val;
}

VtValue MayaHydraMaterialNetworkConverter::ConvertMayaAttrToScaledValue(
    MFnDependencyNode&      node,
    const MString&          plugName,
    const MString&          scaleName,
    const SdfValueTypeName& type,
    const VtValue*          fallback,
    MPlugArray*             outPlug)
{
    VtValue val = ConvertMayaAttrToValue(node, plugName, type, fallback, outPlug);
    MStatus status;
    auto    p = node.findPlug(scaleName, true, &status);
    if (status) {
        if (!p.isNull() && outPlug) {
            outPlug->append(p);
        }
        if (type.GetType() == SdfValueTypeNames->Vector3f.GetType()) {
            val = val.UncheckedGet<GfVec3f>() * p.asFloat();
        } else if (type == SdfValueTypeNames->Float) {
            val = val.UncheckedGet<float>() * p.asFloat();
        } else if (type.GetType() == SdfValueTypeNames->Float2.GetType()) {
            val = val.UncheckedGet<GfVec2f>() * p.asFloat();
        }
    } else {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
            .Msg(
                "MayaHydraMaterialNetworkConverter::ConvertMayaAttrToScaledValue(): "
                "No scaling plug found with name: %s",
                scaleName.asChar());
    }
    return val;
}

VtValue MayaHydraMaterialNetworkConverter::ConvertPlugToValue(
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
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
        .Msg(
            "MayaHydraMaterialNetworkConverter::ConvertPlugToValue(): do not "
            "know how to handle type: %s (cpp type: %s)\n",
            type.GetAsToken().GetText(),
            type.GetCPPTypeName().c_str());
    if (fallback) {
        return *fallback;
    }
    return {};
};

std::mutex                                      _previewShaderParams_mutex;
bool                                            _previewShaderParams_initialized = false;
MayaHydraShaderParams                           _previewShaderParams;
static std::map<TfToken, MayaHydraShaderParams> _defaultShaderParams;

const MayaHydraShaderParams& MayaHydraMaterialNetworkConverter::GetPreviewShaderParams()
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
                    [](const MayaHydraShaderParam& a, const MayaHydraShaderParam& b) -> bool {
                        return a.name < b.name;
                    });
                _previewShaderParams_initialized = true;
            }
        }
    }
    return _previewShaderParams;
}

const MayaHydraShaderParams&
MayaHydraMaterialNetworkConverter::GetShaderParams(const TfToken& shaderNodeIdentifier)
{
    if (shaderNodeIdentifier == UsdImagingTokens->UsdPreviewSurface)
        return GetPreviewShaderParams();

    auto it = _defaultShaderParams.find(shaderNodeIdentifier);
    if (it == _defaultShaderParams.end()) {
        MayaHydraShaderParams params;

        auto&                 shaderReg = SdrRegistry::GetInstance();
        SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifier(shaderNodeIdentifier);
        assert(TF_VERIFY(sdrNode));

        auto inputNames = sdrNode->GetInputNames();
        params.reserve(inputNames.size());

        std::string              inputNameStr;
        std::vector<std::string> inputNameStrs;
        for (auto& inputName : inputNames) {
            inputNameStr = inputName.GetString();
            inputNameStrs.push_back(inputNameStr);
            auto property = sdrNode->GetInput(inputName);
            if (!TF_VERIFY(property)) {
                continue;
            }

            params.emplace_back(
                inputName, property->GetDefaultValue(), property->GetTypeAsSdfType().first);
        }

        std::sort(
            params.begin(),
            params.end(),
            [](const MayaHydraShaderParam& a, const MayaHydraShaderParam& b) -> bool {
                return a.name < b.name;
            });

        return _defaultShaderParams.insert({ shaderNodeIdentifier, params }).first->second;
    }

    return it->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
