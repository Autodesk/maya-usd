#ifndef USDUINODEGRAPHNODE_H
#define USDUINODEGRAPHNODE_H

// =======================================================================
// Copyright 2022 Autodesk, Inc. All rights reserved.
//
// This computer source code and related instructions and comments are the
// unpublished confidential  and proprietary information of Autodesk, Inc.
// and are protected under applicable copyright and trade secret law. They
// may not be disclosed to, copied  or used by any third party without the
// prior written consent of Autodesk, Inc.
// =======================================================================

#include "UsdSceneItem.h"

#include <mayaUsd/base/api.h>

#include <ufe/uiNodeGraphNode.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of Ufe::UINodeGraphNode interface for USD objects.
class MAYAUSD_CORE_PUBLIC UsdUINodeGraphNode : public Ufe::UINodeGraphNode
{
public:
    typedef std::shared_ptr<UsdUINodeGraphNode> Ptr;

    UsdUINodeGraphNode(const UsdSceneItem::Ptr& item);
    ~UsdUINodeGraphNode() override = default;

    UsdUINodeGraphNode(const UsdUINodeGraphNode&) = delete;
    UsdUINodeGraphNode& operator=(const UsdUINodeGraphNode&) = delete;
    UsdUINodeGraphNode(UsdUINodeGraphNode&&) = delete;
    UsdUINodeGraphNode& operator=(UsdUINodeGraphNode&&) = delete;

    //! Create a UsdUINodeGraphNode.
    static UsdUINodeGraphNode::Ptr create(const UsdSceneItem::Ptr& item);

    // Ufe::UsdUINodeGraphNode overrides
    Ufe::SceneItem::Ptr       sceneItem() const override;
    bool                      hasPosition() const override;
    Ufe::Vector2f             getPosition() const override;
    Ufe::UndoableCommand::Ptr setPositionCmd(const Ufe::Vector2f& pos) override;

private:
    UsdSceneItem::Ptr fItem;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDUINODEGRAPHNODE_H
