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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

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
    ~ProxyShapeHierarchy() override;

    // Delete the copy/move constructors assignment operators.
    ProxyShapeHierarchy(const ProxyShapeHierarchy&) = delete;
    ProxyShapeHierarchy& operator=(const ProxyShapeHierarchy&) = delete;
    ProxyShapeHierarchy(ProxyShapeHierarchy&&) = delete;
    ProxyShapeHierarchy& operator=(ProxyShapeHierarchy&&) = delete;

    //! Create a ProxyShapeHierarchy from a UFE hierarchy handler.
    static ProxyShapeHierarchy::Ptr create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);
    static ProxyShapeHierarchy::Ptr
    create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler, const Ufe::SceneItem::Ptr& item);

    void setItem(const Ufe::SceneItem::Ptr& item);

    // Ufe::Hierarchy overrides
    Ufe::SceneItem::Ptr sceneItem() const override;
    bool                hasChildren() const override;
    Ufe::SceneItemList  children() const override;
    UFE_V2(Ufe::SceneItemList filteredChildren(const ChildFilter&) const override;)
    Ufe::SceneItem::Ptr parent() const override;
#ifndef UFE_V2_FEATURES_AVAILABLE
    Ufe::AppendedChild appendChild(const Ufe::SceneItem::Ptr& child) override;
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::SceneItem::Ptr
    createGroup(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;

    Ufe::SceneItem::Ptr defaultParent() const override;

    Ufe::SceneItem::Ptr
    insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;
    Ufe::InsertChildCommand::Ptr
    insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;

    Ufe::UndoableCommand::Ptr reorderCmd(const Ufe::SceneItemList& orderedList) const override;
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 3001)
    Ufe::InsertChildCommand::Ptr
#else
    Ufe::UndoableCommand::Ptr
#endif
    createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;

#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::UndoableCommand::Ptr ungroupCmd() const override;
#endif

private:
    const PXR_NS::UsdPrim& getUsdRootPrim() const;
    Ufe::SceneItemList     createUFEChildList(const PXR_NS::UsdPrimSiblingRange& range) const;

private:
    Ufe::SceneItem::Ptr        fItem;
    Hierarchy::Ptr             fMayaHierarchy;
    Ufe::HierarchyHandler::Ptr fMayaHierarchyHandler;

    // The root prim is initialized on first use and therefore mutable.
    mutable PXR_NS::UsdPrim fUsdRootPrim;
}; // ProxyShapeHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
