// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdObject3dHandler.h"

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usdGeom/imageable.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::Object3dHandler, UsdObject3dHandler);

/*static*/
UsdObject3dHandler::Ptr UsdObject3dHandler::create()
{
    return std::make_shared<UsdObject3dHandler>();
}

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

bool UsdObject3dHandler::canCreateObject3dForItem(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (usdItem) {
        // Test if this item is imageable. If not, then we cannot create an object3d
        // interface for it, which is a valid case (such as for a material node type).
        PXR_NS::UsdGeomImageable primSchema(usdItem->prim());
        if (primSchema)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// UsdObject3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Object3d::Ptr UsdObject3dHandler::object3d(const Ufe::SceneItem::Ptr& item) const
{
    if (canCreateObject3dForItem(item)) {
        UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
        assert(usdItem);
#endif
        return UsdObject3d::create(usdItem);
    }
    return {};
}

} // namespace USDUFE_NS_DEF
