// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#ifndef USDUFE_USDOBJECT3DHANDLER_H
#define USDUFE_USDOBJECT3DHANDLER_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdObject3d.h>

#include <ufe/object3dHandler.h>

namespace USDUFE_NS_DEF {

//! \brief USD run-time 3D object handler.
/*!
        Factory object for Object3d interfaces.
 */
class USDUFE_PUBLIC UsdObject3dHandler : public Ufe::Object3dHandler
{
public:
    typedef std::shared_ptr<UsdObject3dHandler> Ptr;

    UsdObject3dHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdObject3dHandler);

    //! Create a UsdObject3dHandler.
    static UsdObject3dHandler::Ptr create();

    // Helpers
    bool canCreateObject3dForItem(const Ufe::SceneItem::Ptr& item) const;

    // UsdObject3dHandler overrides
    Ufe::Object3d::Ptr object3d(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdObject3dHandler

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDOBJECT3DHANDLER_H
