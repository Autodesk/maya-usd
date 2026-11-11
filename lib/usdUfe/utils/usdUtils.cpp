//
// Copyright 2020 Autodesk
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

#include "usdUtils.h"

#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/utils/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/mapFunction.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/relationshipSpec.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/specializes.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
std::map<std::string, std::string> getDict(const UsdPrimCompositionQueryArc& arc)
{
    std::string arcType;
    switch (arc.GetArcType()) {
    case PcpArcTypeRoot: arcType = "PcpArcTypeRoot"; break;
    case PcpArcTypeReference: arcType = "PcpArcTypeReference"; break;
    case PcpArcTypePayload: arcType = "PcpArcTypePayload"; break;
    case PcpArcTypeInherit: arcType = "PcpArcTypeInherit"; break;
    case PcpArcTypeSpecialize: arcType = "PcpArcTypeSpecialize"; break;
    case PcpArcTypeVariant: arcType = "PcpArcTypeVariant"; break;
    default: break;
    }

    auto introducingLayer = arc.GetIntroducingLayer();
    auto introducingNode = arc.GetIntroducingNode();

    return {
        { "arcType", arcType },
        { "hasSpecs", arc.HasSpecs() ? "True" : "False" },
        { "introLayer", introducingLayer ? introducingLayer->GetRealPath() : "" },
        { "introLayerStack",
          introducingNode
              ? introducingNode.GetLayerStack()->GetIdentifier().rootLayer->GetRealPath()
              : "" },
        { "introPath", arc.GetIntroducingPrimPath().GetString() },
        { "isAncestral", arc.IsAncestral() ? "True" : "False" },
        { "isImplicit", arc.IsImplicit() ? "True" : "False" },
        { "isIntroRootLayer", arc.IsIntroducedInRootLayerStack() ? "True" : "False" },
        { "isIntroRootLayerPrim", arc.IsIntroducedInRootLayerPrimSpec() ? "True" : "False" },
        { "nodeLayerStack",
          arc.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer->GetRealPath() },
        { "nodePath", arc.GetTargetNode().GetPath().GetString() },
    };
}

void replaceInternalReferencePath(
    const UsdPrim&            oldPrim,
    const SdfPath&            newPath,
    const SdfReferencesProxy& referencesList,
    SdfListOpType             op)
{
    // set the listProxy based on the SdfListOpType
    SdfReferencesProxy::ListProxy listProxy = referencesList.GetAppendedItems();
    if (op == SdfListOpTypePrepended) {
        listProxy = referencesList.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = referencesList.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = referencesList.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = referencesList.GetDeletedItems();
    }

    // fetching the existing SdfReference items and using
    // the Replace() method to replace them with updated SdfReference items.
    for (const SdfReference ref : listProxy) {
        if (UsdUfe::isInternalReference(ref)) {
            SdfPath finalPath;
            if (oldPrim.GetPath() == ref.GetPrimPath()) {
                finalPath = newPath;
            } else if (ref.GetPrimPath().HasPrefix(oldPrim.GetPath())) {
                finalPath = ref.GetPrimPath().ReplacePrefix(oldPrim.GetPath(), newPath);
            }

            if (finalPath.IsEmpty()) {
                continue;
            }

            // replace the old reference with new one
            SdfReference newRef;
            newRef.SetPrimPath(finalPath);
            listProxy.Replace(ref, newRef);
        }
    }
}

void removeInternalReferencePath(
    const UsdPrim&            deletedPrim,
    const SdfReferencesProxy& referencesList,
    SdfListOpType             op)
{
    // set the listProxy based on the SdfListOpType
    SdfReferencesProxy::ListProxy listProxy = referencesList.GetAppendedItems();
    if (op == SdfListOpTypePrepended) {
        listProxy = referencesList.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = referencesList.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = referencesList.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = referencesList.GetDeletedItems();
    }

    for (size_t idx = 0; idx < listProxy.size();) {
        const SdfReference ref = listProxy[idx];
        if (UsdUfe::isInternalReference(ref)) {
            if (deletedPrim.GetPath() == ref.GetPrimPath()
                || ref.GetPrimPath().HasPrefix(deletedPrim.GetPath())) {
                listProxy.Erase(idx);
                continue; // Not increasing idx
            }
        }
        ++idx;
    }
}

// This template method updates the SdfPath for inherited or specialized arcs
// when the path to the concrete prim they refer to has changed.
// HS January 13, 2021: Find a better generic way to consolidate this method with
// replaceReferenceItems
template <typename T>
void replacePath(const UsdPrim& oldPrim, const SdfPath& newPath, const T& proxy, SdfListOpType op)
{
    // set the listProxy based on the SdfListOpType
    typename T::ListProxy listProxy { proxy.GetAppendedItems() };
    if (op == SdfListOpTypePrepended) {
        listProxy = proxy.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = proxy.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = proxy.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = proxy.GetDeletedItems();
    }

    for (const SdfPath path : listProxy) {
        SdfPath finalPath;
        if (oldPrim.GetPath() == path.GetPrimPath()) {
            finalPath = newPath;
        } else if (path.GetPrimPath().HasPrefix(oldPrim.GetPath())) {
            finalPath = path.GetPrimPath().ReplacePrefix(oldPrim.GetPath(), newPath);
        }

        if (finalPath.IsEmpty()) {
            continue;
        }

        // replace the old SdfPath with new one
        listProxy.Replace(path, finalPath);
    }
}

