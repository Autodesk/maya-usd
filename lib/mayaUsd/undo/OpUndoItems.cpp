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
#ifdef WANT_UFE_BUILD
#include <mayaUsd/ufe/Utils.h>
#endif

#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#ifdef WANT_UFE_BUILD
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#endif

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
    OpUndoItemList&   undoInfo)
{
    // Avoid deleting the same node twice.
    if (!MObjectHandle(node).isValid())
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

    return MS::kSuccess;
}

MStatus NodeDeletionUndoItem::deleteNode(
    const std::string name,
    const MString&    nodeName,
    const MObject&    node)
{
    return deleteNode(name, nodeName, node, OpUndoItemList::instance());
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

MDagModifier& MDagModifierUndoItem::create(const std::string name)
{
    return create(name, OpUndoItemList::instance());
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

MDGModifier& MDGModifierUndoItem::create(const std::string name)
{
    return create(name, OpUndoItemList::instance());
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

MAYAUSD_NS::UsdUndoableItem& UsdUndoableItemUndoItem::create(const std::string name)
{
    return create(name, OpUndoItemList::instance());
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
    OpUndoItemList&   undoInfo)
{
    auto item = std::make_unique<PythonUndoItem>(
        std::move(name), std::move(pythonDo), std::move(pythonUndo));
    item->redo();
    undoInfo.addItem(std::move(item));
}

void PythonUndoItem::execute(const std::string name, MString pythonDo, MString pythonUndo)
{
    return execute(name, pythonDo, pythonUndo, OpUndoItemList::instance());
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
    OpUndoItemList&       undoInfo)
{
    auto item
        = std::make_unique<FunctionUndoItem>(std::move(name), std::move(redo), std::move(undo));
    undoInfo.addItem(std::move(item));
}

void FunctionUndoItem::create(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo)
{
    create(name, redo, undo, OpUndoItemList::instance());
}

void FunctionUndoItem::execute(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo,
    OpUndoItemList&       undoInfo)
{
    auto item
        = std::make_unique<FunctionUndoItem>(std::move(name), std::move(redo), std::move(undo));
    item->redo();
    undoInfo.addItem(std::move(item));
}

void FunctionUndoItem::execute(
    const std::string     name,
    std::function<bool()> redo,
    std::function<bool()> undo)
{
    execute(name, redo, undo, OpUndoItemList::instance());
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
    OpUndoItemList&         undoInfo)
{
    auto item = std::make_unique<SelectionUndoItem>(std::move(name), selection, selMode);
    item->redo();
    undoInfo.addItem(std::move(item));
}

void SelectionUndoItem::select(
    const std::string       name,
    const MSelectionList&   selection,
    MGlobal::ListAdjustment selMode)
{
    select(name, selection, selMode, OpUndoItemList::instance());
}

void SelectionUndoItem::select(
    const std::string       name,
    const MDagPath&         dagPath,
    MGlobal::ListAdjustment selMode,
    OpUndoItemList&         undoInfo)
{
    MSelectionList selection;
    selection.add(dagPath);
    SelectionUndoItem::select(std::move(name), selection, selMode, undoInfo);
}

void SelectionUndoItem::select(
    const std::string       name,
    const MDagPath&         dagPath,
    MGlobal::ListAdjustment selMode)
{
    select(name, dagPath, selMode, OpUndoItemList::instance());
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

#ifdef WANT_UFE_BUILD
//------------------------------------------------------------------------------
// UfeSelectionUndoItem
//------------------------------------------------------------------------------

UfeSelectionUndoItem::UfeSelectionUndoItem(const std::string& name, const Ufe::Selection& selection)
    : OpUndoItem(name)
    , _selection(selection)
{
}

UfeSelectionUndoItem::~UfeSelectionUndoItem() { }

void UfeSelectionUndoItem::select(
    const std::string&    name,
    const Ufe::Selection& selection,
    OpUndoItemList&       undoInfo)
{
    auto item = std::make_unique<UfeSelectionUndoItem>(name, selection);
    item->redo();
    undoInfo.addItem(std::move(item));
}

void UfeSelectionUndoItem::select(const std::string& name, const Ufe::Selection& selection)
{
    select(name, selection, OpUndoItemList::instance());
}

void UfeSelectionUndoItem::select(
    const std::string& name,
    const MDagPath&    dagPath,
    OpUndoItemList&    undoInfo)
{
    Ufe::Selection sn;
    sn.append(Ufe::Hierarchy::createItem(MayaUsd::ufe::dagPathToUfe(dagPath)));
    select(name, sn, undoInfo);
}

void UfeSelectionUndoItem::select(const std::string& name, const MDagPath& dagPath)
{
    select(name, dagPath, OpUndoItemList::instance());
}

void UfeSelectionUndoItem::clear(const std::string& name, OpUndoItemList& undoInfo)
{
    select(name, Ufe::Selection(), undoInfo);
}

void UfeSelectionUndoItem::clear(const std::string& name)
{
    clear(name, OpUndoItemList::instance());
}

bool UfeSelectionUndoItem::undo()
{
    invert();
    return true;
}

bool UfeSelectionUndoItem::redo()
{
    invert();
    return true;
}

void UfeSelectionUndoItem::invert()
{
    auto globalSn = Ufe::GlobalSelection::get();
    auto previousSelection = *globalSn;
    globalSn->replaceWith(_selection);
    _selection = std::move(previousSelection);
}
#endif

//------------------------------------------------------------------------------
// LockNodesUndoItem
//------------------------------------------------------------------------------

namespace {

//! Lock or unlock hierarchy starting at given root.
void lockNodes(const MDagPath& root, bool state)
{
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
}

} // namespace

LockNodesUndoItem::LockNodesUndoItem(const std::string name, const MDagPath& root, bool lock)
    : OpUndoItem(std::move(name))
    , _root(root)
    , _lock(lock)
{
}

LockNodesUndoItem::~LockNodesUndoItem() { }

void LockNodesUndoItem::lock(
    const std::string name,
    const MDagPath&   root,
    bool              dolock,
    OpUndoItemList&   undoInfo)
{
    auto item = std::make_unique<LockNodesUndoItem>(std::move(name), root, dolock);
    item->redo();
    undoInfo.addItem(std::move(item));
}

void LockNodesUndoItem::lock(const std::string name, const MDagPath& root, bool dolock)
{
    lock(name, root, dolock, OpUndoItemList::instance());
}

bool LockNodesUndoItem::undo()
{
    lockNodes(_root, !_lock);
    return true;
}

bool LockNodesUndoItem::redo()
{
    lockNodes(_root, _lock);
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

MObject
CreateSetUndoItem::create(std::string name, const MString& setName, OpUndoItemList& undoInfo)
{
    auto item = std::make_unique<CreateSetUndoItem>(std::move(name), setName);
    item->redo();
    MObject obj = item->_setObj;
    undoInfo.addItem(std::move(item));
    return obj;
}

MObject CreateSetUndoItem::create(std::string name, const MString& setName)
{
    return create(name, setName, OpUndoItemList::instance());
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
