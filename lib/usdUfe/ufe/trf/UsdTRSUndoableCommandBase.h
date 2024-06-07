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
#ifndef USDUFE_USDTRSUNDOABLECOMMANDBASE_H
#define USDUFE_USDTRSUNDOABLECOMMANDBASE_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/observer.h>
#include <ufe/transform3dUndoableCommands.h>

namespace USDUFE_NS_DEF {

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
template <class V>
class USDUFE_PUBLIC UsdTRSUndoableCommandBase
    : public std::enable_shared_from_this<UsdTRSUndoableCommandBase<V>>
{
protected:
    UsdTRSUndoableCommandBase(double x, double y, double z);
    ~UsdTRSUndoableCommandBase() = default;

    // Initialize the command.
    void initialize();

    // Undo and redo implementations.
    void undoImp();
    void redoImp();

    // Set the new value of the command (for redo), and execute the command.
    void perform(double x, double y, double z);

    // UFE item (and its USD prim) may change after creation time (e.g.
    // parenting change caused by undo / redo of other commands in the undo
    // stack), so always return current data.
    inline PXR_NS::UsdPrim prim() const
    {
        updateItem();
        return _item->prim();
    };

    // Hooks to be implemented by the derived class: name of the attribute set
    // by the command, implementation of perform(), and add empty attribute.
    // Implementation of cannotInit() in this class returns false.
    virtual PXR_NS::TfToken attributeName() const = 0;
    virtual void            performImp(double x, double y, double z) = 0;
    virtual void            addEmptyAttribute() = 0;
    virtual bool            cannotInit() const;

    // Conditionally create a UsdSceneItem::Ptr from the Ufe::Path, if null.
    void updateItem() const;

    // Returns the new Ufe::Path overriden by derived classes (e.g TRS)
    virtual Ufe::Path getPath() const = 0;

private:
    inline PXR_NS::UsdAttribute attribute() const { return prim().GetAttribute(attributeName()); }

    mutable UsdSceneItem::Ptr _item { nullptr };
    V                         _prevValue;
    V                         _newValue;
    bool                      _opAdded { false };
    bool                      _doneOnce { false };

}; // UsdTRSUndoableCommandBase

// shared_ptr requires public ctor, dtor, so derive a class for it.
template <class T> struct MakeSharedEnabler : public T
{
    MakeSharedEnabler(const Ufe::Path& path, double x, double y, double z)
        : T(path, x, y, z)
    {
    }
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDTRSUNDOABLECOMMANDBASE_H
