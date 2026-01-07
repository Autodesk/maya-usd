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
#ifndef USDUFE_USDHIERARCHY_H
#define USDUFE_USDHIERARCHY_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/primFlags.h>

#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/selection.h>

namespace USDUFE_NS_DEF {

//! \brief USD run-time hierarchy interface
/*!
    This class implements the hierarchy interface for normal USD prims, using
    standard USD calls to obtain a prim's parent and children.
*/
class USDUFE_PUBLIC UsdHierarchy : public Ufe::Hierarchy
{
public:
    typedef std::shared_ptr<UsdHierarchy> Ptr;

    UsdHierarchy(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdHierarchy);

    //! Create a UsdHierarchy.
    static UsdHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

    void                   setItem(const UsdSceneItem::Ptr& item);
    const Ufe::Path&       path() const;
    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(_item != nullptr))
            return _item->prim();
        else
            return PXR_NS::UsdPrim();
    }

    UsdSceneItem::Ptr usdSceneItem() const;

    static PXR_NS::Usd_PrimFlagsConjunction usdUfePrimDefaultPredicate();

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

protected:
    //! Called from createUFEChildList() to allow a derived class to process the
    //! child prim and modify the children list for that child.
    //! \return True if the child was processed (createUFEChildList will skip it).
    virtual bool childrenHook(
        const PXR_NS::UsdPrim& child,
        Ufe::SceneItemList&    children,
        bool                   filterInactive) const;

private:
    Ufe::SceneItemList
    createUFEChildList(const PXR_NS::UsdPrimSiblingRange& range, bool filterInactive) const;

private:
    UsdSceneItem::Ptr _item;

}; // UsdHierarchy

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDHIERARCHY_H
