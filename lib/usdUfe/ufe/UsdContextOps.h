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
#ifndef USDUFE_USDCONTEXTOPS_H
#define USDUFE_USDCONTEXTOPS_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/contextOps.h>
#include <ufe/path.h>
#include <ufe/selection.h>

#include <map>
#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

struct SchemaTypeGroup
{
    std::string           _name;
    PXR_NS::TfTokenVector _types;
    bool                  operator==(const std::string rhs) const { return _name == rhs; }
};

//! \brief Interface for scene item context operations.
/*!
    This class defines the interface that USD run-time implements to
    provide contextual operation support (example Outliner context menu).

    This class is not copy-able, nor move-able.

    \see UFE ContextOps class documentation for more details
*/
class USDUFE_PUBLIC UsdContextOps : public Ufe::ContextOps
{
public:
    typedef std::shared_ptr<UsdContextOps> Ptr;

    UsdContextOps(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdContextOps);

    //! Create a UsdContextOps.
    static UsdContextOps::Ptr create(const UsdSceneItem::Ptr& item);

    void                   setItem(const UsdSceneItem::Ptr& item);
    const Ufe::Path&       path() const;
    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(_item != nullptr))
            return _item->prim();
        else
            return PXR_NS::UsdPrim();
    }

    // When we are created from a gateway node ContextOpsHandler we do not have the proper
    // UFE scene item. So it won't return the correct node type. Therefore we set
    // this flag directly.
    void setIsAGatewayType(bool t) { _isAGatewayType = t; }
    bool isAGatewayType() const { return _isAGatewayType; }

    //! Returns true if this contextOps is in Bulk Edit mode.
    //! Meaning there are multiple items selected and the operation will (potentially)
    //! be ran on all of the them.
    bool isBulkEdit() const { return !_bulkItems.empty(); }

    //! When in bulk edit mode returns the type of all the prims
    //! if they are all of the same type. If mixed selection
    //! then empty string is returned.
    const std::string bulkEditType() const { return _bulkType; }

    // Ufe::ContextOps overrides
    Ufe::SceneItem::Ptr       sceneItem() const override;
    Items                     getItems(const ItemPath& itemPath) const override;
    Ufe::UndoableCommand::Ptr doOpCmd(const ItemPath& itemPath) override;

    // Bulk edit helpers:

    // Adds the special Bulk Edit header as the first item.
    void addBulkEditHeader(Ufe::ContextOps::Items& items) const;

    // Will be called from getItems/doOpCmd respectively when in bulk edit mode.
    // Can be overridden by derived class to add to the bulk items.
    virtual Items                     getBulkItems(const ItemPath& itemPath) const;
    virtual Ufe::UndoableCommand::Ptr doBulkOpCmd(const ItemPath& itemPath);

    // Called from getItems() method to replace the USD schema names with a
    // nice UI name (in the context menu).
    // Can be overridden by derived class to add to the map.
    typedef std::map<std::string, std::string> SchemaNameMap;
    virtual SchemaNameMap                      getSchemaPluginNiceNames() const;

protected:
    UsdSceneItem::Ptr _item;
    bool              _isAGatewayType { false };
    std::string       _bulkType;
    Ufe::Selection    _bulkItems;

    // A cache to keep the dynamic listing of plugin types to a minimum
    static std::vector<SchemaTypeGroup> schemaTypeGroups;

}; // UsdContextOps

//! \brief Composite undoable command for Bulk Edit.
/*!
    This class adds to the base Ufe::CompositeUndoableCommand by only keeping
    commands that succeed. This is done because there can be edit restrictions
    that will cause a command to fail, but we still want to execute the
    remaining commands.

*/
class USDUFE_PUBLIC UsdBulkEditCompositeUndoableCommand : public Ufe::CompositeUndoableCommand
{
public:
    typedef Ufe::CompositeUndoableCommand Parent;

    UsdBulkEditCompositeUndoableCommand() = default;

    //! Add the command to the end of the list of commands.
    void addCommand(const Ptr& cmd);

    // Overridden from Ufe::CompositeUndoableCommand
    void execute() override;

private:
    // We keep our own list of command so that we can remove ones that error during
    // execute. The base class list is private and only has const accessor.
    CmdList _cmds;
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDCONTEXTOPS_H
