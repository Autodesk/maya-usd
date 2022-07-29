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

class SetPositionCommand : public Ufe::UndoableCommand
{
public:
    SetPositionCommand(const PXR_NS::UsdPrim& prim, const Ufe::Vector2f& newPos)
        : Ufe::UndoableCommand()
        , _stage(prim.GetStage())
        , _primPath(prim.GetPath())
        , _newPos(PXR_NS::GfVec2f(newPos.x(), newPos.y()))
    {
    }

    void execute() override
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (_stage) {
            const PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);
            if (!prim.HasAPI<PXR_NS::UsdUINodeGraphNodeAPI>()) {
                PXR_NS::UsdUINodeGraphNodeAPI::Apply(prim);
            }
            if (prim.HasAPI<PXR_NS::UsdUINodeGraphNodeAPI>()) {
                PXR_NS::UsdUINodeGraphNodeAPI posApi(prim);
                TF_VERIFY(posApi);
                PXR_NS::UsdAttribute attr = posApi.GetPosAttr();
                if (!attr) {
                    attr = posApi.CreatePosAttr();
                }
                attr.Set(_newPos);
            }
        }
    }

    void undo() override { }
    void redo() override { }

private:
    const PXR_NS::UsdStageWeakPtr _stage;
    const PXR_NS::SdfPath         _primPath;
    const PXR_NS::VtValue         _newPos;
};

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

bool UsdUINodeGraphNode::hasPosition() const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    const PXR_NS::UsdPrim         prim = fItem->prim();
    PXR_NS::UsdUINodeGraphNodeAPI posApi(prim);
    TF_VERIFY(posApi);
    PXR_NS::UsdAttribute attr = posApi.GetPosAttr();
    return attr.IsValid();
}

Ufe::Vector2f UsdUINodeGraphNode::getPosition() const
{
    if (hasPosition()) {
        const PXR_NS::UsdPrim               prim = fItem->prim();
        const PXR_NS::UsdUINodeGraphNodeAPI posApi(prim);
        const PXR_NS::UsdAttribute          attr = posApi.GetPosAttr();
        PXR_NS::VtValue                     v;
        attr.Get(&v);
        const PXR_NS::GfVec2f pos = v.Get<PXR_NS::GfVec2f>();
        return Ufe::Vector2f(pos[0], pos[1]);
    } else {
        return Ufe::Vector2f(0.0f, 0.0f);
    }
}

Ufe::UndoableCommand::Ptr UsdUINodeGraphNode::setPositionCmd(const Ufe::Vector2f& pos)
{
    return std::make_shared<SetPositionCommand>(fItem ? fItem->prim() : PXR_NS::UsdPrim(), pos);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
