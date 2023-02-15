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
#ifndef MAYAUSD_UNDO_OP_UNDO_ITEMS_H
#define MAYAUSD_UNDO_OP_UNDO_ITEMS_H

#include "OpUndoItemList.h"

#include <mayaUsd/undo/UsdUndoableItem.h>

#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#ifdef WANT_UFE_BUILD
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>
#endif

#include <memory>
#include <vector>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// NodeDeletionUndoItem
//------------------------------------------------------------------------------

/// \class NodeDeletionUndoItem
/// \brief Record data needed to undo or redo a Maya DG sub-operation.
class NodeDeletionUndoItem : public OpUndoItem
{
public:
    /// \brief delete a node.
    MAYAUSD_CORE_PUBLIC
    static MStatus deleteNode(
        const std::string name,
        const MString&    nodeName,
        const MObject&    node,
        OpUndoItemList&   undoInfo);

    /// \brief delete a node and keep track of it in the global undo item list.
    MAYAUSD_CORE_PUBLIC
    static MStatus deleteNode(const std::string name, const MString& nodeName, const MObject& node);

    /// \brief construct a Maya DG modifier recorder.
    MAYAUSD_CORE_PUBLIC
    NodeDeletionUndoItem(std::string name)
        : OpUndoItem(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    ~NodeDeletionUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    MDGModifier _modifier;
};

//------------------------------------------------------------------------------
// MDagModifierUndoItem
//------------------------------------------------------------------------------

/// \class MDagModifierUndoItem
/// \brief Record data needed to undo or redo a Maya DAG sub-operation.///
///
/// For node deletion, use the specialized NodeDeletionUndoItem that tracks
/// which objects have already been deleted and avoid double-deletions.
class MDagModifierUndoItem : public OpUndoItem
{
public:
    /// \brief create a Maya DAG modifier recorder and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static MDagModifier& create(const std::string name, OpUndoItemList& undoInfo);

    /// \brief create a Maya DAG modifier recorder and keep track of it in the global undo item
    /// list.
    MAYAUSD_CORE_PUBLIC
    static MDagModifier& create(const std::string name);

    /// \brief construct a Maya DAG modifier recorder.
    MAYAUSD_CORE_PUBLIC
    MDagModifierUndoItem(std::string name)
        : OpUndoItem(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    ~MDagModifierUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

    /// \brief gets the DAG modifier.
    MAYAUSD_CORE_PUBLIC
    MDagModifier& getModifier() { return _modifier; }

private:
    MDagModifier _modifier;
};

//------------------------------------------------------------------------------
// MDGModifierUndoItem
//------------------------------------------------------------------------------

/// \class MDGModifierUndoItem
/// \brief Record data needed to undo or redo a Maya DG sub-operation.
///
/// For node deletion, use the specialized NodeDeletionUndoItem that tracks
/// which objects have already been deleted and avoid double-deletions.
class MDGModifierUndoItem : public OpUndoItem
{
public:
    /// \brief create a Maya DG modifier recorder and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static MDGModifier& create(const std::string name, OpUndoItemList& undoInfo);

    /// \brief create a Maya DG modifier recorder and keep track of it in the global undo item list.
    MAYAUSD_CORE_PUBLIC
    static MDGModifier& create(const std::string name);

    /// \brief construct a Maya DG modifier recorder.
    MAYAUSD_CORE_PUBLIC
    MDGModifierUndoItem(std::string name)
        : OpUndoItem(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    ~MDGModifierUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

    /// \brief gets the DG modifier.
    MAYAUSD_CORE_PUBLIC
    MDGModifier& getModifier() { return _modifier; }

private:
    MDGModifier _modifier;
};

//------------------------------------------------------------------------------
// UsdUndoableItemUndoItem
//------------------------------------------------------------------------------

/// \class UsdUndoableItemUndoItem
/// \brief Record data needed to undo or redo USD sub-operations.
class UsdUndoableItemUndoItem : public OpUndoItem
{
public:
    /// \brief create a USD undo item recorder and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static MAYAUSD_NS::UsdUndoableItem& create(const std::string name, OpUndoItemList& undoInfo);

