//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "UsdMaterialValidator.h"
#include "Utils.h"

#include <mayaUsdAPI/utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderNode.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdUI/backdrop.h>

#include <ufe/pathString.h>

#include <map>
#include <tuple>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
#define TF_TOKEN(s)                                                                                                    \
    const TfToken& s()                                                                                                 \
    {                                                                                                                  \
        static const TfToken tok(#s);                                                                                  \
        return tok;                                                                                                    \
    }

namespace UsdTokens
{
TF_TOKEN(USD)
TF_TOKEN(glslfx)
TF_TOKEN(UsdPrimvarReader)
TF_TOKEN(UsdUVTexture)
TF_TOKEN(st)
TF_TOKEN(varname)
TF_TOKEN(string)
TF_TOKEN(token)
// TODO(LOOKDEVX-2045): Remove when boundary ports get added for soloing connections
TF_TOKEN(Autodesk)
TF_TOKEN(ldx_isSoloingItem)
TF_TOKEN(hidden)
} // namespace UsdTokens

namespace MtlxTokens
{
TF_TOKEN(MaterialX)
TF_TOKEN(mtlx)
TF_TOKEN(ND_standard_surface_surfaceshader)
TF_TOKEN(ND_standard_surface_surfaceshader_100)
TF_TOKEN(ND_open_pbr_surface_surfaceshader)
TF_TOKEN(ND_gltf_pbr_surfaceshader)
TF_TOKEN(ND_surface)
TF_TOKEN(bsdf)
TF_TOKEN(edf)
TF_TOKEN(defaultgeomprop)
TF_TOKEN(geompropvalue)
TF_TOKEN(geomprop)
TF_TOKEN(geomcolor)
TF_TOKEN(texcoord)
TF_TOKEN(uvindex)
TF_TOKEN(bitangent)
TF_TOKEN(tangent)
TF_TOKEN(specular_anisotropy)
TF_TOKEN(specular_roughness_anisotropy)
TF_TOKEN(transmission_scatter_anisotropy)
TF_TOKEN(subsurface_anisotropy)
TF_TOKEN(subsurface_scatter_anisotropy)
TF_TOKEN(coat_anisotropy)
TF_TOKEN(coat_roughness_anisotropy)

// NodeDefs associated with component connections:
TF_TOKEN(ND_combine2_vector2)
TF_TOKEN(ND_combine3_color3)
TF_TOKEN(ND_combine3_vector3)
TF_TOKEN(ND_combine4_color4)
TF_TOKEN(ND_combine4_vector4)
TF_TOKEN(ND_separate2_vector2)
TF_TOKEN(ND_separate3_color3)
TF_TOKEN(ND_separate3_vector3)
TF_TOKEN(ND_separate4_color4)
TF_TOKEN(ND_separate4_vector4)
TF_TOKEN(out)
TF_TOKEN(in)
TF_TOKEN(outr)
TF_TOKEN(outg)
TF_TOKEN(outb)
TF_TOKEN(outa)
TF_TOKEN(outx)
TF_TOKEN(outy)
TF_TOKEN(outz)
TF_TOKEN(outw)

const std::vector<TfToken>& anisotropicNames()
{
    static const auto kAnisotropicNames = std::vector<TfToken>{MtlxTokens::specular_anisotropy(),
                                                               MtlxTokens::specular_roughness_anisotropy(),
                                                               MtlxTokens::transmission_scatter_anisotropy(),
                                                               MtlxTokens::subsurface_anisotropy(),
                                                               MtlxTokens::subsurface_scatter_anisotropy(),
                                                               MtlxTokens::coat_anisotropy(),
                                                               MtlxTokens::coat_roughness_anisotropy()};
    return kAnisotropicNames;
}
} // namespace MtlxTokens

enum class ComponentNodeType
{
    eNone,
    eCombine,
    eSeparate
};

ComponentNodeType isComponentNode(const UsdPrim& prim)
{
    // Using a regex here would make no sense. USD provides a token that can be quickly hashed and compared.
    static const auto kCombineNodeDefs = std::unordered_set<TfToken, TfToken::HashFunctor>{
        MtlxTokens::ND_combine2_vector2(), MtlxTokens::ND_combine3_color3(), MtlxTokens::ND_combine3_vector3(),
        MtlxTokens::ND_combine4_color4(), MtlxTokens::ND_combine4_vector4()};
    static const auto kSeparateNodeDefs = std::unordered_set<TfToken, TfToken::HashFunctor>{
        MtlxTokens::ND_separate2_vector2(), MtlxTokens::ND_separate3_color3(), MtlxTokens::ND_separate3_vector3(),
        MtlxTokens::ND_separate4_color4(), MtlxTokens::ND_separate4_vector4()};

    bool isHidden = false;
    auto adskData = prim.GetCustomDataByKey(UsdTokens::Autodesk());
    if (adskData.IsHolding<PXR_NS::VtDictionary>())
    {
        auto adskDict = adskData.UncheckedGet<PXR_NS::VtDictionary>();
        isHidden = adskDict.find(UsdTokens::hidden().GetText()) != adskDict.end();
    }
    // The prim hidden check is for backwards compatibility. Newer files will use metadata only to hide nodes.
    if (isHidden || prim.IsHidden())
    {
        const auto shader = UsdShadeShader(prim);
        if (shader)
        {
            TfToken shaderId;
            shader.GetShaderId(&shaderId);
            if (kCombineNodeDefs.count(shaderId) > 0)
            {
                return ComponentNodeType::eCombine;
            }
            if (kSeparateNodeDefs.count(shaderId) > 0)
            {
                return ComponentNodeType::eSeparate;
            }
        }
    }
    return ComponentNodeType::eNone;
}
} // namespace

#undef TF_TOKEN

namespace LookdevXUsd
{

struct UsdConnectionInfo
{
    UsdAttribute m_src;
    UsdAttribute m_dst;
};

namespace
{

enum class ErrId
{
    // Node level errors:
    kNotInCompound,
    kNoIdentifier,
    kNotInRegistry,
    kNotInAScope,
    kNotInACompound,
    kWrongChild,
    kNotAShader,
    kMxIndexBased,
    kMxOldDef,
    kBadMatParent,

