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

#include <LookdevXUfe/SoloingHandler.h>

#include <memory>

namespace LookdevXUsd
{

class UsdSoloingHandler : public LookdevXUfe::SoloingHandler
{
public:
    UsdSoloingHandler();
    ~UsdSoloingHandler() override;

    UsdSoloingHandler(const UsdSoloingHandler&) = delete;
    UsdSoloingHandler& operator=(const UsdSoloingHandler&) = delete;
    UsdSoloingHandler(UsdSoloingHandler&&) = delete;
    UsdSoloingHandler& operator=(UsdSoloingHandler&&) = delete;

    static LookdevXUfe::SoloingHandler::Ptr create();

    [[nodiscard]] bool isSoloable(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] bool isSoloable(const Ufe::Attribute::Ptr& attr) const override;

    [[nodiscard]] Ufe::UndoableCommand::Ptr soloCmd(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] Ufe::UndoableCommand::Ptr soloCmd(const Ufe::Attribute::Ptr& attr) const override;

    [[nodiscard]] Ufe::UndoableCommand::Ptr unsoloCmd(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] bool isSoloed(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] bool isSoloed(const Ufe::Attribute::Ptr& attr) const override;

    [[nodiscard]] Ufe::SceneItem::Ptr getSoloedItem(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] bool hasSoloedDescendant(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] Ufe::Attribute::Ptr getSoloedAttribute(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] bool isSoloingItem(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] Ufe::Connection::Ptr replacedConnection(const Ufe::SceneItem::Ptr& item) const override;
};

} // namespace LookdevXUsd
