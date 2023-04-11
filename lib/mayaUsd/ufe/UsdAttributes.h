//
// Copyright 2019 Autodesk
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
#include <mayaUsd/ufe/UsdAttribute.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/attributes.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <ufe/nodeDef.h>
#endif

#include <unordered_map>

#ifdef UFE_V4_2_FEATURES_AVAILABLE
#define UFE_ATTRIBUTES_BASE Ufe::Attributes_v4_2
#else
#define UFE_ATTRIBUTES_BASE Ufe::Attributes
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for USD Attributes.
class UsdAttributes : public UFE_ATTRIBUTES_BASE
{
public:
    typedef std::shared_ptr<UsdAttributes> Ptr;

    UsdAttributes(const UsdSceneItem::Ptr& item);
    ~UsdAttributes() override;

    // Delete the copy/move constructors assignment operators.
    UsdAttributes(const UsdAttributes&) = delete;
    UsdAttributes& operator=(const UsdAttributes&) = delete;
    UsdAttributes(UsdAttributes&&) = delete;
    UsdAttributes& operator=(UsdAttributes&&) = delete;

    //! Create a UsdAttributes.
    static UsdAttributes::Ptr create(const UsdSceneItem::Ptr& item);

    // Ufe::Attributes overrides
    Ufe::SceneItem::Ptr      sceneItem() const override;
    Ufe::Attribute::Type     attributeType(const std::string& name) override;
    Ufe::Attribute::Ptr      attribute(const std::string& name) override;
    std::vector<std::string> attributeNames() const override;
    bool                     hasAttribute(const std::string& name) const override;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::AddAttributeUndoableCommand::Ptr
                              addAttributeCmd(const std::string& name, const Ufe::Attribute::Type& type) override;
    Ufe::UndoableCommand::Ptr removeAttributeCmd(const std::string& name) override;
    Ufe::RenameAttributeUndoableCommand::Ptr
    renameAttributeCmd(const std::string& originalName, const std::string& newName) override;

    inline Ufe::NodeDef::Ptr nodeDef() const;

    // Helpers for validation and execution:
    static bool canAddAttribute(const UsdSceneItem::Ptr& item, const Ufe::Attribute::Type& type);
    static Ufe::Attribute::Ptr doAddAttribute(
        const UsdSceneItem::Ptr&    item,
        const std::string&          name,
        const Ufe::Attribute::Type& type);
    static std::string getUniqueAttrName(const UsdSceneItem::Ptr& item, const std::string& name);
    static bool        canRemoveAttribute(const UsdSceneItem::Ptr& item, const std::string& name);
    static bool        doRemoveAttribute(const UsdSceneItem::Ptr& item, const std::string& name);
    static void        removeAttributesConnections(const PXR_NS::UsdPrim& prim);

    static bool canRenameAttribute(
        const UsdSceneItem::Ptr& sceneItem,
        const std::string&       originalName,
        const std::string&       newName);
    static Ufe::Attribute::Ptr doRenameAttribute(
        const UsdSceneItem::Ptr& sceneItem,
        const std::string&       originalName,
        const std::string&       newName);
#endif
#ifdef UFE_V4_FEATURES_AVAILABLE
#ifdef UFE_ATTRIBUTES_GET_ENUMS
    UFE_ATTRIBUTES_BASE::Enums getEnums(const std::string& attrName) const override;
#endif
#endif

private:
    UsdSceneItem::Ptr fItem;
    PXR_NS::UsdPrim   fPrim;

    typedef std::unordered_map<std::string, Ufe::Attribute::Ptr> AttributeMap;
    AttributeMap                                                 fUsdAttributes;
}; // UsdAttributes

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
