//
// Copyright 2021 Autodesk
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
#include "Utils.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/utils/util.h>

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/fileio/primUpdaterManager.h>
#endif

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#endif

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObjectHandle.h>
#include <ufe/hierarchy.h>
#include <ufe/object3d.h>
#include <ufe/object3dHandler.h>
#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#include <ufe/selection.h>
#include <ufe/types.h>

#include <cassert>
#include <cctype>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor> TokenToSdfTypeMap;

} // anonymous namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------

extern Ufe::Rtid g_MayaRtid;

// Cache of Maya node types we've queried before for inheritance from the
// gateway node type.
std::unordered_map<std::string, bool> g_GatewayType;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

UsdStageWeakPtr getStage(const Ufe::Path& path, bool rebuildCacheIfNeeded)
{
    return UsdStageMap::getInstance().stage(path, rebuildCacheIfNeeded);
}

Ufe::Path stagePath(UsdStageWeakPtr stage) { return UsdStageMap::getInstance().path(stage); }

TfHashSet<UsdStageWeakPtr, TfHash> getAllStages() { return UsdStageMap::getInstance().allStages(); }

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
    // When called we do not make any assumption on whether or not the
    // input path is valid.

    const Ufe::Path ufePrimPath = UsdUfe::stripInstanceIndexFromUfePath(path);

    const Ufe::Path::Segments& segments = ufePrimPath.getSegments();
    if (!TF_VERIFY(!segments.empty(), kIllegalUFEPath, path.string().c_str())) {
        return UsdPrim();
    }
    auto stage = getStage(Ufe::Path(segments[0]));
    if (!stage) {
        // Do not output any TF message here (such as TF_WARN). A low-level function
        // like this should not be outputting any warnings messages. It is allowed to
        // call this method with a properly composed Ufe path, but one that doesn't
        // actually point to any valid prim.
        return UsdPrim();
    }

    // If there is only a single segment in the path, it must point to the
    // proxy shape, otherwise we would not have retrieved a valid stage.
    // The second path segment is the USD path.
    return (segments.size() == 1u)
        ? stage->GetPseudoRoot()
        : stage->GetPrimAtPath(SdfPath(segments[1].string()).GetPrimPath());
}

UsdSceneItem::Ptr
createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName)
{
    auto ufeSiblingPath = ufeSrcPath.sibling(Ufe::PathComponent(siblingName));
    return UsdSceneItem::create(ufeSiblingPath, ufePathToPrim(ufeSiblingPath));
}

std::string uniqueChildNameMayaStandard(const PXR_NS::UsdPrim& usdParent, const std::string& name)
{
    if (!usdParent.IsValid())
        return std::string();

    // See uniqueChildNameDefault() in lib\usdUfe\ufe\Utils.cpp for details.
    TfToken::HashSet allChildrenNames;
    for (auto child : usdParent.GetFilteredChildren(
             UsdTraverseInstanceProxies(UsdPrimIsDefined && !UsdPrimIsAbstract))) {
        allChildrenNames.insert(child.GetName());
    }

    // When setting unique name Maya will look at the numerical suffix of all
    // matching names and set the unique name to +1 on the greatest suffix.
    // Example: with siblings Capsule001 & Capsule006, duplicating Capsule001
    //          will set new unique name to Capsule007.
    std::string childName { name };
    if (allChildrenNames.find(TfToken(childName)) != allChildrenNames.end()) {
        // Get the base name (removing the numerical suffix) so that we can compare
        // that to all the sibling names.
        std::string baseName, suffix;
        splitNumericalSuffix(childName, baseName, suffix);
        int suffixValue = !suffix.empty() ? std::stoi(suffix) : 0;

        std::string                 childBaseName;
        std::pair<std::string, int> largestMatching(childName, suffixValue);
        for (const auto& child : allChildrenNames) {
            // While iterating thru all the children look for ones that match
            // the base name of the input. When we find one check its numerical
            // suffix and store the greatest one.
            splitNumericalSuffix(child.GetString(), childBaseName, suffix);
            if (baseName == childBaseName) {
                int suffixValue = !suffix.empty() ? std::stoi(suffix) : 0;
                if (suffixValue > largestMatching.second) {
                    largestMatching = std::make_pair(child.GetString(), suffixValue);
                }
            }
        }

        // By sending in the largest matching name (instead of the input name)
        // the unique name function will increment its numerical suffix by 1
        // and thus it will be unique and follow Maya naming standard.
        childName = uniqueName(allChildrenNames, largestMatching.first);
    }
    return childName;
}

bool isAGatewayType(const std::string& mayaNodeType)
{
    // If we've seen this node type before, return the cached value.
    auto iter = g_GatewayType.find(mayaNodeType);
    if (iter != std::end(g_GatewayType)) {
        return iter->second;
    }

    // Note: we are calling the MEL interpreter to determine the inherited types,
    //		 but we are then caching the result. So MEL will only be called once
    //		 for each node type.
    // Not seen before, so ask Maya.
    // When the inherited flag is used, the command returns a string array containing
    // the names of all the base node types inherited by the the given node.
    MString      cmd;
    MStringArray inherited;
    bool         isInherited = false;
    cmd.format("nodeType -inherited -isTypeName ^1s", mayaNodeType.c_str());
    if (MS::kSuccess == MGlobal::executeCommand(cmd, inherited)) {
        MString gatewayNodeType(ProxyShapeHandler::gatewayNodeType().c_str());
        isInherited = inherited.indexOf(gatewayNodeType) != -1;
        g_GatewayType[mayaNodeType] = isInherited;
    }
    return isInherited;
}

bool isMaterialsScope(const Ufe::SceneItem::Ptr& item)
{
    if (!item) {
        return false;
    }

    // Must be a scope.
    if (item->nodeType() != "Scope") {
        return false;
    }

    // With the magic name.
    if (item->nodeName() == UsdMayaJobExportArgs::GetDefaultMaterialsScopeName()) {
        return true;
    }

    // Or with only materials inside
    auto scopeHierarchy = Ufe::Hierarchy::hierarchy(item);
    if (scopeHierarchy) {
        for (auto&& child : scopeHierarchy->children()) {
            if (child->nodeType() != "Material") {
                // At least one non material
                return false;
            }
        }
    }

    return true;
}

Ufe::Path dagPathToUfe(const MDagPath& dagPath)
{
    // This function can only create UFE Maya scene items with a single
    // segment, as it is only given a Dag path as input.
    return Ufe::Path(dagPathToPathSegment(dagPath));
}

Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath)
{
    MStatus status;
    // The Ufe path includes a prepended "world" that the dag path doesn't have
    size_t                       numUfeComponents = dagPath.length(&status) + 1;
    Ufe::PathSegment::Components components;
    components.resize(numUfeComponents);
    components[0] = Ufe::PathComponent("world");
    MDagPath path = dagPath; // make an editable copy

    // Pop nodes off the path string one by one, adding them to the correct
    // position in the components vector as we go. Use i>0 as the stopping
    // condition because we've already written to element 0 of the components
    // vector.
    for (int i = numUfeComponents - 1; i > 0; i--) {
        MObject node = path.node(&status);

        if (MS::kSuccess != status)
            return Ufe::PathSegment("", g_MayaRtid, '|');

        std::string componentString(MFnDependencyNode(node).name(&status).asChar());

        if (MS::kSuccess != status)
            return Ufe::PathSegment("", g_MayaRtid, '|');

        components[i] = componentString;
        path.pop(1);
    }

    return Ufe::PathSegment(std::move(components), g_MayaRtid, '|');
}

MDagPath ufeToDagPath(const Ufe::Path& ufePath)
{
    if (ufePath.runTimeId() != g_MayaRtid || ufePath.nbSegments() > 1) {
        return MDagPath();
    }
    return UsdMayaUtil::nameToDagPath(Ufe::PathString::string(ufePath));
}

bool isMayaWorldPath(const Ufe::Path& ufePath)
{
    return (ufePath.runTimeId() == g_MayaRtid && ufePath.size() == 1);
}

MayaUsdProxyShapeBase* getProxyShape(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return nullptr;
    }

    return UsdStageMap::getInstance().proxyShapeNode(path);
}

UsdTimeCode getTime(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return UsdTimeCode::Default();
    }

    // Proxy shape node should not be null.
    auto proxyShape = UsdStageMap::getInstance().proxyShapeNode(path);
    if (!TF_VERIFY(proxyShape)) {
        return UsdTimeCode::Default();
    }

    return proxyShape->getTime();
}

TfTokenVector getProxyShapePurposes(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return TfTokenVector();
    }

    // Proxy shape node should not be null.
    auto proxyShape = UsdStageMap::getInstance().proxyShapeNode(path);
    if (!TF_VERIFY(proxyShape)) {
        return TfTokenVector();
    }

    bool renderPurpose, proxyPurpose, guidePurpose;
    proxyShape->getDrawPurposeToggles(&renderPurpose, &proxyPurpose, &guidePurpose);
    TfTokenVector purposes;
    if (renderPurpose) {
        purposes.emplace_back(UsdGeomTokens->render);
    }
    if (proxyPurpose) {
        purposes.emplace_back(UsdGeomTokens->proxy);
    }
    if (guidePurpose) {
        purposes.emplace_back(UsdGeomTokens->guide);
    }

    return purposes;
}

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    for (PXR_NS::SdfPath path : connectedAttrs) {
        if (path == srcUsdAttr.GetPath()) {
            return true;
        }
    }

    return false;
}

bool canRemoveSrcProperty(const PXR_NS::UsdAttribute& srcAttr)
{

    // Do not remove if it has a value.
    if (srcAttr.HasValue()) {
        return false;
    }

    PXR_NS::SdfPathVector connectedAttrs;
    srcAttr.GetConnections(&connectedAttrs);

    // Do not remove if it has connections.
    if (!connectedAttrs.empty()) {
        return false;
    }

    const auto prim = srcAttr.GetPrim();

    if (!prim) {
        return false;
    }

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);

    if (!ngPrim) {
        const auto primParent = prim.GetParent();

        if (!primParent) {
            return false;
        }

        // Do not remove if there is a connection with a prim.
        for (const auto& childPrim : primParent.GetChildren()) {
            if (childPrim != prim) {
                for (const auto& attribute : childPrim.GetAttributes()) {
                    const PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();
                    if (isConnected(srcAttr, dstUsdAttr)) {
                        return false;
                    }
                }
            }
        }

        // Do not remove if there is a connection with the parent prim.
        for (const auto& attribute : primParent.GetAttributes()) {
            const PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();
            if (isConnected(srcAttr, dstUsdAttr)) {
                return false;
            }
        }

        return true;
    }

    // Do not remove boundary properties even if there are connections.
    return false;
}

bool canRemoveDstProperty(const PXR_NS::UsdAttribute& dstAttr)
{

    // Do not remove if it has a value.
    if (dstAttr.HasValue()) {
        return false;
    }

    PXR_NS::SdfPathVector connectedAttrs;
    dstAttr.GetConnections(&connectedAttrs);

    // Do not remove if it has connections.
    if (!connectedAttrs.empty()) {
        return false;
    }

    const auto prim = dstAttr.GetPrim();

    if (!prim) {
        return false;
    }

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);

    if (!ngPrim) {
        return true;
    }

    UsdShadeMaterial asMaterial(prim);
    if (asMaterial) {
        const TfToken baseName = dstAttr.GetBaseName();
        // Remove Material intrinsic outputs since they are re-created automatically.
        if (baseName == UsdShadeTokens->surface || baseName == UsdShadeTokens->volume
            || baseName == UsdShadeTokens->displacement) {
            return true;
        }
    }

    // Do not remove boundary properties even if there are connections.
    return false;
}

