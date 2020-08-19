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

#include "layerEditorCommand.h"

#include <mayaUsd/utils/query.h>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

#include <pxr/base/tf/diagnostic.h>

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
const char kDiscardEditsFlag[] = "de";
const char kDiscardEditsFlagL[] = "discardEdits";
const char kClearLayerFlag[] = "cl";
const char kClearLayerFlagL[] = "clear";
const char kAddAnonSublayerFlag[] = "aa";
const char kAddAnonSublayerFlagL[] = "addAnonymous";

} // namespace

namespace MAYAUSD_NS {

namespace Impl {

enum class CmdId { kInsert, kRemove, kReplace, kDiscardEdit, kClearLayer, kAddAnonLayer };

class BaseCmd {
public:
    BaseCmd(CmdId id)
        : _cmdId(id)
    {
    }
    virtual ~BaseCmd() { }
    virtual bool doIt(SdfLayerHandle layer) = 0;
    virtual void undoIt(SdfLayerHandle layer) = 0;

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
    for (auto path : sublayers) { holdOnPathIfDirty(layer, path); }
}

class InsertRemoveSubPathBase : public BaseCmd {
public:
    int         _index = -1;
    std::string _subPath;

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
                if (_index < 0 || _index >= (int)layer->GetNumSubLayerPaths()) {
                    return false;
                }
            }
            layer->InsertSubLayerPath(_subPath, _index);
            TF_VERIFY(layer->GetSubLayerPaths()[_index] == _subPath);
        } else {
            if (_index < 0 || _index >= (int)layer->GetNumSubLayerPaths()) {
                return false;
            }
            _subPath = layer->GetSubLayerPaths()[_index];
            holdOnPathIfDirty(layer, _subPath);
            layer->RemoveSubLayerPath(_index);
        }
        return true;
    }
    void undoIt(SdfLayerHandle layer) override
    {
        if (_cmdId == CmdId::kInsert || _cmdId == CmdId::kAddAnonLayer) {
            auto index = _index;
            if (index == -1) {
                index = static_cast<int>(layer->GetNumSubLayerPaths() - 1);
            }
            TF_VERIFY(layer->GetSubLayerPaths()[index] == _subPath);
            layer->RemoveSubLayerPath(index);
        } else {
            TF_VERIFY(_index != -1);
            layer->InsertSubLayerPath(_subPath, _index);
        }
    }
};

class InsertSubPath : public InsertRemoveSubPathBase {
public:
    InsertSubPath()
        : InsertRemoveSubPathBase(CmdId::kInsert)
    {
    }
};

class RemoveSubPath : public InsertRemoveSubPathBase {
public:
    RemoveSubPath()
        : InsertRemoveSubPathBase(CmdId::kRemove)
    {
    }
};

class ReplaceSubPath : public BaseCmd {
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
            MGlobal::displayError(message.c_str());
            return false;
        }

        holdOnPathIfDirty(layer, _oldPath);
        proxy.Replace(_oldPath, _newPath);
        return true;
    }

    void undoIt(SdfLayerHandle layer) override
    {
        auto proxy = layer->GetSubLayerPaths();
        proxy.Replace(_newPath, _oldPath);
        releaseSubLayers();
        holdOnPathIfDirty(layer, _newPath);
    }

    std::string _oldPath, _newPath;
};

class AddAnonSubLayer : public InsertRemoveSubPathBase {
public:
    AddAnonSubLayer()
        : InsertRemoveSubPathBase(CmdId::kAddAnonLayer) {};

    bool doIt(SdfLayerHandle layer) override
    {
        _anonLayer = SdfLayer::CreateAnonymous(_anonName);
        _subPath = _anonLayer->GetIdentifier();
        _index = 0;
        _cmdResult = _subPath;
        return InsertRemoveSubPathBase::doIt(layer);
    }

    void undoIt(SdfLayerHandle layer) override
    {
        InsertRemoveSubPathBase::undoIt(layer);
        _anonLayer = nullptr;
    }

    std::string _anonName;

protected:
    PXR_NS::SdfLayerRefPtr _anonLayer;
};

class BackupLayerBase : public BaseCmd {
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
    void undoIt(SdfLayerHandle layer) override
    {
        if (_backupLayer == nullptr) {
            layer->Reload();
        } else {
            layer->TransferContent(_backupLayer);
            _backupLayer = nullptr;
            releaseSubLayers();
        }
    }

    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
};

class DiscardEdit : public BackupLayerBase {
public:
    DiscardEdit()
        : BackupLayerBase(CmdId::kDiscardEdit)
    {
    }
};

class ClearLayer : public BackupLayerBase {
public:
    ClearLayer()
        : BackupLayerBase(CmdId::kClearLayer)
    {
    }
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
    syntax.addFlag(kRemoveSubPathFlag, kRemoveSubPathFlagL, MSyntax::kLong);
    syntax.makeFlagMultiUse(kRemoveSubPathFlag);
    syntax.addFlag(kReplaceSubPathFlag, kReplaceSubPathFlagL, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(kReplaceSubPathFlag);
    syntax.addFlag(kDiscardEditsFlag, kDiscardEditsFlagL);
    syntax.addFlag(kClearLayerFlag, kClearLayerFlagL);
    syntax.addFlag(kAddAnonSublayerFlag, kAddAnonSublayerFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAddAnonSublayerFlag);

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
                auto cmd = std::make_shared<Impl::RemoveSubPath>();

                MArgList listOfArgs;
                argParser.getFlagArgumentList(kRemoveSubPathFlag, i, listOfArgs);
                cmd->_index = listOfArgs.asInt(0);
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
        (*it)->undoIt(layer); 
    }

    // clang-format on
    return MS::kSuccess;
}

} // namespace MAYAUSD_NS
