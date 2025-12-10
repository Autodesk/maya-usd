//*****************************************************************************
// Copyright (c) 2025 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************

#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION

#include "UsdMxVersionUpgrade.h"

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/namespaceEditor.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdMtlx/materialXConfigAPI.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdShade/shader.h>

#include <ufe/hierarchy.h>

#include <mayaUsdAPI/utils.h>

#include <regex>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Detecting MaterialX version
    (Material)
    (mtlx)
    ((legacyVersionPrefix, "MaterialX v"))
    ((currentMxVersion, MaterialX_VERSION_STRING))

    // All affected node ids:
    (ND_switch_float)
    (ND_switch_color3)
    (ND_switch_color4)
    (ND_switch_vector2)
    (ND_switch_vector3)
    (ND_switch_vector4)
    (ND_switch_floatI)
    (ND_switch_color3I)
    (ND_switch_color4I)
    (ND_switch_vector2I)
    (ND_switch_vector3I)
    (ND_switch_vector4I)
    (ND_swizzle_float_color3)
    (ND_swizzle_float_color4)
    (ND_swizzle_float_vector2)
    (ND_swizzle_float_vector3)
    (ND_swizzle_float_vector4)
    (ND_swizzle_color3_float)
    (ND_swizzle_color3_color3)
    (ND_swizzle_color3_color4)
    (ND_swizzle_color3_vector2)
    (ND_swizzle_color3_vector3)
    (ND_swizzle_color3_vector4)
    (ND_swizzle_color4_float)
    (ND_swizzle_color4_color3)
    (ND_swizzle_color4_color4)
    (ND_swizzle_color4_vector2)
    (ND_swizzle_color4_vector3)
    (ND_swizzle_color4_vector4)
    (ND_swizzle_vector2_float)
    (ND_swizzle_vector2_color3)
    (ND_swizzle_vector2_color4)
    (ND_swizzle_vector2_vector2)
    (ND_swizzle_vector2_vector3)
    (ND_swizzle_vector2_vector4)
    (ND_swizzle_vector3_float)
    (ND_swizzle_vector3_color3)
    (ND_swizzle_vector3_color4)
    (ND_swizzle_vector3_vector2)
    (ND_swizzle_vector3_vector3)
    (ND_swizzle_vector3_vector4)
    (ND_swizzle_vector4_float)
    (ND_swizzle_vector4_color3)
    (ND_swizzle_vector4_color4)
    (ND_swizzle_vector4_vector2)
    (ND_swizzle_vector4_vector3)
    (ND_swizzle_vector4_vector4)
    (ND_dielectric_bsdf)
    (ND_conductor_bsdf)
    (ND_generalized_schlick_bsdf)
    (ND_layer_bsdf)
    (ND_thin_film_bsdf)
    (ND_subsurface_bsdf)
    (ND_atan2_float)
    (ND_atan2_vector2)
    (ND_atan2_vector3)
    (ND_atan2_vector4)
    (ND_normalmap)
    (ND_normalmap_vector2)
    (ND_normalmap_float)

    // Node ids for nodes introduced to help with conversions:
    (ND_convert_vector3_color3)
    (ND_constant_)
    (ND_extract_)
    (ND_convert_)
    (ND_combine)
    (ND_separate)
    (ND_multiply_vector3FA)
    (ND_subtract_vector3FA)
    (ND_normalize_vector3)
    (ND_crossproduct_vector3)

    // MaterialX channel types:
    ((float_, "float"))
    (vector2)
    (vector3)
    (vector4)
    (color3)
    (color4)
    (integer)
    (boolean)
    (filename)
    (string)

    // Input names of interest:
    (in)
    (in1)
    (in2)
    (inx)
    (iny)
    (out)
    (outr)
    (outx)
    (value)
    (which)
    (radius)
    (thickness)
    (thinfilm_thickness)
    (ior)
    (thinfilm_ior)
    (top)
    (base)
    (scatter_mode)
    (channels)
    (index)
    (space)
    (normal)
    (tangent)
    (bitangent)
);
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE
using namespace PXR_NS;

constexpr auto kInvalidChannelIndex = -1;
int _channelIndexMap(const char c)
{
    switch (c)
    {
        case 'r': return 0;
        case 'g': return 1;
        case 'b': return 2;
        case 'a': return 3;
        case 'x': return 0;
        case 'y': return 1;
        case 'z': return 2;
        case 'w': return 3;
    }
    return kInvalidChannelIndex;
}

constexpr auto kInvalidChannelConstant = -1.0F;
float _channelConstantMap(const char c)
{
    switch (c)
    {
        case '0': return 0.0f;
        case '1': return 1.0f;
    }
    return kInvalidChannelConstant;
}

const auto kChannelCountMap = std::unordered_map<TfToken, int, TfToken::HashFunctor>{
        {_tokens->float_, 1},
        {_tokens->color3, 3}, {_tokens->color4, 4},
        {_tokens->vector2, 2}, {_tokens->vector3, 3}, {_tokens->vector4, 4}
    };

bool _isChannelCountPattern(const std::string& pattern, int channelCount)
{
    static const auto kSingleChannelPatterns = std::unordered_set<std::string>{
        "rr", "rrr", "xx", "xxx"
    };
    static const auto kThreeChannelPatterns = std::unordered_set<std::string>{
        "rgb", "xyz"
    };
    static const auto kFourChannelPatterns = std::unordered_set<std::string>{
        "rgb", "xyz",
        "rgba", "xyzw"
    };
    
    switch (channelCount)
    {
        case 1:
            return kSingleChannelPatterns.count(pattern) > 0;
        case 3:
            return kThreeChannelPatterns.count(pattern) > 0;
        case 4:
            return kFourChannelPatterns.count(pattern) > 0;
    }
    return false;
}