// This template method cleans the SdfPath for inherited or specialized arcs
// when the path to the concrete prim they refer to has becomed invalid.
// HS January 13, 2021: Find a better generic way to consolidate this method with
// removeReferenceItems
template <typename T> void removePath(const UsdPrim& deletedPrim, const T& proxy, SdfListOpType op)
{
    // set the listProxy based on the SdfListOpType
    typename T::ListProxy listProxy { proxy.GetAppendedItems() };
    if (op == SdfListOpTypePrepended) {
        listProxy = proxy.GetPrependedItems();
    } else if (op == SdfListOpTypeOrdered) {
        listProxy = proxy.GetOrderedItems();
    } else if (op == SdfListOpTypeAdded) {
        listProxy = proxy.GetAddedItems();
    } else if (op == SdfListOpTypeDeleted) {
        listProxy = proxy.GetDeletedItems();
    }

    for (size_t idx = 0; idx < listProxy.size();) {
        const SdfPath path = listProxy[idx];
        if (deletedPrim.GetPath() == path.GetPrimPath() || path.HasPrefix(deletedPrim.GetPath())) {
            listProxy.Erase(idx);
            continue; // Not increasing idx
        }
        ++idx;
    }
}

// Return the connection or target list editor of the property spec.
SdfPathEditorProxy getPathEditor(const SdfPropertySpecHandle& spec)
{
    if (auto attrSpec = TfDynamic_cast<SdfAttributeSpecHandle>(spec)) {
        return attrSpec->GetConnectionPathList();
    }
    if (auto relSpec = TfDynamic_cast<SdfRelationshipSpecHandle>(spec)) {
        return relSpec->GetTargetPathList();
    }
    return {};
}

// Return the composed connection sources or relationship targets of the property.
SdfPathVector getComposedPaths(const UsdProperty& prop)
{
    SdfPathVector paths;
    if (prop.Is<UsdAttribute>()) {
        prop.As<UsdAttribute>().GetConnections(&paths);
    } else if (prop.Is<UsdRelationship>()) {
        prop.As<UsdRelationship>().GetTargets(&paths);
    }
    return paths;
}

bool contains(const SdfPathVector& paths, const SdfPath& path)
{
    return std::find(paths.begin(), paths.end(), path) != paths.end();
}

// Add the paths at or under the prefix to the result.
void addPathsWithPrefix(const SdfPathVector& paths, const SdfPath& prefix, SdfPathSet& result)
{
    for (const SdfPath& path : paths) {
        if (path.HasPrefix(prefix)) {
            result.insert(path);
        }
    }
}

// Return the paths at or under the prefix that the list editor adds: explicit, added,
// prepended or appended.
SdfPathSet getAddedPaths(const SdfPathEditorProxy& editor, const SdfPath& prefix)
{
    SdfPathSet result;
    for (const SdfPathVector& items : { SdfPathVector(editor.GetExplicitItems()),
                                        SdfPathVector(editor.GetAddedItems()),
                                        SdfPathVector(editor.GetPrependedItems()),
                                        SdfPathVector(editor.GetAppendedItems()) }) {
        addPathsWithPrefix(items, prefix, result);
    }
    return result;
}

// Return the paths at or under the prefix that the list editor authors in any list, the
// deleted one only if includeDeleted is true.
SdfPathSet
getAuthoredPaths(const SdfPathEditorProxy& editor, const SdfPath& prefix, bool includeDeleted)
{
    SdfPathSet result = getAddedPaths(editor, prefix);
    addPathsWithPrefix(SdfPathVector(editor.GetOrderedItems()), prefix, result);
    if (includeDeleted) {
        addPathsWithPrefix(SdfPathVector(editor.GetDeletedItems()), prefix, result);
    }
    return result;
}

// Replace the path in every list of the list editor. A list that already holds the new
// path cannot hold it twice, so the old path is removed from it instead.
void replaceItemEdits(
    const SdfPathEditorProxy& editor,
    const SdfPath&            oldPath,
    const SdfPath&            newPath)
{
    for (SdfPathEditorProxy::ListProxy items : { editor.GetExplicitItems(),
                                                 editor.GetAddedItems(),
                                                 editor.GetPrependedItems(),
                                                 editor.GetAppendedItems(),
                                                 editor.GetDeletedItems(),
                                                 editor.GetOrderedItems() }) {
        if (items.Find(oldPath) == size_t(-1)) {
            continue;
        }
        if (items.Find(newPath) == size_t(-1)) {
            items.Replace(oldPath, newPath);
        } else {
            items.Remove(oldPath);
        }
    }
}

// Return true if the node is in the stage's local layer stack and is reached from the root
// node through variant arcs only. Other nodes bring opinions through references, payloads,
// inherits or specializes.
bool isLocalNode(PcpNodeRef node)
{
    if (node.GetLayerStack() != node.GetRootNode().GetLayerStack()) {
        return false;
    }
    for (; !node.IsRootNode(); node = node.GetParentNode()) {
        if (node.GetArcType() != PcpArcTypeVariant) {
            return false;
        }
    }
    return true;
}

