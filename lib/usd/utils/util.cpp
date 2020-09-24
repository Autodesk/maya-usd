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
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    std::map<std::string,std::string> 
    getDict(const UsdPrimCompositionQueryArc& arc) {
        std::string arcType;
        switch (arc.GetArcType()) {
            case PcpArcTypeRoot:
                arcType = "PcpArcTypeRoot";
                break;
            case PcpArcTypeReference:
                arcType = "PcpArcTypeReference";
                break;
            case PcpArcTypePayload:
                arcType = "PcpArcTypePayload";
                break;
            case PcpArcTypeInherit:
                arcType = "PcpArcTypeInherit";
                break;
            case PcpArcTypeSpecialize:
                arcType = "PcpArcTypeSpecialize";
                break;
            case PcpArcTypeVariant:
                arcType = "PcpArcTypeVariant";
                break;
            default:
                break;
        }

        auto introducingLayer = arc.GetIntroducingLayer();
        auto introducingNode = arc.GetIntroducingNode();

        return {
            {"arcType" , arcType},
            {"hasSpecs", arc.HasSpecs() ? "True" : "False"},
            {"introLayer", introducingLayer ? introducingLayer->GetRealPath() : ""},
            {"introLayerStack", introducingNode ? introducingNode.GetLayerStack()->GetIdentifier().rootLayer->GetRealPath() : ""},
            {"introPath", arc.GetIntroducingPrimPath().GetString()},
            {"isAncestral", arc.IsAncestral() ? "True" : "False"},
            {"isImplicit", arc.IsImplicit() ? "True" : "False"},
            {"isIntroRootLayer", arc.IsIntroducedInRootLayerStack() ? "True" : "False"},
            {"isIntroRootLayerPrim", arc.IsIntroducedInRootLayerPrimSpec() ? "True" : "False" },
            {"nodeLayerStack", arc.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer->GetRealPath()},
            {"nodePath", arc.GetTargetNode().GetPath().GetString()},
        };
    }
}

namespace MayaUsdUtils {

SdfLayerHandle
defPrimSpecLayer(const UsdPrim& prim)
{
    // Iterate over the layer stack, starting at the highest-priority layer.
    // The source layer is the one in which there exists a def primSpec, not
    // an over.

    SdfLayerHandle defLayer;
    auto layerStack = prim.GetStage()->GetLayerStack();

    for (auto layer : layerStack) {
        auto primSpec = layer->GetPrimAtPath(prim.GetPath());
        if (primSpec && (primSpec->GetSpecifier() == SdfSpecifierDef)) {
            defLayer = layer;
            break;
        }
    }
    return defLayer;
}


SdfPrimSpecHandle 
getPrimSpecAtEditTarget(const UsdPrim& prim)
{
    auto stage = prim.GetStage();
    return stage->GetEditTarget().GetPrimSpecForScenePath(prim.GetPath());
}

bool
isInternalReference(const SdfPrimSpecHandle& primSpec)
{
    bool isInternalRef{false};

    for (const SdfReference& ref : primSpec->GetReferenceList().GetAddedOrExplicitItems()) {
        // GetAssetPath returns the asset path to the root layer of the referenced layer
        // this will be empty in the case of an internal reference.
        if (ref.GetAssetPath().empty()) {
            isInternalRef = true;
            break;
        }
    }

    return isInternalRef;
}

void
printCompositionQuery(const UsdPrim& prim, std::ostream& os)
{
    UsdPrimCompositionQuery query(prim);

    os << "[\n";

    // the composition arcs are always returned in order from strongest 
    // to weakest regardless of the filter.
    for (const auto& arc : query.GetCompositionArcs()) {
        const auto& arcDic = getDict(arc);
        os << "{\n";
        std::for_each(arcDic.begin(),arcDic.end(), [&](const auto& it) {
            os << it.first << ": " << it.second << '\n';
        });
        os << "}\n";
    }

    os << "]\n\n";
}

} // MayaUsdUtils
