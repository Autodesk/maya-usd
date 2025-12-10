//*****************************************************************************
// Copyright (c) 2025 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#ifndef USD_MX_VERSION_UPGRADE_H
#define USD_MX_VERSION_UPGRADE_H

#ifdef LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION

#include "Export.h"

#include <ufe/undoableCommand.h>

#include <mayaUsdAPI/undo.h>

#include <ufe/path.h>

namespace LookdevXUsd::Version
{
using namespace PXR_NS;

//! \brief Checks if the given material element path belongs to a legacy MaterialX shader graph requiring upgrade.
//! \param materialElementPath The Ufe::Path of the material element to check.
//! \return An optional string containing the legacy MaterialX version if an upgrade is needed; std::nullopt otherwise.
std::optional<std::string> LOOKDEVX_USD_EXPORT isLegacyShaderGraph(const Ufe::Path& materialElementPath);

//! \brief Upgrades all UsdShade elements in the stage that use legacy MaterialX versions to the current version.
//! \param stagePath The path to UsdStage to upgrade.
void LOOKDEVX_USD_EXPORT UpgradeStage(const Ufe::Path& stagePath);

//! \brief Upgrades all UsdShade elements that use legacy MaterialX versions to the current version.
//! \param materialPath The path to UsdShadeMaterial to upgrade.
void LOOKDEVX_USD_EXPORT UpgradeMaterial(const Ufe::Path& materialPath);

class LOOKDEVX_USD_EXPORT UsdMxUpgradeMaterialCmd : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdMxUpgradeMaterialCmd>;

    static Ptr create(const Ufe::Path& materialPath);

    UsdMxUpgradeMaterialCmd(const Ufe::Path& materialPath);
    ~UsdMxUpgradeMaterialCmd() override;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMxUpgradeMaterialCmd(const UsdMxUpgradeMaterialCmd&) = delete;
    UsdMxUpgradeMaterialCmd& operator=(const UsdMxUpgradeMaterialCmd&) = delete;
    UsdMxUpgradeMaterialCmd(UsdMxUpgradeMaterialCmd&&) = delete;
    UsdMxUpgradeMaterialCmd& operator=(UsdMxUpgradeMaterialCmd&&) = delete;
    //@}

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "MaterialXUpgradeMaterial"; })

    private:

    Ufe::Path _materialPath;
    MayaUsdAPI::UsdUndoableItem   _undoableItem;

};

class LOOKDEVX_USD_EXPORT UsdMxUpgradeStageCmd : public Ufe::CompositeUndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdMxUpgradeStageCmd>;

    static Ptr create(const Ufe::Path& stagePath);

    UsdMxUpgradeStageCmd(const Ufe::Path& stagePath);
    ~UsdMxUpgradeStageCmd() override;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMxUpgradeStageCmd(const UsdMxUpgradeStageCmd&) = delete;
    UsdMxUpgradeStageCmd& operator=(const UsdMxUpgradeStageCmd&) = delete;
    UsdMxUpgradeStageCmd(UsdMxUpgradeStageCmd&&) = delete;
    UsdMxUpgradeStageCmd& operator=(UsdMxUpgradeStageCmd&&) = delete;
    //@}

    UFE_V4(std::string commandString() const override { return "MaterialXUpgradeStage"; })
};


} // namespace LookdevXUsd

#endif // LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
#endif // USD_MX_VERSION_UPGRADE_H
