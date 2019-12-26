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
#ifndef MAYAUSD_UNDO_OP_UNDO_INFO_H
#define MAYAUSD_UNDO_OP_UNDO_INFO_H

#include <mayaUsd/base/api.h>

#include <maya/MObjectHandle.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// OpUndoItem
//------------------------------------------------------------------------------

/// \class OpUndoItem
/// \brief Record data needed to undo or redo a single undo sub-operation.
///
/// See OpUndoItems.h for concrete implementations.

class OpUndoItem
{
public:
    typedef std::unique_ptr<OpUndoItem> Ptr;

    /// \brief construct a single named sub-operation.
    MAYAUSD_CORE_PUBLIC
    OpUndoItem(std::string name)
        : _name(std::move(name))
    {
    }

    MAYAUSD_CORE_PUBLIC
    virtual ~OpUndoItem() = default;

    /// \brief undo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    virtual bool undo() = 0;

    /// \brief redo a single sub-operation.
    MAYAUSD_CORE_PUBLIC
    virtual bool redo() = 0;

    /// \brief get the undo item name, used for debugging and logging.
    MAYAUSD_CORE_PUBLIC
    const std::string& getName() const { return _name; }

private:
    // Name the undo items to help debugging, tracing and logging.
    std::string _name;
};

//------------------------------------------------------------------------------
// OpUndoInfo
//------------------------------------------------------------------------------

/// \class OpUndoInfo
/// \brief Record everything needed to undo or redo a complete operation or command.

class OpUndoInfo
{
public:
    /// \brief construct an undo info.
    MAYAUSD_CORE_PUBLIC
    OpUndoInfo() = default;

    MAYAUSD_CORE_PUBLIC
    OpUndoInfo(OpUndoInfo&&) = default;

    MAYAUSD_CORE_PUBLIC
    OpUndoInfo& operator=(OpUndoInfo&&) = default;

    /// \brief destroy the undo info.
    MAYAUSD_CORE_PUBLIC
    ~OpUndoInfo();

    /// \brief undo a complete PrimUpdaterManager operation.
    MAYAUSD_CORE_PUBLIC
    bool undo();

    /// \brief redo a complete PrimUpdaterManager operation.
    MAYAUSD_CORE_PUBLIC
    bool redo();

    /// \brief add a undo item. This takes ownership of the item.
    MAYAUSD_CORE_PUBLIC
    void addItem(OpUndoItem::Ptr&& item);

    /// \brief add an object that was deleted, to help avoid deleting an object twice.
    MAYAUSD_CORE_PUBLIC
    void addDeleted(const MObjectHandle obj);

    /// \brief verify if an object was already deleted, to help avoid deleting an object twice.
    MAYAUSD_CORE_PUBLIC
    bool isDeleted(const MObjectHandle obj) const;

    /// \brief clear all undo/redo information contained here.
    MAYAUSD_CORE_PUBLIC
    void clear();

    /// \brief extract all undo/redo information contained here and clear.
    MAYAUSD_CORE_PUBLIC
    OpUndoInfo extract();

private:
    OpUndoInfo(const OpUndoInfo&) = delete;
    OpUndoInfo& operator=(const OpUndoInfo&) = delete;

    struct MObjectHandleHash
    {
        size_t operator()(const MObjectHandle obj ) const
        {
            return obj.hashCode();
        }
    };

    using DeletedObjectSet = std::unordered_set<MObjectHandle, MObjectHandleHash>;

    std::vector<OpUndoItem::Ptr> _undoItems;
    DeletedObjectSet             _deletedMayaObjects;
    bool                         _isUndone = false;
};

} // namespace MAYAUSD_NS_DEF

PXR_NAMESPACE_OPEN_SCOPE

using OpUndoInfo = MAYAUSD_NS_DEF::OpUndoInfo;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