// A prim spec contributing to a prim, with the map from its namespace to the stage's.
struct PrimSpecSite
{
    SdfPrimSpecHandle primSpec;
    PcpMapFunction    mapToStage;
    // True if the spec is in the stage's local layer stack and reached through variant arcs
    // only, see isLocalNode().
    bool isLocal;
    // True if the spec is in the stage's local layer stack, whatever arcs reach it.
    bool isInLocalLayerStack;
};

// Return the prim specs contributing to the prim.
std::vector<PrimSpecSite> getPrimSpecSites(const UsdPrim& prim)
{
    std::vector<PrimSpecSite> sites;
    for (const PcpNodeRef node : prim.GetPrimIndex().GetNodeRange()) {
        if (node.IsInert() || !node.HasSpecs()) {
            continue;
        }
        const PcpMapFunction mapToStage = node.GetMapToRoot().Evaluate();
        const bool           isLocal = isLocalNode(node);
        const bool isInLocalLayerStack = node.GetLayerStack() == node.GetRootNode().GetLayerStack();
        for (const SdfLayerRefPtr& layer : node.GetLayerStack()->GetLayers()) {
            if (const SdfPrimSpecHandle primSpec = layer->GetPrimAtPath(node.GetPath())) {
                sites.push_back({ primSpec, mapToStage, isLocal, isInLocalLayerStack });
            }
        }
    }
    return sites;
}

// Return true if the site maps paths to a strict ancestor of the prefix. Composition does not
// remap these paths when the prefix is renamed or deleted, as that mapping does not move.
bool hasMappingAbovePrefix(const PrimSpecSite& site, const SdfPath& prefix)
{
    if (site.mapToStage.HasRootIdentity()) {
        return true;
    }
    for (const auto& entry : site.mapToStage.GetSourceToTargetMap()) {
        if (prefix.HasPrefix(entry.second) && entry.second != prefix) {
            return true;
        }
    }
    return false;
}

// Return where, in the stage, the arc that maps the path is introduced: the target of the
// map function entry that maps it. The result is empty if no entry maps the path.
SdfPath getArcRoot(const PcpMapFunction& mapToStage, const SdfPath& path)
{
    SdfPath source;
    SdfPath target;
    for (const auto& entry : mapToStage.GetSourceToTargetMap()) {
        if (path.HasPrefix(entry.first)
            && (source.IsEmpty()
                || entry.first.GetPathElementCount() > source.GetPathElementCount())) {
            source = entry.first;
            target = entry.second;
        }
    }
    return target;
}

// Return, in the stage namespace, the paths added by the list editor of a spec reached through
// a reference, payload, inherit or specialize arc that map at or under the prefix, but that
// composition does not remap when the prefix is renamed or deleted.
//
// Composition remaps a path when the arc mapping it is introduced at or under the prefix, as
// the arc moves with it. Otherwise, as for a path authored in an asset to a prim the stage
// defines under the referencing prim, the path keeps mapping to the old prim. Paths authored
// under the prefix in the stage's local layer stack, as in a class, are skipped: they are
// updated with the prim that owns them.
SdfPathSet getPathsNotRemapped(
    const PrimSpecSite&       site,
    const SdfPathEditorProxy& editor,
    const SdfPath&            prefix)
{
    SdfPathSet result;
    for (const SdfPath& path : getAddedPaths(editor, SdfPath::AbsoluteRootPath())) {
        if (site.isInLocalLayerStack && path.HasPrefix(prefix)) {
            continue;
        }
        const SdfPath stagePath = site.mapToStage.MapSourceToTarget(path);
        if (stagePath.IsEmpty() || !stagePath.HasPrefix(prefix)) {
            continue;
        }
        if (getArcRoot(site.mapToStage, path).HasPrefix(prefix)) {
            continue;
        }
        result.insert(stagePath);
    }
    return result;
}

// The opinions on one property that author paths at or under a prefix, grouped by what
// the edit target can do about them.
struct PropertyPathOpinions
{
    // The property specs in the edit target layer that author any of the paths, including
    // the ones inside variants.
    std::vector<SdfPropertySpecHandle> editTargetSpecs;
    // Paths authored in those specs.
    SdfPathSet editTargetPaths;
    // Paths added by the spec of the edit target at the property path, which is the one
    // that receives the overrides. A path added by another of its specs, as inside a variant,
    // can be hidden by a stronger opinion.
    SdfPathSet editTargetAddedPaths;
    // Paths deleted by those specs.
    SdfPathSet editTargetDeletedPaths;
    // Paths the edit target can override: the ones added by property specs of weaker layers
    // of the local layer stack, and the ones added through other arcs that composition does
    // not remap, see getPathsNotRemapped().
    SdfPathSet overridablePaths;
    // Paths deleted by property specs of weaker layers of the local layer stack.
    SdfPathSet overridableDeletedPaths;
    // Reorder lists of property specs of weaker layers that contain any of the paths.
    std::vector<SdfPathVector> overridableReorders;
    // Paths the edit target cannot reach, per layer: the ones added by property specs of
    // local layers stronger than the edit target. Other layers are not edited.
    std::map<SdfLayerHandle, SdfPathSet> unreachablePathsByLayer;
};

// Return true if the layer is stronger than the edit target layer in the stage's local layer
// stack, session layers included.
bool isStrongerThanEditTarget(const UsdStagePtr& stage, const SdfLayerHandle& layer)
{
    const SdfLayerHandle editLayer = stage->GetEditTarget().GetLayer();
    for (const SdfLayerHandle& stackLayer : stage->GetLayerStack()) {
        if (stackLayer == layer) {
            return true;
        }
        if (stackLayer == editLayer) {
            return false;
        }
    }
    return false;
}

// Find, per property of the prim, the connection and target paths at or under the prefix
// that are authored in the stage's local layer stack, or that composition does not remap.
// Deleted paths are only included if includeDeleted is true.
//
// Rule: remap opinions where they are authored, never composed results. Paths authored
// locally are absolute and must be updated. Paths reached through a reference, payload,
// inherit or specialize arc are mapped by composition and follow on their own, unless the
// arc does not move with the namespace edit: these are overridden like the paths of the
// other local layers. Paths authored in a local layer stronger than the edit target cannot
// be overridden, they are only reported.
std::map<TfToken, PropertyPathOpinions>
findPropertyPathOpinions(const UsdPrim& prim, const SdfPath& prefix, bool includeDeleted)
{
    std::map<TfToken, PropertyPathOpinions> result;

    const UsdStagePtr    stage = prim.GetStage();
    const UsdEditTarget& editTarget = stage->GetEditTarget();
    const SdfLayerHandle editLayer = editTarget.GetLayer();

    for (const PrimSpecSite& site : getPrimSpecSites(prim)) {
        if (!site.isLocal && !hasMappingAbovePrefix(site, prefix)) {
            continue;
        }
        for (const SdfPropertySpecHandle& propSpec : site.primSpec->GetProperties()) {
            const SdfPathEditorProxy editor = getPathEditor(propSpec);
            if (!editor) {
                continue;
            }

            if (!site.isLocal) {
                const SdfPathSet notRemapped = getPathsNotRemapped(site, editor, prefix);
                if (!notRemapped.empty()) {
                    result[propSpec->GetNameToken()].overridablePaths.insert(
                        notRemapped.begin(), notRemapped.end());
                }
                continue;
            }

            const SdfPathSet authored = getAuthoredPaths(editor, prefix, includeDeleted);
            if (authored.empty()) {
                continue;
            }

            PropertyPathOpinions& paths = result[propSpec->GetNameToken()];
            const SdfPathSet      added = getAddedPaths(editor, prefix);
            if (propSpec->GetLayer() == editLayer) {
                paths.editTargetSpecs.push_back(propSpec);
                paths.editTargetPaths.insert(authored.begin(), authored.end());
                if (propSpec
                    == editTarget.GetPropertySpecForScenePath(
                        prim.GetPath().AppendProperty(propSpec->GetNameToken()))) {
                    paths.editTargetAddedPaths.insert(added.begin(), added.end());
                }
                if (includeDeleted) {
                    addPathsWithPrefix(
                        SdfPathVector(editor.GetDeletedItems()),
                        prefix,
                        paths.editTargetDeletedPaths);
                }
                continue;
            }

            // Opinions inside a variant are weaker than the ones of the edit target outside
            // of variants, whatever their layer.
            if (!propSpec->GetPath().ContainsPrimVariantSelection()
                && isStrongerThanEditTarget(stage, propSpec->GetLayer())) {
                if (!added.empty()) {
                    paths.unreachablePathsByLayer[propSpec->GetLayer()].insert(
                        added.begin(), added.end());
                }
                continue;
            }

            paths.overridablePaths.insert(added.begin(), added.end());
            if (includeDeleted) {
                addPathsWithPrefix(
                    SdfPathVector(editor.GetDeletedItems()), prefix, paths.overridableDeletedPaths);
            }
            const SdfPathVector ordered(editor.GetOrderedItems());
            SdfPathSet          orderedWithPrefix;
            addPathsWithPrefix(ordered, prefix, orderedWithPrefix);
            if (!orderedWithPrefix.empty()) {
                paths.overridableReorders.push_back(ordered);
            }
        }
    }

    return result;
}

// Warn about the paths of the property authored in local layers stronger than the edit target,
// which are left pointing to the renamed or deleted prim.
void warnAboutStrongerLayers(const UsdProperty& prop, const PropertyPathOpinions& paths)
{
    for (const auto& entry : paths.unreachablePathsByLayer) {
        std::vector<std::string> pathStrings;
        for (const SdfPath& path : entry.second) {
            pathStrings.push_back(path.GetString());
        }
        TF_WARN(
            "Layer \"%s\" still points to the old path \"%s\" in \"%s\". It was not updated "
            "because it is stronger than the current edit target. Please update it in that "
            "layer.",
            entry.first->GetDisplayName().c_str(),
            TfStringJoin(pathStrings, "\", \"").c_str(),
            prop.GetPath().GetText());
    }
}

// Return the property spec of the stage's edit target for the property, creating it if needed.
SdfPropertySpecHandle getOrCreateEditTargetSpec(const UsdProperty& prop)
{
    const UsdEditTarget& editTarget = prop.GetStage()->GetEditTarget();
    if (const SdfPropertySpecHandle spec = editTarget.GetPropertySpecForScenePath(prop.GetPath())) {
        return spec;
    }

    const SdfPrimSpecHandle primSpec = SdfCreatePrimInLayer(
        editTarget.GetLayer(), editTarget.MapToSpecPath(prop.GetPrim().GetPath()));
    if (!primSpec) {
        return {};
    }
    if (prop.Is<UsdAttribute>()) {
        const UsdAttribute attr = prop.As<UsdAttribute>();
        return SdfAttributeSpec::New(
            primSpec,
            attr.GetName().GetString(),
            attr.GetTypeName(),
            attr.GetVariability(),
            attr.IsCustom());
    }
    return SdfRelationshipSpec::New(primSpec, prop.GetName().GetString(), prop.IsCustom());
}