    // Connection level errors:
    kTypeMismatch,
    kImplMismatch,
    kCycle,
    kParentMismatch,
    kImplMismatch2,

    // Attribute level errors:
    kMissingNode,
    kMissingAttr,
    kInvalidAttr,
    kUSDNoUV,
    kUSDNoVarname,
    kMxMissingReq,
    kMxNoVarname,
    kNotInNodeDef,
    kNDTypeMismatch,
    kInvalidSeparate,
    kInvalidCombine
};
using ErrorTable = std::vector<std::tuple<ErrId, std::string>>;
const ErrorTable& errorTable()
{
    // Requires same ordering as the ErrId enum. Validated in errorStr function.
    static const auto kErrorTable = ErrorTable{
        // Node level errors:
        {ErrId::kNotInCompound, "N001: Shader node is not inside a Compound or Material."},
        {ErrId::kNoIdentifier, "N002: Shader node is missing an identifier."},
        {ErrId::kNotInRegistry, "N003: Shader node identifier '@' could not be found in registry."},
        {ErrId::kNotInAScope, "N004: USD assets working group recommends grouping material nodes inside a Scope."},
        {ErrId::kNotInACompound, "N005: Compound node is not inside a Compound or Material."},
        {ErrId::kWrongChild, "N006: Node is not a child of material '@'."},
        {ErrId::kNotAShader, "N007: Node is not a shading primitive."},
        {ErrId::kMxIndexBased, "N008: Index based node '@' is not supported in a name based renderer."},
        {ErrId::kMxOldDef, "N009: Consider using the more recent '@' node definition."},
        {ErrId::kBadMatParent, "N010: Material cannot be child of connectable node '@'."},

        // Connection level errors:
        {ErrId::kTypeMismatch, "C001: Type mismatch between a '@' source and a '@' destination."},
        {ErrId::kImplMismatch, "C002: Source node is of type '@' and cannot be used to assemble a shader of type '@'."},
        {ErrId::kCycle, "C003: These connections form a cycle."},
        {ErrId::kParentMismatch, "C004: Connection source and destination are not in the same compound."},
        {ErrId::kImplMismatch2, "C005: Source node of type '@' cannot work with destination node of type '@'."},

        // Attribute level errors:
        {ErrId::kMissingNode, "A001: Is connected to missing node '@'."},
        {ErrId::kMissingAttr, "A002: Is connected to missing attribute '@'."},
        {ErrId::kInvalidAttr, "A003: Is connected to invalid attribute '@'."},
        {ErrId::kUSDNoUV, "A004: Texture node requires a connection to 'UsdPrimvarReader_float2'."},
        {ErrId::kUSDNoVarname, "A005: Varname attribute is undefined."},
        {ErrId::kMxMissingReq, "A006: Node requires @ connection."},
        {ErrId::kMxNoVarname, "A007: Geomprop attribute is undefined."},
        {ErrId::kNotInNodeDef, "A008: Attribute does not exist in '@'."},
        {ErrId::kNDTypeMismatch, "A009: Attribute should be '@' as defined in '@'."},
        {ErrId::kInvalidSeparate, "A010: Invalid component separate setup."},
        {ErrId::kInvalidCombine, "A011: Invalid component combine setup."},

    };
    return kErrorTable;
}

// Dev-only strings when making sure the enum and the array are in sync.
// LCOV_EXCL_START
const std::string& messageArrayOutOfSync()
{
    static const std::string kMessageArrayOutOfSync = "Error message array is out of sync with enum.";
    return kMessageArrayOutOfSync;
}
const std::string& invalidNumberOfArguments()
{
    static const std::string kInvalidNumberOfArguments = "Invalid number of arguments.";
    return kInvalidNumberOfArguments;
}
// LCOV_EXCL_STOP

const std::string& errorStr(ErrId messageId)
{
    const auto& [id, message] = errorTable().at(static_cast<size_t>(messageId));
    // LCOV_EXCL_START
    if (id != messageId)
    {
        return messageArrayOutOfSync();
    }
    if (message.find('@') != std::string::npos)
    {
        return invalidNumberOfArguments();
    }
    // LCOV_EXCL_STOP
    return message;
}

std::string errorStr(ErrId messageId, const std::string& p1)
{
    const auto& [id, message] = errorTable().at(static_cast<size_t>(messageId));
    // LCOV_EXCL_START
    if (id != messageId)
    {
        return messageArrayOutOfSync();
    }
    // LCOV_EXCL_STOP
    auto fragments = LookdevXUsdUtils::splitString(message, "@");
    // LCOV_EXCL_START
    if (fragments.size() != 2)
    {
        return invalidNumberOfArguments();
    }
    // LCOV_EXCL_STOP
    return fragments[0] + p1 + fragments[1];
}

std::string errorStr(ErrId messageId, const std::string& p1, const std::string& p2)
{
    const auto& [id, message] = errorTable().at(static_cast<size_t>(messageId));
    // LCOV_EXCL_START
    if (id != messageId)
    {
        return messageArrayOutOfSync();
    }
    // LCOV_EXCL_STOP
    auto fragments = LookdevXUsdUtils::splitString(message, "@");
    // LCOV_EXCL_START
    if (fragments.size() != 3)
    {
        return invalidNumberOfArguments();
    }
    // LCOV_EXCL_STOP
    return fragments[0] + p1 + fragments[1] + p2 + fragments[2];
}

const std::string& niceSourceName(const TfToken& source)
{
    if (source == MtlxTokens::mtlx())
    {
        return MtlxTokens::MaterialX().GetString();
    }
    if (source == UsdTokens::glslfx())
    {
        return UsdTokens::USD().GetString();
    }
    return source;
}

TfToken inputFullName(const TfToken& baseName)
{
    return UsdShadeUtils::GetFullName(baseName, UsdShadeAttributeType::Input);
}

} // namespace

LookdevXUfe::AttributeComponentInfo UsdMaterialValidator::remapComponentConnectionAttribute(
    const UsdPrim& prim, const TfToken& attrName) const
{
    // Hidden nodes can be from component connection. If that is the case, we need to remap to the associated LookdevX
    // visible node.
    auto componentSetup = isComponentNode(prim);
    if (componentSetup != ComponentNodeType::eNone)
    {

        auto baseNameAndType = UsdShadeUtils::GetBaseNameAndType(attrName);
        TfToken shaderId;
        const auto shader = UsdShadeShader(prim);
        shader.GetShaderId(&shaderId);
        auto& registry = SdrRegistry::GetInstance();
        const auto* nodeDef = registry.GetShaderNodeByIdentifier(shaderId);

        if (componentSetup == ComponentNodeType::eCombine)
        {
            // If the error is about one of the known inputs of a combine, we map to the corresponding component.
            // Please note that the port index is between 1 and 4 (inputs:in1, in2, in3, in4) and can not go over the
            // last digit in the combine category name.
            size_t portIndex = 0; // Keep zero as invalid value.

            if (baseNameAndType.second == UsdShadeAttributeType::Input)
            {
                const auto& portName = baseNameAndType.first.GetString();
                if (portName.size() == 3 && portName[0] == 'i' && portName[1] == 'n')
                {
                    // ND_combineX_vectypeX
                    static const size_t kCombineSizePos = 10;
                    const auto combineMaxIndex = shaderId.GetString()[kCombineSizePos];
                    const auto portCharIndex = portName[2];
                    if (portCharIndex >= '1' && portCharIndex <= combineMaxIndex)
                    {
                        // Valid index:
                        portIndex = static_cast<size_t>(portCharIndex - '0');
                    }
                }
            }
            std::string componentName;
            if (portIndex > 0)
            {
                if (nodeDef)
                {
                    const auto* output = nodeDef->GetShaderOutput(MtlxTokens::out());
                    if (output)
                    {
#if PXR_VERSION > 2408
                        const auto& outputType = output->GetTypeAsSdfType().GetSdfType();
#else
                        const auto& outputType = output->GetTypeAsSdfType().first;
#endif

                        if (outputType == SdfValueTypeNames->Color3f || outputType == SdfValueTypeNames->Color4f)
                        {
                            constexpr auto kRGBA = std::string_view{" rgba"};
                            componentName = kRGBA.substr(portIndex, 1);
                        }
                        else
                        {
                            constexpr auto kXYZW = std::string_view{" xyzw"};
                            componentName = kXYZW.substr(portIndex, 1);
                        }
                    }
                }
            }

            const auto combineIt = m_seenCombineConnections.find(prim.GetPath());
            if (combineIt != m_seenCombineConnections.end())
            {
                auto componentLocation = LookdevXUfe::AttributeComponentInfo{
                    toUfe(combineIt->second.GetPrim()), combineIt->second.GetName(), componentName};
                validateComponentLocation(componentLocation, errorStr(ErrId::kInvalidCombine));
                return componentLocation;
            }
        }
        if (componentSetup == ComponentNodeType::eSeparate)
        {
            static const std::unordered_map<TfToken, std::string, TfToken::HashFunctor> kSeparateComponentMap = {
                {MtlxTokens::outr(), "r"}, {MtlxTokens::outg(), "g"}, {MtlxTokens::outb(), "b"},
                {MtlxTokens::outa(), "a"}, {MtlxTokens::outx(), "x"}, {MtlxTokens::outy(), "y"},
                {MtlxTokens::outz(), "z"}, {MtlxTokens::outw(), "w"}};

            // If the error is about one of the known outputs of a separate, we map to the corresponding component.
            auto componentIt = kSeparateComponentMap.find(baseNameAndType.first);
            if (componentIt != kSeparateComponentMap.end())
            {
                // Make sure the output exists in the node definition:
                if (!nodeDef || !nodeDef->GetShaderOutput(baseNameAndType.first))
                {
                    // Can not confirm this output exists:
                    componentIt = kSeparateComponentMap.end();
                }
            }
            const auto componentName =
                (componentIt != kSeparateComponentMap.end()) ? componentIt->second : std::string{};

            const auto separateInput = shader.GetInput(MtlxTokens::in());
            if (separateInput)
            {
                UsdShadeConnectableAPI source;
                TfToken sourceName;
                auto sourceType = UsdShadeAttributeType::Invalid;
                const auto isConnected = separateInput.GetConnectedSource(&source, &sourceName, &sourceType);
                if (isConnected)
                {
                    auto componentLocation = LookdevXUfe::AttributeComponentInfo{
                        toUfe(source.GetPrim()), UsdShadeUtils::GetFullName(sourceName, sourceType), componentName};
                    validateComponentLocation(componentLocation, errorStr(ErrId::kInvalidSeparate));
                    return componentLocation;
                }
            }
        }
    }
    return {Ufe::Path{}, std::string{}, std::string{}};
}

void UsdMaterialValidator::validateComponentLocation(const LookdevXUfe::AttributeComponentInfo& attrInfo,
                                                     const std::string& errorDesc) const
{
    // If we could not resolve a component then signal that the combine/separate are broken.
    if (attrInfo.component().empty())
    {
        const auto brokenComponent = attrInfo.path().string() + "." + attrInfo.name();
        const auto seenIt = m_brokenComponents.find(brokenComponent);
        if (seenIt == m_brokenComponents.end())
        {
            m_log->addEntry(
                {LookdevXUfe::Log::Severity::kError, errorDesc, {1, LookdevXUfe::AttributeComponentInfo{attrInfo}}});
            m_brokenComponents.insert(brokenComponent);
        }
    }
}

Ufe::Path UsdMaterialValidator::toUfe(const UsdStageWeakPtr& stage, const SdfPath& path)
{
    auto stagePath = MayaUsdAPI::stagePath(stage);
    return Ufe::Path::Segments{stagePath.getSegments()[0], MayaUsdAPI::usdPathToUfePathSegment(path)};
}

Ufe::Path UsdMaterialValidator::toUfe(const UsdPrim& prim)
{
    return {toUfe(prim.GetStage(), prim.GetPath())};
}

LookdevXUfe::AttributeComponentInfo UsdMaterialValidator::toUfe(const UsdAttribute& attrib) const
{
    return toUfe(attrib.GetPrim(), attrib.GetName());
}

LookdevXUfe::AttributeComponentInfo UsdMaterialValidator::toUfe(const UsdPrim& prim, const TfToken& attrName) const
{
    auto componentAttrInfo = remapComponentConnectionAttribute(prim, attrName);
    if (!componentAttrInfo.path().empty())
    {
        return componentAttrInfo;
    }

    return {toUfe(prim), attrName.GetString(), ""};
}

LookdevXUfe::Log::ConnectionInfo UsdMaterialValidator::toUfe(const UsdConnectionInfo& cnx) const
{
    return {toUfe(cnx.m_src), toUfe(cnx.m_dst)};
}

UsdMaterialValidator::UsdMaterialValidator(const UsdShadeMaterial& prim) : m_material(prim)
{
}

LookdevXUfe::ValidationLog::Ptr UsdMaterialValidator::validate()
{
    m_log = LookdevXUfe::ValidationLog::create();

    auto allOutputs = m_material.GetSurfaceOutputs();
    auto displacementOuts = m_material.GetDisplacementOutputs();
    allOutputs.insert(allOutputs.end(), displacementOuts.begin(), displacementOuts.end());
    auto volumeOuts = m_material.GetVolumeOutputs();
    allOutputs.insert(allOutputs.end(), volumeOuts.begin(), volumeOuts.end());

    for (auto&& terminal : allOutputs)
    {
        // Find render context:
        auto tokens = TfStringSplit(terminal.GetFullName(), ":");
        if (tokens.size() == 3)
        {
            m_renderContext = TfToken(tokens[1]);
        }
        else
        {
            m_renderContext = UsdTokens::glslfx();
        }

        visitDestination(terminal.GetAttr());
    }

    // Continue with a traversal of all the nodes below the material, but at the warning level because the remaining
    // unvalidated nodes do not yet belong to any exported shader.
    m_currentSeverity = LookdevXUfe::Log::Severity::kWarning;
    m_renderContext = TfToken();
    std::vector<UsdPrim> componentNodes;
    for (auto&& prim : m_material.GetPrim().GetDescendants())
    {
        if (!m_validatedPrims.count(prim.GetPath()))
        {
            if (isComponentNode(prim) != ComponentNodeType::eNone)
            {
                // Delay checking these until we have processed more of the stage in case they do not come with a
                // companion node.
                componentNodes.push_back(prim);
                continue;
            }
            if (validatePrim(prim))
            {
                if (auto shader = UsdShadeShader(prim))
                {
                    for (auto&& destInput : shader.GetInputs())
                    {
                        visitDestination(destInput.GetAttr());
                    }
                }
            }
        }
    }

    // Now we can process free-floating component nodes.
    for (auto&& prim : componentNodes)
    {
        if (!m_validatedPrims.count(prim.GetPath()))
        {
            if (validatePrim(prim))
            {
                if (auto shader = UsdShadeShader(prim))
                {
                    for (auto&& destInput : shader.GetInputs())
                    {
                        visitDestination(destInput.GetAttr());
                    }
                }
            }
        }
    }

    return std::move(m_log);
}

bool UsdMaterialValidator::visitDestination(const UsdAttribute& dest)
{
    if (!validatePrim(dest.GetPrim()))
    {
        return false;
    }

    if (m_visitedDestinations.count(dest.GetPath()))
    {
        return true;
    }
    if (dest.GetPrim().IsA<UsdShadeShader>())
    {
        // Track visited nodes, but not nodegraphs since they
        // can be re-entered wihtout a cycle if the internals
        // are split into distinct subgraphs.
        m_visitedDestinations.insert(dest.GetPath());
    }

    auto primCnx = UsdShadeConnectableAPI(dest.GetPrim());
    if (!primCnx)
    {
        return false;
    }

    SdfPathVector invalidSourcePaths;
    auto sourceInfoVec = UsdShadeConnectableAPI::GetConnectedSources(dest, &invalidSourcePaths);
    reportInvalidSources(dest, invalidSourcePaths);

    UsdConnectionInfo cnxInfo;
    m_connectionStack.push_back(&cnxInfo);
    cnxInfo.m_dst = dest;
    for (auto&& sourceInfo : sourceInfoVec)
    {
        UsdPrim sourcePrim = sourceInfo.source.GetPrim();
        std::string prefix = UsdShadeUtils::GetPrefixForAttributeType(sourceInfo.sourceType);
        TfToken sourceAttrName(prefix + sourceInfo.sourceName.GetString());
        UsdAttribute sourceAttr = sourcePrim.GetAttribute(sourceAttrName);

        cnxInfo.m_src = sourceAttr;

        if (isComponentNode(sourceAttr.GetPrim()) == ComponentNodeType::eCombine)
        {
            m_seenCombineConnections.emplace(sourceAttr.GetPrim().GetPath(), dest);
        }

        validateConnection();

        if (validateAcyclic())
        {
            traverseConnection();
        }
    }

    m_connectionStack.pop_back();
    return true;
}

bool UsdMaterialValidator::validateShader(const UsdShadeShader& shader)
{
    // Can only have a NodeGraph as parent:
    auto parentNode = shader.GetPrim().GetParent();
    if (!parentNode || !parentNode.IsA<UsdShadeNodeGraph>())
    {
        // Argh... Need to use LookdevX nomenclature instead of USD.
        m_log->addEntry({m_currentSeverity, errorStr(ErrId::kNotInCompound), {1, toUfe(shader.GetPrim())}});
    }

    // Ensure shader validity against Sdr registry:
    TfToken shaderId;
    shader.GetShaderId(&shaderId);
    if (shaderId.IsEmpty())
    {
        m_log->addEntry({m_currentSeverity, errorStr(ErrId::kNoIdentifier), {1, toUfe(shader.GetPrim())}});
        return false;
    }
    SdrShaderNodeConstPtr shaderNode = SdrRegistry::GetInstance().GetShaderNodeByIdentifier(shaderId);
    if (!shaderNode)
    {
        m_log->addEntry(
            {m_currentSeverity, errorStr(ErrId::kNotInRegistry, shaderId.GetString()), {1, toUfe(shader.GetPrim())}});
        return false;
    }

    auto inputNames = std::set<TfToken>{shaderNode->GetInputNames().cbegin(), shaderNode->GetInputNames().cend()};
    for (auto&& input : shader.GetInputs())
    {
        if (inputNames.count(input.GetBaseName()) == 0)
        {
            m_log->addEntry(
                {m_currentSeverity, errorStr(ErrId::kNotInNodeDef, shaderId.GetString()), {1, toUfe(input.GetAttr())}});
            continue;
        }
        auto currentTypeName = input.GetTypeName().GetAsToken();
#if PXR_VERSION > 2408
        auto expectedTypeName = shaderNode->GetInput(input.GetBaseName())->GetTypeAsSdfType().GetSdfType().GetAsToken();
#else
        auto expectedTypeName = shaderNode->GetInput(input.GetBaseName())->GetTypeAsSdfType().first.GetAsToken();
#endif
        if (currentTypeName != expectedTypeName && currentTypeName != UsdTokens::string() &&
            currentTypeName != UsdTokens::token())
        {
            m_log->addEntry({m_currentSeverity,
                             errorStr(ErrId::kNDTypeMismatch, expectedTypeName.GetString(), shaderId.GetString()),
                             {1, toUfe(input.GetAttr())}});
            continue;
        }
    }
    auto outputNames = std::set<TfToken>{shaderNode->GetOutputNames().cbegin(), shaderNode->GetOutputNames().cend()};
    for (auto&& output : shader.GetOutputs())
    {
        if (outputNames.count(output.GetBaseName()) == 0)
        {
            m_log->addEntry({m_currentSeverity,
                             errorStr(ErrId::kNotInNodeDef, shaderId.GetString()),
                             {1, toUfe(output.GetAttr())}});
            continue;
        }
        auto currentTypeName = output.GetTypeName().GetAsToken();
#if PXR_VERSION > 2408
        auto expectedTypeName =
            shaderNode->GetOutput(output.GetBaseName())->GetTypeAsSdfType().GetSdfType().GetAsToken();
#else
        auto expectedTypeName = shaderNode->GetOutput(output.GetBaseName())->GetTypeAsSdfType().first.GetAsToken();
#endif
        if (currentTypeName != expectedTypeName && currentTypeName != UsdTokens::string() &&
            currentTypeName != UsdTokens::token())
        {
            m_log->addEntry({m_currentSeverity,
                             errorStr(ErrId::kNDTypeMismatch, expectedTypeName.GetString(), shaderId.GetString()),
                             {1, toUfe(output.GetAttr())}});
            continue;
        }
    }

    auto connectableAPI = UsdShadeConnectableAPI(shader.GetPrim());
    if (shaderNode->GetSourceType() == UsdTokens::glslfx())
    {
        validateGlslfxShader(connectableAPI, shaderNode);
    }
    else if (shaderNode->GetSourceType() == MtlxTokens::mtlx())
    {
        validateMaterialXShader(connectableAPI, shaderNode);
    }

    return true;
}

void UsdMaterialValidator::validateGlslfxShader(const UsdShadeConnectableAPI& connectableAPI,
                                                SdrShaderNodeConstPtr shaderNode)
{
    if (shaderNode->GetIdentifier() == UsdTokens::UsdUVTexture())
    {
        // Image nodes require a texcoord primvar reader:
        const auto stInput = connectableAPI.GetInput(UsdTokens::st());
        if (!stInput || !UsdShadeConnectableAPI::HasConnectedSource(stInput))
        {
            m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                             errorStr(ErrId::kUSDNoUV),
                             {1, toUfe(connectableAPI.GetPrim(), inputFullName(UsdTokens::st()))}});
        }
    }

    if (shaderNode->GetFamily() == UsdTokens::UsdPrimvarReader())
    {
        // Need to specify the primvar name that a primvar reader uses:
        const auto varnameInput = connectableAPI.GetInput(UsdTokens::varname());
        if (!varnameInput ||
            (!UsdShadeConnectableAPI::HasConnectedSource(varnameInput) && !varnameInput.GetAttr().HasValue()))
        {
            m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                             errorStr(ErrId::kUSDNoVarname),
                             {1, toUfe(connectableAPI.GetPrim(), inputFullName(UsdTokens::varname()))}});
        }
    }
}

