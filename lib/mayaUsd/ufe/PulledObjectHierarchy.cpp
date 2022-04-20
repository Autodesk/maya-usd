//
// Copyright 2021 Autodesk
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
#include "PulledObjectHierarchy.h"

#include <pxr/base/tf/diagnostic.h>

// Needed because of TF_CODING_ERROR
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// PulledObjectHierarchy
//------------------------------------------------------------------------------

PulledObjectHierarchy::PulledObjectHierarchy(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
    const Ufe::SceneItem::Ptr&        item,
    const Ufe::Path&                  pulledPath)
    : Ufe::Hierarchy()
    , _mayaHierarchy(mayaHierarchyHandler->hierarchy(item))
    , _pulledPath(pulledPath)
{
}

/*static*/
PulledObjectHierarchy::Ptr PulledObjectHierarchy::create(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
    const Ufe::SceneItem::Ptr&        item,
    const Ufe::Path&                  pulledPath)
{
    return std::make_shared<PulledObjectHierarchy>(mayaHierarchyHandler, item, pulledPath);
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr PulledObjectHierarchy::sceneItem() const { return _mayaHierarchy->sceneItem(); }

#if (UFE_PREVIEW_VERSION_NUM >= 4004)
bool PulledObjectHierarchy::hasFilteredChildren(const ChildFilter& childFilter) const
{
    return _mayaHierarchy->hasFilteredChildren(childFilter);
}
#endif

bool PulledObjectHierarchy::hasChildren() const { return _mayaHierarchy->hasChildren(); }

Ufe::SceneItemList PulledObjectHierarchy::children() const { return _mayaHierarchy->children(); }

Ufe::SceneItemList PulledObjectHierarchy::filteredChildren(const ChildFilter& childFilter) const
{
    return _mayaHierarchy->filteredChildren(childFilter);
}

Ufe::SceneItem::Ptr PulledObjectHierarchy::parent() const
{
    return Ufe::Hierarchy::createItem(_pulledPath.pop());
}

Ufe::InsertChildCommand::Ptr PulledObjectHierarchy::insertChildCmd(
    const Ufe::SceneItem::Ptr& child,
    const Ufe::SceneItem::Ptr& pos)
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::SceneItem::Ptr
PulledObjectHierarchy::insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::SceneItem::Ptr PulledObjectHierarchy::createGroup(const Ufe::PathComponent& name) const
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::InsertChildCommand::Ptr
PulledObjectHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::UndoableCommand::Ptr
PulledObjectHierarchy::reorderCmd(const Ufe::SceneItemList& orderedList) const
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::UndoableCommand::Ptr PulledObjectHierarchy::ungroupCmd() const
{
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

Ufe::SceneItem::Ptr PulledObjectHierarchy::defaultParent() const
{
    // Pulled objects cannot be unparented.
    TF_CODING_ERROR("Illegal call to unimplemented %s", __func__);
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
