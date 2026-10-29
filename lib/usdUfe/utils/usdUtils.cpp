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

#include <usdUfe/utils/Utils.h>
#include <usdUfe/utils/layers.h>

#include <pxr/usd/pcp/layerStack.h>
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

// Paths at or under a prefix authored in the local layer stack for one property.
struct LocalPropertyPaths
{
    // The property spec at the stage's edit target, if it authors any of the paths.
    SdfPropertySpecHandle editTargetSpec;
    // Paths authored in the property spec at the stage's edit target.
    SdfPathSet editTarget;
    // Paths added by property specs of the other layers of the local layer stack.
    SdfPathSet otherLayers;
    // Paths deleted by property specs of the other layers of the local layer stack.
    SdfPathSet otherLayersDeleted;
};

void addPathsWithPrefix(const SdfPathVector& paths, const SdfPath& prefix, SdfPathSet& result)
{
    for (const SdfPath& path : paths) {
        if (path.HasPrefix(prefix)) {
            result.insert(path);
        }
    }
}

// Find, per property of the prim, the connection and target paths at or under the prefix
// that are authored in the stage's local layer stack.
//
// Rule: remap opinions where they are authored, never composed results. Paths authored
// locally are absolute and must be updated. Paths reached through a reference, payload,
// inherit or specialize arc are mapped by composition and follow on their own.
std::map<TfToken, LocalPropertyPaths>
findLocalPropertyPaths(const UsdPrim& prim, const SdfPath& prefix, bool includeDeleted)
{
    std::map<TfToken, LocalPropertyPaths> result;

    const UsdEditTarget& editTarget = prim.GetStage()->GetEditTarget();

    for (const SdfPrimSpecHandle& primSpec : UsdUfe::getLocalPrimStack(prim)) {
        for (const SdfPropertySpecHandle& propSpec : primSpec->GetProperties()) {
            const SdfPathEditorProxy editor = getPathEditor(propSpec);
            if (!editor) {
                continue;
            }

            SdfPathSet added;
            for (const SdfPathVector& items : { SdfPathVector(editor.GetExplicitItems()),
                                                SdfPathVector(editor.GetAddedItems()),
                                                SdfPathVector(editor.GetPrependedItems()),
                                                SdfPathVector(editor.GetAppendedItems()),
                                                SdfPathVector(editor.GetOrderedItems()) }) {
                addPathsWithPrefix(items, prefix, added);
            }
            SdfPathSet deleted;
            if (includeDeleted) {
                addPathsWithPrefix(editor.GetDeletedItems(), prefix, deleted);
            }
            if (added.empty() && deleted.empty()) {
                continue;
            }

            const TfToken&      name = propSpec->GetNameToken();
            LocalPropertyPaths& paths = result[name];
            if (propSpec
                == editTarget.GetPropertySpecForScenePath(prim.GetPath().AppendProperty(name))) {
                paths.editTargetSpec = propSpec;
                paths.editTarget.insert(added.begin(), added.end());
                paths.editTarget.insert(deleted.begin(), deleted.end());
            } else {
                paths.otherLayers.insert(added.begin(), added.end());
                paths.otherLayersDeleted.insert(deleted.begin(), deleted.end());
            }
        }
    }

    return result;
}

