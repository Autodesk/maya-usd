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

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/attributes.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <ufe/nodeDef.h>
#endif

#include <unordered_map>

namespace USDUFE_NS_DEF {

//! \brief Implementation of AddAttributeCommand
class UsdAddAttributeCommand
    :
#ifdef UFE_V4_FEATURES_AVAILABLE
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

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdAddAttributeCommand);

    //! Create a UsdAddAttributeCommand
    static UsdAddAttributeCommand::Ptr create(
        const UsdSceneItem::Ptr&    sceneItem,
        const std::string&          name,
        const Ufe::Attribute::Type& type);

    Ufe::Attribute::Ptr attribute() const override;

    void executeImplementation() override;

    UFE_V4(std::string commandString() const override;)

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

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdRemoveAttributeCommand);

    //! Create a UsdRemoveAttributeCommand
    static UsdRemoveAttributeCommand::Ptr
    create(const UsdSceneItem::Ptr& sceneItem, const std::string& name);

    void executeImplementation() override;

    UFE_V4(std::string commandString() const override;)

private:
    const Ufe::Path   _sceneItemPath;
    const std::string _name;
}; // UsdRemoveAttributeCommand

#ifdef UFE_V4_FEATURES_AVAILABLE
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

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdRenameAttributeCommand);

    //! Create a UsdRenameAttributeCommand
    static UsdRenameAttributeCommand::Ptr create(
        const UsdSceneItem::Ptr& sceneItem,
        const std::string&       originalName,
        const std::string&       newName);

    void executeImplementation() override;

    Ufe::Attribute::Ptr attribute() const override;

private:
    const Ufe::Path   _sceneItemPath;
    const std::string _originalName;
    std::string       _newName;

    void setNewName(const std::string& newName);

}; // UsdRenameAttributeCommand

#endif

} // namespace USDUFE_NS_DEF

#endif