void UsdMaterialValidator::validateMaterialXShader(const UsdShadeConnectableAPI& connectableAPI,
                                                   SdrShaderNodeConstPtr shaderNode)
{
    // This is problematic because some renderers (looking at you MayaUSD and usdView) will auto-fix these issues,
    // thus teaching bad habits to users.
    if (shaderNode->GetIdentifier() == MtlxTokens::ND_standard_surface_surfaceshader() ||
        shaderNode->GetIdentifier() == MtlxTokens::ND_standard_surface_surfaceshader_100() ||
        shaderNode->GetIdentifier() == MtlxTokens::ND_open_pbr_surface_surfaceshader())
    {
        // Standard surface needs a tangent input if any anisotropic parameter is non-zero.
        bool isAnisotropic = false;
        for (auto&& anisoName : MtlxTokens::anisotropicNames())
        {
            const auto anisoInput = connectableAPI.GetInput(anisoName);
            if (anisoInput &&
                (UsdShadeConnectableAPI::HasConnectedSource(anisoInput) || anisoInput.GetAttr().HasValue()))
            {
                isAnisotropic = true;
                break;
            }
        }
        if (isAnisotropic)
        {
            const auto tangentInput = connectableAPI.GetInput(MtlxTokens::tangent());
            if (!tangentInput || !UsdShadeConnectableAPI::HasConnectedSource(tangentInput))
            {
                std::string detail = "a ";
                detail.append(MtlxTokens::tangent());
                detail.append(" reader");
                m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                                 errorStr(ErrId::kMxMissingReq, detail),
                                 {1, toUfe(connectableAPI.GetPrim(), inputFullName(MtlxTokens::tangent()))}});
            }
        }
    }
    else if (shaderNode->GetIdentifier() == MtlxTokens::ND_gltf_pbr_surfaceshader())
    {
        // The glTf PBR shader has a tangent input, but all the MaterialX computations are isotropic, so connecting it
        // is not required as of MaterialX 1.38.7.
        //
        // The glTf specification requires producing tangents in order to compute displacement. MaterialX does not yet
        // do displacement. See https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
        //
        // There is an anisotropic extension suggested for glTf, but it is not yet integrated in MaterialX. See
        // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_anisotropy/README.md
        //
        // Might need to be revised in a future versions of MaterialX.
    }
    else if (shaderNode->GetIdentifier() == MtlxTokens::ND_surface())
    {
        // Surface node need both an EDF and BSDF connection to produce correct closures for GLSL:
        static const auto mandatoryInputs = std::vector<TfToken>{MtlxTokens::bsdf(), MtlxTokens::edf()};
        for (auto const& mandatoryName : mandatoryInputs)
        {
            const auto mandatoryInput = connectableAPI.GetInput(mandatoryName);
            if (!mandatoryInput || (!UsdShadeConnectableAPI::HasConnectedSource(mandatoryInput)))
            {
                std::string detail;
                if (mandatoryName == MtlxTokens::edf())
                {
                    detail.append("an ");
                }
                else
                {
                    detail.append("a ");
                }
                detail.append(mandatoryName);
                detail.append(" node");
                m_log->addEntry({LookdevXUfe::Log::Severity::kError,
                                 errorStr(ErrId::kMxMissingReq, detail),
                                 {1, toUfe(connectableAPI.GetPrim(), inputFullName(mandatoryName))}});
            }
        }
    }
    else
    {
        // For all the other MaterialX nodes, look for a defaultgeomprop entry, and flag it if it requires a manual
        // reader node.
        for (auto&& inputName : shaderNode->GetInputNames())
        {
            const SdrShaderPropertyConstPtr input = shaderNode->GetShaderInput(inputName);
            const auto hints = input->GetHints();
            const auto defaultgeompropIt = hints.find(MtlxTokens::defaultgeomprop());
            // Position and normal are handled natively by most renderers, and streams are available in USD. This leaves
            // issues with non-default USD primvars, like UVs, tangents, and bitangents.
            if (defaultgeompropIt != hints.cend() && defaultgeompropIt->second.front() != 'N' &&
                defaultgeompropIt->second.front() != 'P')
            {
                const auto geomInput = connectableAPI.GetInput(inputName);
                if (!geomInput || !UsdShadeConnectableAPI::HasConnectedSource(geomInput))
                {
                    std::string streamName;
                    switch (defaultgeompropIt->second.front())
                    {
                    case 'U':
                        streamName = MtlxTokens::texcoord().GetString();
                        break;
                    case 'T':
                        streamName = MtlxTokens::tangent().GetString();
                        break;
                    // No MaterialX node to test these cases:
                    // LCOV_EXCL_START
                    case 'B':
                        streamName = MtlxTokens::bitangent().GetString();
                        break;
                    default:
                        streamName = "UNKNOWN";
                        break;
                        // LCOV_EXCL_STOP
                    }
                    std::string detail = "a ";
                    detail.append(streamName);
                    detail.append(" reader");
                    m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                                     errorStr(ErrId::kMxMissingReq, detail),
                                     {1, toUfe(connectableAPI.GetPrim(), inputFullName(inputName))}});
                }
            }
        }
    }
    if (shaderNode->GetFamily() == MtlxTokens::geompropvalue())
    {
        // Need to specify the primvar name that a geomprop reader uses:
        const auto varnameInput = connectableAPI.GetInput(MtlxTokens::geomprop());
        if (!varnameInput ||
            (!UsdShadeConnectableAPI::HasConnectedSource(varnameInput) && !varnameInput.GetAttr().HasValue()))
        {
            m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                             errorStr(ErrId::kMxNoVarname),
                             {1, toUfe(connectableAPI.GetPrim(), inputFullName(MtlxTokens::geomprop()))}});
        }
    }
    if (shaderNode->GetFamily() == MtlxTokens::geomcolor() || shaderNode->GetFamily() == MtlxTokens::texcoord() ||
        shaderNode->GetFamily() == MtlxTokens::bitangent() || shaderNode->GetFamily() == MtlxTokens::tangent() ||
        (shaderNode->GetInput(MtlxTokens::uvindex()) && shaderNode->GetFamily().GetString().rfind("gltf_", 0) == 0))
    {
        // These MaterialX nodes use index-based streams. Some renderers will convert them to named primvar readers if
        // there is an established naming convention, but support will be limited.
        m_log->addEntry({LookdevXUfe::Log::Severity::kWarning,
                         errorStr(ErrId::kMxIndexBased, shaderNode->GetIdentifier().GetString()),
                         {1, toUfe(connectableAPI.GetPrim())}});
    }
    if (shaderNode->GetIdentifier() == MtlxTokens::ND_standard_surface_surfaceshader_100())
    {
        m_log->addEntry({LookdevXUfe::Log::Severity::kInfo,
                         errorStr(ErrId::kMxOldDef, MtlxTokens::ND_standard_surface_surfaceshader().GetString()),
                         {1, toUfe(connectableAPI.GetPrim())}});
    }
}

