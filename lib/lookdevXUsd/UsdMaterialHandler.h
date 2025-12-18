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

#include <LookdevXUfe/Material.h>
#include <LookdevXUfe/MaterialHandler.h>

namespace LookdevXUsd
{
//! \brief USD run-time Material handler.
/*!
        Factory object for Material interfaces.
 */
class LOOKDEVX_USD_EXPORT UsdMaterialHandler : public LookdevXUfe::MaterialHandler
{
public:
    typedef std::shared_ptr<UsdMaterialHandler> Ptr;

    static LookdevXUfe::MaterialHandler::Ptr create()
    {
        return std::make_shared<UsdMaterialHandler>();
    }

    UsdMaterialHandler();
    ~UsdMaterialHandler() override;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMaterialHandler(const UsdMaterialHandler&) = delete;
    UsdMaterialHandler& operator=(const UsdMaterialHandler&) = delete;
    UsdMaterialHandler(UsdMaterialHandler&&) = delete;
    UsdMaterialHandler& operator=(UsdMaterialHandler&&) = delete;
    //@}

    // UsdMaterialHandler overrides
    LookdevXUfe::Material::Ptr material(const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] LookdevXUfe::ValidationLog::Ptr validateMaterial(const Ufe::SceneItem::Ptr& material) const override;

    [[nodiscard]] bool allowedInNodeGraph(const std::string& nodeDefType) const override;

protected:
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createBackdropCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createNodeGraphCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;
    [[nodiscard]] bool isBackdropImpl(const Ufe::SceneItem::Ptr& item) const override;
    [[nodiscard]] bool isNodeGraphImpl(const Ufe::SceneItem::Ptr& item) const override;
    [[nodiscard]] bool isMaterialImpl(const Ufe::SceneItem::Ptr& item) const override;
    [[nodiscard]] bool isShaderImpl(const Ufe::SceneItem::Ptr& item) const override;
#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
    [[nodiscard]] std::optional<std::string> isLegacyShaderGraph(
        const Ufe::SceneItem::Ptr& graphElement) const override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr upgradeLegacyShaderGraphCmd(
        const Ufe::SceneItem::Ptr& graphElement) const override;
#endif
};

} // namespace LookdevXUsd
