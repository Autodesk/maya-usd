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
#include "adskMaterialCommands.h"

#include <usdUfe/ufe/MaterialUtils.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

const MString
    ADSKMayaUSDGetMaterialsForRenderersCommand::commandName("mayaUsdGetMaterialsFromRenderers");
const MString ADSKMayaUSDGetMaterialsInStageCommand::commandName("mayaUsdGetMaterialsInStage");
const MString ADSKMayaUSDMaterialBindingsCommand::commandName("mayaUsdMaterialBindings");

/*
// ADSKMayaUSDGetMaterialsForRenderersCommand
*/

// plug-in callback to create the command object
void* ADSKMayaUSDGetMaterialsForRenderersCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDGetMaterialsForRenderersCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDGetMaterialsForRenderersCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus ADSKMayaUSDGetMaterialsForRenderersCommand::doIt(const MArgList& argList)
{
    clearResult();

    for (const auto& entry : UsdUfe::getMaterialsFromRenderers()) {
        appendToResult(MString(TfStringPrintf(
                                   "%s/%s|%s",
                                   entry.first.c_str(),
                                   entry.second.label.c_str(),
                                   entry.second.item.c_str())
                                   .c_str()));
    }

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDGetMaterialsForRenderersCommand::createSyntax()
{
    MSyntax syntax;
    return syntax;
}

/*
// ADSKMayaUSDGetMaterialsInStageCommand
*/

// plug-in callback to create the command object
void* ADSKMayaUSDGetMaterialsInStageCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDGetMaterialsInStageCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDGetMaterialsInStageCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus ADSKMayaUSDGetMaterialsInStageCommand::doIt(const MArgList& argList)
{
    clearResult();

    MStatus      status;
    MArgDatabase args(syntax(), argList, &status);
    if (!status)
        return status;

    MString ufePathString = args.commandArgumentString(0);
    if (ufePathString.length() == 0) {
        MGlobal::displayError("Missing argument 'UFE Path'.");
        throw MS::kFailure;
    }

    const auto ufePath = Ufe::PathString::path(ufePathString.asChar());
    for (const auto& materialPath : UsdUfe::getMaterialsInStage(ufePath)) {
        appendToResult(MString(materialPath.GetString().c_str()));
    }

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDGetMaterialsInStageCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addArg(MSyntax::kString);
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

/*
// ADSKMayaUSDMaterialBindingsCommand
*/

// plug-in callback to create the command object
void* ADSKMayaUSDMaterialBindingsCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDMaterialBindingsCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDMaterialBindingsCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus ADSKMayaUSDMaterialBindingsCommand::doIt(const MArgList& argList)
{
    clearResult();

    MStatus      status;
    MArgParser   parser(syntax(), argList);
    MArgDatabase args(syntax(), argList, &status);
    if (!status)
        return status;

    MString ufePathString = args.commandArgumentString(0);
    if (ufePathString.length() == 0) {
        MGlobal::displayError("Missing argument 'UFE Path'.");
        throw MS::kFailure;
    }

    const auto                ufePath = Ufe::PathString::path(ufePathString.asChar());
    const Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(ufePath);
    if (!sceneItem) {
        MGlobal::displayError("Could not find SceneItem:" + ufePathString);
        throw MS::kFailure;
    }

    if (parser.isFlagSet(kHasMaterialBindingFlag)) {
        auto usdSceneItem = UsdUfe::downcast(sceneItem);
        if (!usdSceneItem) {
            MGlobal::displayError("Invalid SceneItem:" + ufePathString);
            throw MS::kFailure;
        }
        const auto prim = usdSceneItem->prim();
        bool       hasMaterialBindingAPI = prim.HasAPI<UsdShadeMaterialBindingAPI>();
        if (!hasMaterialBindingAPI) {
            setResult(false);
        } else {
            auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
            auto materialPath = bindingAPI.GetDirectBinding().GetMaterialPath();
            setResult(!materialPath.IsEmpty());
        }
    } else if (parser.isFlagSet(kCanAssignMaterialToNodeType)) {
        setResult(UsdUfe::canAssignMaterialToNodeType(sceneItem));
    }

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDMaterialBindingsCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addArg(MSyntax::kString);
    syntax.addFlag(kHasMaterialBindingFlag, kHasMaterialBindingFlagLong, MSyntax::kBoolean);
    syntax.addFlag(
        kCanAssignMaterialToNodeType, kCanAssignMaterialToNodeTypeLong, MSyntax::kBoolean);
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF