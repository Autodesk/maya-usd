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
#include <mayaUsd/undo/UsdUndoBlock.h>

#include <ufe/hierarchy.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdAddAttributeCommand::UsdAddAttributeCommand(
    const UsdSceneItem::Ptr&    sceneItem,
    const std::string&          name,
    const Ufe::Attribute::Type& type)
    : Ufe::AddAttributeCommand()
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
    if (UsdAttributes::canAddAttribute(sceneItem, name, type)) {
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

void UsdAddAttributeCommand::execute()
{
    // Gather undo data:
    UsdUndoBlock undoBlock(&_undoableItem);

    // Validation has already been done. Just create the attribute.
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    UsdAttributes::doAddAttribute(sceneItem, _name, _type);
}

void UsdAddAttributeCommand::undo() { _undoableItem.undo(); }

void UsdAddAttributeCommand::redo() { _undoableItem.redo(); }

UsdRemoveAttributeCommand::UsdRemoveAttributeCommand(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       name)
    : Ufe::UndoableCommand()
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

void UsdRemoveAttributeCommand::execute()
{
    // Gather undo data:
    UsdUndoBlock undoBlock(&_undoableItem);

    // Validation has already been done. Just remove the attribute.
    auto sceneItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_sceneItemPath));
    UsdAttributes::doRemoveAttribute(sceneItem, _name);
}

void UsdRemoveAttributeCommand::undo() { _undoableItem.undo(); }

void UsdRemoveAttributeCommand::redo() { _undoableItem.redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
