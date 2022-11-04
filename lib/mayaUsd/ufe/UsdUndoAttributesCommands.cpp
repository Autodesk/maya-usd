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

#include "private/UfeNotifGuard.h"

#include <mayaUsd/ufe/UsdAttributes.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdAddAttributeCommand::UsdAddAttributeCommand(
    const UsdSceneItem::Ptr&    sceneItem,
    const std::string&          name,
    const Ufe::Attribute::Type& type)
    : UsdUndoableCommand<Ufe::AddAttributeCommand>()
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

#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4032)
std::string UsdAddAttributeCommand::commandString() const
{
    return std::string("AddAttribute ") + _name + " " + Ufe::PathString::string(_sceneItemPath);
}
#endif
#endif

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
#if (UFE_PREVIEW_VERSION_NUM >= 4032)
std::string UsdRemoveAttributeCommand::commandString() const
{
    return std::string("RemoveAttribute ") + _name + " " + Ufe::PathString::string(_sceneItemPath);
}
#endif
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
UsdSetMetadataCommand::UsdSetMetadataCommand(
    MayaUsd::ufe::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           newValue)
    : _attr(attr)
    , _key(key)
    , _newValue(newValue)
{
}

UsdSetMetadataCommand::~UsdSetMetadataCommand() { }

//! Create a UsdSetMetadataCommand
UsdSetMetadataCommand::Ptr UsdSetMetadataCommand::create(
    MayaUsd::ufe::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           newValue)
{
    return std::make_shared<UsdSetMetadataCommand>(attr, key, newValue);
}

void UsdSetMetadataCommand::executeUndoBlock()
{
    const MayaUsd::ufe::InAttributeMetadataChange ad;
#ifdef UFE_V4_FEATURES_AVAILABLE
    _attr._setMetadata(_key, _newValue);
#else
    _attr.setMetadata(_key, _newValue);
#endif
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
