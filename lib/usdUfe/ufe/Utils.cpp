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

#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/stage.h>

#include <cctype>
#include <regex>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";

// typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor> TokenToSdfTypeMap;

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

UsdUfe::UfePathToPrimFn gUfePathToPrimFn = nullptr;

} // anonymous namespace

namespace USDUFE_NS_DEF {

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

void setUfePathToPrimFn(UfePathToPrimFn fn)
{
    if (nullptr == fn) {
        throw std::invalid_argument("Path to prim function cannot be empty.");
    }
    gUfePathToPrimFn = fn;
}

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
    if (nullptr == gUfePathToPrimFn) {
        throw std::runtime_error("Path to prim function cannot be empty.");
    }

    return gUfePathToPrimFn(path);
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

    auto        primSpec = getPrimSpecAtEditTarget(prim);
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

} // namespace USDUFE_NS_DEF
