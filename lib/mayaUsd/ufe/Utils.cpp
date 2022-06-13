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

#include "private/Utils.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/utils/editability.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/site.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObjectHandle.h>
#include <ufe/hierarchy.h>
#include <ufe/pathSegment.h>
#include <ufe/rtid.h>
#include <ufe/selection.h>

#include <cassert>
#include <cctype>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

constexpr auto kIllegalUSDPath = "Illegal USD run-time path %s.";
constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";
constexpr auto kInvalidValue = "Invalid value '%s' for data type '%s'";

typedef std::unordered_map<PXR_NS::TfToken, PXR_NS::SdfValueTypeName, PXR_NS::TfToken::HashFunctor>
    TokenToSdfTypeMap;

bool stringBeginsWithDigit(const std::string& inputString)
{
    if (inputString.empty()) {
        return false;
    }

    const char& firstChar = inputString.front();
    if (std::isdigit(static_cast<unsigned char>(firstChar))) {
        return true;
    }

    return false;
}

// This function calculates the position index for a given layer across all
// the site's local LayerStacks
uint32_t findLayerIndex(const UsdPrim& prim, const SdfLayerHandle& layer)
{
    uint32_t position { 0 };

    const PcpPrimIndex& primIndex = prim.GetPrimIndex();

    // iterate through the expanded primIndex
    for (PcpNodeRef node : primIndex.GetNodeRange()) {

        TF_AXIOM(node);

        const PcpLayerStackSite&   site = node.GetSite();
        const PcpLayerStackRefPtr& layerStack = site.layerStack;

        // iterate through the "local" Layer stack for each site
        // to find the layer
        for (SdfLayerRefPtr const& l : layerStack->GetLayers()) {
            if (l == layer) {
                return position;
            }
            ++position;
        }
    }

    return position;
}

} // anonymous namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------

extern UsdStageMap g_StageMap;
extern Ufe::Rtid   g_MayaRtid;

// Cache of Maya node types we've queried before for inheritance from the
// gateway node type.
std::unordered_map<std::string, bool> g_GatewayType;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

UsdStageWeakPtr getStage(const Ufe::Path& path) { return g_StageMap.stage(path); }

Ufe::Path stagePath(UsdStageWeakPtr stage) { return g_StageMap.path(stage); }

TfHashSet<UsdStageWeakPtr, TfHash> getAllStages() { return g_StageMap.allStages(); }

Ufe::PathSegment usdPathToUfePathSegment(const SdfPath& usdPath, int instanceIndex)
{
    const Ufe::Rtid   usdRuntimeId = getUsdRunTimeId();
    static const char separator = SdfPathTokens->childDelimiter.GetText()[0u];

    if (usdPath.IsEmpty()) {
        // Return an empty segment.
        return Ufe::PathSegment(Ufe::PathSegment::Components(), usdRuntimeId, separator);
    }

    std::string pathString = usdPath.GetString();

    if (instanceIndex >= 0) {
        // Note here that we're taking advantage of the fact that identifiers
        // in SdfPaths must be C/Python identifiers; that is, they must *not*
        // begin with a digit. This means that when we see a path component at
        // the end of a USD path segment that does begin with a digit, we can
        // be sure that it represents an instance index and not a prim or other
        // USD entity.
        pathString += TfStringPrintf("%c%d", separator, instanceIndex);
    }

    return Ufe::PathSegment(pathString, usdRuntimeId, separator);
}

Ufe::Path stripInstanceIndexFromUfePath(const Ufe::Path& path)
{
    if (path.empty()) {
        return path;
    }

    // As with usdPathToUfePathSegment() above, we're taking advantage of the
    // fact that identifiers in SdfPaths must be C/Python identifiers; that is,
    // they must *not* begin with a digit. This means that when we see a path
    // component at the end of a USD path segment that does begin with a digit,
    // we can be sure that it represents an instance index and not a prim or
    // other USD entity.
    if (stringBeginsWithDigit(path.back().string())) {
        return path.pop();
    }

    return path;
}

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
    // When called we do not make any assumption on whether or not the
    // input path is valid.

    const Ufe::Path ufePrimPath = stripInstanceIndexFromUfePath(path);

    const Ufe::Path::Segments& segments = ufePrimPath.getSegments();
    if (!TF_VERIFY(!segments.empty(), kIllegalUFEPath, path.string().c_str())) {
        return UsdPrim();
    }
    auto stage = getStage(Ufe::Path(segments[0]));
    if (!TF_VERIFY(stage, kIllegalUFEPath, path.string().c_str())) {
        return UsdPrim();
    }

    // If there is only a single segment in the path, it must point to the
    // proxy shape, otherwise we would not have retrieved a valid stage.
    // The second path segment is the USD path.
    return (segments.size() == 1u)
        ? stage->GetPseudoRoot()
        : stage->GetPrimAtPath(SdfPath(segments[1].string()).GetPrimPath());
}

