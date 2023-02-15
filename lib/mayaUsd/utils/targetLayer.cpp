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

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editTarget.h>

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

MString convertTargetLayerToText(const PXR_NS::UsdStage& stage)
{
    MString text;

    const PXR_NS::UsdEditTarget   target = stage.GetEditTarget();
    if (!target.IsValid())
        return text;

    const PXR_NS::SdfLayerHandle& layer = target.GetLayer();
    if (!layer)
        return text;

    text = layer->GetIdentifier().c_str();

    return text;
}

void setTargetLayerFromText(PXR_NS::UsdStage& stage, const MString& text)
{
    if (text.isEmpty())
        return;

    const std::string layerId(text.asChar());
    for (const auto& layer : stage.GetLayerStack())
        if (layer->GetIdentifier() == layerId)
            stage.SetEditTarget(layer);
}

namespace {

const char targetLayerAttrName[] = "usdStageTargetLayer";

} // namespace

MStatus copyTargetLayerToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, targetLayerAttrName))
        createDynamicAttribute(depNode, targetLayerAttrName);

    MString loadRulesText = convertTargetLayerToText(stage);

    MStatus status = setDynamicAttribute(depNode, targetLayerAttrName, loadRulesText);

    return status;
}

MStatus
copyTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, targetLayerAttrName))
        return MS::kNotFound;

    MString loadRulesText;
    MStatus status = getDynamicAttribute(depNode, targetLayerAttrName, loadRulesText);
    if (status == MS::kSuccess)
        setTargetLayerFromText(stage, loadRulesText);

    return status;
}

} // namespace MAYAUSD_NS_DEF
