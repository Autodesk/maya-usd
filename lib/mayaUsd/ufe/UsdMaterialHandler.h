// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdMaterial.h>

#include <ufe/materialHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time Material handler.
/*!
        Factory object for Material interfaces.
 */
class MAYAUSD_CORE_PUBLIC UsdMaterialHandler : public Ufe::MaterialHandler
{
public:
    typedef std::shared_ptr<UsdMaterialHandler> Ptr;

    UsdMaterialHandler() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdMaterialHandler);

    //! Create a UsdMaterialHandler.
    static UsdMaterialHandler::Ptr create();

    // UsdMaterialHandler overrides
    Ufe::Material::Ptr material(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdMaterialHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
