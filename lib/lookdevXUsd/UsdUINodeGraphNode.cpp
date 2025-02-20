//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************

#include "UsdUINodeGraphNode.h"

#include <pxr/usd/usdUI/nodeGraphNodeAPI.h>

namespace LookdevXUsd
{

UsdUINodeGraphNode::UsdUINodeGraphNode(const Ufe::UINodeGraphNode::Ptr& wrappedUsdUINodeGraphNode)
    : m_wrappedUsdUINodeGraphNode(wrappedUsdUINodeGraphNode)
{
}

LookdevXUfe::UINodeGraphNode::Ptr UsdUINodeGraphNode::create(const Ufe::UINodeGraphNode::Ptr& wrappedUsdUINodeGraphNode)
{
    return std::make_shared<LookdevXUsd::UsdUINodeGraphNode>(wrappedUsdUINodeGraphNode);
}

} // namespace LookdevXUsd
