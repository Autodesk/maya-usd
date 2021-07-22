//
// Copyright 2019 Autodesk
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
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/site.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
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

PXR_NAMESPACE_USING_DIRECTIVE

#ifndef MAYA_MSTRINGARRAY_ITERATOR_CATEGORY
// MStringArray::Iterator is not standard-compliant in Maya 2019, needs the
// following workaround.  Fixed in Maya 2020.  PPT, 20-Jun-2019.
namespace std {
template <> struct iterator_traits<MStringArray::Iterator>
{
    typedef std::bidirectional_iterator_tag iterator_category;
};
} // namespace std
#endif

namespace {

constexpr auto kIllegalUSDPath = "Illegal USD run-time path %s.";

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
uint32_t findLayerIndex(const UsdPrim& prim, const PXR_NS::SdfLayerHandle& layer)
{
    uint32_t position { 0 };

    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

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

extern UsdStageMap  g_StageMap;
extern Ufe::Rtid    g_MayaRtid;
constexpr auto      USD_UFE_RUNTIME_NAME = "USD";
static Ufe::Rtid usdUfeRtid = 0;

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
    const Ufe::Path ufePrimPath = stripInstanceIndexFromUfePath(path);

    // Assume that there are only two segments in the path, the first a Maya
    // Dag path segment to the proxy shape, which identifies the stage, and
    // the second the USD segment.
    // When called we do not make any assumption on whether or not the
    // input path is valid.
    const Ufe::Path::Segments& segments = ufePrimPath.getSegments();
    if (!TF_VERIFY(segments.size() == 2u, kIllegalUSDPath, path.string().c_str())) {
        return UsdPrim();
    }

    UsdPrim prim;
    if (auto stage = getStage(Ufe::Path(segments[0]))) {
        const SdfPath usdPath = SdfPath(segments[1].string());
        prim = stage->GetPrimAtPath(usdPath.GetPrimPath());
    }
    return prim;
}

int ufePathToInstanceIndex(const Ufe::Path& path, PXR_NS::UsdPrim* prim)
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
        TF_RUNTIME_ERROR(kIllegalUSDPath, path.string().c_str());
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
        auto    iter2 = std::find(inherited.begin(), inherited.end(), gatewayNodeType);
        isInherited = (iter2 != inherited.end());
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
    std::string fullPathName = dagPath.fullPathName().asChar();
    return Ufe::PathSegment("world" + fullPathName, g_MayaRtid, '|');
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
                std::string err = TfStringPrintf(
                    "Cannot edit [%s] attribute because there is a stronger opinion in [%s].",
                    attr.GetBaseName().GetText(),
                    strongestLayer->GetDisplayName().c_str());

                *errMsg = err;
            }

            return false;
        }
    }

    return true;
}

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;

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

bool isEditTargetLayerModifiable(const PXR_NS::UsdStageWeakPtr stage, std::string* errMsg)
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

Ufe::Rtid getUsdUfeRuntimeId()
{
    if (usdUfeRtid == 0) {
        try {
            usdUfeRtid = UFE_NS::RunTimeMgr::instance().getId(USD_UFE_RUNTIME_NAME);
        }
        // This shoudl catch ufe's InvalidRunTimeName exception, but they don't
        // expose that!
        catch (...) {
            TF_WARN("USD UFE Runtime plugin not loaded!\n");
        }
    }

    return usdUfeRtid;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
