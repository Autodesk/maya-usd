//
// Copyright 2021 Autodesk
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
#include "OpUndoItems.h"

#include <mayaUsd/utils/util.h>

#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// NodeDeletionUndoItem
//------------------------------------------------------------------------------

namespace {

MStringArray getDagName(const MObject& node)
{
    MStringArray strings;

    MSelectionList sel;
    sel.add(node);
    sel.getSelectionStrings(strings);

    return strings;
}

MString formatCommand(const MString& commandName, const MObject& commandArg)
{
    MStringArray arg = getDagName(commandArg);

    MString cmd;
    cmd.format("^1s \"^2s\"", commandName.asChar(), arg[0].asChar());

    return cmd;
}

} // namespace

MStatus NodeDeletionUndoItem::deleteNode(
    const std::string name,
    const MString&    nodeName,
    const MObject&    node,
    OpUndoItemList&       undoInfo)
{
    // Avoid deleting the same node twice.
    if (undoInfo.isDeleted(node))
        return MS::kSuccess;

    const MString cmd = formatCommand("delete", node);

    std::string fullName
        = name + std::string(" \"") + std::string(cmd.asChar()) + std::string("\"");
    auto item = std::make_unique<NodeDeletionUndoItem>(std::move(fullName));

    MStatus status = item->_modifier.commandToExecute(cmd);
    if (status != MS::kSuccess)
        return status;

    status = item->_modifier.doIt();
    if (status != MS::kSuccess)
        return status;

    undoInfo.addItem(std::move(item));
    undoInfo.addDeleted(node);

    return MS::kSuccess;
}

NodeDeletionUndoItem::~NodeDeletionUndoItem() { }

bool NodeDeletionUndoItem::undo() { return _modifier.undoIt() == MS::kSuccess; }

bool NodeDeletionUndoItem::redo() { return _modifier.doIt() == MS::kSuccess; }

//------------------------------------------------------------------------------
// MDagModifierUndoItem
//------------------------------------------------------------------------------

MDagModifier& MDagModifierUndoItem::create(const std::string name, OpUndoItemList& undoInfo)
{
    auto          item = std::make_unique<MDagModifierUndoItem>(std::move(name));
    MDagModifier& mod = item->getModifier();
    undoInfo.addItem(std::move(item));
    return mod;
}

MDagModifierUndoItem::~MDagModifierUndoItem() { }

bool MDagModifierUndoItem::undo() { return _modifier.undoIt() == MS::kSuccess; }

bool MDagModifierUndoItem::redo() { return _modifier.doIt() == MS::kSuccess; }

//------------------------------------------------------------------------------
// MDGModifierUndoItem
//------------------------------------------------------------------------------

MDGModifier& MDGModifierUndoItem::create(const std::string name, OpUndoItemList& undoInfo)
{
    auto         item = std::make_unique<MDGModifierUndoItem>(std::move(name));
    MDGModifier& mod = item->getModifier();
    undoInfo.addItem(std::move(item));
    return mod;
}

MDGModifierUndoItem::~MDGModifierUndoItem() { }

bool MDGModifierUndoItem::undo() { return _modifier.undoIt() == MS::kSuccess; }

bool MDGModifierUndoItem::redo() { return _modifier.doIt() == MS::kSuccess; }

//------------------------------------------------------------------------------
// UsdUndoableItemUndoItem
//------------------------------------------------------------------------------

MAYAUSD_NS::UsdUndoableItem&
UsdUndoableItemUndoItem::create(const std::string name, OpUndoItemList& undoInfo)
{
    auto                         item = std::make_unique<UsdUndoableItemUndoItem>(std::move(name));
    MAYAUSD_NS::UsdUndoableItem& mod = item->getUndoableItem();
    undoInfo.addItem(std::move(item));
    return mod;
}

UsdUndoableItemUndoItem::~UsdUndoableItemUndoItem() { }

bool UsdUndoableItemUndoItem::undo()
{
    _item.undo();
    return true;
}

bool UsdUndoableItemUndoItem::redo()
{
    _item.redo();
    return true;
}

//------------------------------------------------------------------------------
// PythonUndoItem
//------------------------------------------------------------------------------

PythonUndoItem::PythonUndoItem(const std::string name, MString pythonDo, MString pythonUndo)
    : OpUndoItem(std::move(name))
    , _pythonDo(std::move(pythonDo))
    , _pythonUndo(std::move(pythonUndo))
{
}

PythonUndoItem::~PythonUndoItem() { }

void PythonUndoItem::execute(
    const std::string name,
    MString           pythonDo,
    MString           pythonUndo,
    OpUndoItemList&       undoInfo)
{
    auto item = std::make_unique<PythonUndoItem>(
        std::move(name), std::move(pythonDo), std::move(pythonUndo));
    item->redo();
    undoInfo.addItem(std::move(item));
}

namespace {
bool executePython(const MString& python)
{
    if (python.length() == 0)
        return true;

    return MGlobal::executePythonCommand(python) == MS::kSuccess;
}
} // namespace

bool PythonUndoItem::undo() { return executePython(_pythonUndo); }

bool PythonUndoItem::redo() { return executePython(_pythonDo); }

//------------------------------------------------------------------------------
// FunctionUndoItem
//------------------------------------------------------------------------------