namespace {
// Do not expose that function. The input parameter does not provide enough information to
// distinguish between kEnum and kEnumString.
Ufe::Attribute::Type _UsdTypeToUfe(const SdfValueTypeName& usdType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<size_t, Ufe::Attribute::Type> sUsdTypeToUfe {
        { SdfValueTypeNames->Bool.GetHash(), Ufe::Attribute::kBool },           // bool
        { SdfValueTypeNames->Int.GetHash(), Ufe::Attribute::kInt },             // int32_t
        { SdfValueTypeNames->Float.GetHash(), Ufe::Attribute::kFloat },         // float
        { SdfValueTypeNames->Double.GetHash(), Ufe::Attribute::kDouble },       // double
        { SdfValueTypeNames->String.GetHash(), Ufe::Attribute::kString },       // std::string
        { SdfValueTypeNames->Token.GetHash(), Ufe::Attribute::kString },        // TfToken
        { SdfValueTypeNames->Int3.GetHash(), Ufe::Attribute::kInt3 },           // GfVec3i
        { SdfValueTypeNames->Float3.GetHash(), Ufe::Attribute::kFloat3 },       // GfVec3f
        { SdfValueTypeNames->Double3.GetHash(), Ufe::Attribute::kDouble3 },     // GfVec3d
        { SdfValueTypeNames->Color3f.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3f
        { SdfValueTypeNames->Color3d.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3d
#ifdef UFE_V4_FEATURES_AVAILABLE
        { SdfValueTypeNames->Asset.GetHash(), Ufe::Attribute::kFilename },      // SdfAssetPath
        { SdfValueTypeNames->Float2.GetHash(), Ufe::Attribute::kFloat2 },       // GfVec2f
        { SdfValueTypeNames->Float4.GetHash(), Ufe::Attribute::kFloat4 },       // GfVec4f
        { SdfValueTypeNames->Color4f.GetHash(), Ufe::Attribute::kColorFloat4 }, // GfVec4f
        { SdfValueTypeNames->Color4d.GetHash(), Ufe::Attribute::kColorFloat4 }, // GfVec4d
        { SdfValueTypeNames->Matrix3d.GetHash(), Ufe::Attribute::kMatrix3d },   // GfMatrix3d
        { SdfValueTypeNames->Matrix4d.GetHash(), Ufe::Attribute::kMatrix4d },   // GfMatrix4d
#endif
    };
    const auto iter = sUsdTypeToUfe.find(usdType.GetHash());
    if (iter != sUsdTypeToUfe.end()) {
        return iter->second;
    } else {
        static const std::unordered_map<std::string, Ufe::Attribute::Type> sCPPTypeToUfe {
            // There are custom Normal3f, Point3f types in USD. They can all be recognized by the
            // underlying CPP type and if there is a Ufe type that matches, use it.
            { "GfVec3i", Ufe::Attribute::kInt3 },   { "GfVec3d", Ufe::Attribute::kDouble3 },
            { "GfVec3f", Ufe::Attribute::kFloat3 },
#ifdef UFE_V4_FEATURES_AVAILABLE
            { "GfVec2f", Ufe::Attribute::kFloat2 }, { "GfVec4f", Ufe::Attribute::kFloat4 },
#endif
        };

        const auto iter = sCPPTypeToUfe.find(usdType.GetCPPTypeName());
        if (iter != sCPPTypeToUfe.end()) {
            return iter->second;
        } else {
            return Ufe::Attribute::kGeneric;
        }
    }
}
} // namespace

Ufe::Attribute::Type usdTypeToUfe(const SdrShaderPropertyConstPtr& shaderProperty)
{
    Ufe::Attribute::Type retVal = Ufe::Attribute::kInvalid;

    const SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    if (typeName.GetHash() == SdfValueTypeNames->Token.GetHash()) {
        static const TokenToSdfTypeMap tokenTypeToSdfType
            = { { SdrPropertyTypes->Int, SdfValueTypeNames->Int },
                { SdrPropertyTypes->String, SdfValueTypeNames->String },
                { SdrPropertyTypes->Float, SdfValueTypeNames->Float },
                { SdrPropertyTypes->Color, SdfValueTypeNames->Color3f },
#if defined(USD_HAS_COLOR4_SDR_SUPPORT)
                { SdrPropertyTypes->Color4, SdfValueTypeNames->Color4f },
#endif
                { SdrPropertyTypes->Point, SdfValueTypeNames->Point3f },
                { SdrPropertyTypes->Normal, SdfValueTypeNames->Normal3f },
                { SdrPropertyTypes->Vector, SdfValueTypeNames->Vector3f },
                { SdrPropertyTypes->Matrix, SdfValueTypeNames->Matrix4d } };
        TokenToSdfTypeMap::const_iterator it
            = tokenTypeToSdfType.find(shaderProperty->GetTypeAsSdfType().second);
        if (it != tokenTypeToSdfType.end()) {
            retVal = _UsdTypeToUfe(it->second);
        } else {
#if PXR_VERSION < 2205
            // Pre-22.05 boolean inputs are special:
            if (shaderProperty->GetType() == SdfValueTypeNames->Bool.GetAsToken()) {
                retVal = _UsdTypeToUfe(SdfValueTypeNames->Bool);
            } else
#endif
                // There is no Matrix3d type in Sdr, so we need to infer it from Sdf until a fix
                // similar to what was done to booleans is submitted to USD. This also means that
                // there will be no default value for that type.
                if (shaderProperty->GetType() == SdfValueTypeNames->Matrix3d.GetAsToken()) {
                retVal = _UsdTypeToUfe(SdfValueTypeNames->Matrix3d);
            } else {
                retVal = Ufe::Attribute::kGeneric;
            }
        }
    } else {
        retVal = _UsdTypeToUfe(typeName);
    }

    if (retVal == Ufe::Attribute::kString) {
        if (!shaderProperty->GetOptions().empty()) {
            retVal = Ufe::Attribute::kEnumString;
        }
#ifdef UFE_V4_FEATURES_AVAILABLE
        else if (shaderProperty->IsAssetIdentifier()) {
            retVal = Ufe::Attribute::kFilename;
        }
#endif
    }

    return retVal;
}

Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::UsdAttribute& usdAttr)
{
    if (usdAttr.IsValid()) {
        const SdfValueTypeName typeName = usdAttr.GetTypeName();
        Ufe::Attribute::Type   type = _UsdTypeToUfe(typeName);
        if (type == Ufe::Attribute::kString) {
            // Both std::string and TfToken resolve to kString, but if there is a list of allowed
            // tokens, then we use kEnumString instead.
            if (usdAttr.GetPrim().GetPrimDefinition().GetPropertyMetadata<VtTokenArray>(
                    usdAttr.GetName(), SdfFieldKeys->AllowedTokens, nullptr)) {
                type = Ufe::Attribute::kEnumString;
            }
            UsdShadeNodeGraph asNodeGraph(usdAttr.GetPrim());
            if (asNodeGraph) {
                // NodeGraph inputs can have enum metadata on them when they export an inner enum.
                const auto portType = UsdShadeUtils::GetBaseNameAndType(usdAttr.GetName()).second;
                if (portType == UsdShadeAttributeType::Input) {
                    const auto input = UsdShadeInput(usdAttr);
                    if (!input.GetSdrMetadataByKey(TfToken("enum")).empty()) {
                        return Ufe::Attribute::kEnumString;
                    }
                }
                // TfToken is also used in UsdShade as a Generic placeholder for connecting struct
                // I/O.
                if (usdAttr.GetTypeName() == SdfValueTypeNames->Token
                    && portType != UsdShadeAttributeType::Invalid) {
                    type = Ufe::Attribute::kGeneric;
                }
            }
        }
        return type;
    }

    TF_RUNTIME_ERROR("Invalid USDAttribute: %s", usdAttr.GetPath().GetAsString().c_str());
    return Ufe::Attribute::kInvalid;
}

SdfValueTypeName ufeTypeToUsd(const Ufe::Attribute::Type ufeType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<Ufe::Attribute::Type, SdfValueTypeName> sUfeTypeToUsd {
        { Ufe::Attribute::kBool, SdfValueTypeNames->Bool },
        { Ufe::Attribute::kInt, SdfValueTypeNames->Int },
        { Ufe::Attribute::kFloat, SdfValueTypeNames->Float },
        { Ufe::Attribute::kDouble, SdfValueTypeNames->Double },
        { Ufe::Attribute::kString, SdfValueTypeNames->String },
        // Not enough info at this point to differentiate between TfToken and std:string.
        { Ufe::Attribute::kEnumString, SdfValueTypeNames->Token },
        { Ufe::Attribute::kInt3, SdfValueTypeNames->Int3 },
        { Ufe::Attribute::kFloat3, SdfValueTypeNames->Float3 },
        { Ufe::Attribute::kDouble3, SdfValueTypeNames->Double3 },
        { Ufe::Attribute::kColorFloat3, SdfValueTypeNames->Color3f },
        { Ufe::Attribute::kGeneric, SdfValueTypeNames->Token },
#ifdef UFE_V4_FEATURES_AVAILABLE
        { Ufe::Attribute::kFilename, SdfValueTypeNames->Asset },
        { Ufe::Attribute::kFloat2, SdfValueTypeNames->Float2 },
        { Ufe::Attribute::kFloat4, SdfValueTypeNames->Float4 },
        { Ufe::Attribute::kColorFloat4, SdfValueTypeNames->Color4f },
        { Ufe::Attribute::kMatrix3d, SdfValueTypeNames->Matrix3d },
        { Ufe::Attribute::kMatrix4d, SdfValueTypeNames->Matrix4d },
#endif
    };

    const auto iter = sUfeTypeToUsd.find(ufeType);
    if (iter != sUfeTypeToUsd.end()) {
        return iter->second;
    } else {
        return SdfValueTypeName();
    }
}

VtValue vtValueFromString(const SdfValueTypeName& typeName, const std::string& strValue)
{
    static const std::unordered_map<std::string, std::function<VtValue(const std::string&)>>
        sUsdConverterMap {
            // Using the CPPTypeName prevents having to repeat converters for types that share the
            // same VtValue representation like Float3, Color3f, Normal3f, Point3f, allowing support
            // for more Sdf types without having to list them all.
            { SdfValueTypeNames->Bool.GetCPPTypeName(),
              [](const std::string& s) { return VtValue("true" == s ? true : false); } },
            { SdfValueTypeNames->Int.GetCPPTypeName(),
              [](const std::string& s) {
                  return s.empty() ? VtValue() : VtValue(std::stoi(s.c_str()));
              } },
            { SdfValueTypeNames->Float.GetCPPTypeName(),
              [](const std::string& s) {
                  return s.empty() ? VtValue() : VtValue(std::stof(s.c_str()));
              } },
            { SdfValueTypeNames->Double.GetCPPTypeName(),
              [](const std::string& s) {
                  return s.empty() ? VtValue() : VtValue(std::stod(s.c_str()));
              } },
            { SdfValueTypeNames->String.GetCPPTypeName(),
              [](const std::string& s) { return VtValue(s); } },
            { SdfValueTypeNames->Token.GetCPPTypeName(),
              [](const std::string& s) { return VtValue(TfToken(s)); } },
            { SdfValueTypeNames->Asset.GetCPPTypeName(),
              [](const std::string& s) { return VtValue(SdfAssetPath(s)); } },
            { SdfValueTypeNames->Int3.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 3) {
                      return VtValue(GfVec3i(
                          std::stoi(tokens[0].c_str()),
                          std::stoi(tokens[1].c_str()),
                          std::stoi(tokens[2].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Float2.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 2) {
                      return VtValue(
                          GfVec2f(std::stof(tokens[0].c_str()), std::stof(tokens[1].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Float3.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 3) {
                      return VtValue(GfVec3f(
                          std::stof(tokens[0].c_str()),
                          std::stof(tokens[1].c_str()),
                          std::stof(tokens[2].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Float4.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 4) {
                      return VtValue(GfVec4f(
                          std::stof(tokens[0].c_str()),
                          std::stof(tokens[1].c_str()),
                          std::stof(tokens[2].c_str()),
                          std::stof(tokens[3].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Double3.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 3) {
                      return VtValue(GfVec3d(
                          std::stod(tokens[0].c_str()),
                          std::stod(tokens[1].c_str()),
                          std::stod(tokens[2].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Double4.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 4) {
                      return VtValue(GfVec4d(
                          std::stod(tokens[0].c_str()),
                          std::stod(tokens[1].c_str()),
                          std::stod(tokens[2].c_str()),
                          std::stod(tokens[3].c_str())));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Matrix3d.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 9) {
                      double m[3][3];
                      for (int i = 0, k = 0; i < 3; ++i) {
                          for (int j = 0; j < 3; ++j, ++k) {
                              m[i][j] = std::stod(tokens[k].c_str());
                          }
                      }
                      return VtValue(GfMatrix3d(m));
                  }
                  return VtValue();
              } },
            { SdfValueTypeNames->Matrix4d.GetCPPTypeName(),
              [](const std::string& s) {
                  std::vector<std::string>tokens = splitString(s, "()[], ");
                  if (tokens.size() == 16) {
                      double m[4][4];
                      for (int i = 0, k = 0; i < 4; ++i) {
                          for (int j = 0; j < 4; ++j, ++k) {
                              m[i][j] = std::stod(tokens[k].c_str());
                          }
                      }
                      return VtValue(GfMatrix4d(m));
                  }
                  return VtValue();
              } },
        };
    const auto iter = sUsdConverterMap.find(typeName.GetCPPTypeName());
    if (iter != sUsdConverterMap.end()) {
        return iter->second(strValue);
    }
    return {};
}

std::vector<std::string> splitString(const std::string& str, const std::string& separators)
{
    std::vector<std::string> split;

    std::string::size_type lastPos = str.find_first_not_of(separators, 0);
    std::string::size_type pos = str.find_first_of(separators, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos) {
        split.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(separators, pos);
        pos = str.find_first_of(separators, lastPos);
    }

    return split;
}

#ifdef MAYA_HAS_DISPLAY_LAYER_API
template <typename PathType>
void handleDisplayLayer(
    const PathType&                                    displayLayerPath,
    const std::function<void(const MFnDisplayLayer&)>& handler)
{
    MFnDisplayLayerManager displayLayerManager(
        MFnDisplayLayerManager::currentDisplayLayerManager());

    MObject displayLayerObj = displayLayerManager.getLayer(displayLayerPath);
    if (displayLayerObj.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(displayLayerObj);
        // UFE display layers coming from referenced files are not yet supported in Maya
        // and their usage leads to a crash, so skip those for the time being
        if (!displayLayer.isFromReferencedFile()) {
            handler(displayLayer);
        }
    }
}
#endif

void ReplicateExtrasFromUSD::initRecursive(Ufe::SceneItem::Ptr ufeItem) const
{
    auto hier = Ufe::Hierarchy::hierarchy(ufeItem);
    if (hier) {
        // Go through the entire hierarchy
        for (auto child : hier->children()) {
            initRecursive(child);
        }
    }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    MString displayLayerPath(Ufe::PathString::string(ufeItem->path()).c_str());
    handleDisplayLayer(displayLayerPath, [this, &ufeItem](const MFnDisplayLayer& displayLayer) {
        if (displayLayer.name() != "defaultLayer") {
            _displayLayerMap[ufeItem->path()] = displayLayer.object();
        }
    });
#endif
}

void ReplicateExtrasFromUSD::processItem(const Ufe::Path& path, const MObject& mayaObject) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    auto it = _displayLayerMap.find(path);
    if (it != _displayLayerMap.end() && it->second.hasFn(MFn::kDisplayLayer)) {
        MDagPath dagPath;
        if (MDagPath::getAPathTo(mayaObject, dagPath) == MStatus::kSuccess) {
            MFnDisplayLayer displayLayer(it->second);
            displayLayer.add(dagPath);

            // In case display layer membership was removed from the USD prim that we are
            // replicating, we want to restore it here to make sure that the prim will stay in its
            // display layer on DiscardEdits
            displayLayer.add(Ufe::PathString::string(path).c_str());
        }
    }
#endif
}

void ReplicateExtrasToUSD::processItem(const MDagPath& dagPath, const SdfPath& usdPath) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Populate display layer membership map

    // Since multiple dag paths may lead to a single USD path (like transform and node),
    // we have to make sure we don't overwrite a non-default layer with a default one
    bool displayLayerAssigned = false;
    auto entry = _primToLayerMap.find(usdPath);
    if (entry != _primToLayerMap.end() && entry->second.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(entry->second);
        displayLayerAssigned = (displayLayer.name() != "defaultLayer");
    }

    if (!displayLayerAssigned) {
        handleDisplayLayer(dagPath, [this, &usdPath](const MFnDisplayLayer& displayLayer) {
            _primToLayerMap[usdPath] = displayLayer.object();
        });
    }
#endif
}

void ReplicateExtrasToUSD::initRecursive(const Ufe::SceneItem::Ptr& item) const
{
    auto hier = Ufe::Hierarchy::hierarchy(item);
    if (hier) {
        // Go through the entire hierarchy.
        for (auto child : hier->children()) {
            initRecursive(child);
        }
    }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    MString displayLayerPath(Ufe::PathString::string(item->path()).c_str());
    handleDisplayLayer(displayLayerPath, [this, &item](const MFnDisplayLayer& displayLayer) {
        if (displayLayer.name() != "defaultLayer") {
            auto usdItem = downcast(item);
            if (usdItem) {
                auto prim = usdItem->prim();
                _primToLayerMap[prim.GetPath()] = displayLayer.object();
            }
        }
    });
#endif
}

void ReplicateExtrasToUSD::finalize(const Ufe::Path& stagePath, const RenamedPaths& renamed) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    for (const auto& entry : _primToLayerMap) {
        if (entry.second.hasFn(MFn::kDisplayLayer)) {
            auto usdPrimPath = entry.first;
            for (const auto& oldAndNew : renamed) {
                const PXR_NS::SdfPath& oldPrefix = oldAndNew.first;
                if (!usdPrimPath.HasPrefix(oldPrefix))
                    continue;

                const PXR_NS::SdfPath& newPrefix = oldAndNew.second;
                usdPrimPath = usdPrimPath.ReplacePrefix(oldPrefix, newPrefix);
            }

            auto                primPath = UsdUfe::usdPathToUfePathSegment(usdPrimPath);
            Ufe::Path::Segments segments { stagePath.getSegments()[0], primPath };
            Ufe::Path           ufePath(std::move(segments));

            MFnDisplayLayer displayLayer(entry.second);
            displayLayer.add(Ufe::PathString::string(ufePath).c_str());
        }
    }
#endif
}

#ifdef HAS_ORPHANED_NODES_MANAGER

static Ufe::BBox3d transformBBox(const PXR_NS::GfMatrix4d& matrix, const Ufe::BBox3d& bbox)
{
    Ufe::BBox3d transformed(bbox);

    transformed.min = toUfe(matrix.Transform(toUsd(bbox.min)));
    transformed.max = toUfe(matrix.Transform(toUsd(bbox.max)));

    return transformed;
}

static Ufe::BBox3d transformBBox(const Ufe::Matrix4d& matrix, const Ufe::BBox3d& bbox)
{
    return transformBBox(toUsd(matrix), bbox);
}

static Ufe::BBox3d transformBBox(Ufe::SceneItem::Ptr& item, const Ufe::BBox3d& bbox)
{
    Ufe::Transform3d::Ptr t3d = Ufe::Transform3d::transform3d(item);
    if (!t3d)
        return bbox;

    return transformBBox(t3d->matrix(), bbox);
}

static Ufe::BBox3d getTransformedBBox(const MDagPath& path)
{
    MFnDagNode node(path);

    MBoundingBox mayaBBox = node.boundingBox();
    return Ufe::BBox3d(
        Ufe::Vector3d(mayaBBox.min().x, mayaBBox.min().y, mayaBBox.min().z),
        Ufe::Vector3d(mayaBBox.max().x, mayaBBox.max().y, mayaBBox.max().z));
}

#endif /* HAS_ORPHANED_NODES_MANAGER */

Ufe::BBox3d getPulledPrimsBoundingBox(const Ufe::Path& path)
{
    Ufe::BBox3d ufeBBox;

#ifdef HAS_ORPHANED_NODES_MANAGER
    const auto& updaterMgr = PXR_NS::PrimUpdaterManager::getInstance();
    PXR_NS::PrimUpdaterManager::PulledPrimPaths pulledPaths = updaterMgr.getPulledPrimPaths();
    for (const auto& paths : pulledPaths) {
        const Ufe::Path& pulledPath = paths.first;

        if (pulledPath == path)
            continue;

        if (!pulledPath.startsWith(path))
            continue;

        // Note: Maya implementation of the Object3d UFE interface does not
        //       implement the boundingBox() function. So we ask the DAG instead.
        const MDagPath& mayaPath = paths.second;
        Ufe::BBox3d     pulledBBox = getTransformedBBox(mayaPath);

        for (auto parentPath = pulledPath.pop(); parentPath != path;
             parentPath = parentPath.pop()) {
            Ufe::SceneItem::Ptr parentItem = Ufe::Hierarchy::createItem(parentPath);
            if (!parentItem)
                continue;
            pulledBBox = transformBBox(parentItem, pulledBBox);
        }

        ufeBBox = UsdUfe::combineUfeBBox(ufeBBox, pulledBBox);
    }
#endif /* HAS_ORPHANED_NODES_MANAGER */

    return ufeBBox;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