    /// \brief create a USD undo item recorder and keep track of it in the global undo item list.
    MAYAUSD_CORE_PUBLIC
    static MAYAUSD_NS::UsdUndoableItem& create(const std::string name);

    /// \brief construct a USD undo item recorder.
    MAYAUSD_CORE_PUBLIC
    UsdUndoableItemUndoItem(std::string name)
        : OpUndoItem(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    ~UsdUndoableItemUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

    /// \brief gets the DG modifier.
    MAYAUSD_CORE_PUBLIC
    MAYAUSD_NS::UsdUndoableItem& getUndoableItem() { return _item; }

private:
    MAYAUSD_NS::UsdUndoableItem _item;
};

//------------------------------------------------------------------------------
// PythonUndoItem
//------------------------------------------------------------------------------

/// \class PythonUndoItem
/// \brief Record data needed to undo or redo python sub-operations.
class PythonUndoItem : public OpUndoItem
{
public:
    /// \brief create and execute python and and how to undo it and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool
    execute(const std::string name, MString pythonDo, MString pythonUndo, OpUndoItemList& undoInfo);

    /// \brief create and execute python and and how to undo it and keep track of it in the global
    /// list.
    MAYAUSD_CORE_PUBLIC
    static bool execute(const std::string name, MString pythonDo, MString pythonUndo);

    /// \brief create a python undo item.
    MAYAUSD_CORE_PUBLIC
    PythonUndoItem(const std::string name, MString pythonDo, MString pythonUndo);

    MAYAUSD_CORE_PUBLIC
    ~PythonUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    MString _pythonDo;
    MString _pythonUndo;
};

//------------------------------------------------------------------------------
// FunctionUndoItem
//------------------------------------------------------------------------------

/// \class FunctionUndoItem
/// \brief Record data needed to undo or redo generic functions sub-operations.
class FunctionUndoItem : public OpUndoItem
{
public:
    /// \brief create but do *not* execute functions and keep track of it.
    ///        Useful if the item execution has already been done.
    MAYAUSD_CORE_PUBLIC
    static void create(
        const std::string     name,
        std::function<bool()> redo,
        std::function<bool()> undo,
        OpUndoItemList&       undoInfo);

    /// \brief create but do *not* execute functions and keep track of it in the global undo list.
    ///        Useful if the item execution has already been done.
    MAYAUSD_CORE_PUBLIC
    static void
    create(const std::string name, std::function<bool()> redo, std::function<bool()> undo);

    /// \brief create and execute functions and how to undo it and keep track of it.
    ///        Useful if the item execution has *not* already been done but must done now.
    MAYAUSD_CORE_PUBLIC
    static bool execute(
        const std::string     name,
        std::function<bool()> redo,
        std::function<bool()> undo,
        OpUndoItemList&       undoInfo);

    /// \brief create and execute functions and how to undo it and keep track of it in the global
    /// undo list.
    ///
    /// Useful if the item execution has *not* already been done but must done now.
    MAYAUSD_CORE_PUBLIC
    static bool
    execute(const std::string name, std::function<bool()> redo, std::function<bool()> undo);

    /// \brief create a function undo item.
    MAYAUSD_CORE_PUBLIC
    FunctionUndoItem(
        const std::string     name,
        std::function<bool()> redo,
        std::function<bool()> undo);

    MAYAUSD_CORE_PUBLIC
    ~FunctionUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    std::function<bool()> _redo;
    std::function<bool()> _undo;
};

//------------------------------------------------------------------------------
// SelectionUndoItem
//------------------------------------------------------------------------------

/// \class SelectionUndoItem
/// \brief Record data needed to undo or redo select nodes sub-operations.
class SelectionUndoItem : public OpUndoItem
{
public:
    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool select(
        const std::string       name,
        const MSelectionList&   selection,
        MGlobal::ListAdjustment selMode,
        OpUndoItemList&         undoInfo);

    /// \brief create and execute a select node undo item and keep track of it in the global list.
    MAYAUSD_CORE_PUBLIC
    static bool select(
        const std::string       name,
        const MSelectionList&   selection,
        MGlobal::ListAdjustment selMode);

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool select(
        const std::string       name,
        const MDagPath&         dagPath,
        MGlobal::ListAdjustment selMode,
        OpUndoItemList&         undoInfo);

    /// \brief create and execute a select node undo item and keep track of it in the global list.
    MAYAUSD_CORE_PUBLIC
    static bool
    select(const std::string name, const MDagPath& dagPath, MGlobal::ListAdjustment selMode);

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool
    select(const std::string name, const MSelectionList& selection, OpUndoItemList& undoInfo)
    {
        return SelectionUndoItem::select(name, selection, MGlobal::kReplaceList, undoInfo);
    }

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string name, const MSelectionList& selection)
    {
        return SelectionUndoItem::select(name, selection, MGlobal::kReplaceList);
    }

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string name, const MDagPath& dagPath, OpUndoItemList& undoInfo)
    {
        return SelectionUndoItem::select(name, dagPath, MGlobal::kReplaceList, undoInfo);
    }

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string name, const MDagPath& dagPath)
    {
        return SelectionUndoItem::select(name, dagPath, MGlobal::kReplaceList);
    }

    MAYAUSD_CORE_PUBLIC
    SelectionUndoItem(
        const std::string       name,
        const MSelectionList&   selection,
        MGlobal::ListAdjustment selMode);

    MAYAUSD_CORE_PUBLIC
    ~SelectionUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    MSelectionList          _selection;
    MSelectionList          _previousSelection;
    MGlobal::ListAdjustment _selMode;
};

#ifdef WANT_UFE_BUILD

//------------------------------------------------------------------------------
// UfeSelectionUndoItem
//------------------------------------------------------------------------------

/// \class UfeSelectionUndoItem
/// \brief Record data needed to undo or redo select nodes sub-operations.
class UfeSelectionUndoItem : public OpUndoItem
{
public:
    /// \brief Create and execute a select node undo item and keep track of it.  The global
    /// selection is replaced.
    MAYAUSD_CORE_PUBLIC
    static bool
    select(const std::string& name, const Ufe::Selection& selection, OpUndoItemList& undoInfo);

    /// \brief create and execute a select node undo item and keep track of it in the global list.
    /// The global selection is replaced.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string& name, const Ufe::Selection& selection);

    /// \brief create and execute a select node undo item and keep track of it.
    /// The global selection is replaced.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string& name, const MDagPath& dagPath, OpUndoItemList& undoInfo);

    /// \brief create and execute a select node undo item and keep track of it on the global list.
    /// The global selection is replaced.
    MAYAUSD_CORE_PUBLIC
    static bool select(const std::string& name, const MDagPath& dagPath);

    /// \brief Create and execute a clear selection undo item and keep track of it.  The global
    /// selection is cleared.
    MAYAUSD_CORE_PUBLIC
    static bool clear(const std::string& name, OpUndoItemList& undoInfo);

    /// \brief create and execute a clear selection undo item and keep track of it in the global
    /// list. The global selection is cleared.
    MAYAUSD_CORE_PUBLIC
    static bool clear(const std::string& name);

    MAYAUSD_CORE_PUBLIC
    UfeSelectionUndoItem(const std::string& name, const Ufe::Selection& selection);

    MAYAUSD_CORE_PUBLIC
    ~UfeSelectionUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    void invert();

