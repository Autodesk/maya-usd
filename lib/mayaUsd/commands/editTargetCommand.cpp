//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//ı
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "editTargetCommand.h"

#include <mayaUsd/utils/query.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
const char kTargetFlag[] = "et";
const char kTargetFlagL[] = "editTarget";

void reportError(const MString& errorString) { MGlobal::displayError(errorString); }

} // namespace

namespace MAYAUSD_NS_DEF {

namespace Impl {
class SetEditTarget
{
public:
    bool doIt(UsdStagePtr stage)
    {
        auto layerHandle = SdfLayer::Find(_newTarget);
        if (!layerHandle) {
            return false;
        }

        auto currentTarget = stage->GetEditTarget().GetLayer();
        if (currentTarget) {
            _oldTarget = currentTarget->GetIdentifier();
        }

        stage->SetEditTarget(layerHandle);
        return true;
    }
    void undoIt(UsdStagePtr stage)
    {
        SdfLayerHandle layerToSet = stage->GetRootLayer();
        if (!_oldTarget.empty()) {
            auto oldLayer = SdfLayer::Find(_oldTarget);
            if (oldLayer) {
                layerToSet = oldLayer;
            }
        }
        stage->SetEditTarget(layerToSet);
    }

    std::string _newTarget;
    std::string _oldTarget;
};
} // namespace Impl

const char EditTargetCommand::commandName[] = "mayaUsdEditTarget";

// plug-in callback to create the command object
void* EditTargetCommand::creator() { return static_cast<MPxCommand*>(new EditTargetCommand()); }

// plug-in callback to register the command syntax
MSyntax EditTargetCommand::createSyntax()
{
    MSyntax syntax;

    syntax.enableQuery(true);
    syntax.enableEdit(true);

    // proxy shape name
    syntax.setObjectType(MSyntax::kStringObjects, 1, 1);

    syntax.addFlag(kTargetFlag, kTargetFlagL, MSyntax::kString);

    return syntax;
}

// private argument parsing helper
MStatus EditTargetCommand::parseArgs(const MArgList& argList)
{
    setCommandString(commandName);

    MStatus    status;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess) {
        return MS::kInvalidParameter;
    }
    if (argParser.isQuery()) {
        _cmdMode = Mode::kQuery;
    } else if (argParser.isEdit()) {
        _cmdMode = Mode::kEdit;
    } else {
        _cmdMode = Mode::kCreate;
    }

    MStringArray objects;
    argParser.getObjects(objects);
    _proxyShapePath = objects[0];

    auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.asChar());
    if (prim == UsdPrim()) {
        reportError(MString("Invalid proxy shape \"") + MString(_proxyShapePath.asChar()) + "\"");
        return MS::kInvalidParameter;
    }

    if (isQuery()) {
        MStringArray results;
        if (argParser.isFlagSet(kTargetFlag)) {
            auto stage = prim.GetStage();
            auto target = stage->GetEditTarget();
            auto layer = target.GetLayer();
            layer->GetIdentifier();
            results.append(layer->GetIdentifier().c_str());
        }

        setResult(results);
    } else {
        if (argParser.isFlagSet(kTargetFlag)) {
            _setEditTarget = std::make_unique<Impl::SetEditTarget>();
            _setEditTarget->_newTarget = argParser.flagArgumentString(kTargetFlag, 0).asChar();
        }
    }

    return MS::kSuccess;
}

// MPxCommand undo ability callback
bool EditTargetCommand::isUndoable() const { return !isQuery(); }

// main MPxCommand execution point
MStatus EditTargetCommand::doIt(const MArgList& argList)
{
    MStatus status(MS::kSuccess);
    clearResult();

    status = parseArgs(argList);
    if (status != MS::kSuccess)
        return status;

    return redoIt();
}

// main MPxCommand execution point
MStatus EditTargetCommand::redoIt()
{
    auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.asChar());
    auto stage = prim.GetStage();
    if (!stage) {
        return MS::kInvalidParameter;
    }

    if (_setEditTarget) {
        if (!_setEditTarget->doIt(stage))
            return MS::kFailure;
    }

    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus EditTargetCommand::undoIt()
{
    auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.asChar());
    auto stage = prim.GetStage();
    if (!stage) {
        return MS::kInvalidParameter;
    }

    if (_setEditTarget) {
        _setEditTarget->undoIt(stage);
    }

    return MS::kSuccess;
}

} //  namespace MAYAUSD_NS_DEF
