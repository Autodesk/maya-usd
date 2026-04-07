// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#ifndef USDUFE_USDMATERIALHANDLER_H
#define USDUFE_USDMATERIALHANDLER_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdMaterial.h>

#include <ufe/materialHandler.h>

namespace USDUFE_NS_DEF {

//! \brief USD run-time Material handler.
/*!
        Factory object for Material interfaces.
 */
class USDUFE_PUBLIC UsdMaterialHandler : public Ufe::MaterialHandler
{
public:
    typedef std::shared_ptr<UsdMaterialHandler> Ptr;

    UsdMaterialHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdMaterialHandler);

    //! Create a UsdMaterialHandler.
    static UsdMaterialHandler::Ptr create();

    // UsdMaterialHandler overrides
    Ufe::Material::Ptr material(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdMaterialHandler

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDMATERIALHANDLER_H
