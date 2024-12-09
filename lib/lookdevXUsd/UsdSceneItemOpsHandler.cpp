//**************************************************************************/
// Copyright 2024 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
#include "UsdSceneItemOpsHandler.h"
#include "UsdSceneItemOps.h"

#include <ufe/runTimeMgr.h>

namespace LookdevXUsd
{
UsdSceneItemOpsHandler::UsdSceneItemOpsHandler(Ufe::SceneItemOpsHandler::Ptr mayaUsdSceneItemOpsHandler)
    : m_mayaUsdSceneItemOpsHandler(std::move(mayaUsdSceneItemOpsHandler))
{
}

/*static*/
UsdSceneItemOpsHandler::Ptr UsdSceneItemOpsHandler::create(
    const Ufe::SceneItemOpsHandler::Ptr& mayaUsdSceneItemOpsHandler)
{
    return std::make_shared<UsdSceneItemOpsHandler>(mayaUsdSceneItemOpsHandler);
}

//------------------------------------------------------------------------------
// SceneItemOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemOps::Ptr UsdSceneItemOpsHandler::sceneItemOps(const Ufe::SceneItem::Ptr& item) const
{
    // Get the sceneItemOps interface from the next handler.
    const auto nextSceneItemOps = m_mayaUsdSceneItemOpsHandler->sceneItemOps(item);

    // Wrap it up inside our decorator sceneItemOps.
    return UsdSceneItemOps::create(nextSceneItemOps);
}

} // namespace LookdevXUsd