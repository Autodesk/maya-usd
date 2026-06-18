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

#include "layerEditorCommand.h"

#include <LayerEditorCommands.h>
#include <layerEditorDCCFunctions.h>

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/utils/layerLocking.h>
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>
#endif
#include <mayaUsd/utils/layerMuting.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoableItem.h>
#include <usdUfe/utils/uiCallback.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/usd/flattenUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdUtils/stitch.h>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>

#include <ghc/fs_std.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
const char kInsertSubPathFlag[] = "is";
const char kInsertSubPathFlagL[] = "insertSubPath";
const char kRemoveSubPathFlag[] = "rs";
const char kRemoveSubPathFlagL[] = "removeSubPath";
const char kReplaceSubPathFlag[] = "rp";
const char kReplaceSubPathFlagL[] = "replaceSubPath";
const char kMoveSubPathFlag[] = "mv";
const char kMoveSubPathFlagL[] = "moveSubPath";
const char kDiscardEditsFlag[] = "de";
const char kDiscardEditsFlagL[] = "discardEdits";
const char kClearLayerFlag[] = "cl";
const char kClearLayerFlagL[] = "clear";
const char kFlattenLayerFlag[] = "fl";
const char kFlattenLayerFlagL[] = "flatten";
const char kAddAnonSublayerFlag[] = "aa";
const char kAddAnonSublayerFlagL[] = "addAnonymous";
const char kMuteLayerFlag[] = "mt";
const char kMuteLayerFlagL[] = "muteLayer";
const char kLockLayerFlag[] = "lk";
const char kLockLayerFlagL[] = "lockLayer";
const char kSkipSystemLockedFlag[] = "ssl";
const char kSkipSystemLockedFlagL[] = "skipSystemLocked";
const char kRefreshSystemLockFlag[] = "rl";
const char kRefreshSystemLockFlagL[] = "refreshSystemLock";
const char kStitchLayersFlag[] = "sl";
const char kStitchLayersFlagL[] = "stitchLayers";

// We assume the indexes given to the command are the original indexes
// of the layers. Since each command is executed individually and in
// order, each one may affect the index of subsequent commands. We
// records adjustements that must be applied to indexes in the map.
// Removal of a layer creates a negative adjustment, insertion of a
// layer creates a positive adjustment.
class IndexAdjustments
{
public:
    IndexAdjustments() = default;

    // Convenience method that retrieve the adjusted index and adds
    // the insertion index adjustment.
    int insertionAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addInsertionAdjustment(originalIndex);
        return adjustedIndex;
    }

    // Convenience method that retrieve the adjusted index and adds
    // the removal index adjustment.
    int removalAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addRemovalAdjustment(originalIndex);
        return adjustedIndex;
    }

private:
    // Insertion and removal additional adjustment.
    // Must be called with the original index as provided by the user.
    void addInsertionAdjustment(int index) { _indexAdjustments[index] += 1; }
    void addRemovalAdjustment(int index) { _indexAdjustments[index] -= 1; }

    // Calculate the adjusted index from the user-supplied index that
    // need to be used by the command to account for previous commands.
    int getAdjustedIndex(int index) const
    {
        // Apply all adjustment that were done on indexes lower or
        // equal to the input index.
        int adjustedIndex = index;
        for (const auto& indexAndAdjustement : _indexAdjustments) {
            if (indexAndAdjustement.first > index)
                break;
            adjustedIndex += indexAndAdjustement.second;
        }
        return adjustedIndex;
    }

    std::map<int, int> _indexAdjustments;
};

} // namespace

