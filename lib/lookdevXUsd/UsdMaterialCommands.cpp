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
#include "UsdMaterialCommands.h"

#include <usdUfe/ufe/Utils.h>

#ifdef LDX_USD_USE_MAYAUSDAPI
#include <mayaUsdAPI/utils.h>
#endif

namespace LookdevXUsd
{

UsdCreateMaterialParentCommand::UsdCreateMaterialParentCommand(Ufe::SceneItem::Ptr ancestor)
    : m_ancestor(std::move(ancestor)), m_materialParent(nullptr), m_cmd(nullptr)
{
}

UsdCreateMaterialParentCommand::Ptr UsdCreateMaterialParentCommand::create(const Ufe::SceneItem::Ptr& ancestor)
{
    if (!ancestor)
    {
        return nullptr;
    }

    return std::make_shared<UsdCreateMaterialParentCommand>(ancestor);
}

Ufe::SceneItem::Ptr UsdCreateMaterialParentCommand::sceneItem() const
{
    return m_materialParent;
}

void UsdCreateMaterialParentCommand::execute()
{
    auto usdItem = UsdUfe::downcast(m_ancestor);
    auto prim = usdItem ? usdItem->prim() : PXR_NS::UsdPrim();
    if (!prim.IsValid())
    {
        return;
    }

    // If m_ancestor is a materials scope, it can be used as material parent.
    if (UsdUfe::isMaterialsScope(m_ancestor))
    {
        m_materialParent = m_ancestor;
        return;
    }

#ifdef LDX_USD_USE_MAYAUSDAPI
    // Create a materials scope.
    m_cmd = std::dynamic_pointer_cast<Ufe::SceneItemResultUndoableCommand>(
        MayaUsdAPI::createMaterialsScopeCommand(m_ancestor));
#endif
    if (!m_cmd)
    {
        return;
    }
    m_cmd->execute();
    m_materialParent = m_cmd->sceneItem();
}

void UsdCreateMaterialParentCommand::undo()
{
    if (m_cmd)
    {
        m_cmd->undo();
        m_materialParent.reset();
    }
}

void UsdCreateMaterialParentCommand::redo()
{
    if (m_cmd)
    {
        m_cmd->redo();
    }
}

} // namespace LookdevXUsd
