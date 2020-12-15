// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdObject3d.h>

#include <ufe/object3dHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time 3D object handler.
/*!
        Factory object for Object3d interfaces.
 */
class MAYAUSD_CORE_PUBLIC UsdObject3dHandler : public Ufe::Object3dHandler
{
public:
    typedef std::shared_ptr<UsdObject3dHandler> Ptr;

    UsdObject3dHandler();
    ~UsdObject3dHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdObject3dHandler(const UsdObject3dHandler&) = delete;
    UsdObject3dHandler& operator=(const UsdObject3dHandler&) = delete;
    UsdObject3dHandler(UsdObject3dHandler&&) = delete;
    UsdObject3dHandler& operator=(UsdObject3dHandler&&) = delete;

    //! Create a UsdObject3dHandler.
    static UsdObject3dHandler::Ptr create();

    // UsdObject3dHandler overrides
    Ufe::Object3d::Ptr object3d(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdObject3dHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
