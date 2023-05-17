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
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/site.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/resolver.h>
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

constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor> TokenToSdfTypeMap;

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

    const PcpPrimIndex& primIndex = prim.ComputeExpandedPrimIndex();

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

bool allowedInStrongerLayer(
    const UsdPrim&                          prim,
    const SdfPrimSpecHandleVector&          primStack,
    const std::set<PXR_NS::SdfLayerRefPtr>& sessionLayers,
    bool                                    allowStronger)
{
    // If the flag to allow edits in a stronger layer if off, then it is not allowed.
    if (!allowStronger)
        return false;

    // If allowed, verify if the target layer is stronger than any existing layer with an opinion.
    auto stage = prim.GetStage();
    auto targetLayer = stage->GetEditTarget().GetLayer();
    auto topLayer = primStack.front()->GetLayer();

    const SdfLayerHandle searchRoot = isSessionLayer(targetLayer, sessionLayers)
        ? stage->GetSessionLayer()
        : stage->GetRootLayer();

    return getStrongerLayer(searchRoot, targetLayer, topLayer) == targetLayer;
}

} // namespace

void applyCommandRestriction(
    const UsdPrim&     prim,
    const std::string& commandName,
    bool               allowStronger)
{
    // return early if prim is the pseudo-root.
    // this is a special case and could happen when one tries to drag a prim under the
    // proxy shape in outliner. Also note if prim is the pseudo-root, no def primSpec will be found.
    if (prim.IsPseudoRoot()) {
        return;
    }

    const auto stage = prim.GetStage();
    auto       targetLayer = stage->GetEditTarget().GetLayer();

    const bool includeTopLayer = true;
    const auto sessionLayers = getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);
    const bool isTargetingSession = isSessionLayer(targetLayer, sessionLayers);

    auto        primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    auto        primStack = prim.GetPrimStack();
    std::string layerDisplayName;

    // When the command is forbidden even for the strongest layer, that means
    // that the operation is a multi-layers operation and there is no target
    // layer that would allow it to proceed. In that case, do not suggest changing
    // the target.
    std::string message = allowStronger ? "It is defined on another layer. " : "";
    std::string instructions = allowStronger ? "Please set %s as the target layer to proceed."
                                             : "It would orphan opinions on the layer %s.";

    // iterate over the prim stack, starting at the highest-priority layer.
    for (const auto& spec : primStack) {
        // Only take session layers opinions into consideration when the target itself
        // is a session layer (top a sub-layer of session).
        //
        // We isolate session / non-session this way because these opinions
        // are owned by the application and we don't want to block the user
        // commands and user data due to them.
        const auto layer = spec->GetLayer();
        if (isSessionLayer(layer, sessionLayers) != isTargetingSession)
            continue;

        const auto& layerName = layer->GetDisplayName();

        // skip if there is no primSpec for the selected prim in the current stage's local layer.
        if (!primSpec) {
            // add "," separator for multiple layers
            if (!layerDisplayName.empty()) {
                layerDisplayName.append(",");
            }
            layerDisplayName.append("[" + layerName + "]");
            continue;
        }

        // one reason for skipping the reference is to not clash
        // with the over that may be created in the stage's sessionLayer.
        // another reason is that one should be able to edit a referenced prim that
        // either as over/def as long as it has a primSpec in the selected edit target layer.
        if (spec->HasReferences()) {
            break;
        }

        // if exists a def/over specs
        if (spec->GetSpecifier() == SdfSpecifierDef || spec->GetSpecifier() == SdfSpecifierOver) {
            // if spec exists in another layer ( e.g sessionLayer or layer other than stage's local
            // layers ).
            if (primSpec->GetLayer() != spec->GetLayer()) {
                layerDisplayName.append("[" + layerName + "]");
                if (allowStronger) {
                    message = "It has a stronger opinion on another layer. ";
                }
                break;
            }
            continue;
        }
    }

    // Per design request, we need a more clear message to indicate that renaming a prim inside a
    // variantset is not allowed. This restriction was already caught in the above loop but the
    // message was a bit generic.
    UsdPrimCompositionQuery query(prim);
    for (const auto& compQueryArc : query.GetCompositionArcs()) {
        if (!primSpec && PcpArcTypeVariant == compQueryArc.GetArcType()) {
            if (allowedInStrongerLayer(prim, primStack, sessionLayers, allowStronger))
                return;
            std::string err = TfStringPrintf(
                "Cannot %s [%s] because it is defined inside the variant composition arc %s.",
                commandName.c_str(),
                prim.GetName().GetString().c_str(),
                layerDisplayName.c_str());
            throw std::runtime_error(err.c_str());
        }
    }

    if (!layerDisplayName.empty()) {
        if (allowedInStrongerLayer(prim, primStack, sessionLayers, allowStronger))
            return;
        std::string formattedInstructions
            = TfStringPrintf(instructions.c_str(), layerDisplayName.c_str());
        std::string err = TfStringPrintf(
            "Cannot %s [%s]. %s%s",
            commandName.c_str(),
            prim.GetName().GetString().c_str(),
            message.c_str(),
            formattedInstructions.c_str());
        throw std::runtime_error(err.c_str());
    }
}