// Update the connections and relationship targets of the prim that point to or under the
// renamed prim. Opinions in the edit target are rewritten in place, preserving their
// list-op. Opinions in other local layers are overridden in the edit target.
void replacePropertyPaths(const UsdPrim& prim, const SdfPath& oldPath, const SdfPath& newPath)
{
    for (const auto& entry : findLocalPropertyPaths(prim, oldPath, true)) {
        const UsdProperty         prop = prim.GetProperty(entry.first);
        const LocalPropertyPaths& paths = entry.second;

        if (!paths.editTarget.empty()) {
            SdfPathEditorProxy editor = getPathEditor(paths.editTargetSpec);
            for (const SdfPath& path : paths.editTarget) {
                editor.ReplaceItemEdits(path, path.ReplacePrefix(oldPath, newPath));
            }
        }

        const SdfPathVector composedPaths = getComposedPaths(prop);
        for (const SdfPath& path : paths.otherLayers) {
            if (std::count(composedPaths.begin(), composedPaths.end(), path) == 0) {
                continue;
            }
            const SdfPath finalPath = path.ReplacePrefix(oldPath, newPath);
            if (prop.Is<UsdAttribute>()) {
                UsdAttribute attr = prop.As<UsdAttribute>();
                attr.RemoveConnection(path);
                attr.AddConnection(finalPath);
            } else if (prop.Is<UsdRelationship>()) {
                UsdRelationship rel = prop.As<UsdRelationship>();
                rel.RemoveTarget(path);
                rel.AddTarget(finalPath);
            }
        }

        // Paths deleted in other layers must stay deleted at their new location, unless the
        // edit target has its own opinion about them.
        for (const SdfPath& path : paths.otherLayersDeleted) {
            if (paths.editTarget.count(path) > 0) {
                continue;
            }
            const SdfPath finalPath = path.ReplacePrefix(oldPath, newPath);
            if (prop.Is<UsdAttribute>()) {
                prop.As<UsdAttribute>().RemoveConnection(finalPath);
            } else if (prop.Is<UsdRelationship>()) {
                prop.As<UsdRelationship>().RemoveTarget(finalPath);
            }
        }
    }
}

// Remove the connections and relationship targets of the prim that point to or under the
// deleted prim. Opinions in the edit target are removed in place. Opinions in other local
// layers are deleted by the edit target.
void removePropertyPaths(const UsdPrim& prim, const SdfPath& deletedPath)
{
    for (const auto& entry : findLocalPropertyPaths(prim, deletedPath, false)) {
        UsdProperty               prop = prim.GetProperty(entry.first);
        const LocalPropertyPaths& paths = entry.second;

        if (!paths.editTarget.empty()) {
            SdfPathEditorProxy editor = getPathEditor(paths.editTargetSpec);
            for (const SdfPath& path : paths.editTarget) {
                editor.RemoveItemEdits(path);
            }
        }

        const SdfPathVector composedPaths = getComposedPaths(prop);
        bool                hasOverrides = false;
        for (const SdfPath& path : paths.otherLayers) {
            if (std::count(composedPaths.begin(), composedPaths.end(), path) == 0) {
                continue;
            }
            hasOverrides = true;
            if (prop.Is<UsdAttribute>()) {
                prop.As<UsdAttribute>().RemoveConnection(path);
            } else if (prop.Is<UsdRelationship>()) {
                prop.As<UsdRelationship>().RemoveTarget(path);
            }
        }

        // Remove the edit target opinion when nothing is left connected or targeted.
        if (paths.editTarget.empty() || hasOverrides || !composedPaths.empty()) {
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

        auto primSpec = getPrimSpecAtEditTarget(p);
        // check different composition arcs
        if (p.HasAuthoredReferences()) {
            if (primSpec) {

                SdfReferencesProxy referencesList = primSpec->GetReferenceList();

                // update append/prepend lists individually
                replaceInternalReferencePath(
                    oldPrim, newPath, referencesList, SdfListOpTypeAppended);
                replaceInternalReferencePath(
                    oldPrim, newPath, referencesList, SdfListOpTypePrepended);
            }
        } else if (p.HasAuthoredInherits()) {
            if (primSpec) {

                SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

                // update append/prepend lists individually
                replacePath<SdfInheritsProxy>(
                    oldPrim, newPath, inheritsList, SdfListOpTypeAppended);
                replacePath<SdfInheritsProxy>(
                    oldPrim, newPath, inheritsList, SdfListOpTypePrepended);
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

        // Need to repath connections and relationships:
        replacePropertyPaths(p, oldPrim.GetPath(), newPath);
    }

    return true;
}

bool cleanReferencedPath(const UsdPrim& deletedPrim)
{
    SdfChangeBlock changeBlock;

    for (const auto& p : deletedPrim.GetStage()->TraverseAll()) {

        auto primSpec = getPrimSpecAtEditTarget(p);
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
                removePath<SdfSpecializesProxy>(
                    deletedPrim, specializesList, SdfListOpTypeAppended);
                removePath<SdfSpecializesProxy>(
                    deletedPrim, specializesList, SdfListOpTypePrepended);
            }
        }

        // Need to remove connections and relationship targets:
        removePropertyPaths(p, deletedPrim.GetPath());
    }

    return true;
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
