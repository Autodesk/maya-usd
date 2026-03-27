// ===========================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdUINodeGraphNodeHandler.h"

#include "UsdUINodeGraphNode.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/usdUI/nodeGraphNodeAPI.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UINodeGraphNodeHandler, UsdUINodeGraphNodeHandler);

UsdUINodeGraphNodeHandler::Ptr UsdUINodeGraphNodeHandler::create()
{
    return std::make_shared<UsdUINodeGraphNodeHandler>();
}

Ufe::UINodeGraphNode::Ptr
UsdUINodeGraphNodeHandler::uiNodeGraphNode(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    auto usdItem = downcast(item);
    TF_VERIFY(usdItem);

    const UsdPrim prim = usdItem->prim();
    if (prim.IsValid() && UsdUINodeGraphNodeAPI::CanApply(prim)) {
        return UsdUINodeGraphNode::create(usdItem);
    }
    return nullptr;
}

} // namespace USDUFE_NS_DEF
