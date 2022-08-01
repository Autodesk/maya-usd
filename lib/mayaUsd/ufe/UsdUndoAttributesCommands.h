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
#include <mayaUsd/ufe/UsdAttribute.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/attributes.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4008)
#include <ufe/nodeDef.h>
#endif
#endif

#include <unordered_map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of AddAttributeCommand
class UsdAddAttributeCommand : public UsdUndoableCommand<Ufe::AddAttributeCommand>
{
public:
    typedef std::shared_ptr<UsdAddAttributeCommand> Ptr;

    UsdAddAttributeCommand(
        const UsdSceneItem::Ptr&    sceneItem,
        const std::string&          name,
        const Ufe::Attribute::Type& type);
    ~UsdAddAttributeCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdAddAttributeCommand(const UsdAddAttributeCommand&) = delete;
    UsdAddAttributeCommand& operator=(const UsdAddAttributeCommand&) = delete;
    UsdAddAttributeCommand(UsdAddAttributeCommand&&) = delete;
    UsdAddAttributeCommand& operator=(UsdAddAttributeCommand&&) = delete;

    //! Create a UsdAddAttributeCommand
    static UsdAddAttributeCommand::Ptr create(
        const UsdSceneItem::Ptr&    sceneItem,
        const std::string&          name,
        const Ufe::Attribute::Type& type);

    Ufe::Attribute::Ptr attribute() const override;

    void executeUndoBlock() override;

private:
    const Ufe::Path            _sceneItemPath;
    const std::string          _name;
    const Ufe::Attribute::Type _type;
}; // UsdAddAttributeCommand

//! \brief Implementation of AddAttributeCommand
class UsdRemoveAttributeCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    typedef std::shared_ptr<UsdRemoveAttributeCommand> Ptr;

    UsdRemoveAttributeCommand(const UsdSceneItem::Ptr& sceneItem, const std::string& name);
    ~UsdRemoveAttributeCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdRemoveAttributeCommand(const UsdRemoveAttributeCommand&) = delete;
    UsdRemoveAttributeCommand& operator=(const UsdRemoveAttributeCommand&) = delete;
    UsdRemoveAttributeCommand(UsdRemoveAttributeCommand&&) = delete;
    UsdRemoveAttributeCommand& operator=(UsdRemoveAttributeCommand&&) = delete;

    //! Create a UsdRemoveAttributeCommand
    static UsdRemoveAttributeCommand::Ptr
    create(const UsdSceneItem::Ptr& sceneItem, const std::string& name);

    void executeUndoBlock() override;

private:
    const Ufe::Path   _sceneItemPath;
    const std::string _name;
}; // UsdRemoveAttributeCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
