//*****************************************************************************
// Copyright (c) 2023 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************

#ifndef USD_SCENE_ITEM_UI_HANDLER_H
#define USD_SCENE_ITEM_UI_HANDLER_H

#include <LookdevXUfe/SceneItemUIHandler.h>

namespace LookdevXUsd
{

//! \brief USD run-time Scene Item UI handler.
/*!
        Factory object for Scene Item UI interfaces.
 */
class UsdSceneItemUIHandler : public LookdevXUfe::SceneItemUIHandler
{
public:
    using Ptr = std::shared_ptr<UsdSceneItemUIHandler>;

    UsdSceneItemUIHandler() = default;
    ~UsdSceneItemUIHandler() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdSceneItemUIHandler(const UsdSceneItemUIHandler&) = delete;
    UsdSceneItemUIHandler& operator=(const UsdSceneItemUIHandler&) = delete;
    UsdSceneItemUIHandler(UsdSceneItemUIHandler&&) = delete;
    UsdSceneItemUIHandler& operator=(UsdSceneItemUIHandler&&) = delete;

    //! Create a UsdSceneItemUIHandler.
    static UsdSceneItemUIHandler::Ptr create();

    // UsdSceneItemUIHandler overrides
    [[nodiscard]] LookdevXUfe::SceneItemUI::Ptr sceneItemUI(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdSceneItemUIHandler

} // namespace LookdevXUsd

#endif