// Return the reorder list that puts paths, deleted and appended back under their new path,
// at the position they had in the composed list.
//
// Appended paths end up last. A reorder moves each listed path together with the unlisted
// paths that follow it. So each run of appended paths is listed followed by the path that
// came after it, and a run at the end of the list is listed last. When no path follows
// them, the appended paths are already in place.
SdfPathVector getReorderForAppendedPaths(
    const SdfPathVector& composedPaths,
    const SdfPathSet&    appendedPaths,
    const SdfPath&       oldPath,
    const SdfPath&       newPath)
{
    SdfPathVector ordered;
    bool          isFollowed = false;
    for (size_t i = 0; i < composedPaths.size(); ++i) {
        const bool isAppended = appendedPaths.count(composedPaths[i]) > 0;
        const bool followsAppended = i > 0 && appendedPaths.count(composedPaths[i - 1]) > 0;
        if (isAppended || followsAppended) {
            ordered.push_back(composedPaths[i].ReplacePrefix(oldPath, newPath));
            isFollowed = isFollowed || !isAppended;
        }
    }
    return isFollowed ? ordered : SdfPathVector();
}

// What the edit target authors to override the paths it cannot rename in place.
struct PathOverride
{
    SdfPathVector toDelete;
    SdfPathVector toAppend;
    SdfPathVector toOrder;

    bool isEmpty() const { return toDelete.empty() && toAppend.empty() && toOrder.empty(); }
};

// Return the override of the paths authored outside of the edit target that point to or under
// the renamed prim. composedPaths are the composed paths of the property before any edit.
PathOverride computePathOverride(
    const PropertyPathOpinions& paths,
    const SdfPathVector&        composedPaths,
    const SdfPath&              oldPath,
    const SdfPath&              newPath)
{
    PathOverride pathOverride;

    // Paths added by other layers that still compose are deleted, then appended back
    // under their new path, unless the edit target adds them itself.
    SdfPathSet appendedPaths;
    for (const SdfPath& path : composedPaths) {
        if (paths.overridablePaths.count(path) == 0) {
            continue;
        }
        pathOverride.toDelete.push_back(path);
        if (paths.editTargetAddedPaths.count(path) == 0) {
            pathOverride.toAppend.push_back(path.ReplacePrefix(oldPath, newPath));
            appendedPaths.insert(path);
        }
    }

    // Paths added by other layers and deleted by the edit target stay deleted. The edit
    // target's deletion now applies to the new path, so the old path is deleted too.
    for (const SdfPath& path : paths.overridablePaths) {
        if (paths.editTargetDeletedPaths.count(path) > 0 && !contains(composedPaths, path)) {
            pathOverride.toDelete.push_back(path);
        }
    }

    // Paths deleted by other layers stay deleted under their new path, unless they
    // compose anyway, under their old or new path. An opinion of the edit target inside
    // a variant does not prevent this, as it is weaker than the opinions outside of it.
    for (const SdfPath& path : paths.overridableDeletedPaths) {
        const SdfPath renamedPath = path.ReplacePrefix(oldPath, newPath);
        if (!contains(composedPaths, path) && !contains(composedPaths, renamedPath)) {
            pathOverride.toDelete.push_back(renamedPath);
        }
    }

    // Reorders of other layers keep applying to the renamed paths, and appended paths
    // go back to their composed position.
    for (const SdfPathVector& ordered : paths.overridableReorders) {
        for (const SdfPath& path : ordered) {
            pathOverride.toOrder.push_back(path.ReplacePrefix(oldPath, newPath));
        }
    }
    const SdfPathVector reorder
        = getReorderForAppendedPaths(composedPaths, appendedPaths, oldPath, newPath);
    pathOverride.toOrder.insert(pathOverride.toOrder.end(), reorder.begin(), reorder.end());

    return pathOverride;
}

// Return the reorder list of the edit target that also orders the paths to order: all of
// them, renamed, in their composed order. Paths the edit target already orders that do not
// compose are kept after.
SdfPathVector mergeReorder(
    const SdfPathVector& currentOrder,
    const SdfPathVector& toOrder,
    const SdfPathVector& composedPaths,
    const SdfPath&       oldPath,
    const SdfPath&       newPath)
{
    SdfPathSet listed(toOrder.begin(), toOrder.end());
    listed.insert(currentOrder.begin(), currentOrder.end());

    SdfPathVector newOrder;
    for (const SdfPath& path : composedPaths) {
        const SdfPath renamedPath = path.ReplacePrefix(oldPath, newPath);
        if (listed.erase(renamedPath) > 0) {
            newOrder.push_back(renamedPath);
        }
    }
    for (const SdfPath& path : currentOrder) {
        if (listed.count(path) > 0) {
            newOrder.push_back(path);
        }
    }
    return newOrder;
}