bool UsdMaterialValidator::validateMaterial(const UsdShadeMaterial& material)
{
    // We recommend having a Scope as parent, but it is not a USD hard rule:
    auto parentNode = material.GetPrim().GetParent();
    if (!parentNode || !parentNode.IsA<UsdGeomScope>())
    {
        m_log->addEntry(
            {LookdevXUfe::Log::Severity::kInfo, errorStr(ErrId::kNotInAScope), {1, toUfe(material.GetPrim())}});
    }

    // But not having a connectable parent is a USD hard rule:
    while (parentNode)
    {
        if (auto connectableParent = UsdShadeConnectableAPI(parentNode))
        {
            auto stage = parentNode.GetStage();
            auto parentPath = Ufe::PathString::string(toUfe(stage, parentNode.GetPath()));
            m_log->addEntry({LookdevXUfe::Log::Severity::kError,
                             errorStr(ErrId::kBadMatParent, parentPath),
                             {1, toUfe(material.GetPrim())}});
            break;
        }
        parentNode = parentNode.GetParent();
    }

    return true;
}

bool UsdMaterialValidator::validateNodeGraph(const UsdShadeNodeGraph& nodegraph)
{
    // Can only have a NodeGraph as parent:
    auto parentNode = nodegraph.GetPrim().GetParent();
    if (!parentNode || !parentNode.IsA<UsdShadeNodeGraph>())
    {
        // Argh... Need to use LookdevX nomenclature instead of USD.
        m_log->addEntry({m_currentSeverity, errorStr(ErrId::kNotInACompound), {1, toUfe(nodegraph.GetPrim())}});
    }

    return true;
}

