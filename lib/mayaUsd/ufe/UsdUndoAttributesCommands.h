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
#ifndef USD_ATTRIBUTES_COMMANDS
#define USD_ATTRIBUTES_COMMANDS

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdAttribute.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/attributes.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4010)
#include <ufe/nodeDef.h>
#endif
#endif

#include <unordered_map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of AddAttributeCommand
class UsdAddAttributeCommand
    :
#if (UFE_PREVIEW_VERSION_NUM >= 4034)
    public UsdUndoableCommand<Ufe::AddAttributeUndoableCommand>
#else
    public UsdUndoableCommand<Ufe::AddAttributeCommand>
#endif
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

#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4032)
    std::string commandString() const override;
#endif
#endif

private:
    const Ufe::Path            _sceneItemPath;
    std::string                _name;
    const Ufe::Attribute::Type _type;

    void setName(const std::string& newName);

}; // UsdAddAttributeCommand

//! \brief Implementation of RemoveAttributeCommand
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

#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4032)
    std::string commandString() const override;
#endif
#endif

private:
    const Ufe::Path   _sceneItemPath;
    const std::string _name;
}; // UsdRemoveAttributeCommand

#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4034)
//! \brief Implementation of RenameAttributeCommand
class UsdRenameAttributeCommand : public UsdUndoableCommand<Ufe::RenameAttributeUndoableCommand>
{
public:
    typedef std::shared_ptr<UsdRenameAttributeCommand> Ptr;

    UsdRenameAttributeCommand(
        const UsdSceneItem::Ptr& sceneItem,
        const std::string&       originalName,
        const std::string&       newName);
    ~UsdRenameAttributeCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdRenameAttributeCommand(const UsdRenameAttributeCommand&) = delete;
    UsdRenameAttributeCommand& operator=(const UsdRenameAttributeCommand&) = delete;
    UsdRenameAttributeCommand(UsdRenameAttributeCommand&&) = delete;
    UsdRenameAttributeCommand& operator=(UsdRenameAttributeCommand&&) = delete;

    //! Create a UsdRenameAttributeCommand
    static UsdRenameAttributeCommand::Ptr create(
        const UsdSceneItem::Ptr& sceneItem,
        const std::string&       originalName,
        const std::string&       newName);

    void executeUndoBlock() override;

    Ufe::Attribute::Ptr attribute() const override;

private:
    const Ufe::Path   _sceneItemPath;
    const std::string _originalName;
    std::string       _newName;

    void setNewName(const std::string& newName);

}; // UsdRenameAttributeCommand

#endif
#endif
} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