using _TfTokenSet = std::unordered_set<TfToken, TfToken::HashFunctor>;

const auto kSwitchNodes = _TfTokenSet{
    _tokens->ND_switch_float,
    _tokens->ND_switch_color3,
    _tokens->ND_switch_color4,
    _tokens->ND_switch_vector2,
    _tokens->ND_switch_vector3,
    _tokens->ND_switch_vector4,
    _tokens->ND_switch_floatI,
    _tokens->ND_switch_color3I,
    _tokens->ND_switch_color4I,
    _tokens->ND_switch_vector2I,
    _tokens->ND_switch_vector3I,
    _tokens->ND_switch_vector4I
};

const auto kSwizzleNodes = _TfTokenSet{
    _tokens->ND_swizzle_float_color3,
    _tokens->ND_swizzle_float_color4,
    _tokens->ND_swizzle_float_vector2,
    _tokens->ND_swizzle_float_vector3,
    _tokens->ND_swizzle_float_vector4,
    _tokens->ND_swizzle_color3_float,
    _tokens->ND_swizzle_color3_color3,
    _tokens->ND_swizzle_color3_color4,
    _tokens->ND_swizzle_color3_vector2,
    _tokens->ND_swizzle_color3_vector3,
    _tokens->ND_swizzle_color3_vector4,
    _tokens->ND_swizzle_color4_float,
    _tokens->ND_swizzle_color4_color3,
    _tokens->ND_swizzle_color4_color4,
    _tokens->ND_swizzle_color4_vector2,
    _tokens->ND_swizzle_color4_vector3,
    _tokens->ND_swizzle_color4_vector4,
    _tokens->ND_swizzle_vector2_float,
    _tokens->ND_swizzle_vector2_color3,
    _tokens->ND_swizzle_vector2_color4,
    _tokens->ND_swizzle_vector2_vector2,
    _tokens->ND_swizzle_vector2_vector3,
    _tokens->ND_swizzle_vector2_vector4,
    _tokens->ND_swizzle_vector3_float,
    _tokens->ND_swizzle_vector3_color3,
    _tokens->ND_swizzle_vector3_color4,
    _tokens->ND_swizzle_vector3_vector2,
    _tokens->ND_swizzle_vector3_vector3,
    _tokens->ND_swizzle_vector3_vector4,
    _tokens->ND_swizzle_vector4_float,
    _tokens->ND_swizzle_vector4_color3,
    _tokens->ND_swizzle_vector4_color4,
    _tokens->ND_swizzle_vector4_vector2,
    _tokens->ND_swizzle_vector4_vector3,
    _tokens->ND_swizzle_vector4_vector4
};

const auto kAtanNodes = _TfTokenSet{
    _tokens->ND_atan2_float,
    _tokens->ND_atan2_vector2,
    _tokens->ND_atan2_vector3,
    _tokens->ND_atan2_vector4
};

const auto kThinFilmBSDF = _TfTokenSet{
        _tokens->ND_dielectric_bsdf,
        _tokens->ND_conductor_bsdf,
        _tokens->ND_generalized_schlick_bsdf
    };

const auto kSwizzleRegex = std::regex("ND_swizzle_([^_]+)_([^_]+)");

const auto kMaterialXToUsdType = std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor>{
        {_tokens->float_, SdfValueTypeNames->Float},
        {_tokens->vector2, SdfValueTypeNames->Float2},
        {_tokens->vector3, SdfValueTypeNames->Float3},
        {_tokens->vector4, SdfValueTypeNames->Float4},
        {_tokens->color3, SdfValueTypeNames->Color3f},
        {_tokens->color4, SdfValueTypeNames->Color4f},
        {_tokens->integer, SdfValueTypeNames->Int},
        {_tokens->boolean, SdfValueTypeNames->Bool},
        {_tokens->filename, SdfValueTypeNames->Asset},
        {_tokens->string, SdfValueTypeNames->String},
    };



namespace {

Ufe::Path _getMaterialPath(const Ufe::Path& materialElementPath)
{
    if (materialElementPath.empty()) {
        return {};
    }

    auto sceneItem = Ufe::Hierarchy::createItem(materialElementPath);

    while (sceneItem && sceneItem->nodeType() != _tokens->Material) {
        auto hierarchy = Ufe::Hierarchy::hierarchy(sceneItem);
        sceneItem = hierarchy->parent();
    }
    if (!sceneItem) {
        return {};
    }
    return sceneItem->path();
}

std::optional<std::string> _isLegacyMaterial(const UsdShadeMaterial& material)
{
    auto mxSurfaceOutput = material.GetSurfaceOutput(_tokens->mtlx);
    if (!mxSurfaceOutput || !mxSurfaceOutput.HasConnectedSource())
    {
        // No MaterialX shading in this material.
        return std::nullopt;
    }
    // Fetch the version from the MaterialXConfigAPI schema:
    auto materialXVersion = std::string{"1.38"}; // Default to 1.38 if not authored.

    const auto configAPI = UsdMtlxMaterialXConfigAPI(material);
    if (configAPI)
    {
        const auto versionAttr = configAPI.GetConfigMtlxVersionAttr();
        if (versionAttr)
        {
            // Got an authored version.
            versionAttr.Get(&materialXVersion);
        }
    }

    if (materialXVersion != _tokens->currentMxVersion.GetString())
    {
        return _tokens->legacyVersionPrefix.GetString() + materialXVersion;
    }
    
    return std::nullopt;
}

std::pair<std::string, std::string> _splitNumericalSuffix(const std::string& srcName)
{
    // Compiled regular expression to find a numerical suffix to a path component.
    // It searches for any number of characters followed by a single non-numeric,
    // then one or more digits at end of string.
    static const auto reSuffix = std::regex("(.*[^0-9])([0-9]+)$");
    std::smatch base_match;
 
    if (std::regex_match(srcName, base_match, reSuffix)) {
        return std::make_pair(base_match[1].str(), base_match[2].str());
    }
    return std::make_pair(srcName, "");
}

TfToken _uniqueName(const std::set<std::string>& existingNames, const std::string& srcName)
{
    auto [base, suffixStr] = _splitNumericalSuffix(srcName);
    size_t suffix = 1;
    size_t suffixLen = 1;
    if (!suffixStr.empty()) {
        suffix = std::stoi(suffixStr) + 1;
        suffixLen = suffixStr.size();
    }

    // Create a suffix string from the number keeping the same number of digits as
    // numerical suffix from input srcName (padding with 0's if needed).
    const auto buildName = [](auto suffix, auto suffixLen, auto& suffixStr, const auto& base) -> std::string {
        suffixStr = std::to_string(suffix);
        suffixStr = std::string("0", suffixLen - std::min(suffixLen, suffixStr.size())) + suffixStr;
        return base + suffixStr;
    };

    auto dstName = buildName(suffix, suffixLen, suffixStr, base);
    while (existingNames.count(dstName) > 0) {
        suffix += 1;
        dstName = buildName(suffix, suffixLen, suffixStr, base);
    }
    return TfToken(dstName);
}

TfToken _uniqueChildName(UsdPrim usdParent, const std::string& name)
{
    if (!usdParent.IsValid()) {
        return {};
    }
    // The prim GetChildren method used the UsdPrimDefaultPredicate which includes
    // active prims. We also need the inactive ones.
    //
    // const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate =
    //			UsdPrimIsActive && UsdPrimIsDefined &&
    //			UsdPrimIsLoaded && !UsdPrimIsAbstract;
    // Note: removed 'UsdPrimIsLoaded' from the predicate. When it is present the
    //		 filter doesn't properly return the inactive prims. UsdView doesn't
    //		 use loaded either in _computeDisplayPredicate().
    // Note: removed 'UsdPrimIsAbstract' from the predicate since when naming
    //       we want to consider all the prims (even if hidden) to generate a real
    //       unique sibling.
    //
    // Note: our UsdHierarchy uses instance proxies, so we also use them here.
    std::set<std::string> childrenNames;
    static const auto predicate = UsdPrimIsActive && UsdPrimIsDefined;
    for (const auto& i : usdParent.GetFilteredChildren(UsdTraverseInstanceProxies(predicate)))
    {
        childrenNames.insert(i.GetName());
    }
    return _uniqueName(childrenNames, name);
}

UsdShadeShader _createSiblingNode(UsdShadeShader node, TfToken shaderId, const std::string& name)
{
    auto ngPrim = node.GetPrim().GetParent();
    // childOrder = ngPrim.GetAllChildrenNames()
    auto newNode = UsdShadeShader::Define(ngPrim.GetStage(), ngPrim.GetPath().AppendChild(_uniqueChildName(ngPrim, name)));
    newNode.SetShaderId(shaderId);
    // TODO: Find a way to make sure the sibling appears in the right order in the outliner.
    //       There is a Usd.Prim.SetChildrenReorder(), but it is only metadata applied
    //       after the fact. We would like a more direct approach.
    // childOrder.insert(childOrder.index(node.GetPrim().GetName()) + 1, newNode.GetPrim().GetName())
    // ngPrim.SetChildrenReorder(childOrder)
    return newNode;
}

using _SourceMap = std::unordered_map<SdfPath, UsdShadeConnectableAPI, SdfPath::Hash>;
using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

_SourceMap _getUpstreamNodes(UsdShadeConnectableAPI startNode, const _TfTokenSet& idFilter, _PathSet& visited)
{
    // Recursively traverse connections of startNode trying to find nodes with ID in idFilter set.
    _SourceMap retVal;
    for (const auto& input : startNode.GetInputs()) 
    {
        for (const auto& sourceInfo : startNode.GetConnectedSources(input))
        {
            auto source = sourceInfo.source;
            auto sourcePrim = source.GetPrim();
            if (visited.count(sourcePrim.GetPath()) > 0) {
                continue;
            }
            visited.insert(sourcePrim.GetPath());
            auto sourceShader = UsdShadeShader(sourcePrim);
            TfToken shaderId;
            if (sourceShader && sourceShader.GetShaderId(&shaderId) && idFilter.count(shaderId) > 0) {
                retVal.emplace(sourcePrim.GetPath(), sourceShader);
            }
            
            const auto upstreamNodes = _getUpstreamNodes(source, idFilter, visited);
            for (const auto& [path, node] : upstreamNodes) {
                retVal.emplace(path, node);
            }
        }
    }
    
    return retVal;
}

using _DownstreamOutputPortList = std::vector<std::pair<UsdShadeConnectableAPI, UsdShadeOutput>>;
using _DownstreamInputPortList = std::vector<std::pair<UsdShadeConnectableAPI, UsdShadeInput>>;

_DownstreamOutputPortList _getDownstreamOutputPorts(UsdShadeConnectableAPI node)
{
    // Returns a list(node, port) of all items that connect to the output port of the node passed in as parameter.
    //    
    // We assume a nicely behaved graph without connections teleporting across NodeGraph boundaries.
    _DownstreamOutputPortList retVal;
    auto ng = UsdShadeConnectableAPI(node.GetPrim().GetParent());
    if (!ng) {
        return retVal;
    }
    // Look for NodeGraph connections:
    for (const auto& output : ng.GetOutputs())
    {    
        for (const auto& sourceInfo : ng.GetConnectedSources(output))
        {
            if (sourceInfo.source.GetPrim() == node.GetPrim())
            {
                retVal.push_back(std::make_pair(ng, output));
            }
        }
    }
    return retVal;
}

_DownstreamInputPortList _getDownstreamInputPorts(UsdShadeConnectableAPI node)
{
    // Returns a list(node, port) of all items that connect to the output port of the node passed in as parameter.
    //    
    // We assume a nicely behaved graph without connections teleporting across NodeGraph boundaries.
    _DownstreamInputPortList retVal;
    auto ng = UsdShadeConnectableAPI(node.GetPrim().GetParent());
    if (!ng) {
        return retVal;
    }
    // Check every node inside the graph:
    for (const auto& child : ng.GetPrim().GetChildren())
    {
        auto shader = UsdShadeShader(child);
        if (!shader) {
            continue;
        }
        for (const auto& input : shader.GetInputs()) {
            for (const auto& sourceInfo : shader.ConnectableAPI().GetConnectedSources(input)) {
                if (sourceInfo.source.GetPrim() == node.GetPrim()) {
                    retVal.push_back(std::make_pair(shader.ConnectableAPI(), input));
                }
            }

        }
    }

    return retVal;
}

void _moveInput(UsdShadeShader sourceNode, const TfToken& sourceInputName, UsdShadeShader destNode, const TfToken& destInputName)
{
    auto editor = UsdNamespaceEditor(sourceNode.GetPrim().GetStage());
    editor.ReparentProperty(sourceNode.GetPrim().GetAttribute(UsdShadeUtils::GetFullName(sourceInputName, UsdShadeAttributeType::Input)),
                            destNode.GetPrim(), UsdShadeUtils::GetFullName(destInputName, UsdShadeAttributeType::Input));
    if (editor.CanApplyEdits()) {
        editor.ApplyEdits();
    } else {
        TF_WARN("Failed to move input '%s' from node '%s' to input '%s' on node '%s'. Please make sure the material layer is writable.",
                sourceInputName.GetText(),
                sourceNode.GetPrim().GetPath().GetText(),
                destInputName.GetText(),
                destNode.GetPrim().GetPath().GetText());
    }
}

void _upgradeMaterial(UsdShadeMaterial usdMaterial)
{
    TF_AXIOM(usdMaterial);

    // This is the upgrade from 1.38 to 1.39. Feel free to split into multiple
    // functions if the upgrade process gets more complex.

    // If this material is already 1.39, then nothing to do:
    if (usdMaterial.GetPrim().HasAPI<UsdMtlxMaterialXConfigAPI>()) {
        auto configAPI = UsdMtlxMaterialXConfigAPI(usdMaterial.GetPrim());
        auto versionAttr = configAPI.GetConfigMtlxVersionAttr();
        std::string versionStr;
        if (versionAttr && versionAttr.Get(&versionStr)) {
            if (versionStr == _tokens->currentMxVersion.GetString()) {
                return;
            }
        }
    }
       
    // Build list of nodes upfront since we will be adding
    // nodes mid-flight which might throw off iterators:
    // Using basic map since we want the same processing order as the Python script we used to develop this code in order to make sure tests match.
    using ShadeNodeMap = std::map<SdfPath, UsdShadeShader>;
    ShadeNodeMap allNodes;
    using PrimList = std::vector<UsdPrim>;
    PrimList toVisit = {usdMaterial.GetPrim()};
    _PathSet visited;
    while (!toVisit.empty()) {
        auto node = toVisit.back();
        toVisit.pop_back();
        if (visited.find(node.GetPath()) != visited.end()) {
            continue;
        }
        visited.insert(node.GetPath());
        auto nodeGraph = UsdShadeNodeGraph(node);
        if (nodeGraph)
        {
            for (const auto& child : node.GetChildren()) {
                toVisit.push_back(child);
            }
            continue;
        }
        auto shader = UsdShadeShader(node);
        if (shader) {
            allNodes.insert({shader.GetPath(), shader});
        }
    }
    
    // No need to look for "channels" as this feature was never supported in USD.

    // Update all nodes.
    PrimList unusedNodes;
    for (auto [nodePath, node] : allNodes) {
        TfToken shaderID;
        if (!node.GetShaderId(&shaderID)) {
            continue;
        }
        if (shaderID == _tokens->ND_layer_bsdf) {

            // Convert layering of thin_film_BSDF nodes to thin-film parameters on the affected BSDF nodes.
            if (!node.GetPrim().HasAttribute(UsdShadeUtils::GetFullName(_tokens->top, UsdShadeAttributeType::Input)) or !node.GetPrim().HasAttribute(UsdShadeUtils::GetFullName(_tokens->base, UsdShadeAttributeType::Input))) {
                continue;
            }
            
            UsdShadeConnectableAPI topSource;
            TfToken sourceName;
            UsdShadeAttributeType sourceType;
            if (!node.ConnectableAPI().GetConnectedSource(node.GetInput(_tokens->top), &topSource, &sourceName, &sourceType)) {
                continue;
            }
            UsdShadeConnectableAPI baseSource;
            if (!node.ConnectableAPI().GetConnectedSource(node.GetInput(_tokens->base), &baseSource, &sourceName, &sourceType)) {
                continue;
            }
            TfToken topSourceShaderId;
            if (!UsdShadeShader(topSource.GetPrim()).GetShaderId(&topSourceShaderId)) {
                continue;
            }
            if (topSourceShaderId == _tokens->ND_thin_film_bsdf) {
                // Apply thin-film parameters to all supported BSDF's upstream.
                _PathSet visitedPaths;
                for (const auto& [upsteamPath, upstream] : _getUpstreamNodes(node.ConnectableAPI(), kThinFilmBSDF, visitedPaths)) {
                    auto scatterModeInputName = UsdShadeUtils::GetFullName(_tokens->scatter_mode, UsdShadeAttributeType::Input);
                    if (upstream.GetPrim().HasAttribute(scatterModeInputName)) {
                        std::string scatterMode = "T";
                        if (upstream.GetPrim().GetAttribute(scatterModeInputName).Get(&scatterMode) && scatterMode == "T") {
                            continue;
                        }
                    }
                    topSource.GetInput(_tokens->thickness).GetAttr().FlattenTo(upstream.GetPrim(), UsdShadeUtils::GetFullName(_tokens->thinfilm_thickness, UsdShadeAttributeType::Input));
                    topSource.GetInput(_tokens->ior).GetAttr().FlattenTo(upstream.GetPrim(), UsdShadeUtils::GetFullName(_tokens->thinfilm_ior, UsdShadeAttributeType::Input));
                }

                // Bypass the thin-film layer operator.
                for (const auto& [downstreamNode, downstreamOutputPort] : _getDownstreamOutputPorts(node.ConnectableAPI())) {
                    downstreamNode.DisconnectSource(downstreamOutputPort, node.GetOutput(_tokens->out));
                    downstreamOutputPort.ConnectToSource(baseSource.GetOutput(_tokens->out));
                }
                for (const auto& [downstreamNode, downstreamInputPort] : _getDownstreamInputPorts(node.ConnectableAPI())) {
                    downstreamNode.DisconnectSource(downstreamInputPort, node.GetOutput(_tokens->out));
                    downstreamInputPort.ConnectToSource(baseSource.GetOutput(_tokens->out));
                }

                // Mark original nodes as unused.
                unusedNodes.push_back(node.GetPrim());
                unusedNodes.push_back(topSource.GetPrim());
            }
        }
        else if (shaderID == _tokens->ND_subsurface_bsdf) {
            auto radiusInput = node.GetInput(_tokens->radius);
            if (radiusInput && radiusInput.GetTypeName() == kMaterialXToUsdType.at(_tokens->vector3)) {
                auto convertNode = _createSiblingNode(node, _tokens->ND_convert_vector3_color3, "convert");
                _moveInput(node, _tokens->radius, convertNode, _tokens->in);
                auto radiusInput = node.CreateInput(_tokens->radius, kMaterialXToUsdType.at(_tokens->color3));
                radiusInput.ConnectToSource(convertNode.CreateOutput(_tokens->out, kMaterialXToUsdType.at(_tokens->color3)));
            }
        }
        else if (kSwitchNodes.count(shaderID) > 0) {
            // Upgrade switch nodes from 5 to 10 inputs, handling the fallback behavior for
            // constant "which" values that were previously out of range.
            auto which = node.GetInput(_tokens->which);
            if (which && which.GetAttr().HasAuthoredValue()) {
                if (which.GetTypeName() == SdfValueTypeNames->Float) {
                    float whichValue = 0.0F;
                    if (which.Get(&whichValue) && whichValue >= 5.0F) {
                        which.Set(0.0F);
                    }
                } else {
                    int whichValue = 0;
                    if (which.Get(&whichValue) && whichValue >= 5) {
                        which.Set(0);
                    }
                }
            }
        }
        else if (kSwizzleNodes.count(shaderID) > 0) {
            auto inInput = node.GetInput(_tokens->in);
            std::smatch swizzleMatch;
            if (!std::regex_match(shaderID.GetString(), swizzleMatch, kSwizzleRegex)) {
                continue;
            }
            
            auto sourceType = TfToken{swizzleMatch[1].str()};
            auto destType = TfToken{swizzleMatch[2].str()};

            auto channelsInput = node.GetInput(_tokens->channels);
            auto channelString = std::string{};
            if (channelsInput) {
                channelsInput.Get(&channelString);
            }
            auto sourceChannelCount = kChannelCountMap.at(sourceType);
            auto destChannelCount = kChannelCountMap.at(destType);

            // We convert to a constant node if "in" input is a constant value:
            auto convertToConstantNode = !inInput || (inInput.GetAttr().HasAuthoredValue() && !inInput.HasConnectedSource());
            // We also convert to a constant node if every destination
            // channel is constant:
            //   eg: "ND_swizzle_color3_color3" node with
            //       "010" in the "channels" input.
            if (!convertToConstantNode) {
                convertToConstantNode = true;
                for (size_t i = 0; i < destChannelCount; ++i) {
                    if (i < channelString.size()) {
                        if (_channelConstantMap(channelString[i]) != kInvalidChannelConstant) {
                            // Still in constant territory:
                            continue;
                        }
                    }
                    // Every other scenario: not constant
                    convertToConstantNode = false;
                    break;
                }
            }

            if (convertToConstantNode) {
                node.SetShaderId(TfToken(_tokens->ND_constant_.GetString() + destType.GetString()));

                std::vector<float> currentValue;
                if (inInput) {
                    if (sourceType == _tokens->float_) {
                        float floatVal = 0.0F;
                        inInput.Get(&floatVal);
                        currentValue.push_back(floatVal);   
                    } else if (sourceType == _tokens->vector2) {
                        GfVec2f vec2Val;
                        inInput.Get(&vec2Val);
                        currentValue.push_back(vec2Val[0]);
                        currentValue.push_back(vec2Val[1]);   
                    } else if (sourceType == _tokens->vector3 || sourceType == _tokens->color3) {
                        GfVec3f vec3Val;
                        inInput.Get(&vec3Val);
                        currentValue.push_back(vec3Val[0]);
                        currentValue.push_back(vec3Val[1]);   
                        currentValue.push_back(vec3Val[2]);
                    } else if (sourceType == _tokens->vector4 || sourceType == _tokens->color4) {
                        GfVec4f vec4Val;
                        inInput.Get(&vec4Val);
                        currentValue.push_back(vec4Val[0]);
                        currentValue.push_back(vec4Val[1]);
                        currentValue.push_back(vec4Val[2]);
                        currentValue.push_back(vec4Val[3]);
                    } 
                }
                else{
                    currentValue.insert(currentValue.begin(), kChannelCountMap.at(sourceType), 0.0F);
                }
                std::vector<float> newValue;
                for (int i = 0; i < destChannelCount; ++i) {
                    if (i < channelString.size()) {
                        auto index = _channelIndexMap(channelString[i]);
                        if (index != kInvalidChannelIndex) {
                            if (index < currentValue.size()) {
                                newValue.push_back(currentValue[index]);
                                continue;
                            }
                        }
                        auto constant = _channelConstantMap(channelString[i]);
                        if (constant != kInvalidChannelConstant) {
                            newValue.push_back(constant);
                            continue;
                        }
                    }
                     // Invalid channel name, or missing channel name:
                    newValue.push_back(currentValue[0]);
                }

                auto valueInput = node.CreateInput ( _tokens->value, kMaterialXToUsdType.at(destType));
                if (destType == _tokens->float_) {
                    valueInput.Set(newValue[0]);
                } else if (destType == _tokens->vector2) {
                    GfVec2f vec2Val(newValue[0], newValue[1]);
                    valueInput.Set(vec2Val);
                } else if (destType == _tokens->vector3 || destType == _tokens->color3) {
                    GfVec3f vec3Val(newValue[0], newValue[1], newValue[2]);
                    valueInput.Set(vec3Val);
                } else if (destType == _tokens->vector4 || destType == _tokens->color4) {
                    GfVec4f vec4Val(newValue[0], newValue[1], newValue[2], newValue[3]);
                    valueInput.Set(vec4Val);
                }

                if (inInput) {
                    auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
                    editor.DeleteProperty(inInput.GetAttr());
                    if (editor.CanApplyEdits()) {
                        editor.ApplyEdits();
                    } else {
                        TF_WARN("Failed to delete 'in' input after upgrading swizzle to constant node. Please make sure the material layer is writable.");
                    }
                }
            }
            else if (destChannelCount == 1) {
                // Replace swizzle with extract.
                node.SetShaderId(TfToken(_tokens->ND_extract_.GetString() + sourceType.GetString()));
                auto index = _channelIndexMap(channelString[0]);
                if (index != kInvalidChannelIndex) {
                    node.CreateInput(_tokens->index, kMaterialXToUsdType.at(_tokens->integer)).Set(index);
                }
            }

            else if (sourceType != destType and _isChannelCountPattern(channelString, sourceChannelCount)) {
                // Replace swizzle with convert.
                node.SetShaderId(TfToken(_tokens->ND_convert_.GetString() + sourceType.GetString() + "_" + destType.GetString()));
            }
            else if (sourceChannelCount == 1) {
                // Replace swizzle with combine.
                node.SetShaderId(TfToken(_tokens->ND_combine.GetString() + std::to_string(destChannelCount) + "_" + destType.GetString()));
                for (int i = 0; i < destChannelCount; ++i) {
                    if (i < channelString.size() && _channelConstantMap(channelString[i]) != kInvalidChannelConstant) {
                        auto constant = _channelConstantMap(channelString[i]);
                        auto combineInInput = node.CreateInput(TfToken(_tokens->in.GetString() + std::to_string(i+1)), kMaterialXToUsdType.at(_tokens->float_));
                        combineInInput.Set(constant);
                        continue;
                    }
                    else {
                        inInput.GetAttr().FlattenTo(node.GetPrim(), UsdShadeUtils::GetFullName(TfToken(_tokens->in.GetString() + std::to_string(i+1)), UsdShadeAttributeType::Input));
                    }
                }
                auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
                editor.DeleteProperty(inInput.GetAttr());
                if (editor.CanApplyEdits()) {
                    editor.ApplyEdits();
                } else {
                    TF_WARN("Failed to delete 'in' input after upgrading swizzle to combine node. Please make sure the material layer is writable.");
                }
            }
            else {
                // Replace swizzle with separate and combine.
                auto separateNode = _createSiblingNode(node, TfToken(_tokens->ND_separate.GetString() + std::to_string(sourceChannelCount) + "_" + sourceType.GetString()), "separate");

                node.SetShaderId(TfToken(_tokens->ND_combine.GetString() + std::to_string(destChannelCount) + "_" + destType.GetString()));
                for (int i = 0; i < destChannelCount; ++i) {
                    auto combineInInput = node.CreateInput(TfToken(_tokens->in.GetString() + std::to_string(i+1)), kMaterialXToUsdType.at(_tokens->float_));
                    if (i < channelString.size()) {
                        auto index = _channelIndexMap(channelString[i]);
                        auto constant = _channelConstantMap(channelString[i]);
                        if (index != kInvalidChannelIndex) {
                            combineInInput.ConnectToSource(separateNode.CreateOutput(TfToken(_tokens->out.GetString() + std::string(1, channelString[i])), kMaterialXToUsdType.at(_tokens->float_)));
                            continue;
                        }
                        else if (constant != kInvalidChannelConstant) {
                            combineInInput.Set(constant);
                            continue;
                        }
                    }
                    // Invalid channel name, or missing channel name:
                    auto separateOutputName = (sourceType == _tokens->color3 || sourceType == _tokens->color4) ? _tokens->outr : _tokens->outx;
                    combineInInput.ConnectToSource(separateNode.CreateOutput(separateOutputName, kMaterialXToUsdType.at(_tokens->float_)));
                }
                _moveInput(node, inInput.GetBaseName(), separateNode, _tokens->in);
            }

            // Remove the channels input from the converted node.
            if (channelsInput) {
                auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
                editor.DeleteProperty(channelsInput.GetAttr());
                if (editor.CanApplyEdits()) {
                    editor.ApplyEdits();
                } else {
                    TF_WARN("Failed to delete 'channels' input after upgrading swizzle node. Please make sure the material layer is writable.");
                }
            }
        }
        else if (kAtanNodes.count(shaderID)) {
            auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
            auto input1 = node.GetInput(_tokens->in1);
            if (input1) {
                input1.GetAttr().FlattenTo(node.GetPrim(), UsdShadeUtils::GetFullName(_tokens->iny, UsdShadeAttributeType::Input));
                editor.DeleteProperty(input1.GetAttr());
                if (editor.CanApplyEdits()) {
                    editor.ApplyEdits();
                } else {
                    TF_WARN("Failed to delete 'in1' input after upgrading atan2 node. Please make sure the material layer is writable.");
                }
            }
            auto input2 = node.GetInput(_tokens->in2);
            if (input2) {
                input2.GetAttr().FlattenTo(node.GetPrim(), UsdShadeUtils::GetFullName(_tokens->inx, UsdShadeAttributeType::Input));
                editor.DeleteProperty(input2.GetAttr());
                if (editor.CanApplyEdits()) {
                    editor.ApplyEdits();
                } else {
                    TF_WARN("Failed to delete 'in2' input after upgrading atan2 node. Please make sure the material layer is writable.");
                }
            }
        }
        else if (shaderID == _tokens->ND_normalmap || shaderID == _tokens->ND_normalmap_vector2) {
            auto space = node.GetInput(_tokens->space);
            auto spaceValue = std::string{};
            if (space) {
                space.Get(&spaceValue);
            }
            if (space && spaceValue == "object") {
                // Replace object-space normalmap with a graph.
                auto multiply = _createSiblingNode(node, _tokens->ND_multiply_vector3FA, "multiply");
                _moveInput(node, _tokens->in, multiply, _tokens->in1);
                multiply.CreateInput(_tokens->in2, kMaterialXToUsdType.at(_tokens->float_)).Set(2.0F);
                auto subtract = _createSiblingNode(node, _tokens->ND_subtract_vector3FA, "subtract");
                subtract.CreateInput(_tokens->in1, kMaterialXToUsdType.at(_tokens->vector3)).ConnectToSource(multiply.CreateOutput(_tokens->out, kMaterialXToUsdType.at(_tokens->vector3)));
                subtract.CreateInput(_tokens->in2, kMaterialXToUsdType.at(_tokens->float_)).Set(1.0F);
                node.SetShaderId(_tokens->ND_normalize_vector3);
                for (auto input : node.GetInputs()) {
                    auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
                    editor.DeleteProperty(input.GetAttr());
                    if (editor.CanApplyEdits()) {
                        editor.ApplyEdits();
                    } else {
                        TF_WARN("Failed to delete input '%s' after upgrading normalmap node. Please make sure the material layer is writable.",
                                input.GetBaseName().GetText());
                    }
                }
                node.CreateInput(_tokens->in, kMaterialXToUsdType.at(_tokens->vector3)).ConnectToSource(subtract.CreateOutput(_tokens->out, kMaterialXToUsdType.at(_tokens->vector3)));
            }
            else {
                // Clear tangent-space input.
                auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
                editor.DeleteProperty(space.GetAttr());
                if (editor.CanApplyEdits()) {
                    editor.ApplyEdits();
                } else {
                    TF_WARN("Failed to delete 'space' input after upgrading normalmap node. Please make sure the material layer is writable.");
                }

                // If the normal or tangent inputs are set and the bitangent input is not, 
                // the bitangent should be set to normalize(cross(N, T))
                auto normalInput = node.GetInput(_tokens->normal);
                auto tangentInput = node.GetInput(_tokens->tangent);
                auto bitangentInput = node.GetInput(_tokens->bitangent);
                if ((normalInput || tangentInput) && !bitangentInput) {
                    auto ngPrim = node.GetPrim().GetParent();
                    auto crossNode = _createSiblingNode(node, _tokens->ND_crossproduct_vector3, "normalmap_cross");
                    normalInput.GetAttr().FlattenTo(crossNode.GetPrim(), UsdShadeUtils::GetFullName(_tokens->in1, UsdShadeAttributeType::Input));
                    tangentInput.GetAttr().FlattenTo(crossNode.GetPrim(), UsdShadeUtils::GetFullName(_tokens->in2, UsdShadeAttributeType::Input));

                    auto normalizeNode = _createSiblingNode(node, _tokens->ND_normalize_vector3, "normalmap_cross_norm");
                    normalizeNode.CreateInput(_tokens->in, kMaterialXToUsdType.at(_tokens->vector3)).ConnectToSource(crossNode.CreateOutput(_tokens->out, kMaterialXToUsdType.at(_tokens->vector3)));
                    node.CreateInput(_tokens->bitangent, kMaterialXToUsdType.at(_tokens->vector3)).ConnectToSource(normalizeNode.CreateOutput(_tokens->out, kMaterialXToUsdType.at(_tokens->vector3)));
                }

                node.SetShaderId(_tokens->ND_normalmap_float);
            }
        }
    }
    // Delete unused nodes:    
    auto editor = UsdNamespaceEditor(usdMaterial.GetPrim().GetStage());
    for (auto node : unusedNodes) {
        editor.DeletePrim(node.GetPrim());
        if (editor.CanApplyEdits()){
            editor.ApplyEdits();
        } else {
            TF_WARN("Failed to delete obsolete node '%s' after material upgrade. Please make sure the material layer is writable.",
                    node.GetPath().GetText());
        }
    }

    // Update the version attribute using the MaterialXConfigAPI schema:
    auto configAPI = UsdMtlxMaterialXConfigAPI::Apply(usdMaterial.GetPrim());
    auto versionAttr = configAPI.CreateConfigMtlxVersionAttr();
    versionAttr.Set(_tokens->currentMxVersion.GetString());
}

} // namespace

