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
#include <ufe/path.h>
#include <ufe/selection.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface
/*!
    This class implements the hierarchy interface for normal USD prims, using
    standard USD calls to obtain a prim's parent and children.
*/
class MAYAUSD_CORE_PUBLIC UsdHierarchy : public Ufe::Hierarchy
{
public:
    typedef std::shared_ptr<UsdHierarchy> Ptr;

    UsdHierarchy(const UsdSceneItem::Ptr& item);
    ~UsdHierarchy() override;

    // Delete the copy/move constructors assignment operators.
    UsdHierarchy(const UsdHierarchy&) = delete;
    UsdHierarchy& operator=(const UsdHierarchy&) = delete;
    UsdHierarchy(UsdHierarchy&&) = delete;
    UsdHierarchy& operator=(UsdHierarchy&&) = delete;

    //! Create a UsdHierarchy.
    static UsdHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

    void                   setItem(const UsdSceneItem::Ptr& item);
    const Ufe::Path&       path() const;
    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_AXIOM(fItem != nullptr);
        return fItem->prim();
    }

    UsdSceneItem::Ptr usdSceneItem() const;

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

#if (UFE_PREVIEW_VERSION_NUM >= 3005)
    Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override;
#else
    Ufe::SceneItem::Ptr
    createGroup(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 3001)
    Ufe::InsertChildCommand::Ptr
#else
    Ufe::UndoableCommand::Ptr
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 3005)
    createGroupCmd(const Ufe::PathComponent& name) const override;
#else
    createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const override;
#endif

    Ufe::SceneItem::Ptr defaultParent() const override;

    Ufe::SceneItem::Ptr
    insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;
    Ufe::InsertChildCommand::Ptr
    insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos) override;

    Ufe::UndoableCommand::Ptr reorderCmd(const Ufe::SceneItemList& orderedList) const override;
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::UndoableCommand::Ptr ungroupCmd() const override;
#endif

private:
    Ufe::SceneItemList createUFEChildList(const PXR_NS::UsdPrimSiblingRange& range) const;

private:
    UsdSceneItem::Ptr fItem;

}; // UsdHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