// Update the connections and relationship targets of the prim that point to or under the
// renamed prim, where they are authored in the stage's local layer stack. Opinions in the
// edit target layer are rewritten in place, preserving their list-op. Opinions in other
// local layers are overridden in the edit target, preserving the composed order.
void replacePropertyPaths(const UsdPrim& prim, const SdfPath& oldPath, const SdfPath& newPath)
{
    for (const auto& entry : findPropertyPathOpinions(prim, oldPath, true)) {
        const UsdProperty           prop = prim.GetProperty(entry.first);
        const PropertyPathOpinions& paths = entry.second;
        warnAboutStrongerLayers(prop, paths);

        // The composed order before any edit is the order to preserve.
        const SdfPathVector composedPaths = getComposedPaths(prop);

        for (const SdfPropertySpecHandle& spec : paths.editTargetSpecs) {
            const SdfPathEditorProxy editor = getPathEditor(spec);
            for (const SdfPath& path : paths.editTargetPaths) {
                replaceItemEdits(editor, path, path.ReplacePrefix(oldPath, newPath));
            }
        }

        const PathOverride pathOverride
            = computePathOverride(paths, composedPaths, oldPath, newPath);
        if (pathOverride.isEmpty()) {
            continue;
        }
        const SdfPropertySpecHandle spec = getOrCreateEditTargetSpec(prop);
        if (!spec) {
            TF_WARN(
                "Could not update the paths of \"%s\" authored in other layers.",
                prop.GetPath().GetText());
            continue;
        }
        SdfPathEditorProxy editor = getPathEditor(spec);
        for (const SdfPath& path : pathOverride.toDelete) {
            editor.Remove(path);
        }
        for (const SdfPath& path : pathOverride.toAppend) {
            editor.Append(path);
        }

        // An explicit list is not reordered: it replaces the weaker opinions, their reorders
        // included, and its own paths were renamed in place. A reorder would also make it
        // non-explicit, dropping its paths.
        if (pathOverride.toOrder.empty() || editor.IsExplicit()) {
            continue;
        }
        SdfPathEditorProxy::ListProxy orderedItems = editor.GetOrderedItems();
        orderedItems = mergeReorder(
            SdfPathVector(orderedItems), pathOverride.toOrder, composedPaths, oldPath, newPath);
    }
}

// Remove the connections and relationship targets of the prim that point to or under the
// deleted prim, where they are authored in the stage's local layer stack. Opinions in the
// edit target layer are removed in place. Opinions in other local layers are deleted by the
// edit target.
void removePropertyPaths(const UsdPrim& prim, const SdfPath& deletedPath)
{
    const UsdEditTarget& editTarget = prim.GetStage()->GetEditTarget();

    for (const auto& entry : findPropertyPathOpinions(prim, deletedPath, false)) {
        UsdProperty                 prop = prim.GetProperty(entry.first);
        const PropertyPathOpinions& paths = entry.second;
        warnAboutStrongerLayers(prop, paths);

        for (const SdfPropertySpecHandle& spec : paths.editTargetSpecs) {
            SdfPathEditorProxy editor = getPathEditor(spec);
            for (const SdfPath& path : paths.editTargetPaths) {
                editor.RemoveItemEdits(path);
            }
        }

        // Read after the edits above, inside the change block of the caller. This is valid
        // as connections and targets are composed from the current data of the layers,
        // which the change block does not delay.
        const SdfPathVector composedPaths = getComposedPaths(prop);
        bool                hasOverrides = false;
        for (const SdfPath& path : paths.overridablePaths) {
            if (!contains(composedPaths, path)) {
                continue;
            }
            hasOverrides = true;
            if (prop.Is<UsdAttribute>()) {
                prop.As<UsdAttribute>().RemoveConnection(path);
            } else if (prop.Is<UsdRelationship>()) {
                prop.As<UsdRelationship>().RemoveTarget(path);
            }
        }

        // Remove the edit target opinion when nothing is left connected or targeted. Only
        // done for the spec at the property path, a spec inside a variant is left empty.
        // An opinion that deletes paths is kept, as other layers may add them.
        const SdfPropertySpecHandle sceneSpec
            = editTarget.GetPropertySpecForScenePath(prop.GetPath());
        const bool isSceneSpecEdited
            = std::find(paths.editTargetSpecs.begin(), paths.editTargetSpecs.end(), sceneSpec)
            != paths.editTargetSpecs.end();
        if (!sceneSpec || !isSceneSpecEdited || hasOverrides || !composedPaths.empty()
            || !getPathEditor(sceneSpec).GetDeletedItems().empty()) {
            continue;
        }
        if (prop.Is<UsdAttribute>()) {
            UsdAttribute attr = prop.As<UsdAttribute>();
            attr.ClearConnections();
            if (!attr.HasValue()) {
                prop.GetPrim().RemoveProperty(prop.GetName());
            }
        } else if (prop.Is<UsdRelationship>()) {
            prop.As<UsdRelationship>().ClearTargets(true);
        }
    }
}

// Paths of a property spec to replace or remove in place, in the namespace of its layer.
struct InPlacePathEdit
{
    SdfPropertySpecHandle spec;
    SdfPathSet            paths;
    // The prefix of the paths and its replacement. The replacement is empty when the paths
    // are removed.
    SdfPath oldPrefix;
    SdfPath newPrefix;
};

