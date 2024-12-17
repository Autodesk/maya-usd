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
#ifndef USD_UI_NODE_GRAPH_NODE_H
#define USD_UI_NODE_GRAPH_NODE_H

#include <LookdevXUfe/UINodeGraphNode.h>

#include <ufe/sceneItem.h>

namespace LookdevXUsd
{
/**
 * @brief Class to handle UINodeGraphNode data that doesn't exist
 * in Ufe::UINodeGraphNode.
 */
class UsdUINodeGraphNode : public LookdevXUfe::UINodeGraphNode
{
public:
    using Ptr = std::shared_ptr<UsdUINodeGraphNode>;

    //! Constructor.
    explicit UsdUINodeGraphNode(const Ufe::UINodeGraphNode::Ptr& wrappedUsdUINodeGraphNode);
    //! Destructor.
    ~UsdUINodeGraphNode() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdUINodeGraphNode(const UsdUINodeGraphNode&) = delete;
    UsdUINodeGraphNode& operator=(const UsdUINodeGraphNode&) = delete;
    UsdUINodeGraphNode(UsdUINodeGraphNode&&) = delete;
    UsdUINodeGraphNode& operator=(UsdUINodeGraphNode&&) = delete;
    //@}

    //! Create a UsdUINodeGraphNode.
    static LookdevXUfe::UINodeGraphNode::Ptr create(const Ufe::UINodeGraphNode::Ptr& wrappedUsdUINodeGraphNode);

    // Forward all wrapped API calls.
    //-----------------------------------------------------------------------------------

    // LCOV_EXCL_START

    [[nodiscard]] Ufe::SceneItem::Ptr sceneItem() const override
    {
        return m_wrappedUsdUINodeGraphNode->sceneItem();
    }

    [[nodiscard]] bool hasPosition() const override
    {
        return m_wrappedUsdUINodeGraphNode->hasPosition();
    }

    [[nodiscard]] Ufe::Vector2f getPosition() const override
    {
        return m_wrappedUsdUINodeGraphNode->getPosition();
    }

    [[nodiscard]] Ufe::UndoableCommand::Ptr setPositionCmd(const Ufe::Vector2f& pos) override
    {
        return m_wrappedUsdUINodeGraphNode->setPositionCmd(pos);
    }

    [[nodiscard]] bool hasSize() const override
    {
        return m_wrappedUsdUINodeGraphNode->hasSize();
    }

    [[nodiscard]] Ufe::Vector2f getSize() const override
    {
        return m_wrappedUsdUINodeGraphNode->getSize();
    }

    [[nodiscard]] Ufe::UndoableCommand::Ptr setSizeCmd(const Ufe::Vector2f& size) override
    {
        return m_wrappedUsdUINodeGraphNode->setSizeCmd(size);
    }

    [[nodiscard]] bool hasDisplayColor() const override
    {
        return m_wrappedUsdUINodeGraphNode->hasDisplayColor();
    }

    [[nodiscard]] Ufe::Color3f getDisplayColor() const override
    {
        return m_wrappedUsdUINodeGraphNode->getDisplayColor();
    }

    [[nodiscard]] Ufe::UndoableCommand::Ptr setDisplayColorCmd(const Ufe::Color3f& color) override
    {
        return m_wrappedUsdUINodeGraphNode->setDisplayColorCmd(color);
    }

    // LCOV_EXCL_STOP

    //-----------------------------------------------------------------------------------

private:
    Ufe::UINodeGraphNode::Ptr m_wrappedUsdUINodeGraphNode;
};

} // namespace LookdevXUsd

#endif
