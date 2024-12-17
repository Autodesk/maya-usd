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

#include "UsdSceneItemUIHandler.h"
#include "UsdSceneItemUI.h"

#include <mayaUsdAPI/utils.h>

namespace LookdevXUsd
{

/*static*/
UsdSceneItemUIHandler::Ptr UsdSceneItemUIHandler::create()
{
    return std::make_shared<UsdSceneItemUIHandler>();
}

//------------------------------------------------------------------------------
// UsdSceneItemUIHandler overrides
//------------------------------------------------------------------------------

LookdevXUfe::SceneItemUI::Ptr UsdSceneItemUIHandler::sceneItemUI(const Ufe::SceneItem::Ptr& item) const
{
#if !defined(NDEBUG)
    assert(MayaUsdAPI::isUsdSceneItem(item));
#endif

    return LookdevXUsd::UsdSceneItemUI::create(item);
}

} // namespace LookdevXUsd
