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

#include <memory>
#include <vector>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// NodeDeletionUndoItem
//------------------------------------------------------------------------------

/// \class NodeDeletionUndoItem
/// \brief Record data needed to undo or redo a Maya DG sub-operation.
///
/// For node deletion, use the specialized undo item that tracks which objects
/// have already been deleted and avoid double-deletions.
class NodeDeletionUndoItem : public OpUndoItem
{
public:
    /// \brief delete a node.
    MAYAUSD_CORE_PUBLIC
    static MStatus deleteNode(
        const std::string name,
        const MString&    nodeName,
        const MObject&    node,
        OpUndoItemList&       undoInfo);

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
    static void
    execute(const std::string name, MString pythonDo, MString pythonUndo, OpUndoItemList& undoInfo);

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
        OpUndoItemList&           undoInfo);

    /// \brief create and execute functions and how to undo it and keep track of it.
    ///        Useful if the item execution has *not* already been done but must done now.
    MAYAUSD_CORE_PUBLIC
    static void execute(
        const std::string     name,
        std::function<bool()> redo,
        std::function<bool()> undo,
        OpUndoItemList&           undoInfo);

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
    static void select(
        const std::string       name,
        const MSelectionList&   selection,
        MGlobal::ListAdjustment selMode,
        OpUndoItemList&             undoInfo);

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static void select(
        const std::string       name,
        const MDagPath&         dagPath,
        MGlobal::ListAdjustment selMode,
        OpUndoItemList&             undoInfo);

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static void
    select(const std::string name, const MSelectionList& selection, OpUndoItemList& undoInfo)
    {
        SelectionUndoItem::select(name, selection, MGlobal::kReplaceList, undoInfo);
    }

    /// \brief create and execute a select node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static void select(const std::string name, const MDagPath& dagPath, OpUndoItemList& undoInfo)
    {
        SelectionUndoItem::select(name, dagPath, MGlobal::kReplaceList, undoInfo);
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

//------------------------------------------------------------------------------
// LockNodesUndoItem
//------------------------------------------------------------------------------

/// \class LockNodesUndoItem
/// \brief Record data needed to undo or redo lock or unlock nodes sub-operations.
class LockNodesUndoItem : public OpUndoItem
{
public:
    /// \brief create and execute a lock node undo item and keep track of it.
    MAYAUSD_CORE_PUBLIC
    static void lock(const std::string name, const MDagPath& root, bool lock, OpUndoItemList& undoInfo);

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
    MString _rootName;
    bool    _lock;
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

PXR_NAMESPACE_OPEN_SCOPE

using OpUndoItemList = MAYAUSD_NS_DEF::OpUndoItemList;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