FunctionUndoItem::FunctionUndoItem(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo)
    : OpUndoItem(std::move(name))
    , _redo(std::move(redo))
    , _undo(std::move(undo))
{
}

FunctionUndoItem::~FunctionUndoItem() { }

void FunctionUndoItem::create(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo,
    OpUndoItemList&           undoInfo)
{
    auto item
        = std::make_unique<FunctionUndoItem>(std::move(name), std::move(redo), std::move(undo));
    undoInfo.addItem(std::move(item));
}

void FunctionUndoItem::execute(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo,
    OpUndoItemList&           undoInfo)
{
    auto item
        = std::make_unique<FunctionUndoItem>(std::move(name), std::move(redo), std::move(undo));
    item->redo();
    undoInfo.addItem(std::move(item));
}

bool FunctionUndoItem::undo()
{
    if (!_undo)
        return false;

    return _undo();
}

bool FunctionUndoItem::redo()
{
    if (!_redo)
        return false;

    return _redo();
}

//------------------------------------------------------------------------------
// SelectionUndoItem
//------------------------------------------------------------------------------

SelectionUndoItem::SelectionUndoItem(
    const std::string       name,
    const MSelectionList&   selection,
    MGlobal::ListAdjustment selMode)
    : OpUndoItem(std::move(name))
    , _selection(selection)
    , _selMode(selMode)
{
}

SelectionUndoItem::~SelectionUndoItem() { }

void SelectionUndoItem::select(
    const std::string       name,
    const MSelectionList&   selection,
    MGlobal::ListAdjustment selMode,
    OpUndoItemList&             undoInfo)
{
    auto item = std::make_unique<SelectionUndoItem>(std::move(name), selection, selMode);
    item->redo();
    undoInfo.addItem(std::move(item));
}

void SelectionUndoItem::select(
    const std::string       name,
    const MDagPath&         dagPath,
    MGlobal::ListAdjustment selMode,
    OpUndoItemList&             undoInfo)
{
    MSelectionList selection;
    selection.add(dagPath);
    SelectionUndoItem::select(std::move(name), selection, selMode, undoInfo);
}

bool SelectionUndoItem::undo()
{
    MStatus status = MGlobal::setActiveSelectionList(_previousSelection);
    return status == MS::kSuccess;
}

bool SelectionUndoItem::redo()
{
    MGlobal::getActiveSelectionList(_previousSelection);
    MStatus status = MGlobal::setActiveSelectionList(_selection, _selMode);
    return status == MS::kSuccess;
}

//------------------------------------------------------------------------------
// LockNodesUndoItem
//------------------------------------------------------------------------------

namespace {

//! Lock or unlock hierarchy starting at given root.
MStatus lockNodes(const MString& rootName, bool state)
{
    MObject obj;
    MStatus status = PXR_NS::UsdMayaUtil::GetMObjectByName(rootName, obj);
    if (status != MStatus::kSuccess)
        return status;

    MDagPath root;
    status = MDagPath::getAPathTo(obj, root);
    if (status != MStatus::kSuccess)
        return status;

    MItDag dagIt;
    dagIt.reset(root);
    for (; !dagIt.isDone(); dagIt.next()) {
        MFnDependencyNode node(dagIt.currentItem());
        if (node.isFromReferencedFile()) {
            dagIt.prune();
            continue;
        }
        node.setLocked(state);
    }

    return MS::kSuccess;
}

} // namespace

LockNodesUndoItem::LockNodesUndoItem(const std::string name, const MDagPath& root, bool lock)
    : OpUndoItem(std::move(name))
    , _rootName(root.fullPathName())
    , _lock(lock)
{
}

LockNodesUndoItem::~LockNodesUndoItem() { }

void LockNodesUndoItem::lock(
    const std::string name,
    const MDagPath&   root,
    bool              lock,
    OpUndoItemList&       undoInfo)
{
    auto item = std::make_unique<LockNodesUndoItem>(std::move(name), root, lock);
    item->redo();
    undoInfo.addItem(std::move(item));
}

bool LockNodesUndoItem::undo()
{
    lockNodes(_rootName, !_lock);
    return true;
}

bool LockNodesUndoItem::redo()
{
    lockNodes(_rootName, _lock);
    return true;
}

//------------------------------------------------------------------------------
// CreateSetUndoItem
//------------------------------------------------------------------------------

CreateSetUndoItem::CreateSetUndoItem(const std::string name, const MString& setName)
    : OpUndoItem(std::move(name))
    , _setName(setName)
{
}

CreateSetUndoItem::~CreateSetUndoItem() { }

MObject CreateSetUndoItem::create(std::string name, const MString& setName, OpUndoItemList& undoInfo)
{
    auto item = std::make_unique<CreateSetUndoItem>(std::move(name), setName);
    item->redo();
    MObject obj = item->_setObj;
    undoInfo.addItem(std::move(item));
    return obj;
}

bool CreateSetUndoItem::undo()
{
    const MStatus status = MGlobal::deleteNode(_setObj);
    _setObj = MObject();
    return status == MS::kSuccess;
}

bool CreateSetUndoItem::redo()
{
    MSelectionList selList;
    MStatus        status = MS::kSuccess;
    MFnSet         setFn;
    _setObj = setFn.create(selList, MFnSet::kNone, &status);
    if (status == MS::kSuccess)
        setFn.setName(_setName, &status);
    return status == MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF