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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/observer.h>
#include <ufe/transform3dUndoableCommands.h>
#include <mayaUsd/ufe/UsdTransform3d.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Base class for translate, rotate, scale undoable commands.
//
// As of 9-Apr-2020, rotate and scale use GfVec3f and translate uses GfVec3d,
// so this class is templated on the vector type.
//
// This class provides services to the translate, rotate, and scale
// undoable commands.  It will:
// - Create the attribute if it does not yet exist.
// - Get the previous value and set it on undo.
// - Keep track of the new value, in case it is set repeatedly (e.g. during
//   interactive command use when manipulating, before the manipulation
//   ends and the command is committed).
// - Keep track of the scene item, in case its path changes (e.g. when the
//   prim is renamed or reparented).  A command can be created before it's
//   used, or the undo / redo stack can cause an item to be renamed or
//   reparented.  In such a case, the prim in the command's scene item
//   becomes stale, and the prim in the updated scene item should be used.
//
class MAYAUSD_CORE_PUBLIC UsdTRSUndoableCommandBase : public Ufe::Observer,
        public std::enable_shared_from_this<UsdTRSUndoableCommandBase>
{
protected:

    UsdTRSUndoableCommandBase(const UsdTransform3d& item,
        const GfVec3f& vec, UsdGeomXformOp::Type opType);

    UsdTRSUndoableCommandBase(const UsdTransform3d& item,
        const GfVec3d& vec, UsdGeomXformOp::Type opType);

    ~UsdTRSUndoableCommandBase();

    // Undo and redo implementations.
    void undo();
    void redo();

    // Set the new value of the command (for redo), and execute the command.
    void perform(double x, double y, double z);

    // UFE item (and its USD prim) may change after creation time (e.g.
    // parenting change caused by undo / redo of other commands in the undo
    // stack), so always return current data.
    inline UsdPrim prim() const { return fItem->prim(); }
    inline Ufe::Path path() const { return fItem->path(); }

    bool initialize();

private:
    class ExtendedUndo;

    // Overridden from Ufe::Observer
    void operator()(const Ufe::Notification& notification) override;

    template<class N> void checkNotification(const N* notification);

    UsdSceneItem::Ptr fItem;
    std::unique_ptr<ExtendedUndo> fExtendedUndo;
}; // UsdTRSUndoableCommandBase

// shared_ptr requires public ctor, dtor, so derive a class for it.
template<class T>
struct MakeSharedEnabler : public T {
    template <typename V>
    MakeSharedEnabler(const UsdTransform3d& item, const V& vec)
        : T(item, vec) {}
};

} // namespace ufe
} // namespace MayaUsd
