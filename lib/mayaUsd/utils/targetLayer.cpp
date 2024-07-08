//
// Copyright 2023 Autodesk
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

#include "targetLayer.h"

#include "dynamicAttribute.h"

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/usd/editTarget.h>

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MString.h>

namespace {

const char kTargetLayerAttrName[] = "usdStageTargetLayer";
const char kTargetLayerPrimPathAttrName[] = "usdStageTargetLayerPrimPath";

/// \brief Find prim node that matches given layer id
PXR_NS::PcpNodeRef
findPrimNode(const PXR_NS::PcpNodeRef& primNode, const std::string& targetLayerId)
{
    // We need to store and check current depth level first (BFS), this is to
    // preserve the opinions from strong to week
    std::vector<PXR_NS::PcpNodeRef> childNodes;
    TF_FOR_ALL(child, primNode.GetChildrenRange())
    {
        // The prim node can't have a variant.
        if (child->GetPath().ContainsPrimVariantSelection()) {
            continue;
        }
        // Confirm that the primNode is a direct contributor to the root prim.
        bool result = child->IsRootNode();
        // Confirm we are looking at one of the composition arcs that could lead us to a prim
        // instance and not some other contributor like a specializes or such.
        if (!result) {
            auto arcType = child->GetArcType();
            result = (arcType == PcpArcTypeReference) || (arcType == PcpArcTypePayload)
                || (arcType == PcpArcTypeVariant);
        }

        if (result) {
            if (child->GetLayerStack()->GetIdentifier().rootLayer->GetIdentifier()
                == targetLayerId) {
                return *child;
            }
            childNodes.emplace_back(*child);
        }
    }

    for (const auto& child : childNodes) {
        auto childPrimNode = findPrimNode(child, targetLayerId);
        if (childPrimNode) {
            return childPrimNode;
        }
    }
    return {};
}

} // namespace

namespace MAYAUSD_NS_DEF {

MString convertTargetLayerToText(const PXR_NS::UsdStage& stage)
{
    MString text;

    const PXR_NS::UsdEditTarget target = stage.GetEditTarget();
    if (!target.IsValid())
        return text;

    const PXR_NS::SdfLayerHandle& layer = target.GetLayer();
    if (!layer)
        return text;

    text = layer->GetIdentifier().c_str();

    return text;
}

PXR_NS::SdfLayerHandle getTargetLayerFromText(PXR_NS::UsdStage& stage, const MString& text)
{
    if (text.length() == 0)
        return {};

    const std::string layerId(text.asChar());
    for (const auto& layer : stage.GetUsedLayers())
        if (layer->GetIdentifier() == layerId)
            return layer;

    return {};
}

bool setTargetLayerFromText(PXR_NS::UsdStage& stage, const MString& text)
{
    PXR_NS::SdfLayerHandle layer = getTargetLayerFromText(stage, text);
    if (!layer)
        return false;

    stage.SetEditTarget(layer);
    return true;
}

MStatus copyTargetLayerToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MString     targetLayerText;
    MString     targetLayerPrimPath;
    const auto& editTarget = stage.GetEditTarget();
    if (editTarget.IsValid()) {
        const auto& editTargetLayer = editTarget.GetLayer();
        targetLayerText = editTargetLayer->GetIdentifier().c_str();
        if (!stage.HasLocalLayer(editTargetLayer)) {
            const auto& pathMap(editTarget.GetMapFunction().GetSourceToTargetMap());
            if (!pathMap.empty()) {
                // Save the top most prim path as the reference prim path for this edit target,
                // when restoring edit target , we will need a prim path to locate to
                // this layer.
                targetLayerPrimPath = pathMap.begin()->second.GetString().c_str();
            }
        }
    }

    MFnDependencyNode depNode(proxyObj);
    // Don't set the attribute if it already has the same value to avoid
    // update loops.
    MString previousTargetLayerText;
    getDynamicAttribute(depNode, kTargetLayerAttrName, previousTargetLayerText);

    MString previousTargetLayerPrimPathText;
    getDynamicAttribute(depNode, kTargetLayerPrimPathAttrName, previousTargetLayerPrimPathText);

    if (previousTargetLayerText == targetLayerText
        && previousTargetLayerPrimPathText == targetLayerPrimPath)
        return MS::kSuccess;

    // Create and set dynamic attribute only when needed
    MStatus status = setDynamicAttribute(depNode, kTargetLayerAttrName, targetLayerText);
    if (status == MS::kSuccess && targetLayerPrimPath.length()) {
        status = setDynamicAttribute(depNode, kTargetLayerPrimPathAttrName, targetLayerPrimPath);
    }

    return status;
}

MString getTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape)
{
    MString targetLayerText;

    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return targetLayerText;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, kTargetLayerAttrName))
        return targetLayerText;

    getDynamicAttribute(depNode, kTargetLayerAttrName, targetLayerText);
    return targetLayerText;
}

PXR_NS::SdfLayerHandle
getTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    const MString layerId = getTargetLayerFromAttribute(proxyShape);
    return getTargetLayerFromText(stage, layerId);
}

MStatus
copyTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    const MString targetLayerText = getTargetLayerFromAttribute(proxyShape);
    return setTargetLayerFromText(stage, targetLayerText) ? MS::kSuccess : MS::kNotFound;
}

PXR_NS::UsdEditTarget
getEditTargetFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull()) {
        return {};
    }

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, kTargetLayerAttrName)) {
        return {};
    }

    MString targetLayerText;
    if (getDynamicAttribute(depNode, kTargetLayerAttrName, targetLayerText) != MS::kSuccess) {
        return {};
    }

    PXR_NS::SdfLayerHandle layer = getTargetLayerFromText(stage, targetLayerText);
    if (stage.HasLocalLayer(layer)) {
        // Exit early if the layer in local layer stack
        return layer;
    }

    MString targetLayerPrimPathText;
    if (hasDynamicAttribute(depNode, kTargetLayerPrimPathAttrName)) {
        getDynamicAttribute(depNode, kTargetLayerPrimPathAttrName, targetLayerPrimPathText);
    }

    if (!targetLayerPrimPathText.length()) {
        return {};
    }

    std::string layerId = layer->GetIdentifier();

    auto prim = stage.GetPrimAtPath(PXR_NS::SdfPath(targetLayerPrimPathText.asChar()));
    if (!prim) {
        MGlobal::displayError(
            MString("Failed to construct non local edit target from layer id \"") + layerId.c_str()
            + "\", reference prim path does not exist");
        return {};
    }

    PXR_NS::PcpNodeRef primNode = prim.GetPrimIndex().GetRootNode();
    if (primNode.GetLayerStack()->GetIdentifier().rootLayer->GetIdentifier() != layerId) {
        // Search target layer from children recursively
        primNode = findPrimNode(primNode, layerId);
    }

    if (!primNode) {
        MGlobal::displayError(
            MString("Failed to construct non local edit target from layer id \"") + layerId.c_str()
            + "\", cannot find reference prim path \"" + targetLayerPrimPathText + "\"");
        return {};
    }

    return PXR_NS::UsdEditTarget(layer, primNode);
}

} // namespace MAYAUSD_NS_DEF