bool applyCommandRestrictionNoThrow(
    const UsdPrim&     prim,
    const std::string& commandName,
    bool               allowStronger)
{
    try {
        applyCommandRestriction(prim, commandName, allowStronger);
    } catch (const std::exception& e) {
        std::string errMsg(e.what());
        TF_WARN(errMsg);
        return false;
    }
    return true;
}

bool isPrimMetadataEditAllowed(
    const UsdPrim&         prim,
    const TfToken&         metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg)
{
    return isPropertyMetadataEditAllowed(prim, TfToken(), metadataName, keyPath, errMsg);
}

bool isPropertyMetadataEditAllowed(
    const UsdPrim&         prim,
    const PXR_NS::TfToken& propName,
    const TfToken&         metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg)
{
    // If the intended target layer is not modifiable as a whole,
    // then no metadata edits are allowed at all.
    const UsdStagePtr& stage = prim.GetStage();
    if (!isEditTargetLayerModifiable(stage, errMsg))
        return false;

    // Find the highest layer that has the metadata authored. The prim
    // expanded PCP index, which contains all locations that contribute
    // to the prim, is scanned for the first metadata authoring.
    //
    // Note: as far as we know, there are no USD API to retrieve the list
    //       of authored locations for a metadata, unlike properties.
    //
    //       The code here is inspired by code from USD, according to
    //       the following call sequence:
    //          - UsdObject::GetAllAuthoredMetadata()
    //          - UsdStage::_GetAllMetadata()
    //          - UsdStage::_GetMetadataImpl()
    //          - UsdStage::_GetGeneralMetadataImpl()
    //          - Usd_Resolver class
    //          - _ComposeGeneralMetadataImpl()
    //          - ExistenceComposer::ConsumeAuthored()
    //          - SdfLayer::HasFieldDictKey()
    //
    //          - UsdPrim::GetVariantSets
    //          - UsdVariantSet::GetVariantSelection()
    SdfLayerHandle topAuthoredLayer;
    {
        const SdfPath       primPath = prim.GetPath();
        const PcpPrimIndex& primIndex = prim.ComputeExpandedPrimIndex();

        // We need special processing for variant selection.
        //
        // Note: we would also need spacial processing for reference and payload,
        //       but let's postpone them until we actually need it since it would
        //       add yet more complexities.
        const bool isVariantSelection = (metadataName == SdfFieldKeys->VariantSelection);

        // Note: specPath is important even if prop name is empty, it then means
        //       a metadata on the prim itself.
        Usd_Resolver resolver(&primIndex);
        SdfPath      specPath = resolver.GetLocalPath(propName);

        for (bool isNewNode = false; resolver.IsValid(); isNewNode = resolver.NextLayer()) {
            if (isNewNode)
                specPath = resolver.GetLocalPath(propName);

            // Consume an authored opinion here, if one exists.
            SdfLayerRefPtr const& layer = resolver.GetLayer();
            const bool            gotOpinion = keyPath.IsEmpty() || isVariantSelection
                ? layer->HasField(specPath, metadataName)
                : layer->HasFieldDictKey(specPath, metadataName, keyPath);

            if (gotOpinion) {
                if (isVariantSelection) {
                    using SelMap = SdfVariantSelectionMap;
                    const SelMap variantSel = layer->GetFieldAs<SelMap>(specPath, metadataName);
                    if (variantSel.count(keyPath) == 0) {
                        continue;
                    }
                }
                topAuthoredLayer = layer;
                break;
            }
        }
    }

    // Get the layer where we intend to author a new opinion.
    const UsdEditTarget& editTarget = stage->GetEditTarget();
    const SdfLayerHandle targetLayer = editTarget.GetLayer();

    // Verify that the intended target layer is stronger than existing authored opinions.
    const auto strongestLayer = getStrongerLayer(stage, targetLayer, topAuthoredLayer, true);
    bool       allowed = (strongestLayer == targetLayer);
    if (!allowed && errMsg) {
        *errMsg = TfStringPrintf(
            "Cannot edit [%s] attribute because there is a stronger opinion in [%s].",
            metadataName.GetText(),
            strongestLayer ? strongestLayer->GetDisplayName().c_str() : "some layer");
    }
    return allowed;
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

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName, std::string* errMsg)
{
    TF_AXIOM(prim);
    TF_AXIOM(!attrName.IsEmpty());

    UsdGeomXformable xformable(prim);
    if (xformable) {
        if (UsdGeomXformOp::IsXformOp(attrName)) {
            // check for the attribute in XformOpOrderAttr first
            if (!isAttributeEditAllowed(xformable.GetXformOpOrderAttr(), errMsg)) {
                return false;
            }
        }
    }
    // check the attribute itself
    if (!isAttributeEditAllowed(prim.GetAttribute(attrName), errMsg)) {
        return false;
    }

    return true;
}

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(prim, attrName, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return false;
    }

    return true;
}

