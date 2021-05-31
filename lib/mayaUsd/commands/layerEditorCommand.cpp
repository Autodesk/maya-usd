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

#include <mayaUsd/utils/query.h>

#include <pxr/base/tf/diagnostic.h>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#endif

#include <cstddef>
#include <string>

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
const char kAddAnonSublayerFlag[] = "aa";
const char kAddAnonSublayerFlagL[] = "addAnonymous";
const char kMuteLayerFlag[] = "mt";
const char kMuteLayerFlagL[] = "muteLayer";

} // namespace

namespace MAYAUSD_NS_DEF {

namespace Impl {

enum class CmdId
{
    kInsert,
    kRemove,
    kMove,
    kReplace,
    kDiscardEdit,
    kClearLayer,
    kAddAnonLayer,
    kMuteLayer
};

class BaseCmd
{
public:
    BaseCmd(CmdId id)
        : _cmdId(id)
    {
    }
    virtual ~BaseCmd() { }
    virtual bool doIt(SdfLayerHandle layer) = 0;
    virtual bool undoIt(SdfLayerHandle layer) = 0;

    std::string _cmdResult; // set if the command returns something

protected:
    CmdId _cmdId;
    // we need to hold on to dirty sublayers if we remove them
    std::vector<PXR_NS::SdfLayerRefPtr> _subLayersRefs;
    void                                holdOntoSubLayers(SdfLayerHandle layer);
    void                                releaseSubLayers() { _subLayersRefs.clear(); }
    void                                holdOnPathIfDirty(SdfLayerHandle layer, std::string path);
};

void BaseCmd::holdOnPathIfDirty(SdfLayerHandle layer, std::string path)
{
    auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, path);
    if (subLayerHandle != nullptr) {
        if (subLayerHandle->IsDirty() || subLayerHandle->IsAnonymous()) {
            _subLayersRefs.push_back(subLayerHandle);
            holdOntoSubLayers(subLayerHandle); // we'll need to hold onto children as well
        }
    }
}

// hold references to any anon or dirty sublayer
void BaseCmd::holdOntoSubLayers(SdfLayerHandle layer)
{
    const std::vector<std::string>& sublayers = layer->GetSubLayerPaths();
    for (auto path : sublayers) {
        holdOnPathIfDirty(layer, path);
    }
}

class InsertRemoveSubPathBase : public BaseCmd
{
public:
    int         _index = -1;
    std::string _subPath;
    std::string _proxyShapePath;