bool UsdMaterialValidator::validatePrim(const UsdPrim& prim)
{
    if (auto foundIt = m_validatedPrims.find(prim.GetPath()); foundIt != m_validatedPrims.end())
    {
        return foundIt->second;
    }
    bool retVal = true;

    if (!prim.GetPath().HasPrefix(m_material.GetPath()))
    {
        auto materialPath = Ufe::PathString::string(toUfe(m_material.GetPrim()));
        m_log->addEntry({m_currentSeverity, errorStr(ErrId::kWrongChild, materialPath), {1, toUfe(prim)}});
    }

    if (auto shader = UsdShadeShader(prim))
    {
        retVal = validateShader(shader);
    }
    else if (auto material = UsdShadeMaterial(prim))
    {
        retVal = validateMaterial(material);
    }
    else if (auto nodegraph = UsdShadeNodeGraph(prim))
    {
        retVal = validateNodeGraph(nodegraph);
    }
    else
    {
        if (!prim.IsA<UsdUIBackdrop>())
        {
            m_log->addEntry({LookdevXUfe::Log::Severity::kError, errorStr(ErrId::kNotAShader), {1, toUfe(prim)}});
            retVal = false;
        }
    }

    m_validatedPrims.insert({prim.GetPath(), retVal});
    return retVal;
}

