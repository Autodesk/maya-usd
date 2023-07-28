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

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/contextOps.h>
#include <ufe/path.h>

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
    ~UsdContextOps() override;

    // Delete the copy/move constructors assignment operators.
    UsdContextOps(const UsdContextOps&) = delete;
    UsdContextOps& operator=(const UsdContextOps&) = delete;
    UsdContextOps(UsdContextOps&&) = delete;
    UsdContextOps& operator=(UsdContextOps&&) = delete;

    //! Create a UsdContextOps.
    static UsdContextOps::Ptr create(const UsdSceneItem::Ptr& item);

    void                   setItem(const UsdSceneItem::Ptr& item);
    const Ufe::Path&       path() const;
    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(fItem != nullptr))
            return fItem->prim();
        else
            return PXR_NS::UsdPrim();
    }

    // When we are created from a gateway node ContextOpsHandler we do not have the proper
    // UFE scene item. So it won't return the correct node type. Therefore we set
    // this flag directly.
    void setIsAGatewayType(bool t) { fIsAGatewayType = t; }
    bool isAGatewayType() const { return fIsAGatewayType; }

    // Ufe::ContextOps overrides
    Ufe::SceneItem::Ptr       sceneItem() const override;
    Items                     getItems(const ItemPath& itemPath) const override;
    Ufe::UndoableCommand::Ptr doOpCmd(const ItemPath& itemPath) override;

    // Helpers

    // Called from getItems() method to replace the USD schema names with a
    // nice UI name (in the context menu).
    // Can be overridden by derived class to add to the map.
    typedef std::map<std::string, std::string> SchemaNameMap;
    virtual SchemaNameMap                      getSchemaPluginNiceNames() const;

protected:
    UsdSceneItem::Ptr fItem;
    bool              fIsAGatewayType { false };

    // A cache to keep the dynamic listing of plugin types to a minimum
    static std::vector<SchemaTypeGroup> schemaTypeGroups;

}; // UsdContextOps

} // namespace USDUFE_NS_DEF
