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

#include <unordered_map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for USD Attributes.
class UsdAttributes : public Ufe::Attributes
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

private:
    Ufe::Attribute::Type getUfeTypeForAttribute(const PXR_NS::UsdAttribute& usdAttr) const;

private:
    UsdSceneItem::Ptr fItem;
    PXR_NS::UsdPrim   fPrim;

    typedef std::unordered_map<std::string, Ufe::Attribute::Ptr> AttributeMap;
    AttributeMap                                                 fAttributes;

}; // UsdAttributes

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