int ufePathToInstanceIndex(const Ufe::Path& path, UsdPrim* prim)
{
    int instanceIndex = UsdImagingDelegate::ALL_INSTANCES;

    const UsdPrim usdPrim = ufePathToPrim(path);
    if (prim) {
        *prim = usdPrim;
    }
    if (!usdPrim || !usdPrim.IsA<UsdGeomPointInstancer>()) {
        return instanceIndex;
    }

    // Once more as above in usdPathToUfePathSegment() and
    // stripInstanceIndexFromUfePath(), a path component at the tail of the
    // path that begins with a digit is assumed to represent an instance index.
    const std::string& tailComponentString = path.back().string();
    if (stringBeginsWithDigit(path.back().string())) {
        instanceIndex = std::stoi(tailComponentString);
    }

    return instanceIndex;
}

bool isRootChild(const Ufe::Path& path)
{
    // When called we make the assumption that we are given a valid
    // path and we are only testing whether or not we are a root child.
    auto segments = path.getSegments();
    if (segments.size() != 2) {
        TF_RUNTIME_ERROR(kIllegalUFEPath, path.string().c_str());
    }
    return (segments[1].size() == 1);
}

UsdSceneItem::Ptr
createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName)
{
    auto ufeSiblingPath = ufeSrcPath.sibling(Ufe::PathComponent(siblingName));
    return UsdSceneItem::create(ufeSiblingPath, ufePathToPrim(ufeSiblingPath));
}

std::string uniqueName(const TfToken::HashSet& existingNames, std::string srcName)
{
    // Compiled regular expression to find a numerical suffix to a path component.
    // It searches for any number of characters followed by a single non-numeric,
    // then one or more digits at end of string.
    std::regex  re("(.*)([^0-9])([0-9]+)$");
    std::string base { srcName };
    int         suffix { 1 };
    std::smatch match;
    if (std::regex_match(srcName, match, re)) {
        base = match[1].str() + match[2].str();
        suffix = std::stoi(match[3].str()) + 1;
    }
    std::string dstName = base + std::to_string(suffix);
    while (existingNames.count(TfToken(dstName)) > 0) {
        dstName = base + std::to_string(++suffix);
    }
    return dstName;
}

