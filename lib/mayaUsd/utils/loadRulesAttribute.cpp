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

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

namespace {

const char loadRulesAttrName[] = "usdStageLoadRules";

} // namespace

bool hasLoadRulesAttribute(const PXR_NS::MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return false;

    return hasDynamicAttribute(MFnDependencyNode(proxyObj), loadRulesAttrName);
}

MStatus copyLoadRulesToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, loadRulesAttrName))
        createDynamicAttribute(depNode, loadRulesAttrName);

    MString loadRulesText = convertLoadRulesToText(stage);

    MStatus status = setDynamicAttribute(depNode, loadRulesAttrName, loadRulesText);

    return status;
}

MStatus copyLoadRulesFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage)
{
    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull())
        return MS::kFailure;

    MFnDependencyNode depNode(proxyObj);
    if (!hasDynamicAttribute(depNode, loadRulesAttrName))
        return MS::kNotFound;

    MString loadRulesText;
    MStatus status = getDynamicAttribute(depNode, loadRulesAttrName, loadRulesText);
    if (status == MS::kSuccess)
        setLoadRulesFromText(stage, loadRulesText);

    return status;
}

} // namespace MAYAUSD_NS_DEF
