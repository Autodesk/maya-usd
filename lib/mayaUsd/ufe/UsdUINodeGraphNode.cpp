// =======================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// This computer source code and related instructions and comments are the
// unpublished confidential  and proprietary information of Autodesk, Inc.
// and are protected under applicable copyright and trade secret law. They
// may not be disclosed to, copied  or used by any third party without the
// prior written consent of Autodesk, Inc.
// =======================================================================

#include "UsdUINodeGraphNode.h"

#include <pxr/usd/usdUI/nodeGraphNodeAPI.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUINodeGraphNode::SetPosOrSizeCommand::SetPosOrSizeCommand(
    CoordType              coordType,
    const PXR_NS::UsdPrim& prim,
    const Ufe::Vector2f&   newValue)
    : _coordType(coordType)
    , _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _newValue(PXR_NS::GfVec2f(newValue.x(), newValue.y()))
{
}

void UsdUINodeGraphNode::SetPosOrSizeCommand::executeImplementation()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (_stage) {
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (!prim.HasAPI<UsdUINodeGraphNodeAPI>()) {
            UsdUINodeGraphNodeAPI::Apply(prim);
        }
        if (prim.HasAPI<UsdUINodeGraphNodeAPI>()) {
            UsdUINodeGraphNodeAPI posApi(prim);
            TF_VERIFY(posApi);
            UsdAttribute attr
                = _coordType == CoordType::Position ? posApi.GetPosAttr() : posApi.GetSizeAttr();
            if (!attr) {
                attr = _coordType == CoordType::Position ? posApi.CreatePosAttr()
                                                         : posApi.CreateSizeAttr();
            }
            attr.Set(_newValue);
        }
    }
}

UsdUINodeGraphNode::UsdUINodeGraphNode(const UsdSceneItem::Ptr& item)
    : Ufe::UINodeGraphNode()
    , fItem(item)
{
}

UsdUINodeGraphNode::Ptr UsdUINodeGraphNode::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdUINodeGraphNode>(item);
}

Ufe::SceneItem::Ptr UsdUINodeGraphNode::sceneItem() const { return fItem; }

bool UsdUINodeGraphNode::hasPosition() const { return hasPosOrSize(CoordType::Position); }

Ufe::Vector2f UsdUINodeGraphNode::getPosition() const { return getPosOrSize(CoordType::Position); }

Ufe::UndoableCommand::Ptr UsdUINodeGraphNode::setPositionCmd(const Ufe::Vector2f& pos)
{
    return std::make_shared<SetPosOrSizeCommand>(
        CoordType::Position, fItem ? fItem->prim() : PXR_NS::UsdPrim(), pos);
}

#ifdef UFE_V5_FEATURES_AVAILABLE
bool UsdUINodeGraphNode::hasSize() const { return hasPosOrSize(CoordType::Size); }

Ufe::Vector2f UsdUINodeGraphNode::getSize() const { return getPosOrSize(CoordType::Size); }

Ufe::UndoableCommand::Ptr UsdUINodeGraphNode::setSizeCmd(const Ufe::Vector2f& size)
{
    return std::make_shared<SetPosOrSizeCommand>(
        CoordType::Size, fItem ? fItem->prim() : PXR_NS::UsdPrim(), size);
}
#endif

bool UsdUINodeGraphNode::hasPosOrSize(CoordType coordType) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    const UsdPrim         prim = fItem->prim();
    UsdUINodeGraphNodeAPI posApi(prim);
    if (!posApi) {
        return false;
    }
    UsdAttribute attr
        = coordType == CoordType::Position ? posApi.GetPosAttr() : posApi.GetSizeAttr();
    return attr.IsValid();
}

Ufe::Vector2f UsdUINodeGraphNode::getPosOrSize(CoordType coordType) const
{
    if (hasPosOrSize(coordType)) {
        const PXR_NS::UsdPrim               prim = fItem->prim();
        const PXR_NS::UsdUINodeGraphNodeAPI posApi(prim);
        const PXR_NS::UsdAttribute          attr
            = coordType == CoordType::Position ? posApi.GetPosAttr() : posApi.GetSizeAttr();
        PXR_NS::VtValue v;
        attr.Get(&v);
        const PXR_NS::GfVec2f val = v.Get<PXR_NS::GfVec2f>();
        return Ufe::Vector2f(val[0], val[1]);
    } else {
        return Ufe::Vector2f(0.0f, 0.0f);
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
