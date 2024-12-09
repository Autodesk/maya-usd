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

#include <ufe/sceneItem.h>

namespace LookdevXUsd
{

//! \brief USD run-time material interface
/*!
    This class implements the Material interface for USD prims.
*/
class LOOKDEVX_USD_EXPORT UsdMaterial : public LookdevXUfe::Material
{
public:
    using Ptr = std::shared_ptr<UsdMaterial>;

    explicit UsdMaterial(Ufe::SceneItem::Ptr item);
    ~UsdMaterial() override;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMaterial(const UsdMaterial&) = delete;
    UsdMaterial& operator=(const UsdMaterial&) = delete;
    UsdMaterial(UsdMaterial&&) = delete;
    UsdMaterial& operator=(UsdMaterial&&) = delete;
    //@}

    //! Create a UsdMaterial.
    static UsdMaterial::Ptr create(const Ufe::SceneItem::Ptr& item);

    [[nodiscard]] std::vector<Ufe::SceneItem::Ptr> getMaterials() const override;

    [[nodiscard]] bool hasMaterial() const override;

private:
    Ufe::SceneItem::Ptr m_item;
}; // UsdMaterial

} // namespace LookdevXUsd