void UsdMaterialValidator::validateConnection()
{
    if (m_connectionStack.empty())
    {
        return;
    }

    auto stackIter = m_connectionStack.rbegin();
    auto const& cnx = *stackIter;

    // If we do not have a global render context we can still validate connections using the destination as render
    // context reference.
    TfToken renderContext = m_renderContext;
    if (renderContext.IsEmpty() && !cnx->m_dst.GetPrim().IsA<UsdShadeNodeGraph>())
    {
        TfToken id;
        UsdShadeShader(cnx->m_dst.GetPrim()).GetShaderId(&id);
        SdrShaderNodeConstPtr dstShaderNode = SdrRegistry::GetInstance().GetShaderNodeByIdentifier(id);
        if (dstShaderNode)
        {
            renderContext = dstShaderNode->GetSourceType();
        }
    }

    // Validate the the source type matches the destination type:
    if (renderContext == MtlxTokens::mtlx() && cnx->m_src.GetTypeName() != cnx->m_dst.GetTypeName())
    {
        // If the source is a component combine output, then it is quite broken. Just mark it as such
        auto emitError = true;
        if (isComponentNode(cnx->m_src.GetPrim()) == ComponentNodeType::eCombine &&
            UsdShadeUtils::GetBaseNameAndType(cnx->m_src.GetName()).second == UsdShadeAttributeType::Output)
        {
            toUfe(cnx->m_src);
            emitError = false;
        }
        // If the destination is a component separate input, then it is quite broken. Just mark it as such
        if (isComponentNode(cnx->m_dst.GetPrim()) == ComponentNodeType::eSeparate &&
            UsdShadeUtils::GetBaseNameAndType(cnx->m_dst.GetName()).second == UsdShadeAttributeType::Input)
        {
            toUfe(cnx->m_src);
            emitError = false;
        }
        // MaterialX is extremely strict:
        if (emitError)
        {
            m_log->addEntry({m_currentSeverity,
                             errorStr(ErrId::kTypeMismatch, cnx->m_src.GetTypeName().GetAsToken(),
                                      cnx->m_dst.GetTypeName().GetAsToken()),
                             {1, toUfe(*cnx)}});
        }
    }
    if (renderContext == UsdTokens::glslfx() &&
        cnx->m_src.GetTypeName().GetCPPTypeName() != cnx->m_dst.GetTypeName().GetCPPTypeName())
    {
        // USD allows connecting if the C++ type matches, allowing the float3 output of UsdUVTexture to connect to the
        // color3f input of UsdPreviewSurface.
        m_log->addEntry({m_currentSeverity,
                         errorStr(ErrId::kTypeMismatch, cnx->m_src.GetTypeName().GetAsToken(),
                                  cnx->m_dst.GetTypeName().GetAsToken()),
                         {1, toUfe(*cnx)}});
    }

    if (cnx->m_src.GetPrimPath().GetParentPath() != cnx->m_dst.GetPrimPath().GetParentPath())
    {
        // The source and destination are not exactly under the same parent. Do a finer check:
        const bool srcIsShader = cnx->m_src.GetPrim().IsA<UsdShadeShader>();
        const bool dstIsShader = cnx->m_dst.GetPrim().IsA<UsdShadeShader>();
        bool isProblematic = false;
        if (srcIsShader && dstIsShader)
        {
            // src and dst are both shaders, they should be in the same compound:
            isProblematic = true;
        }
        else
        {
            if (srcIsShader)
            {
                // src is a shader inside compound dst
                if (cnx->m_src.GetPrimPath().GetParentPath() != cnx->m_dst.GetPrimPath())
                {
                    isProblematic = true;
                }
            }
            else if (dstIsShader)
            {
                // dst is a shader inside compound src
                if (cnx->m_dst.GetPrimPath().GetParentPath() != cnx->m_src.GetPrimPath())
                {
                    isProblematic = true;
                }
            }
            else
            {
                // Two compounds: one must be child of the other:
                if (cnx->m_src.GetPrimPath().GetParentPath() != cnx->m_dst.GetPrimPath() &&
                    cnx->m_dst.GetPrimPath().GetParentPath() != cnx->m_src.GetPrimPath())
                {
                    isProblematic = true;
                }
            }
        }
        if (isProblematic)
        {
            // Soloing currently breaks the rules and requires silencing this error:
            // TODO(LOOKDEVX-2045): Remove when boundary ports get added for soloing connections
            auto adskData = cnx->m_dst.GetPrim().GetCustomDataByKey(UsdTokens::Autodesk());
            if (adskData.IsHolding<PXR_NS::VtDictionary>())
            {
                auto adskDict = adskData.UncheckedGet<PXR_NS::VtDictionary>();
                auto itSolo = adskDict.find(UsdTokens::ldx_isSoloingItem().GetText());
                if (itSolo != adskDict.end())
                {
                    isProblematic = false;
                }
            }
        }
        if (isProblematic)
        {
            m_log->addEntry({m_currentSeverity, errorStr(ErrId::kParentMismatch), {1, toUfe(*cnx)}});
        }
    }

    // Validate that shader connections are between the same type of nodes
    const auto& srcNode = cnx->m_src.GetPrim();
    if (!validatePrim(srcNode) || !srcNode.IsA<UsdShadeShader>())
    {
        // Only checking shader to shader connections when looking for family mismatch.
        return;
    }

    if (renderContext == MtlxTokens::mtlx() || renderContext == UsdTokens::glslfx())
    {
        // Make sure the node implementations all match:
        TfToken id;
        UsdShadeShader(srcNode).GetShaderId(&id);
        SdrShaderNodeConstPtr srcShaderNode = SdrRegistry::GetInstance().GetShaderNodeByIdentifier(id);

        if (srcShaderNode->GetSourceType() != renderContext)
        {
            if (renderContext == m_renderContext)
            {
                m_log->addEntry({m_currentSeverity,
                                 errorStr(ErrId::kImplMismatch, niceSourceName(srcShaderNode->GetSourceType()),
                                          niceSourceName(renderContext)),
                                 {1, toUfe(*cnx)}});
            }
            else
            {
                // Unconnected island, different wording:
                m_log->addEntry({m_currentSeverity,
                                 errorStr(ErrId::kImplMismatch2, niceSourceName(srcShaderNode->GetSourceType()),
                                          niceSourceName(renderContext)),
                                 {1, toUfe(*cnx)}});
            }
        }
    }
}

