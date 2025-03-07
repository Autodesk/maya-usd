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
#pragma once

#include "Export.h"

#include <LookdevXUfe/LookdevHandler.h>

namespace LookdevXUsd
{
//! \brief USD run-time Lookdev handler.
/*!
        Factory object for Lookdev interfaces.
 */
class LOOKDEVX_USD_EXPORT UsdLookdevHandler : public LookdevXUfe::LookdevHandler
{
public:
    using Ptr = std::shared_ptr<UsdLookdevHandler>;

    static Ptr create()
    {
        return std::make_shared<UsdLookdevHandler>();
    }

    UsdLookdevHandler() = default;
    ~UsdLookdevHandler() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdLookdevHandler(const UsdLookdevHandler&) = delete;
    UsdLookdevHandler& operator=(const UsdLookdevHandler&) = delete;
    UsdLookdevHandler(UsdLookdevHandler&&) = delete;
    UsdLookdevHandler& operator=(UsdLookdevHandler&&) = delete;
    //@}

    // UsdLookdevHandler overrides
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevContainerCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevContainerCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::NodeDef::Ptr& nodeDef) const override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevEnvironmentCmdImpl(
        const Ufe::SceneItem::Ptr& ancestor, Ufe::Rtid targetRunTimeId) const override;
    [[nodiscard]] bool isLookdevContainerImpl(const Ufe::SceneItem::Ptr& item) const override;
};

} // namespace LookdevXUsd