std::string uniqueChildName(const UsdPrim& usdParent, const std::string& name)
{
    if (!usdParent.IsValid())
        return std::string();

    TfToken::HashSet childrenNames;

    // The prim GetChildren method used the UsdPrimDefaultPredicate which includes
    // active prims. We also need the inactive ones.
    //
    // const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate =
    //			UsdPrimIsActive && UsdPrimIsDefined &&
    //			UsdPrimIsLoaded && !UsdPrimIsAbstract;
    // Note: removed 'UsdPrimIsLoaded' from the predicate. When it is present the
    //		 filter doesn't properly return the inactive prims. UsdView doesn't
    //		 use loaded either in _computeDisplayPredicate().
    //
    // Note: our UsdHierarchy uses instance proxies, so we also use them here.
    for (auto child : usdParent.GetFilteredChildren(
             UsdTraverseInstanceProxies(UsdPrimIsDefined && !UsdPrimIsAbstract))) {
        childrenNames.insert(child.GetName());
    }
    std::string childName { name };
    if (childrenNames.find(TfToken(childName)) != childrenNames.end()) {
        childName = uniqueName(childrenNames, childName);
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
    if (ufePath.runTimeId() != g_MayaRtid ||
#ifdef UFE_V2_FEATURES_AVAILABLE
        ufePath.nbSegments()
#else
        ufePath.getSegments().size()
#endif
            > 1) {
        return MDagPath();
    }
    return UsdMayaUtil::nameToDagPath(
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::PathString::string(ufePath)
#else
        // We have a single segment, so no path segment separator to consider.
        ufePath.popHead().string()
#endif
    );
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

    return g_StageMap.proxyShapeNode(path);
}

UsdTimeCode getTime(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return UsdTimeCode::Default();
    }

    // Proxy shape node should not be null.
    auto proxyShape = g_StageMap.proxyShapeNode(path);
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
    auto proxyShape = g_StageMap.proxyShapeNode(path);
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

bool isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr, std::string* errMsg)
{
    if (Editability::isLocked(attr)) {
        if (errMsg) {
            *errMsg = TfStringPrintf(
                "Cannot edit [%s] attribute because its lock metadata is [on].",
                attr.GetBaseName().GetText());
        }
        return false;
    }

    // get the property spec in the edit target's layer
    const auto& prim = attr.GetPrim();
    const auto& stage = prim.GetStage();
    const auto& editTarget = stage->GetEditTarget();

    if (!isEditTargetLayerModifiable(stage, errMsg)) {
        return false;
    }

    // get the index to edit target layer
    const auto targetLayerIndex = findLayerIndex(prim, editTarget.GetLayer());

    // HS March 22th,2021
    // TODO: "Value Clips" are UsdStage-level feature, unknown to Pcp.So if the attribute in
    // question is affected by Value Clips, we would will likely get the wrong answer. See Spiff
    // comment for more information :
    // https://groups.google.com/g/usd-interest/c/xTxFYQA_bRs/m/lX_WqNLoBAAJ

    // Read on Value Clips here:
    // https://graphics.pixar.com/usd/docs/api/_usd__page__value_clips.html

    // get the strength-ordered ( strong-to-weak order ) list of property specs that provide
    // opinions for this property.
    const auto& propertyStack = attr.GetPropertyStack();

    if (!propertyStack.empty()) {
        // get the strongest layer that has the attr.
        auto strongestLayer = attr.GetPropertyStack().front()->GetLayer();

        // compare the calculated index between the "attr" and "edit target" layers.
        if (findLayerIndex(prim, strongestLayer) < targetLayerIndex) {
            if (errMsg) {
                *errMsg = TfStringPrintf(
                    "Cannot edit [%s] attribute because there is a stronger opinion in [%s].",
                    attr.GetBaseName().GetText(),
                    strongestLayer->GetDisplayName().c_str());
            }

            return false;
        }
    }

    return true;
}

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;

    TF_AXIOM(prim);
    TF_AXIOM(!attrName.IsEmpty());

    UsdGeomXformable xformable(prim);
    if (xformable) {
        if (UsdGeomXformOp::IsXformOp(attrName)) {
            // check for the attribute in XformOpOrderAttr first
            if (!isAttributeEditAllowed(xformable.GetXformOpOrderAttr(), &errMsg)) {
                MGlobal::displayError(errMsg.c_str());
                return false;
            }
        }
    }
    // check the attribute itself
    if (!isAttributeEditAllowed(prim.GetAttribute(attrName), &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return false;
    }

    return true;
}

