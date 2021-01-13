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

#include "util.h"

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/specializes.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/references.h>
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
        if (MayaUsdUtils::isInternalReference(ref)) {
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

// This template method updates the SdfPath for inherited or specialized arcs 
// when the path to the concrete prim they refer to has changed.
// HS January 13, 2021: Find a better generic way to consolidate this method with replaceReferenceItems
template<typename T>
void replacePath(
    const UsdPrim&            oldPrim,
    const SdfPath&            newPath,
    const T&                  proxy,
    SdfListOpType             op)
{
    // set the listProxy based on the SdfListOpType
    T::ListProxy listProxy = proxy.GetAppendedItems();
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

} // namespace

namespace MayaUsdUtils {

SdfLayerHandle defPrimSpecLayer(const UsdPrim& prim)
{
    // Iterate over the layer stack, starting at the highest-priority layer.
    // The source layer is the one in which there exists a def primSpec, not
    // an over.

    SdfLayerHandle defLayer;
    auto           layerStack = prim.GetStage()->GetLayerStack();

    for (auto layer : layerStack) {
        auto primSpec = layer->GetPrimAtPath(prim.GetPath());
        if (primSpec && (primSpec->GetSpecifier() == SdfSpecifierDef)) {
            defLayer = layer;
            break;
        }
    }
    return defLayer;
}

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
                replaceInternalReferencePath(oldPrim, newPath, referencesList, SdfListOpTypeAppended);
                replaceInternalReferencePath(oldPrim, newPath, referencesList, SdfListOpTypePrepended);
            }
        }
        else if (p.HasAuthoredInherits()) {
            if (primSpec) {

                SdfInheritsProxy inheritsList = primSpec->GetInheritPathList();

                // update append/prepend lists individually
                replacePath<SdfInheritsProxy>(oldPrim, newPath, inheritsList, SdfListOpTypeAppended);
                replacePath<SdfInheritsProxy>(oldPrim, newPath, inheritsList, SdfListOpTypePrepended);
            }
        }
        else if (p.HasAuthoredSpecializes()) {
            if (primSpec) 
            {
                SdfSpecializesProxy specializesList = primSpec->GetSpecializesList();

                // update append/prepend lists individually
                replacePath<SdfSpecializesProxy>(oldPrim, newPath, specializesList, SdfListOpTypeAppended);
                replacePath<SdfSpecializesProxy>(oldPrim, newPath, specializesList, SdfListOpTypePrepended);
            }
        }
    }

    return true;
}

bool isInternalReference(const SdfReference& ref)
{
#if USD_VERSION_NUM >= 2008
    return ref.IsInternal();
#else
    return ref.GetAssetPath().empty();
#endif
}

} // namespace MayaUsdUtils
