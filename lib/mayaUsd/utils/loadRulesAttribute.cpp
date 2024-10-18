//
// Copyright 2022 Autodesk
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

#include "dynamicAttribute.h"
#include "loadRules.h"

#include <usdUfe/utils/loadRules.h>

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

namespace {

const char kLoadRulesAttrName[] = "usdStageLoadRules";

} // namespace

bool hasLoadRulesAttribute(const PXR_NS::MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return false;

    return hasDynamicAttribute(MFnDependencyNode(proxyObj), kLoadRulesAttrName);
}

MStatus copyLoadRulesToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, kLoadRulesAttrName))
        createDynamicAttribute(depNode, kLoadRulesAttrName);

    auto loadRulesText = UsdUfe::convertLoadRulesToText(stage);

    MStatus status = setDynamicAttribute(depNode, kLoadRulesAttrName, loadRulesText.c_str());

    return status;
}

MStatus copyLoadRulesFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    PXR_NS::UsdStageLoadRules rules;
    MStatus status = MayaUsd::getLoadRulesFromAttribute(proxyShape.thisMObject(), rules);
    if (status == MS::kSuccess)
        UsdUfe::setLoadRules(stage, rules);

    return status;
}

MStatus getLoadRulesFromAttribute(const MObject& proxyObj, PXR_NS::UsdStageLoadRules& rules)
{
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, kLoadRulesAttrName))
        return MS::kNotFound;

    MString loadRulesText;
    MStatus status = getDynamicAttribute(depNode, kLoadRulesAttrName, loadRulesText);
    if (!status)
        return status;

    rules = UsdUfe::createLoadRulesFromText(loadRulesText.asChar());
    return MS::kSuccess;
}

MStatus setLoadRulesAttribute(const PXR_NS::MayaUsdProxyShapeBase& proxyShape, bool loadAllPayloads)
{
    return setLoadRulesAttribute(proxyShape.thisMObject(), loadAllPayloads);
}

MStatus setLoadRulesAttribute(const MObject& proxyObj, bool loadAllPayloads)
{
    if (proxyObj.isNull())
        return MS::kFailure;

    PXR_NS::UsdStageLoadRules rules;
    if (loadAllPayloads) {
        rules.LoadWithDescendants(PXR_NS::SdfPath("/"));
    } else {
        rules.Unload(PXR_NS::SdfPath("/"));
    }

    const std::string loadRulesText = UsdUfe::convertLoadRulesToText(rules);

    MFnDependencyNode depNode(proxyObj);
    return setDynamicAttribute(depNode, kLoadRulesAttrName, loadRulesText.c_str());
}

} // namespace MAYAUSD_NS_DEF