bool isEditTargetLayerModifiable(const UsdStageWeakPtr stage, std::string* errMsg)
{
    const auto editTarget = stage->GetEditTarget();
    const auto editLayer = editTarget.GetLayer();

    if (editLayer && !editLayer->PermissionToEdit()) {
        if (errMsg) {
            std::string err = TfStringPrintf(
                "Cannot edit [%s] because it is read-only. Set PermissionToEdit = true to proceed.",
                editLayer->GetDisplayName().c_str());

            *errMsg = err;
        }

        return false;
    }

    if (stage->IsLayerMuted(editLayer->GetIdentifier())) {
        if (errMsg) {
            std::string err = TfStringPrintf(
                "Cannot edit [%s] because it is muted. Unmute [%s] to proceed.",
                editLayer->GetDisplayName().c_str(),
                editLayer->GetDisplayName().c_str());
            *errMsg = err;
        }

        return false;
    }

    return true;
}

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::Attribute::Type usdTypeToUfe(const SdfValueTypeName& usdType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<size_t, Ufe::Attribute::Type> sUsdTypeToUfe
    {
        { SdfValueTypeNames->Bool.GetHash(), Ufe::Attribute::kBool },               // bool
            { SdfValueTypeNames->Int.GetHash(), Ufe::Attribute::kInt },             // int32_t
            { SdfValueTypeNames->Float.GetHash(), Ufe::Attribute::kFloat },         // float
            { SdfValueTypeNames->Double.GetHash(), Ufe::Attribute::kDouble },       // double
            { SdfValueTypeNames->String.GetHash(), Ufe::Attribute::kString },       // std::string
            { SdfValueTypeNames->Token.GetHash(), Ufe::Attribute::kEnumString },    // TfToken
            { SdfValueTypeNames->Int3.GetHash(), Ufe::Attribute::kInt3 },           // GfVec3i
            { SdfValueTypeNames->Float3.GetHash(), Ufe::Attribute::kFloat3 },       // GfVec3f
            { SdfValueTypeNames->Double3.GetHash(), Ufe::Attribute::kDouble3 },     // GfVec3d
            { SdfValueTypeNames->Color3f.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3f
            { SdfValueTypeNames->Color3d.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3d
#if (UFE_PREVIEW_VERSION_NUM >= 4015)
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
        return Ufe::Attribute::kGeneric;
    }
}

Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::SdrShaderPropertyConstPtr& shaderProperty)
{
    const PXR_NS::SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    if (typeName.GetHash() == PXR_NS::SdfValueTypeNames->Token.GetHash()) {
        static const TokenToSdfTypeMap tokenTypeToSdfType
            = { { PXR_NS::SdrPropertyTypes->Int, PXR_NS::SdfValueTypeNames->Int },
                { PXR_NS::SdrPropertyTypes->String, PXR_NS::SdfValueTypeNames->String },
                { PXR_NS::SdrPropertyTypes->Float, PXR_NS::SdfValueTypeNames->Float },
                { PXR_NS::SdrPropertyTypes->Color, PXR_NS::SdfValueTypeNames->Color3f },
                { PXR_NS::SdrPropertyTypes->Point, PXR_NS::SdfValueTypeNames->Point3f },
                { PXR_NS::SdrPropertyTypes->Normal, PXR_NS::SdfValueTypeNames->Normal3f },
                { PXR_NS::SdrPropertyTypes->Vector, PXR_NS::SdfValueTypeNames->Vector3f },
                { PXR_NS::SdrPropertyTypes->Matrix, PXR_NS::SdfValueTypeNames->Matrix4d } };
        TokenToSdfTypeMap::const_iterator it
            = tokenTypeToSdfType.find(shaderProperty->GetTypeAsSdfType().second);
        if (it != tokenTypeToSdfType.end()) {
            return usdTypeToUfe(it->second);
        } else {
#if PXR_VERSION < 2205
            // Pre-22.05 boolean inputs are special:
            if (shaderProperty->GetType() == SdfValueTypeNames->Bool.GetAsToken()) {
                return usdTypeToUfe(PXR_NS::SdfValueTypeNames->Bool);
            }
#endif
            return usdTypeToUfe(PXR_NS::SdfValueTypeNames->Token);
        }
    } else {
        return usdTypeToUfe(typeName);
    }
}

PXR_NS::SdfValueTypeName ufeTypeToUsd(const std::string& ufeType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<Ufe::Attribute::Type, PXR_NS::SdfValueTypeName> sUfeTypeToUsd {
        { Ufe::Attribute::kBool, PXR_NS::SdfValueTypeNames->Bool },           // bool
        { Ufe::Attribute::kInt, PXR_NS::SdfValueTypeNames->Int },             // int32_t
        { Ufe::Attribute::kFloat, PXR_NS::SdfValueTypeNames->Float },         // float
        { Ufe::Attribute::kDouble, PXR_NS::SdfValueTypeNames->Double },       // double
        { Ufe::Attribute::kString, PXR_NS::SdfValueTypeNames->String },       // std::string
        { Ufe::Attribute::kEnumString, PXR_NS::SdfValueTypeNames->Token },    // TfToken
        { Ufe::Attribute::kInt3, PXR_NS::SdfValueTypeNames->Int3 },           // GfVec3i
        { Ufe::Attribute::kFloat3, PXR_NS::SdfValueTypeNames->Float3 },       // GfVec3f
        { Ufe::Attribute::kDouble3, PXR_NS::SdfValueTypeNames->Double3 },     // GfVec3d
        { Ufe::Attribute::kColorFloat3, PXR_NS::SdfValueTypeNames->Color3f }, // GfVec3f
    };

    const auto iter = sUfeTypeToUsd.find(ufeType);
    if (iter != sUfeTypeToUsd.end()) {
        return iter->second;
    } else {
        return SdfValueTypeName();
    }
}

PXR_NS::VtValue vtValueFromString(const std::string& typeName, const std::string& strValue)
{
    PXR_NS::VtValue result;
    if (typeName == Ufe::Attribute::kBool) {
        result = "true" == strValue ? true : false;
    } else if (typeName == Ufe::Attribute::kInt) {
        result = std::stoi(strValue.c_str());
    } else if (typeName == Ufe::Attribute::kFloat) {
        result = std::stof(strValue.c_str());
    } else if (typeName == Ufe::Attribute::kDouble) {
        result = std::stod(strValue.c_str());
    } else if (typeName == Ufe::Attribute::kString) {
        result = strValue;
    } else if (typeName == Ufe::Attribute::kEnumString) {
        result = PXR_NS::TfToken(strValue.c_str());
    } else if (typeName == Ufe::Attribute::kInt3) {
        std::vector<std::string> tokens = splitString(strValue, "()[], ");
        if (TF_VERIFY(tokens.size() == 3, kInvalidValue, strValue, typeName)) {
            result = GfVec3i(
                std::stoi(tokens[0].c_str()),
                std::stoi(tokens[1].c_str()),
                std::stoi(tokens[2].c_str()));
        }
    } else if (typeName == Ufe::Attribute::kFloat3 || typeName == Ufe::Attribute::kColorFloat3) {
        std::vector<std::string> tokens = splitString(strValue, "()[], ");
        if (TF_VERIFY(tokens.size() == 3, kInvalidValue, strValue, typeName)) {
            result = GfVec3f(
                std::stof(tokens[0].c_str()),
                std::stof(tokens[1].c_str()),
                std::stof(tokens[2].c_str()));
        }
    } else if (typeName == Ufe::Attribute::kDouble3) {
        std::vector<std::string> tokens = splitString(strValue, "()[], ");
        if (TF_VERIFY(tokens.size() == 3, kInvalidValue, strValue, typeName)) {
            result = GfVec3d(
                std::stod(tokens[0].c_str()),
                std::stod(tokens[1].c_str()),
                std::stod(tokens[2].c_str()));
        }
    } else if (typeName == Ufe::Attribute::kMatrix3d) {
        std::vector<std::string> tokens = splitString(strValue, "()[], ");
        if (TF_VERIFY(tokens.size() == 9, kInvalidValue, strValue, typeName)) {
            double m[3][3];
            for (int i = 0, k = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j, ++k) {
                    m[i][j] = std::stod(tokens[k].c_str());
                }
            }
            result = GfMatrix3d(m);
        }
    } else if (typeName == Ufe::Attribute::kMatrix4d) {
        std::vector<std::string> tokens = splitString(strValue, "()[], ");
        if (TF_VERIFY(tokens.size() == 16, kInvalidValue, strValue, typeName)) {
            double m[4][4];
            for (int i = 0, k = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j, ++k) {
                    m[i][j] = std::stod(tokens[k].c_str());
                }
            }
            result = GfMatrix4d(m);
        }
    }
    return result;
}

#endif

Ufe::Selection removeDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath)
{
    // Filter the src selection, removing items below the filterPath
    Ufe::Selection dst;
    for (const auto& item : src) {
        const auto& itemPath = item->path();
        // The filterPath itself is still valid.
        if (!itemPath.startsWith(filterPath) || itemPath == filterPath) {
            dst.append(item);
        }
    }
    return dst;
}

Ufe::Selection recreateDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath)
{
    // If a src selection item starts with the filterPath, re-create it.
    Ufe::Selection dst;
    for (const auto& item : src) {
        const auto& itemPath = item->path();
        // The filterPath itself is still valid.
        if (!itemPath.startsWith(filterPath) || itemPath == filterPath) {
            dst.append(item);
        } else {
            auto recreatedItem = Ufe::Hierarchy::createItem(item->path());
            TF_AXIOM(recreatedItem);
            dst.append(recreatedItem);
        }
    }
    return dst;
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

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
