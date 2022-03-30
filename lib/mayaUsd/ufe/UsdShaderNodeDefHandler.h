//
// Copyright 2022 Autodesk
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

#include <ufe/nodeDefHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdShaderNodeDefHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdShaderNodeDefHandler : public Ufe::NodeDefHandler
{
public:
    typedef std::shared_ptr<UsdShaderNodeDefHandler> Ptr;

    UsdShaderNodeDefHandler();
    ~UsdShaderNodeDefHandler();

    // Delete the copy/move constructors assignment operators.
    UsdShaderNodeDefHandler(const UsdShaderNodeDefHandler&) = delete;
    UsdShaderNodeDefHandler& operator=(const UsdShaderNodeDefHandler&) = delete;
    UsdShaderNodeDefHandler(UsdShaderNodeDefHandler&&) = delete;
    UsdShaderNodeDefHandler& operator=(UsdShaderNodeDefHandler&&) = delete;

    //! Create a UsdShaderNodeDefHandler.
    static Ptr create();

    //! Returns a node definition for the given scene item. If the
    // definition associated with the scene item's type is not found, a
    // nullptr is returned.
    Ufe::NodeDef::Ptr definition(const Ufe::SceneItem::Ptr& item) const override;

    //! Returns a node definition for the given type. If the definition
    // associated with the type is not found, a nullptr is returned.
    Ufe::NodeDef::Ptr definition(const std::string& type) const override;

    //! Returns the node definitions that match the provided category.
    Ufe::NodeDefs definitions(const std::string& category) const override;

}; // UsdShaderNodeDefHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