void enforceAttributeEditAllowed(const PXR_NS::UsdAttribute& attr)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(attr, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        throw std::runtime_error(errMsg);
    }
}

void enforceAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(prim, attrName, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        throw std::runtime_error(errMsg);
    }
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
            // TfToken is also used in UsdShade as a Generic placeholder for connecting struct I/O.
            UsdShadeNodeGraph asNodeGraph(usdAttr.GetPrim());
            if (asNodeGraph && usdAttr.GetTypeName() == SdfValueTypeNames->Token) {
                if (UsdShadeUtils::GetBaseNameAndType(usdAttr.GetName()).second
                    != UsdShadeAttributeType::Invalid) {
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

std::string pathSegmentSeparator()
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    return Ufe::PathString::pathSegmentSeparator();
#else
    return ",";
#endif
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

void ReplicateExtrasToUSD::finalize(
    const Ufe::Path&       stagePath,
    const PXR_NS::SdfPath* oldPrefix,
    const PXR_NS::SdfPath* newPrefix) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    for (const auto& entry : _primToLayerMap) {
        if (entry.second.hasFn(MFn::kDisplayLayer)) {
            auto usdPrimPath = entry.first;
            if (oldPrefix && newPrefix) {
                usdPrimPath = usdPrimPath.ReplacePrefix(*oldPrefix, *newPrefix);
            }

            auto                primPath = MayaUsd::ufe::usdPathToUfePathSegment(usdPrimPath);
            Ufe::Path::Segments segments { stagePath.getSegments()[0], primPath };
            Ufe::Path           ufePath(std::move(segments));

            MFnDisplayLayer displayLayer(entry.second);
            displayLayer.add(Ufe::PathString::string(ufePath).c_str());
        }
    }
#endif
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