namespace LookdevXUsd::Version
{

std::optional<std::string> isLegacyShaderGraph(const Ufe::Path& materialElementPath)
{
    const auto materialPath = _getMaterialPath(materialElementPath);
    if (materialPath.empty()) {
        return std::nullopt;
    }

    const auto materialItem = Ufe::Hierarchy::createItem(materialPath);
    auto materialPrim = UsdShadeMaterial(MayaUsdAPI::getPrimForUsdSceneItem(materialItem));
    if (!materialPrim)
    {
        return std::nullopt;
    }
    return _isLegacyMaterial(materialPrim);
}

void UpgradeStage(const Ufe::Path& stagePath)
{
    auto stage = MayaUsdAPI::getStage(stagePath);
    if (!stage)
    {
        return;
    }

    for (const auto& prim : stage->Traverse()) {
        if (UsdShadeMaterial materialPrim = UsdShadeMaterial(prim); materialPrim) {
            if (_isLegacyMaterial(materialPrim)) {
                _upgradeMaterial(materialPrim);
            }
        }
    }
}

void UpgradeMaterial(const Ufe::Path& materialPath)
{
    const auto adjustedMaterialPath = _getMaterialPath(materialPath);
    const auto materialItem = Ufe::Hierarchy::createItem(adjustedMaterialPath);
    auto materialPrim = UsdShadeMaterial(MayaUsdAPI::getPrimForUsdSceneItem(materialItem));
    if (materialPrim && _isLegacyMaterial(materialPrim)) {
        _upgradeMaterial(materialPrim);
    }
}

UsdMxUpgradeMaterialCmd::Ptr UsdMxUpgradeMaterialCmd::create(const Ufe::Path& materialPath)
{
    const auto adjustedMaterialPath = _getMaterialPath(materialPath);
    if (!adjustedMaterialPath.empty() && isLegacyShaderGraph(adjustedMaterialPath)) {
        return std::make_shared<UsdMxUpgradeMaterialCmd>(adjustedMaterialPath);
    }
    return {};
}

UsdMxUpgradeMaterialCmd::UsdMxUpgradeMaterialCmd(const Ufe::Path& materialPath) : _materialPath(materialPath)
{
}

UsdMxUpgradeMaterialCmd::~UsdMxUpgradeMaterialCmd() = default;

void UsdMxUpgradeMaterialCmd::execute()
{
    // I do hope the undo block is strong enough to track multi-layer changes because the upgrade process can go dig into a nested material layer and execute some changes there.
    MayaUsdAPI::UsdUndoBlock undoBlock(&_undoableItem);

    const auto materialItem = Ufe::Hierarchy::createItem(_materialPath);
    auto materialPrim = UsdShadeMaterial(MayaUsdAPI::getPrimForUsdSceneItem(materialItem));
    if (materialPrim && _isLegacyMaterial(materialPrim)) {
        _upgradeMaterial(materialPrim);
    }
}

void UsdMxUpgradeMaterialCmd::undo()
{
    _undoableItem.undo();
}

void UsdMxUpgradeMaterialCmd::redo()
{
    _undoableItem.redo();
}

UsdMxUpgradeStageCmd::Ptr UsdMxUpgradeStageCmd::create(const Ufe::Path& stagePath)
{
    auto retVal = std::make_shared<UsdMxUpgradeStageCmd>(stagePath);
    if (retVal->cmdsList().empty()) {
        return nullptr;
    }
    return retVal;
}

UsdMxUpgradeStageCmd::UsdMxUpgradeStageCmd(const Ufe::Path& stagePath)
{
    // Could traverse the stage using only Ufe::Hierarchy, but I think going full USD is going to be faster.
    auto stage = MayaUsdAPI::getStage(stagePath);
    TF_AXIOM(stage);

    for (const auto& prim : stage->Traverse()) {
        if (const UsdShadeMaterial materialPrim = UsdShadeMaterial(prim); materialPrim) {
            if (_isLegacyMaterial(materialPrim)) {
                // Recreate Ufe path:
                const PXR_NS::SdfPath& materialSdfPath = materialPrim.GetPath();
                const Ufe::Path materialUfePath = MayaUsdAPI::usdPathToUfePathSegment(materialSdfPath);

                // Construct a UFE path consisting of two segments:
                // 1. The path to the USD stage
                // 2. The path to our material
                const auto stagePathSegments = stagePath.getSegments();
                const auto& materialPathSegments = materialUfePath.getSegments();
                if (stagePathSegments.empty() || materialPathSegments.empty())
                {
                    continue;
                }

                append(UsdMxUpgradeMaterialCmd::create({{stagePathSegments[0], materialPathSegments[0]}}));
            }
        }
    }
}

UsdMxUpgradeStageCmd::~UsdMxUpgradeStageCmd() = default;

} // namespace LookdevXUsd::Version

#endif // LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION