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

#include <mayaUsdAPI/undo.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace LookdevXUsd::Version {
using namespace PXR_NS;

//! \brief Checks if the given UsdShadeMaterial path belongs to a legacy MaterialX shader graph
//! requiring upgrade. \param materialPath The Ufe::Path of the UsdShadeMaterial to check.
//! \return An optional string containing the legacy MaterialX version if an upgrade is needed;
//! std::nullopt otherwise.
std::optional<std::string>
    LOOKDEVX_USD_EXPORT isLegacyShaderGraph(const Ufe::Path& materialPath);

//! \brief Upgrades all UsdShade nodes that use legacy MaterialX versions to the current version.
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
    Ufe::Path                   _materialPath;
    MayaUsdAPI::UsdUndoableItem _undoableItem;
};

} // namespace LookdevXUsd::Version

#endif // LOOKDEVXUFE_HAS_LEGACY_MTLX_DETECTION
#endif // USD_MX_VERSION_UPGRADE_H
