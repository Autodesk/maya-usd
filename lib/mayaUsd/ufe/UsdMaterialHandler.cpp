// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdMaterialHandler.h"

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usdGeom/imageable.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdMaterialHandler::UsdMaterialHandler()
    : Ufe::MaterialHandler()
{
}

UsdMaterialHandler::~UsdMaterialHandler() { }

/*static*/
UsdMaterialHandler::Ptr UsdMaterialHandler::create()
{
    return std::make_shared<UsdMaterialHandler>();
}

//------------------------------------------------------------------------------
// UsdMaterialHandler overrides
//------------------------------------------------------------------------------

Ufe::Material::Ptr UsdMaterialHandler::material(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }

    // Test if this item is imageable. If not, then we cannot create a material
    // interface for it, which is a valid case (such as for a material node type).
    PXR_NS::UsdGeomImageable primSchema(usdItem->prim());
    if (!primSchema) {
        return nullptr;
    }

    return UsdMaterial::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