// Collect the edits of the connections and relationship targets of the prim that point to or
// under the renamed or deleted prim, in place in every layer contributing to the prim, or only
// in onlyLayer if it is not null. newPath is empty when the prim is deleted.
//
// This is for namespace edits that rename or delete the prim in those layers, as in component
// stages: composition does not map these paths then, as the root of the referenced or
// payloaded layers does not move.
void collectInPlacePathEdits(
    const UsdPrim&                prim,
    const SdfPath&                oldPath,
    const SdfPath&                newPath,
    const SdfLayerHandle&         onlyLayer,
    std::vector<InPlacePathEdit>& edits)
{
    const bool isRemoval = newPath.IsEmpty();
    for (const PrimSpecSite& site : getPrimSpecSites(prim)) {
        if (onlyLayer && site.primSpec->GetLayer() != onlyLayer) {
            continue;
        }
        // The paths authored in the spec are in the namespace of its layer.
        const SdfPath oldSpecPath = site.mapToStage.MapTargetToSource(oldPath);
        const SdfPath newSpecPath
            = isRemoval ? SdfPath() : site.mapToStage.MapTargetToSource(newPath);
        if (oldSpecPath.IsEmpty() || (!isRemoval && newSpecPath.IsEmpty())) {
            continue;
        }
        for (const SdfPropertySpecHandle& propSpec : site.primSpec->GetProperties()) {
            const SdfPathEditorProxy editor = getPathEditor(propSpec);
            if (!editor) {
                continue;
            }
            // A deletion of a renamed path is renamed, a deletion of a removed path is kept.
            SdfPathSet paths = getAuthoredPaths(editor, oldSpecPath, !isRemoval);
            if (!paths.empty()) {
                edits.push_back({ propSpec, std::move(paths), oldSpecPath, newSpecPath });
            }
        }
    }
}

// Return true if the layers of the in-place edits, and of the prim stack of the prim if it is
// valid, can be edited. Otherwise, warn about the first layer that cannot be.
bool canEditLayers(
    const std::vector<InPlacePathEdit>& edits,
    const UsdPrim&                      prim,
    const SdfPath&                      primPath,
    const char*                         operation)
{
    SdfLayerHandleSet layers;
    for (const InPlacePathEdit& edit : edits) {
        layers.insert(edit.spec->GetLayer());
    }
    if (prim) {
        for (const SdfPrimSpecHandle& spec : prim.GetPrimStack()) {
            layers.insert(spec->GetLayer());
        }
    }
    for (const SdfLayerHandle& layer : layers) {
        if (!layer->PermissionToEdit()) {
            TF_WARN(
                "Cannot %s \"%s\": layer \"%s\" cannot be edited.",
                operation,
                primPath.GetText(),
                layer->GetDisplayName().c_str());
            return false;
        }
    }
    return true;
}

void applyInPlacePathEdits(const std::vector<InPlacePathEdit>& edits)
{
    for (const InPlacePathEdit& edit : edits) {
        // Only the edit target layer is tracked for undo by default.
        UsdUfe::UsdUndoManager::instance().trackLayerStates(edit.spec->GetLayer());
        SdfPathEditorProxy editor = getPathEditor(edit.spec);
        for (const SdfPath& path : edit.paths) {
            if (edit.newPrefix.IsEmpty()) {
                editor.RemoveItemEdits(path);
            } else {
                replaceItemEdits(editor, path, path.ReplacePrefix(edit.oldPrefix, edit.newPrefix));
            }
        }
    }
}

// Update the internal references, inherits and specializes authored on the prim in the edit
// target that point to or under the renamed prim.
void replaceCompositionArcPaths(const UsdPrim& p, const UsdPrim& oldPrim, const SdfPath& newPath)
{
    auto primSpec = UsdUfe::getPrimSpecAtEditTarget(p);
    // check different composition arcs
    if (p.HasAuthoredReferences()) {
        if (primSpec) {

            SdfReferencesProxy referencesList = primSpec->GetReferenceList();

            // update append/prepend lists individually
            replaceInternalReferencePath(oldPrim, newPath, referencesList, SdfListOpTypeAppended);
            replaceInternalReferencePath(oldPrim, newPath, referencesList, SdfListOpTypePrepended);
        }
    } else if (p.HasAuthoredInherits()) {
        if (primSpec) {

            SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

            // update append/prepend lists individually
            replacePath<SdfInheritsProxy>(oldPrim, newPath, inheritsList, SdfListOpTypeAppended);
            replacePath<SdfInheritsProxy>(oldPrim, newPath, inheritsList, SdfListOpTypePrepended);
        }
    } else if (p.HasAuthoredSpecializes()) {
        if (primSpec) {
            SdfSpecializesProxy specializesList = primSpec->GetSpecializesList();

            // update append/prepend lists individually
            replacePath<SdfSpecializesProxy>(
                oldPrim, newPath, specializesList, SdfListOpTypeAppended);
            replacePath<SdfSpecializesProxy>(
                oldPrim, newPath, specializesList, SdfListOpTypePrepended);
        }
    }
}

// Remove the internal references, inherits and specializes authored on the prim in the edit
// target that point to or under the deleted prim.
void removeCompositionArcPaths(const UsdPrim& p, const UsdPrim& deletedPrim)
{
    auto primSpec = UsdUfe::getPrimSpecAtEditTarget(p);
    // check different composition arcs
    if (p.HasAuthoredReferences()) {
        if (primSpec) {

            SdfReferencesProxy referencesList = primSpec->GetReferenceList();

            // update append/prepend lists individually
            removeInternalReferencePath(deletedPrim, referencesList, SdfListOpTypeAppended);
            removeInternalReferencePath(deletedPrim, referencesList, SdfListOpTypePrepended);
        }
    } else if (p.HasAuthoredInherits()) {
        if (primSpec) {

            SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

            // update append/prepend lists individually
            removePath<SdfInheritsProxy>(deletedPrim, inheritsList, SdfListOpTypeAppended);
            removePath<SdfInheritsProxy>(deletedPrim, inheritsList, SdfListOpTypePrepended);
        }
    } else if (p.HasAuthoredSpecializes()) {
        if (primSpec) {
            SdfSpecializesProxy specializesList = primSpec->GetSpecializesList();

            // update append/prepend lists individually
            removePath<SdfSpecializesProxy>(deletedPrim, specializesList, SdfListOpTypeAppended);
            removePath<SdfSpecializesProxy>(deletedPrim, specializesList, SdfListOpTypePrepended);
        }
    }
}

} // namespace

namespace USDUFE_NS_DEF {

SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim& prim)
{
    auto stage = prim.GetStage();
    return stage->GetEditTarget().GetPrimSpecForScenePath(prim.GetPath());
}

void printCompositionQuery(const UsdPrim& prim, std::ostream& os)
{
    UsdPrimCompositionQuery query(prim);

    os << "[\n";

    // the composition arcs are always returned in order from strongest
    // to weakest regardless of the filter.
    for (const auto& arc : query.GetCompositionArcs()) {
        const auto& arcDic = getDict(arc);
        os << "{\n";
        std::for_each(arcDic.begin(), arcDic.end(), [&](const auto& it) {
            os << it.first << ": " << it.second << '\n';
        });
        os << "}\n";
    }

    os << "]\n\n";
}

bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath)
{
    SdfChangeBlock changeBlock;

    // All prims, as classes, inactive and over-only prims can hold local opinions too.
    for (const auto& p : oldPrim.GetStage()->TraverseAll()) {
        replaceCompositionArcPaths(p, oldPrim, newPath);
        replacePropertyPaths(p, oldPrim.GetPath(), newPath);
    }

    return true;
}

bool updateReferencedPathInAllLayers(const UsdPrim& oldPrim, const SdfPath& newPath)
{
    // The paths are only edited once every layer to edit, the ones the prim is renamed in
    // included, is known to be editable. Otherwise the rename would fail with the paths
    // already updated.
    std::vector<InPlacePathEdit> inPlaceEdits;
    for (const auto& p : oldPrim.GetStage()->TraverseAll()) {
        collectInPlacePathEdits(p, oldPrim.GetPath(), newPath, {}, inPlaceEdits);
    }
    if (!canEditLayers(inPlaceEdits, oldPrim, oldPrim.GetPath(), "rename")) {
        return false;
    }

    SdfChangeBlock changeBlock;

    applyInPlacePathEdits(inPlaceEdits);
    for (const auto& p : oldPrim.GetStage()->TraverseAll()) {
        replaceCompositionArcPaths(p, oldPrim, newPath);
    }

    return true;
}

bool cleanReferencedPath(const UsdPrim& deletedPrim)
{
    SdfChangeBlock changeBlock;

    for (const auto& p : deletedPrim.GetStage()->TraverseAll()) {
        removeCompositionArcPaths(p, deletedPrim);
        removePropertyPaths(p, deletedPrim.GetPath());
    }

    return true;
}

bool cleanReferencedPathInAllLayers(const UsdPrim& deletedPrim)
{
    // As for rename, the paths are only edited once every layer to edit is known to be
    // editable.
    std::vector<InPlacePathEdit> inPlaceEdits;
    for (const auto& p : deletedPrim.GetStage()->TraverseAll()) {
        collectInPlacePathEdits(p, deletedPrim.GetPath(), SdfPath(), {}, inPlaceEdits);
    }
    if (!canEditLayers(inPlaceEdits, deletedPrim, deletedPrim.GetPath(), "delete")) {
        return false;
    }

    SdfChangeBlock changeBlock;

    applyInPlacePathEdits(inPlaceEdits);
    for (const auto& p : deletedPrim.GetStage()->TraverseAll()) {
        removeCompositionArcPaths(p, deletedPrim);
    }

    return true;
}

bool cleanReferencedPathInLayer(const UsdPrim& deletedPrim, const SdfLayerHandle& layer)
{
    std::vector<InPlacePathEdit> inPlaceEdits;
    for (const auto& p : deletedPrim.GetStage()->TraverseAll()) {
        collectInPlacePathEdits(p, deletedPrim.GetPath(), SdfPath(), layer, inPlaceEdits);
    }
    if (!canEditLayers(inPlaceEdits, UsdPrim(), deletedPrim.GetPath(), "delete")) {
        return false;
    }

    // Removed first, so that the opinions removed in place are not also overridden from the
    // edit target.
    {
        SdfChangeBlock changeBlock;
        applyInPlacePathEdits(inPlaceEdits);
    }

    return cleanReferencedPath(deletedPrim);
}

bool isInternalReference(const SdfReference& ref) { return ref.IsInternal(); }

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

} // namespace USDUFE_NS_DEF
