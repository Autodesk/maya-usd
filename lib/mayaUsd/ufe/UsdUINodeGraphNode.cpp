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

// Ensure that UsdUINodeGraphNode is properly setup.
#ifdef UFE_V4_1_FEATURES_AVAILABLE
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::UINodeGraphNode_v4_1, UsdUINodeGraphNode);
#else
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::UINodeGraphNode, UsdUINodeGraphNode);
#endif

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

#ifdef UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR
UsdUINodeGraphNode::SetDisplayColorCommand::SetDisplayColorCommand(
    const PXR_NS::UsdPrim& prim,
    const Ufe::Color3f&    newColor)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _newValue(PXR_NS::GfVec3f(newColor.r(), newColor.g(), newColor.b()))

{
}

void UsdUINodeGraphNode::SetDisplayColorCommand::executeImplementation()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (_stage) {
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (!prim.HasAPI<UsdUINodeGraphNodeAPI>()) {
            UsdUINodeGraphNodeAPI::Apply(prim);
        }
        if (prim.HasAPI<UsdUINodeGraphNodeAPI>()) {
            UsdUINodeGraphNodeAPI displayColorApi(prim);
            TF_VERIFY(displayColorApi, "Unable to access display color\n");
            UsdAttribute attr = displayColorApi.GetDisplayColorAttr();
            if (!attr) {
                attr = displayColorApi.CreateDisplayColorAttr();
            }
            attr.Set(_newValue);
        }
    }
}
#endif // UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR

UsdUINodeGraphNode::UsdUINodeGraphNode(const UsdSceneItem::Ptr& item)
    : fItem(item)
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

#ifdef UFE_UINODEGRAPHNODE_HAS_SIZE
bool UsdUINodeGraphNode::hasSize() const { return hasPosOrSize(CoordType::Size); }

Ufe::Vector2f UsdUINodeGraphNode::getSize() const { return getPosOrSize(CoordType::Size); }

Ufe::UndoableCommand::Ptr UsdUINodeGraphNode::setSizeCmd(const Ufe::Vector2f& size)
{
    return std::make_shared<SetPosOrSizeCommand>(
        CoordType::Size, fItem ? fItem->prim() : PXR_NS::UsdPrim(), size);
}
#endif

#ifdef UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR
bool UsdUINodeGraphNode::hasDisplayColor() const
{
    const PXR_NS::UsdPrim               prim = fItem->prim();
    const PXR_NS::UsdUINodeGraphNodeAPI displayColorApi(prim);
    if (!displayColorApi)
        return false;

    const PXR_NS::UsdAttribute attr = displayColorApi.GetDisplayColorAttr();
    if (!attr.IsValid())
        return false;

    PXR_NS::VtValue v;
    attr.Get(&v);
    return v.IsHolding<GfVec3f>();
}

Ufe::Color3f UsdUINodeGraphNode::getDisplayColor() const
{
    const PXR_NS::UsdPrim               prim = fItem->prim();
    const PXR_NS::UsdUINodeGraphNodeAPI displayColorApi(prim);
    if (displayColorApi) {
        const PXR_NS::UsdAttribute attr = displayColorApi.GetDisplayColorAttr();
        if (attr.IsValid()) {
            PXR_NS::VtValue v;
            attr.Get(&v);
            if (v.IsHolding<GfVec3f>()) {
                const PXR_NS::GfVec3f val = v.Get<PXR_NS::GfVec3f>();
                return Ufe::Color3f(val[0], val[1], val[2]);
            }
        }
    }
    return {};
}

Ufe::UndoableCommand::Ptr UsdUINodeGraphNode::setDisplayColorCmd(const Ufe::Color3f& color)
{
    return std::make_shared<SetDisplayColorCommand>(fItem->prim(), color);
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
    if (!attr.IsValid()) {
        return false;
    }
    VtValue v;
    attr.Get(&v);
    return v.IsHolding<GfVec2f>();
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
