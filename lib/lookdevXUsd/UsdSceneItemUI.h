//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************

#ifndef USD_SCENE_ITEM_UI_H
#define USD_SCENE_ITEM_UI_H

#include <LookdevXUfe/SceneItemUI.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/sceneItem.h>

namespace LookdevXUsd
{

//! \brief USD run-time Scene Item UI interface
/*!
    This class implements the Scene Item UI interface for USD prims.
*/
class UsdSceneItemUI : public LookdevXUfe::SceneItemUI
{
public:
    using Ptr = std::shared_ptr<UsdSceneItemUI>;

    explicit UsdSceneItemUI(Ufe::SceneItem::Ptr item, const PXR_NS::UsdPrim& prim);
    ~UsdSceneItemUI() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdSceneItemUI(const UsdSceneItemUI&) = delete;
    UsdSceneItemUI& operator=(const UsdSceneItemUI&) = delete;
    UsdSceneItemUI(UsdSceneItemUI&&) = delete;
    UsdSceneItemUI& operator=(UsdSceneItemUI&&) = delete;

    //! Create a UsdSceneItemUI.
    static UsdSceneItemUI::Ptr create(const Ufe::SceneItem::Ptr& item);

    // Ufe::SceneItemUI overrides
    Ufe::SceneItem::Ptr sceneItem() const override;
    bool hidden() const override;
    Ufe::UndoableCommand::Ptr setHiddenCmd(bool hidden) override;

private:
    Ufe::SceneItem::Ptr m_item;
    const PXR_NS::UsdStageWeakPtr m_stage;
    const PXR_NS::SdfPath m_path;
};

} // namespace LookdevXUsd

#endif
