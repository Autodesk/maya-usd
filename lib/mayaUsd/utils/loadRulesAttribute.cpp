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

#include "loadRules.h"

#include <maya/MCommandResult.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

namespace {

const char loadRulesAttrName[] = "usdStageLoadRules";

bool hasLoadRulesAttribute(const MFnDependencyNode& depNode)
{
    MString nodeName = depNode.absoluteName();
    MString cmd;
    cmd.format("attributeQuery -exists -n \"^2s\" \"^1s\"", loadRulesAttrName, nodeName);

    int        result = 0;
    const bool display = false;
    const bool undoable = false;
    MGlobal::executeCommand(cmd, result, display, undoable);

    return result != 0;
}

MStatus createLoadRulesAttribute(const MFnDependencyNode& depNode)
{
    MStatus status = MS::kSuccess;

    MString nodeName = depNode.absoluteName();
    MString cmd;
    cmd.format(
        "addAttr -longName \"^1s\" -dataType \"string\" -hidden true -keyable false -writable true "
        "-storable true \"^2s\";",
        loadRulesAttrName,
        nodeName);

    const bool display = false;
    const bool undoable = false;
    status = MGlobal::executeCommand(cmd, display, undoable);

    return status;
}

MStatus getLoadRulesAttribute(const MFnDependencyNode& depNode, MString& value)
{
    MStatus status = MS::kSuccess;

    MString nodeName = depNode.absoluteName();
    MString cmd;
    cmd.format("getAttr \"^2s.^1s\";", loadRulesAttrName, nodeName);

    const bool display = false;
    const bool undoable = false;
    status = MGlobal::executeCommand(cmd, value, display, undoable);

    return status;
}

MStatus setLoadRulesAttribute(const MFnDependencyNode& depNode, const MString& value)
{
    MStatus status = MS::kSuccess;

    MString nodeName = depNode.absoluteName();
    MString cmd;
    cmd.format("setAttr \"^2s.^1s\" -type \"string\" \"^3s\";", loadRulesAttrName, nodeName, value);

    MString    result;
    const bool display = false;
    const bool undoable = false;
    status = MGlobal::executeCommand(cmd, result, display, undoable);

    return status;
}

} // namespace

bool hasLoadRulesAttribute(const MObject& obj)
{
    return hasLoadRulesAttribute(MFnDependencyNode(obj));
}

MStatus copyLoadRulesToAttribute(const PXR_NS::UsdStage& stage, MObject& obj)
{
    MStatus status = MS::kSuccess;

    MFnDependencyNode depNode(obj);
    if (!hasLoadRulesAttribute(depNode))
        createLoadRulesAttribute(depNode);

    MString loadRulesText = convertLoadRulesToText(stage);

    status = setLoadRulesAttribute(depNode, loadRulesText);

    return status;
}

MStatus copyLoadRulesFromAttribute(const MObject& obj, PXR_NS::UsdStage& stage)
{
    MStatus status = MS::kSuccess;

    MFnDependencyNode depNode(obj);
    if (!hasLoadRulesAttribute(depNode))
        return MS::kNotFound;

    MString loadRulesText;
    status = getLoadRulesAttribute(depNode, loadRulesText);
    if (status == MS::kSuccess)
        setLoadRulesFromText(stage, loadRulesText);

    return status;
}

} // namespace MAYAUSD_NS_DEF
