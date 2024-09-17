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
#ifndef MAYAUSD_PROXYSHAPEHIERARCHY_H
#define MAYAUSD_PROXYSHAPEHIERARCHY_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/hierarchy.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/selection.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD gateway node hierarchy interface.
/*!
    This class defines a hierarchy interface for a single kind of Maya node,
    the USD gateway node.  This node is special in that its parent is a Maya
    node, but its children are children of the USD root prim.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHierarchy : public Ufe::Hierarchy
{
public:
    typedef std::shared_ptr<ProxyShapeHierarchy> Ptr;

    ProxyShapeHierarchy(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(ProxyShapeHierarchy);

    //! Create a ProxyShapeHierarchy from a UFE hierarchy handler.
    static ProxyShapeHierarchy::Ptr create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);
    static ProxyShapeHierarchy::Ptr
    create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler, const Ufe::SceneItem::Ptr& item);

    void setItem(const Ufe::SceneItem::Ptr& item);

    // Ufe::Hierarchy overrides
    Ufe::SceneItem::Ptr sceneItem() const override;
    bool                hasChildren() const override;
    Ufe::SceneItemList  children() const override;
    UFE_V4(bool hasFilteredChildren(const ChildFilter&) const override;)
    Ufe::SceneItemList  filteredChildren(const ChildFilter&) const override;
    Ufe::SceneItem::Ptr parent() const override;

#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::SceneItem::Ptr          createGroup(const Ufe::PathComponent& name) const override;
    Ufe::InsertChildCommand::Ptr createGroupCmd(const Ufe::PathComponent& name) const override;
#else
    Ufe::SceneItem::Ptr
    createGroup(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;
    Ufe::UndoableCommand::Ptr
    createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;
#endif

    Ufe::SceneItem::Ptr defaultParent() const override;

    Ufe::SceneItem::Ptr
    insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;
    Ufe::InsertChildCommand::Ptr
    insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;

    Ufe::UndoableCommand::Ptr reorderCmd(const Ufe::SceneItemList& orderedList) const override;

#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::UndoableCommand::Ptr ungroupCmd() const override;
#endif

private:
    const PXR_NS::UsdPrim& getUsdRootPrim() const;
    Ufe::SceneItemList
    createUFEChildList(const PXR_NS::UsdPrimSiblingRange& range, bool filterInactive) const;

private:
    Ufe::SceneItem::Ptr        _item;
    Hierarchy::Ptr             _mayaHierarchy;
    Ufe::HierarchyHandler::Ptr _mayaHierarchyHandler;

    // The root prim is initialized on first use and therefore mutable.
    mutable PXR_NS::UsdPrim _usdRootPrim;
}; // ProxyShapeHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_PROXYSHAPEHIERARCHY_H
