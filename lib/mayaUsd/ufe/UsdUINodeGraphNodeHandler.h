#ifndef USDUINODEGRAPHNODEHANDLER_H
#define USDUINODEGRAPHNODEHANDLER_H

// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include <mayaUsd/base/api.h>

#include <ufe/uiNodeGraphNodeHandler.h>

#include "UsdSceneItem.h"

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of Ufe::UINodeGraphNodeHandler interface for USD objects.
class MAYAUSD_CORE_PUBLIC UsdUINodeGraphNodeHandler : public Ufe::UINodeGraphNodeHandler
{
public:
    typedef std::shared_ptr<UsdUINodeGraphNodeHandler> Ptr;

    UsdUINodeGraphNodeHandler() = default;
    ~UsdUINodeGraphNodeHandler() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdUINodeGraphNodeHandler(const UsdUINodeGraphNodeHandler&) = delete;
    UsdUINodeGraphNodeHandler& operator=(const UsdUINodeGraphNodeHandler&) = delete;
    UsdUINodeGraphNodeHandler(UsdUINodeGraphNodeHandler&&) = delete;
    UsdUINodeGraphNodeHandler& operator=(UsdUINodeGraphNodeHandler&&) = delete;

    //! Create a UsdUINodeGraphNodeHandler.
    static UsdUINodeGraphNodeHandler::Ptr create();

    // Ufe::UINodeGraphNodeHandler overrides
    Ufe::UINodeGraphNode::Ptr uiNodeGraphNode(const Ufe::SceneItem::Ptr& item) const override;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif /* USDUINODEGRAPHNODEHANDLER_H */
