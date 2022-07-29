// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdUINodeGraphNodeHandler.h"

#include "UsdUINodeGraphNode.h"

#include <pxr/usd/usdUI/nodeGraphNodeAPI.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUINodeGraphNodeHandler::Ptr UsdUINodeGraphNodeHandler::create()
{
    return std::make_shared<UsdUINodeGraphNodeHandler>();
}

Ufe::UINodeGraphNode::Ptr
UsdUINodeGraphNodeHandler::uiNodeGraphNode(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    TF_VERIFY(usdItem);

    const PXR_NS::UsdPrim prim = usdItem->prim();
    if (prim.IsValid() && PXR_NS::UsdUINodeGraphNodeAPI::CanApply(prim)) {
        return UsdUINodeGraphNode::create(usdItem);
    }
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
