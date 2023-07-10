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

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/specializes.h>
#include <pxr/usd/usd/stage.h>

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

void replacePropertyPath(const UsdPrim& oldPrim, const SdfPath& newPath, UsdProperty& prop)
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute  attr = prop.As<UsdAttribute>();
        SdfPathVector sources;
        attr.GetConnections(&sources);
        bool hasChanged = false;
        for (size_t i = 0; i < sources.size(); ++i) {
            const SdfPath& path = sources[i];
            SdfPath        finalPath = path.ReplacePrefix(oldPrim.GetPath(), newPath);
            if (path != finalPath) {
                sources[i] = finalPath;
                hasChanged = true;
            }
        }
        if (hasChanged) {
            attr.SetConnections(sources);
        }
    } else if (prop.Is<UsdRelationship>()) {
        UsdRelationship rel = prop.As<UsdRelationship>();
        SdfPathVector   targets;
        rel.GetTargets(&targets);
        bool hasChanged = false;
        for (size_t i = 0; i < targets.size(); ++i) {
            const SdfPath& path = targets[i];
            SdfPath        finalPath = path.ReplacePrefix(oldPrim.GetPath(), newPath);
            if (path != finalPath) {
                targets[i] = finalPath;
                hasChanged = true;
            }
        }
        if (hasChanged) {
            rel.SetTargets(targets);
        }
    }
}

void removePropertyPath(const UsdPrim& deletedPrim, UsdProperty& prop)
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute  attr = prop.As<UsdAttribute>();
        SdfPathVector sources;
        attr.GetConnections(&sources);
        bool hasChanged = false;
        for (size_t i = 0; i < sources.size();) {
            const SdfPath& path = sources[i];
            if (deletedPrim.GetPath() == path.GetPrimPath()
                || path.HasPrefix(deletedPrim.GetPath())) {
                hasChanged = true;
                sources.erase(sources.cbegin() + i);
                continue;
            }
            ++i;
        }
        if (hasChanged) {
            if (sources.empty()) {
                attr.ClearConnections();
                if (!attr.HasValue()) {
                    prop.GetPrim().RemoveProperty(prop.GetName());
                }
            } else {
                attr.SetConnections(sources);
            }
        }
    } else if (prop.Is<UsdRelationship>()) {
        UsdRelationship rel = prop.As<UsdRelationship>();
        SdfPathVector   targets;
        rel.GetTargets(&targets);
        bool hasChanged = false;
        for (size_t i = 0; i < targets.size();) {
            const SdfPath& path = targets[i];
            if (deletedPrim.GetPath() == path.GetPrimPath()
                || path.HasPrefix(deletedPrim.GetPath())) {
                hasChanged = true;
                targets.erase(targets.cbegin() + i);
                continue;
            }
            ++i;
        }
        if (hasChanged) {
            if (targets.empty()) {
                rel.ClearTargets(true);
            } else {
                rel.SetTargets(targets);
            }
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

    for (const auto& p : oldPrim.GetStage()->Traverse()) {

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
        for (auto& prop : p.GetProperties()) {
            replacePropertyPath(oldPrim, newPath, prop);
        }
    }

    return true;
}

bool cleanReferencedPath(const UsdPrim& deletedPrim)
{
    SdfChangeBlock changeBlock;

    for (const auto& p : deletedPrim.GetStage()->Traverse()) {

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

        // Need to repath connections and relationships:
        for (auto& prop : p.GetProperties()) {
            removePropertyPath(deletedPrim, prop);
        }
    }

    return true;
}

bool isInternalReference(const SdfReference& ref) { return ref.IsInternal(); }

} // namespace USDUFE_NS_DEF
