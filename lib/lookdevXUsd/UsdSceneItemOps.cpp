//**************************************************************************/
// Copyright 2024 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
#include "UsdSceneItemOps.h"
#include "UsdDeleteCommand.h"

#include <mayaUsdAPI/utils.h>

#include <pxr/base/tf/diagnostic.h>

using namespace PXR_NS;

// Note: Some of the functions are not covered by unit tests because they are simple wrappers around the
// MayaUsd::ufe::UsdSceneItemOps functions. If we add our own logic to these functions, we should add unit tests for
// them.
namespace LookdevXUsd
{
/*static*/
UsdSceneItemOps::Ptr UsdSceneItemOps::create(Ufe::SceneItemOps::Ptr wrappedMayaUsdSceneItemOps)
{
    return std::make_shared<UsdSceneItemOps>(wrappedMayaUsdSceneItemOps);
}

Ufe::SceneItem::Ptr UsdSceneItemOps::sceneItem() const
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->sceneItem();
    // LCOV_EXCL_STOP
}

Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmdNoExecute()
{
    assert(m_wrappedMayaUsdSceneItemOps);

    // Wrap the MayaUsd::ufe::UsdUndoDeleteCommand
    return UsdDeleteCommand::create(m_wrappedMayaUsdSceneItemOps->deleteItemCmdNoExecute(),
                                    m_wrappedMayaUsdSceneItemOps->sceneItem());
}

Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmd()
{
    auto deleteCmd = deleteItemCmdNoExecute();
    deleteCmd->execute();
    return deleteCmd;
}

bool UsdSceneItemOps::deleteItem()
{
    auto prim = [this]() {
        if (!TF_VERIFY(MayaUsdAPI::isUsdSceneItem(m_wrappedMayaUsdSceneItemOps->sceneItem()), "Invalid item\n"))
        {
            return PXR_NS::UsdPrim();
        }
        return MayaUsdAPI::getPrimForUsdSceneItem(m_wrappedMayaUsdSceneItemOps->sceneItem());
    };

    // Same check as in MayaUsd::ufe::UsdSceneItemOps::deleteItem()
    if (prim())
    {
        auto deleteCmd = deleteItemCmd();
        if (deleteCmd)
        {
            return true;
        }
    }

    return false;
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdSceneItemOps::duplicateItemCmdNoExecute()
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->duplicateItemCmdNoExecute();
    // LCOV_EXCL_STOP
}

Ufe::Duplicate UsdSceneItemOps::duplicateItemCmd()
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->duplicateItemCmd();
    // LCOV_EXCL_STOP
}

Ufe::SceneItem::Ptr UsdSceneItemOps::duplicateItem()
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->duplicateItem();
    // LCOV_EXCL_STOP
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdSceneItemOps::renameItemCmdNoExecute(const Ufe::PathComponent& newName)
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->renameItemCmdNoExecute(newName);
    // LCOV_EXCL_STOP
}

Ufe::Rename UsdSceneItemOps::renameItemCmd(const Ufe::PathComponent& newName)
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->renameItemCmd(newName);
    // LCOV_EXCL_STOP
}

Ufe::SceneItem::Ptr UsdSceneItemOps::renameItem(const Ufe::PathComponent& newName)
{
    // LCOV_EXCL_START
    assert(m_wrappedMayaUsdSceneItemOps);
    return m_wrappedMayaUsdSceneItemOps->renameItem(newName);
    // LCOV_EXCL_STOP
}
} // namespace LookdevXUsd