    InsertRemoveSubPathBase(CmdId id)
        : BaseCmd(id)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        if (_cmdId == CmdId::kInsert || _cmdId == CmdId::kAddAnonLayer) {
            if (_index == -1) {
                _index = (int)layer->GetNumSubLayerPaths();
            }
            if (_index != 0) {
                if (!validateAndReportIndex(layer, _index, (int)layer->GetNumSubLayerPaths() + 1)) {
                    return false;
                }
            }

            // According to USD codebase, we should always call SdfLayer::InsertSubLayerPath()
            // with a layer's identifier. So, if the layer exists, override _subPath with the
            // identifier in case this command was called with a filesystem path. Otherwise,
            // adding the layer with the filesystem path can cause issue when muting the layer
            // on Windows if the path is absolute and start with a capital drive letter.
            //
            // Note: It's possible that SdfLayer::FindOrOpen() fail because we
            //       allow user to add layer that does not exists.
            auto layerToAdd = SdfLayer::FindOrOpen(_subPath);
            if (layerToAdd) {
                _subPath = layerToAdd->GetIdentifier();
            }

            layer->InsertSubLayerPath(_subPath, _index);
            TF_VERIFY(layer->GetSubLayerPaths()[_index] == _subPath);
        } else {
            TF_VERIFY(_cmdId == CmdId::kRemove);
            if (!validateAndReportIndex(layer, _index, (int)layer->GetNumSubLayerPaths())) {
                return false;
            }
            _subPath = layer->GetSubLayerPaths()[_index];
            holdOnPathIfDirty(layer, _subPath);

            // if the current edit target is the layer to remove or
            // a sublayer of the layer to remove,
            // set the root layer as the current edit target
            auto layerToRemove = SdfLayer::FindRelativeToLayer(layer, _subPath);
            auto stage = getStage();
            auto currentTarget = stage->GetEditTarget().GetLayer();

            // Helper function to find if a layer is in the
            // hierarchy of another layer
            //
            // rootLayer: The root layer of the hierarchy
            // layer: The layer to find
            // ignore : Optional layer used has the root of a hierarchy that
            //          we don't want to check in.
            // ignoreSubPath : Optional subpath used whith ignore layer.
            auto isInHierarchy = [](const SdfLayerHandle& rootLayer,
                                    const SdfLayerHandle& layer,
                                    const SdfLayerHandle* ignore = nullptr,
                                    const std::string*    ignoreSubPath = nullptr) {
                // Impl used for recursive call
                auto isInHierarchyImpl = [](const SdfLayerHandle& rootLayer,
                                            const SdfLayerHandle& layer,
                                            const SdfLayerHandle* ignore,
                                            const std::string*    ignoreSubPath,
                                            auto&                 implRef) {
                    if (!rootLayer || !layer)
                        return false;

                    if (rootLayer->GetIdentifier() == layer->GetIdentifier())
                        return true;

                    const auto subLayerPaths = rootLayer->GetSubLayerPaths();
                    for (const auto& subLayerPath : subLayerPaths) {

                        if (ignore && ignoreSubPath
                            && (*ignore)->GetIdentifier() == rootLayer->GetIdentifier()
                            && *ignoreSubPath == subLayerPath)
                            continue;

                        const auto subLayer
                            = SdfLayer::FindRelativeToLayer(rootLayer, subLayerPath);
                        if (implRef(subLayer, layer, ignore, ignoreSubPath, implRef))
                            return true;
                    }
                    return false;
                };
                return isInHierarchyImpl(
                    rootLayer, layer, ignore, ignoreSubPath, isInHierarchyImpl);
            };

            if (isInHierarchy(layerToRemove, currentTarget)) {
                // The current edit layer is in the hierarchy of the layer to remove,
                // now we need to be sure the edit target layer is not also a sublayer
                // of another layer in the stage.
                if (!isInHierarchy(stage->GetRootLayer(), currentTarget, &layer, &_subPath)) {
                    _editTargetPath = currentTarget->GetIdentifier();
                    stage->SetEditTarget(stage->GetRootLayer());
                }
            }

            layer->RemoveSubLayerPath(_index);
        }
        return true;
    }
    bool undoIt(SdfLayerHandle layer) override
    {
        if (_cmdId == CmdId::kInsert || _cmdId == CmdId::kAddAnonLayer) {
            auto index = _index;
            if (index == -1) {
                index = static_cast<int>(layer->GetNumSubLayerPaths() - 1);
            }
            if (validateUndoIndex(layer, _index)) {
                TF_VERIFY(layer->GetSubLayerPaths()[index] == _subPath);
                layer->RemoveSubLayerPath(index);
            } else {
                return false;
            }
        } else {
            TF_VERIFY(_index != -1);
            if (validateUndoIndex(layer, _index)) {
                layer->InsertSubLayerPath(_subPath, _index);

                // if the removed layer was the edit target,
                // set it back to the current edit target
                if (!_editTargetPath.empty()) {
                    auto stage = getStage();
                    auto subLayerHandle = SdfLayer::FindRelativeToLayer(layer, _editTargetPath);
                    stage->SetEditTarget(subLayerHandle);
                }
            } else {
                return false;
            }
        }
        return true;
    }
    static bool validateUndoIndex(SdfLayerHandle layer, int index)
    { // allow re-inserting at the last index + 1, but -1 should have been changed to 0
        return !(index < 0 || index > (int)layer->GetNumSubLayerPaths());
    }

    static bool validateAndReportIndex(SdfLayerHandle layer, int index, int maxIndex)
    {
        if (index < 0 || index >= maxIndex) {
            std::string message = std::string("Index ") + std::to_string(index)
                + std::string(" out-of-bound for ") + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        } else {
            return true;
        }
    }

protected:
    std::string _editTargetPath;

    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }
};

class InsertSubPath : public InsertRemoveSubPathBase
{
public:
    InsertSubPath()
        : InsertRemoveSubPathBase(CmdId::kInsert)
    {
    }
};