bool UsdMaterialValidator::validateAcyclic()
{
    // Take last source on the connection stack:
    if (m_connectionStack.empty())
    {
        return true;
    }

    auto stackIter = m_connectionStack.rbegin();
    const auto& lastSrcNode = (*stackIter)->m_src.GetPrim();
    if (lastSrcNode.IsA<UsdShadeNodeGraph>())
    {
        // Not checking NodeGraph boundaries as flattening the NodeGraph might resolve the cycle. See the NotACycle
        // test scene for an example.
        return true;
    }

    // Component separate and combine nodes are not part of the cycle
    auto isComponentCnx = [](const auto& cnx) {
        return (isComponentNode(cnx.m_src.GetPrim()) == ComponentNodeType::eCombine ||
                isComponentNode(cnx.m_dst.GetPrim()) == ComponentNodeType::eSeparate);
    };

    for (; stackIter != m_connectionStack.rend(); ++stackIter)
    {
        if (lastSrcNode == (*stackIter)->m_dst.GetPrim())
        {
            // Found a cycle
            LookdevXUfe::Log::Locations locations;
            locations.reserve(std::distance(m_connectionStack.rbegin(), stackIter) + 1);
            // Top of CnxStack is the first back-edge found, so make it the first item in the list:
            for (auto locIt = m_connectionStack.rbegin(); locIt != stackIter; ++locIt)
            {
                if (!isComponentCnx(**locIt))
                {
                    locations.emplace_back(toUfe(**locIt));
                }
            }
            if (!isComponentCnx(**stackIter))
            {
                locations.emplace_back(toUfe(**stackIter));
            }

            m_log->addEntry({m_currentSeverity, errorStr(ErrId::kCycle), std::move(locations)});
            return false;
        }
    }

    return true;
}

