//
// Copyright 2025 Autodesk
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
#ifndef USD_USDSCENEITEMOPS_H
#define USD_USDSCENEITEMOPS_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/path.h>
#include <ufe/sceneItemOps.h>

namespace USDUFE_NS_DEF {

//! \brief Interface for scene item operations.
class USDUFE_PUBLIC UsdSceneItemOps : public Ufe::SceneItemOps
{
public:
    typedef std::shared_ptr<UsdSceneItemOps> Ptr;

    UsdSceneItemOps(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdSceneItemOps);

    //! Create a UsdSceneItemOps.
    static UsdSceneItemOps::Ptr create(const UsdSceneItem::Ptr& item);

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
    UsdSceneItem::Ptr usdSceneItem() const { return _item; }

    //@{
    // Ufe::SceneItemOps overrides
    Ufe::SceneItem::Ptr       sceneItem() const override;
    Ufe::UndoableCommand::Ptr deleteItemCmd() override;
    bool                      deleteItem() override;
    Ufe::Duplicate            duplicateItemCmd() override;
    Ufe::SceneItem::Ptr       duplicateItem() override;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::UndoableCommand::Ptr                deleteItemCmdNoExecute() override;
    Ufe::SceneItemResultUndoableCommand::Ptr duplicateItemCmdNoExecute() override;
    Ufe::SceneItemResultUndoableCommand::Ptr
    renameItemCmdNoExecute(const Ufe::PathComponent& newName) override;
#endif
    Ufe::Rename         renameItemCmd(const Ufe::PathComponent& newName) override;
    Ufe::SceneItem::Ptr renameItem(const Ufe::PathComponent& newName) override;
    //@}

private:
    UsdSceneItem::Ptr _item;

}; // UsdSceneItemOps

} // namespace USDUFE_NS_DEF

#endif // USD_USDSCENEITEMOPS_H
