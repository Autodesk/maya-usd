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
#include "UsdUndoAttributesCommands.h"

#include <mayaUsd/ufe/UsdAttributes.h>

#include <ufe/hierarchy.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdAddAttributeCommand::UsdAddAttributeCommand(
    const UsdSceneItem::Ptr&    sceneItem,
    const std::string&          name,
    const Ufe::Attribute::Type& type)
#if (UFE_PREVIEW_VERSION_NUM >= 4033)
    : UsdUndoableCommand<Ufe::AddAttributeUndoableCommand>()
#else
    : UsdUndoableCommand<Ufe::AddAttributeCommand>()
#endif
    , _sceneItemPath(sceneItem->path())
    , _name(name)
    , _type(type)
{
}

UsdAddAttributeCommand::~UsdAddAttributeCommand() { }

UsdAddAttributeCommand::Ptr UsdAddAttributeCommand::create(
    const UsdSceneItem::Ptr&    sceneItem,
    const std::string&          name,
    const Ufe::Attribute::Type& type)
{
    if (UsdAttributes::canAddAttribute(sceneItem, type)) {
        return std::make_shared<UsdAddAttributeCommand>(sceneItem, name, type);
    }
    return nullptr;
}

Ufe::Attribute::Ptr UsdAddAttributeCommand::attribute() const
{
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    return UsdAttributes(sceneItem).attribute(_name);
}

void UsdAddAttributeCommand::setName(const std::string& newName) { _name = newName; }

void UsdAddAttributeCommand::executeUndoBlock()
{
    // Validation has already been done. Just create the attribute.
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    auto attrPtr = UsdAttributes::doAddAttribute(sceneItem, _name, _type);

    // Set the name, since it could have been changed in order to be unique.
    if (attrPtr) {
        setName(attrPtr->name());
    }
}

UsdRemoveAttributeCommand::UsdRemoveAttributeCommand(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       name)
    : UsdUndoableCommand<Ufe::UndoableCommand>()
    , _sceneItemPath(sceneItem->path())
    , _name(name)
{
}

UsdRemoveAttributeCommand::~UsdRemoveAttributeCommand() { }

UsdRemoveAttributeCommand::Ptr
UsdRemoveAttributeCommand::create(const UsdSceneItem::Ptr& sceneItem, const std::string& name)
{
    if (UsdAttributes::canRemoveAttribute(sceneItem, name)) {
        return std::make_shared<UsdRemoveAttributeCommand>(sceneItem, name);
    }
    return nullptr;
}

void UsdRemoveAttributeCommand::executeUndoBlock()
{
    // Validation has already been done. Just remove the attribute.
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    UsdAttributes::doRemoveAttribute(sceneItem, _name);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4033)
UsdRenameAttributeCommand::UsdRenameAttributeCommand(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       originalName,
    const std::string&       newName)
    : UsdUndoableCommand<Ufe::RenameAttributeUndoableCommand>()
    , _sceneItemPath(sceneItem->path())
    , _originalName(originalName)
    , _newName(newName)
{
}

UsdRenameAttributeCommand::~UsdRenameAttributeCommand() { }

UsdRenameAttributeCommand::Ptr UsdRenameAttributeCommand::create(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       originalName,
    const std::string&       newName)
{
    if (UsdAttributes::canRenameAttribute(sceneItem, originalName, newName)) {
        return std::make_shared<UsdRenameAttributeCommand>(sceneItem, originalName, newName);
    }

    return nullptr;
}

void UsdRenameAttributeCommand::executeUndoBlock()
{
    // Validation has already been done. Just rename the attribute.
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    auto renamedAttr = UsdAttributes::doRenameAttribute(sceneItem, _originalName, _newName);

    // Set the new name, since it could have been changed in order to be unique.
    if (renamedAttr) {
        setNewName(renamedAttr->name());
    }
}

Ufe::Attribute::Ptr UsdRenameAttributeCommand::attribute() const
{
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    return UsdAttributes(sceneItem).attribute(_newName);
}

void UsdRenameAttributeCommand::setNewName(const std::string& newName) { _newName = newName; };

#endif
#endif
} // namespace ufe
} // namespace MAYAUSD_NS_DEF