namespace MAYAUSD_NS_DEF {

const char LayerEditorCommand::commandName[] = "mayaUsdLayerEditor";

// plug-in callback to create the command object
void* LayerEditorCommand::creator() { return static_cast<MPxCommand*>(new LayerEditorCommand()); }

// plug-in callback to register the command syntax
MSyntax LayerEditorCommand::createSyntax()
{
    MSyntax syntax;

    // syntax.enableQuery(true);
    syntax.enableEdit(true);

    // layer id
    syntax.setObjectType(MSyntax::kStringObjects, 1, 1);

    syntax.addFlag(kInsertSubPathFlag, kInsertSubPathFlagL, MSyntax::kLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kInsertSubPathFlag);
    syntax.addFlag(kRemoveSubPathFlag, kRemoveSubPathFlagL, MSyntax::kLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kRemoveSubPathFlag);
    syntax.addFlag(kReplaceSubPathFlag, kReplaceSubPathFlagL, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(kReplaceSubPathFlag);
    syntax.addFlag(
        kMoveSubPathFlag,
        kMoveSubPathFlagL,
        MSyntax::kString,    // path to move
        MSyntax::kString,    // new parent layer
        MSyntax::kUnsigned); // layer index inside the new parent
    syntax.addFlag(kDiscardEditsFlag, kDiscardEditsFlagL);
    syntax.addFlag(kClearLayerFlag, kClearLayerFlagL);
    syntax.addFlag(kFlattenLayerFlag, kFlattenLayerFlagL);
    // parameter: new layer name
    syntax.addFlag(kAddAnonSublayerFlag, kAddAnonSublayerFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAddAnonSublayerFlag);
    // parameter: proxy shape name
    syntax.addFlag(kMuteLayerFlag, kMuteLayerFlagL, MSyntax::kBoolean, MSyntax::kString);
    syntax.addFlag(
        kLockLayerFlag, kLockLayerFlagL, MSyntax::kLong, MSyntax::kBoolean, MSyntax::kString);
    // parameter 1: proxy shape name
    // parameter 2: refresh sub layers
    syntax.addFlag(
        kRefreshSystemLockFlag, kRefreshSystemLockFlagL, MSyntax::kString, MSyntax::kBoolean);
    syntax.addFlag(kSkipSystemLockedFlag, kSkipSystemLockedFlagL);
    // parameter 1: proxy shape name
    // parameter 2: layer identifier
    syntax.addFlag(kStitchLayersFlag, kStitchLayersFlagL, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(kStitchLayersFlag);

    return syntax;
}

// private argument parsing helper
MStatus LayerEditorCommand::parseArgs(const MArgList& argList)
{
    setCommandString(commandName);

    MStatus    status;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess) {
        return MS::kInvalidParameter;
    }
    if (argParser.isQuery()) {
        _cmdMode = Mode::Query;
    } else if (argParser.isEdit()) {
        _cmdMode = Mode::Edit;
    } else {
        _cmdMode = Mode::Create;
    }

    MStringArray objects;
    argParser.getObjects(objects);
    _layerIdentifier = objects[0].asChar();

    if (!isQuery()) {

        auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
        if (!layer) {
            displayError(MString("Layer not found: ") + _layerIdentifier.c_str());
            return MS::kInvalidParameter;
        }

        IndexAdjustments indexAdjustments;

        const bool skipSystemLockedLayers = argParser.isFlagSet(kSkipSystemLockedFlag);

        if (argParser.isFlagSet(kInsertSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kInsertSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kInsertSubPathFlag, i, listOfArgs);
                const int originalIndex = listOfArgs.asInt(0);
                const int adjustedIndex = indexAdjustments.insertionAdjustment(originalIndex);
                _subCommands.push_back(std::make_shared<UsdLayerEditor::InsertSubPathCmd>(
                    UsdStageRefPtr {}, layer, listOfArgs.asString(1).asUTF8(), adjustedIndex));
            }
        }

        if (argParser.isFlagSet(kRemoveSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kRemoveSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kRemoveSubPathFlag, i, listOfArgs);
                auto shapePath = listOfArgs.asString(1);
                auto prim = UsdMayaQuery::GetPrim(shapePath.asChar());
                if (prim == UsdPrim()) {
                    displayError(MString("Invalid proxy shape \"") + shapePath.asChar() + "\"");
                    return MS::kInvalidParameter;
                }
                UsdStageRefPtr stage      = prim.GetStage();
                const int      originalIndex = listOfArgs.asInt(0);
                const int      adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);
                _subCommands.push_back(
                    std::make_shared<UsdLayerEditor::RemoveSubPathCmd>(stage, layer, adjustedIndex));
            }
        }