void UsdMaterialValidator::reportInvalidSources(const UsdAttribute& dest, const SdfPathVector& invalidSourcePaths)
{
    if (invalidSourcePaths.empty())
    {
        return;
    }

    auto stage = dest.GetPrim().GetStage();
    for (SdfPath const& sourcePath : invalidSourcePaths)
    {

        // Make sure the source node exists
        auto srcPath = Ufe::PathString::string(toUfe(stage, sourcePath.GetPrimPath()));
        UsdPrim sourcePrim = stage->GetPrimAtPath(sourcePath.GetPrimPath());
        if (!sourcePrim)
        {
            m_log->addEntry({m_currentSeverity, errorStr(ErrId::kMissingNode, srcPath), {1, toUfe(dest)}});
            continue;
        }

        // Make sure the source attribute exists
        srcPath = Ufe::PathString::string(toUfe(stage, sourcePath));
        UsdAttribute sourceAttr = stage->GetAttributeAtPath(sourcePath);
        if (!sourceAttr || !sourceAttr.IsAuthored())
        {
            m_log->addEntry({m_currentSeverity, errorStr(ErrId::kMissingAttr, srcPath), {1, toUfe(dest)}});
            continue;
        }

        // Check that the attribute has a legal prefix
        auto [sourceName, sourceType] = UsdShadeUtils::GetBaseNameAndType(sourcePath.GetNameToken());
        if (sourceType == UsdShadeAttributeType::Invalid)
        {
            m_log->addEntry({m_currentSeverity, errorStr(ErrId::kInvalidAttr, srcPath), {1, toUfe(dest)}});
            continue;
        }
    }
}

void UsdMaterialValidator::traverseConnection()
{
    // Look at the source prim:
    const UsdPrim& srcPrim = m_connectionStack.back()->m_src.GetPrim();

    // Find all destinations of this node:
    std::vector<UsdAttribute> destinations;
    if (auto srcNG = UsdShadeNodeGraph(srcPrim))
    {
        // Traverse the NodeGraph connection:
        destinations.push_back(m_connectionStack.back()->m_src);
    }
    else if (auto srcShade = UsdShadeShader(srcPrim))
    {
        // Traverse all inputs:
        for (auto&& input : srcShade.GetInputs())
        {
            destinations.push_back(input.GetAttr());
        }
    }

    for (auto&& dest : destinations)
    {
        visitDestination(dest);
    }
}

} // namespace LookdevXUsd
