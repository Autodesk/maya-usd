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
#pragma once

#include <mayaUsd/base/api.h>

#include <ufe/hierarchy.h>
#include <ufe/hierarchyHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya hierarchy interface for pulled Maya objects.
/*!
    See PulledObjectHierarchyHandler for pulled object data model details.
    The hierarchy interface of the pulled sub-hierarchy root is a
    PulledObjectHierarchy object.  Its children are the normal Maya children;
    its parent is the USD parent of the pulled prim.  This allows the pulled
    Maya data to respond correctly to hierarchy viewing (e.g. the Outliner) and
    navigation (e.g. pick walking).
 */
class MAYAUSD_CORE_PUBLIC PulledObjectHierarchy : public Ufe::Hierarchy
{
public:
    typedef std::shared_ptr<PulledObjectHierarchy> Ptr;

    PulledObjectHierarchy(
        const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
        const Ufe::SceneItem::Ptr&        item,
        const Ufe::Path&                  pulledPath);

    // Delete the copy/move constructors assignment operators.
    PulledObjectHierarchy(const PulledObjectHierarchy&) = delete;
    PulledObjectHierarchy& operator=(const PulledObjectHierarchy&) = delete;
    PulledObjectHierarchy(PulledObjectHierarchy&&) = delete;
    PulledObjectHierarchy& operator=(PulledObjectHierarchy&&) = delete;

    //! Create a PulledObjectHierarchy from a UFE hierarchy handler.
    static PulledObjectHierarchy::Ptr create(
        const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
        const Ufe::SceneItem::Ptr&        item,
        const Ufe::Path&                  pulledPath);

    // Ufe::Hierarchy overrides
    Ufe::SceneItem::Ptr sceneItem() const override;
    bool                hasChildren() const override;
    Ufe::SceneItemList  children() const override;
    Ufe::SceneItemList  filteredChildren(const ChildFilter&) const override;
    Ufe::SceneItem::Ptr parent() const override;

    Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override;

    Ufe::InsertChildCommand::Ptr createGroupCmd(const Ufe::PathComponent& name) const override;

    Ufe::SceneItem::Ptr defaultParent() const override;

    Ufe::SceneItem::Ptr
    insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;
    Ufe::InsertChildCommand::Ptr
    insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;

    Ufe::UndoableCommand::Ptr reorderCmd(const Ufe::SceneItemList& orderedList) const override;

    Ufe::UndoableCommand::Ptr ungroupCmd() const override;

private:
    Hierarchy::Ptr             _mayaHierarchy;
    Ufe::Path                  _pulledPath;
}; // PulledObjectHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