        if (argParser.isFlagSet(kReplaceSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kReplaceSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kReplaceSubPathFlag, i, listOfArgs);
                _subCommands.push_back(std::make_shared<UsdLayerEditor::ReplaceSubPathCmd>(
                    layer, listOfArgs.asString(0).asUTF8(), listOfArgs.asString(1).asUTF8()));
            }
        }

        if (argParser.isFlagSet(kMoveSubPathFlag)) {
            MString subPath;
            argParser.getFlagArgument(kMoveSubPathFlag, 0, subPath);
            MString newParentLayerStr;
            argParser.getFlagArgument(kMoveSubPathFlag, 1, newParentLayerStr);
            int originalIndex { 0 };
            argParser.getFlagArgument(kMoveSubPathFlag, 2, originalIndex);
            const int adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);

            SdfLayerHandle newParentLayerH;
            if (layer->GetIdentifier() == newParentLayerStr.asUTF8()) {
                newParentLayerH = layer;
            } else {
                newParentLayerH = SdfLayer::Find(newParentLayerStr.asUTF8());
                if (!newParentLayerH) {
                    displayError(MString("Layer not found: ") + newParentLayerStr);
                    return MS::kInvalidParameter;
                }
            }
            _subCommands.push_back(std::make_shared<UsdLayerEditor::MoveSubPathCmd>(
                layer, newParentLayerH, subPath.asUTF8(), adjustedIndex));
        }

        if (argParser.isFlagSet(kDiscardEditsFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::DiscardEditCmd>(layer));
        }

        if (argParser.isFlagSet(kClearLayerFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::ClearLayerCmd>(layer));
        }

        if (argParser.isFlagSet(kFlattenLayerFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::FlattenLayerCmd>(layer));
        }

        if (argParser.isFlagSet(kAddAnonSublayerFlag)) {
            auto count = argParser.numberOfFlagUses(kAddAnonSublayerFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kAddAnonSublayerFlag, i, listOfArgs);
                auto cmd = std::make_shared<UsdLayerEditor::AddAnonSubLayerCmd>(
                    UsdStageRefPtr {}, layer);
                cmd->_anonName = listOfArgs.asString(0).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kMuteLayerFlag)) {
            bool muteIt = true;
            argParser.getFlagArgument(kMuteLayerFlag, 0, muteIt);
            MString proxyShapeName;
            argParser.getFlagArgument(kMuteLayerFlag, 1, proxyShapeName);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            _subCommands.push_back(
                std::make_shared<UsdLayerEditor::MuteLayerCmd>(stage, layer, muteIt));
        }

        if (argParser.isFlagSet(kLockLayerFlag)) {
            int lockValue = 0;
            // 0 = Unlocked
            // 1 = Locked
            // 2 = SystemLocked
            argParser.getFlagArgument(kLockLayerFlag, 0, lockValue);
            bool includeSublayers = false;
            argParser.getFlagArgument(kLockLayerFlag, 1, includeSublayers);
            MString proxyShapeName;
            argParser.getFlagArgument(kLockLayerFlag, 2, proxyShapeName);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr                stage = prim.GetStage();
            UsdLayerEditor::LayerLockType lockType;
            switch (lockValue) {
            case 1:  lockType = UsdLayerEditor::LayerLock_Locked;       break;
            case 2:  lockType = UsdLayerEditor::LayerLock_SystemLocked; break;
            default: lockType = UsdLayerEditor::LayerLock_Unlocked;     break;
            }
            _subCommands.push_back(std::make_shared<UsdLayerEditor::LockLayerCmd>(
                stage, layer, lockType, includeSublayers, skipSystemLockedLayers));
        }

        if (argParser.isFlagSet(kRefreshSystemLockFlag)) {
            MString proxyShapeName;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 0, proxyShapeName);
            bool refreshSubLayers = true;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 1, refreshSubLayers);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            auto cmd = std::make_shared<UsdLayerEditor::RefreshSystemLockLayerCmd>(
                stage, layer, refreshSubLayers);
            cmd->addCallbackContext(
                "proxyShapePath", PXR_NS::VtValue(std::string(proxyShapeName.asChar())));
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kStitchLayersFlag)) {
            std::vector<std::string> layerIdentifiers;
            const auto               layerCount = argParser.numberOfFlagUses(kStitchLayersFlag);
            MString                  proxyShapeName;
            for (unsigned i = 0; i < layerCount; ++i) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kStitchLayersFlag, i, listOfArgs);
                if (i == 0)
                    proxyShapeName = listOfArgs.asString(0);
                layerIdentifiers.push_back(listOfArgs.asString(1).asChar());
            }
            const UsdPrim prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            _subCommands.push_back(
                std::make_shared<UsdLayerEditor::StitchLayersCmd>(stage, layerIdentifiers));
        }

    }

    return MS::kSuccess;
}

// MPxCommand undo ability callback
bool LayerEditorCommand::isUndoable() const { return !isQuery(); }

// main MPxCommand execution point
MStatus LayerEditorCommand::doIt(const MArgList& argList)
{
    MStatus status(MS::kSuccess);
    clearResult();

    status = parseArgs(argList);
    if (status != MS::kSuccess)
        return status;

    return redoIt();
}

// main MPxCommand execution point
MStatus LayerEditorCommand::redoIt()
{
    for (auto& cmd : _subCommands) {
        cmd->redo();
        // AddAnonSubLayerCmd is the only command that returns a result
        // (the new anonymous layer identifier).
        if (auto* anon = dynamic_cast<UsdLayerEditor::AddAnonSubLayerCmd*>(cmd.get()))
            appendToResult(anon->addedLayer().c_str());
    }
    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus LayerEditorCommand::undoIt()
{
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it)
        (*it)->undo();
    return MS::kSuccess;
}

void LayerEditorCommand::registerBackupStagesProvider()
{
    UsdLayerEditor::BackupLayerBaseCmd::setStagesProvider(
        []() { return MayaUsd::ufe::ProxyShapeHandler::getAllStages(); });
}

void LayerEditorCommand::unregisterBackupStagesProvider()
{
    UsdLayerEditor::BackupLayerBaseCmd::setStagesProvider(nullptr);
}

} // namespace MAYAUSD_NS_DEF