    Ufe::Selection _selection;
};

//------------------------------------------------------------------------------
// UfeCommandUndoItem
//------------------------------------------------------------------------------

/// \class UfeCommandUndoItem
/// \brief Record data needed to undo or redo a UFE command.
class UfeCommandUndoItem : public OpUndoItem
{
public:
    /// \brief Execute a UFE command and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool execute(
        const std::string&                           name,
        const std::shared_ptr<Ufe::UndoableCommand>& command,
        OpUndoItemList&                              undoInfo);

    /// \brief Execute a UFE command and keep track of it in the global list.
    MAYAUSD_CORE_PUBLIC
    static bool
    execute(const std::string& name, const std::shared_ptr<Ufe::UndoableCommand>& command);

    /// \brief Keep track of a UFE command.
    MAYAUSD_CORE_PUBLIC
    static bool
    add(const std::string&                           name,
        const std::shared_ptr<Ufe::UndoableCommand>& command,
        OpUndoItemList&                              undoInfo);

    /// \brief Keep track of a UFE command in the global list.
    MAYAUSD_CORE_PUBLIC
    static bool add(const std::string& name, const std::shared_ptr<Ufe::UndoableCommand>& command);

    MAYAUSD_CORE_PUBLIC
    UfeCommandUndoItem(
        const std::string&                           name,
        const std::shared_ptr<Ufe::UndoableCommand>& command);

    MAYAUSD_CORE_PUBLIC
    ~UfeCommandUndoItem() override;

    /// \brief execute the UFE command.
    MAYAUSD_CORE_PUBLIC
    bool execute() override;

    /// \brief undo the UFE command.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo the UFE command.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    std::shared_ptr<Ufe::UndoableCommand> _command;
};

#endif

//------------------------------------------------------------------------------
// LockNodesUndoItem
//------------------------------------------------------------------------------

/// \class LockNodesUndoItem
/// \brief Record data needed to undo / redo the lock / unlock of Maya nodes.
///
/// The node at the Dag path root, and all its children, will be locked.
/// Since referenced nodes cannot be deleted, locking such nodes is not a
/// useful workflow for maya-usd.  Therefore, if a child of the Dag path root
/// is a referenced node, the lock traversal is pruned at that point, for
/// efficiency.

class LockNodesUndoItem : public OpUndoItem
{
public:
    /// \brief create and execute a lock node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static bool
    lock(const std::string name, const MDagPath& root, bool lock, OpUndoItemList& undoInfo);

    /// \brief create and execute a lock node undo item and keep track of it in the global undo
    /// list.
    MAYAUSD_CORE_PUBLIC
    static bool lock(const std::string name, const MDagPath& root, bool lock);

    MAYAUSD_CORE_PUBLIC
    LockNodesUndoItem(const std::string name, const MDagPath& root, bool lock);

    MAYAUSD_CORE_PUBLIC
    ~LockNodesUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

private:
    const MDagPath _root;
    const bool     _lock;
};

//------------------------------------------------------------------------------
// CreateSetUndoItem
//------------------------------------------------------------------------------

/// \class CreateSetUndoItem
/// \brief Record data needed to undo or redo creation of a node set sub-operations.
class CreateSetUndoItem : public OpUndoItem
{
public:
    /// \brief create and execute a set creation undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static MObject create(const std::string name, const MString& setName, OpUndoItemList& undoInfo);

    /// \brief create and execute a set creation undo item and keep track of it in the global undo
    /// list.
    MAYAUSD_CORE_PUBLIC
    static MObject create(const std::string name, const MString& setName);

    MAYAUSD_CORE_PUBLIC
    CreateSetUndoItem(const std::string name, const MString& setName);

    MAYAUSD_CORE_PUBLIC
    ~CreateSetUndoItem() override;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool undo() override;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    bool redo() override;

    /// \brief gets the set object.
    MAYAUSD_CORE_PUBLIC
    MObject& getSetObject() { return _setObj; }

private:
    MString _setName;
    MObject _setObj;
};

} // namespace MAYAUSD_NS_DEF

#endif