class RemoveSubPath : public InsertRemoveSubPathBase
{
public:
    RemoveSubPath()
        : InsertRemoveSubPathBase(CmdId::kRemove)
    {
    }
};

// Move a sublayer into another layer.
class MoveSubPath : public BaseCmd
{
public:
    MoveSubPath()
        : BaseCmd(CmdId::kMove)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        auto subPathIndex = proxy.Find(_path);
        if (subPathIndex == size_t(-1)) {
            std::string message = std::string("path ") + _path + std::string(" not found on layer ")
                + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        }

        _oldIndex = subPathIndex; // save for undo

        SdfLayerHandle newParentLayer;

        if (layer->GetIdentifier() == _newParentLayer) {

            if (_newIndex > layer->GetNumSubLayerPaths() - 1) {
                std::string message = std::string("Index ") + std::to_string(_newIndex)
                    + std::string(" out-of-bound for ") + layer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }

            newParentLayer = layer;
        } else {
            newParentLayer = SdfLayer::Find(_newParentLayer);
            if (!newParentLayer) {
                std::string message
                    = std::string("Layer ") + _newParentLayer + std::string(" not found!");
                return false;
            }

            if (_newIndex > newParentLayer->GetNumSubLayerPaths()) {
                std::string message = std::string("Index ") + std::to_string(_newIndex)
                    + std::string(" out-of-bound for ") + newParentLayer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }

            // make sure the subpath is not already in the new parent layer.
            // Otherwise, the SdfLayer::InsertSubLayerPath() below will do nothing
            // and the subpath will be removed from it's current parent.
            if (newParentLayer->GetSubLayerPaths().Find(_path) != size_t(-1)) {
                std::string message = std::string("SubPath ") + _path
                    + std::string(" already exist in layer ") + newParentLayer->GetIdentifier();
                MPxCommand::displayError(message.c_str());
                return false;
            }
        }

        // When the subLayer is moved inside the current parent,
        // Remove it from it's current location and insert it into it's
        // new location. The order of remove / insert is important
        // oterwise InsertSubLayerPath() will fail because the subLayer
        // already exists.
        layer->RemoveSubLayerPath(subPathIndex);
        newParentLayer->InsertSubLayerPath(_path, _newIndex);

        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        if (layer->GetIdentifier() == _newParentLayer) {
            // When the subLayer is moved inside the current parent,
            // Remove it from it's current location and insert it into it's
            // new location. The order of remove / insert is important
            // oterwise InsertSubLayerPath() will fail because the subLayer
            // already exists.
            layer->RemoveSubLayerPath(_newIndex);
            layer->InsertSubLayerPath(_path, _oldIndex);
        } else {
            auto newParentLayer = SdfLayer::Find(_newParentLayer);
            newParentLayer->RemoveSubLayerPath(_newIndex);
            layer->InsertSubLayerPath(_path, _oldIndex);
        }

        return true;
    }

    std::string  _path;
    std::string  _newParentLayer;
    unsigned int _newIndex;

private:
    unsigned int _oldIndex { 0 };
};

class ReplaceSubPath : public BaseCmd
{
public:
    ReplaceSubPath()
        : BaseCmd(CmdId::kReplace)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        if (proxy.Find(_oldPath) == size_t(-1)) {
            std::string message = std::string("path ") + _oldPath
                + std::string(" not found on layer ") + layer->GetIdentifier();
            MPxCommand::displayError(message.c_str());
            return false;
        }

        holdOnPathIfDirty(layer, _oldPath);
        proxy.Replace(_oldPath, _newPath);
        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        proxy.Replace(_newPath, _oldPath);
        releaseSubLayers();
        holdOnPathIfDirty(layer, _newPath);
        return true;
    }

    std::string _oldPath, _newPath;
};

class AddAnonSubLayer : public InsertRemoveSubPathBase
{
public:
    AddAnonSubLayer()
        : InsertRemoveSubPathBase(CmdId::kAddAnonLayer) {};

    bool doIt(SdfLayerHandle layer) override
    {
        // the first time, USD will create a layer with a certain identifier
        // on undo(), we will remove the path, but hold onto the layer
        // on redo, we want to put back that same identifier, for later commands
        if (_anonIdentifier.empty()) {
            _anonLayer = SdfLayer::CreateAnonymous(_anonName);
            _anonIdentifier = _anonLayer->GetIdentifier();
        }
        _subPath = _anonIdentifier;
        _index = 0;
        _cmdResult = _subPath;
        return InsertRemoveSubPathBase::doIt(layer);
    }

    bool undoIt(SdfLayerHandle layer) override { return InsertRemoveSubPathBase::undoIt(layer); }

    std::string _anonName;

protected:
    PXR_NS::SdfLayerRefPtr _anonLayer;
    std::string            _anonIdentifier;
};

class BackupLayerBase : public BaseCmd
{
    // commands that need to backup the whole layer for undo
public:
    BackupLayerBase(CmdId id)
        : BaseCmd(id)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        // using reload will correctly reset the dirty bit
        if (layer->IsDirty()) {
            _backupLayer = SdfLayer::CreateAnonymous();
            _backupLayer->TransferContent(layer);
        }
        holdOntoSubLayers(layer);

        if (_cmdId == CmdId::kDiscardEdit) {
            layer->Reload();
        } else if (_cmdId == CmdId::kClearLayer) {
            layer->Clear();
        }
        return true;
    }
    bool undoIt(SdfLayerHandle layer) override
    {
        if (_backupLayer == nullptr) {
            layer->Reload();
        } else {
            layer->TransferContent(_backupLayer);
            _backupLayer = nullptr;
            releaseSubLayers();
        }
        return true;
    }

    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
};

class DiscardEdit : public BackupLayerBase
{
public:
    DiscardEdit()
        : BackupLayerBase(CmdId::kDiscardEdit)
    {
    }
};

class ClearLayer : public BackupLayerBase
{
public:
    ClearLayer()
        : BackupLayerBase(CmdId::kClearLayer)
    {
    }
};

class MuteLayer : public BaseCmd
{
public:
    MuteLayer()
        : BaseCmd(CmdId::kMuteLayer)
    {
    }

    bool doIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;
        if (_muteIt) {
            // Muting a layer will cause all scene items under the proxy shape
            // to be stale.
            saveSelection();
            stage->MuteLayer(layer->GetIdentifier());
        } else {
            stage->UnmuteLayer(layer->GetIdentifier());
            restoreSelection();
        }

        // we perfer not holding to pointers needlessly, but we need to hold on to the layer if we
        // mute it otherwise usd will let go of it and its modifications, and any dirty children
        // will also be lost
        _mutedLayer = layer;
        return true;
    }

    bool undoIt(SdfLayerHandle layer) override
    {
        auto stage = getStage();
        if (!stage)
            return false;
        if (_muteIt) {
            stage->UnmuteLayer(layer->GetIdentifier());
            restoreSelection();
        } else {
            // Muting a layer will cause all scene items under the proxy shape
            // to be stale.
            saveSelection();
            stage->MuteLayer(layer->GetIdentifier());
        }
        // we can release the pointer
        _mutedLayer = nullptr;
        return true;
    }

    std::string _proxyShapePath;
    bool        _muteIt = true;

private:
    UsdStageWeakPtr getStage()
    {
        auto prim = UsdMayaQuery::GetPrim(_proxyShapePath.c_str());
        auto stage = prim.GetStage();
        return stage;
    }

    void saveSelection()
    {
#if defined(WANT_UFE_BUILD)
        // Make a copy of the global selection, to restore it on unmute.
        auto globalSn = Ufe::GlobalSelection::get();
        _savedSn.replaceWith(*globalSn);
        // Filter the global selection, removing items below our proxy shape.
        // We know the path to the proxy shape has a single segment.  Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        globalSn->replaceWith(MayaUsd::ufe::removeDescendants(_savedSn, path));
#endif
    }

    void restoreSelection()
    {
#if defined(WANT_UFE_BUILD)
        // Restore the saved selection to the global selection.  If a saved
        // selection item started with the proxy shape path, re-create it.
        // We know the path to the proxy shape has a single segment.  Not
        // using Ufe::PathString::path() for UFE v1 compatibility, which
        // unfortunately reveals leading "world" path component implementation
        // detail.
        Ufe::Path path(
            Ufe::PathSegment("world" + _proxyShapePath, MayaUsd::ufe::getMayaRunTimeId(), '|'));
        auto globalSn = Ufe::GlobalSelection::get();
        globalSn->replaceWith(MayaUsd::ufe::recreateDescendants(_savedSn, path));
#endif
    }

#if defined(WANT_UFE_BUILD)
    Ufe::Selection _savedSn;
#endif
    PXR_NS::SdfLayerRefPtr _mutedLayer;
};

} // namespace Impl

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
    // parameter: new layer name
    syntax.addFlag(kAddAnonSublayerFlag, kAddAnonSublayerFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAddAnonSublayerFlag);
    // paramter: proxy shape name
    syntax.addFlag(kMuteLayerFlag, kMuteLayerFlagL, MSyntax::kBoolean, MSyntax::kString);

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
        if (argParser.isFlagSet(kInsertSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kInsertSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::InsertSubPath>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kInsertSubPathFlag, i, listOfArgs);
                cmd->_index = listOfArgs.asInt(0);
                cmd->_subPath = listOfArgs.asString(1).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kRemoveSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kRemoveSubPathFlag);
            for (unsigned i = 0; i < count; i++) {

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kRemoveSubPathFlag, i, listOfArgs);

                auto index = listOfArgs.asInt(0);
                auto shapePath = listOfArgs.asString(1);
                auto prim = UsdMayaQuery::GetPrim(shapePath.asChar());
                if (prim == UsdPrim()) {
                    displayError(MString("Invalid proxy shape \"") + shapePath.asChar() + "\"");
                    return MS::kInvalidParameter;
                }

                auto cmd = std::make_shared<Impl::RemoveSubPath>();
                cmd->_index = index;
                cmd->_proxyShapePath = shapePath.asChar();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kReplaceSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kReplaceSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::ReplaceSubPath>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kReplaceSubPathFlag, i, listOfArgs);
                cmd->_oldPath = listOfArgs.asString(0).asUTF8();
                cmd->_newPath = listOfArgs.asString(1).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kMoveSubPathFlag)) {
            auto cmd = std::make_shared<Impl::MoveSubPath>();

            MString subPath;
            argParser.getFlagArgument(kMoveSubPathFlag, 0, subPath);

            MString newParentLayer;
            argParser.getFlagArgument(kMoveSubPathFlag, 1, newParentLayer);

            unsigned int index {0};
            argParser.getFlagArgument(kMoveSubPathFlag, 2, index);

            cmd->_path = subPath.asUTF8();
            cmd->_newParentLayer = newParentLayer.asUTF8();
            cmd->_newIndex = index;
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kDiscardEditsFlag)) {
            auto cmd = std::make_shared<Impl::DiscardEdit>();
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kClearLayerFlag)) {
            auto cmd = std::make_shared<Impl::ClearLayer>();
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kAddAnonSublayerFlag)) {
            auto count = argParser.numberOfFlagUses(kAddAnonSublayerFlag);
            for (unsigned i = 0; i < count; i++) {
                auto cmd = std::make_shared<Impl::AddAnonSubLayer>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kAddAnonSublayerFlag, i, listOfArgs);
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
                    MString("Invalid proxy shape \"") + MString(proxyShapeName.asChar()) + "\"");
                return MS::kInvalidParameter;
            }

            auto cmd = std::make_shared<Impl::MuteLayer>();
            cmd->_muteIt = muteIt;
            cmd->_proxyShapePath = proxyShapeName.asChar();
            _subCommands.push_back(std::move(cmd));
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

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    for (auto it = _subCommands.begin(); it != _subCommands.end(); ++it) {
        if (!(*it)->doIt(layer)) {
            return MS::kFailure;
        }
        const auto& result = (*it)->_cmdResult;
        if (!result.empty()) {
            appendToResult(result.c_str());
        }
    }

    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus LayerEditorCommand::undoIt()
{

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    // clang-format off
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it) { 
        if (!(*it)->undoIt(layer)) {
            return MS::kFailure;
        }
    }

    // clang-format on
    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
