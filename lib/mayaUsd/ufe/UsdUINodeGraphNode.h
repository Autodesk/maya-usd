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

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <ufe/uiNodeGraphNode.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of Ufe::UINodeGraphNode interface for USD objects.
class MAYAUSD_CORE_PUBLIC UsdUINodeGraphNode
    : public
#ifdef UFE_V4_1_FEATURES_AVAILABLE
      Ufe::UINodeGraphNode_v4_1
#else
      Ufe::UINodeGraphNode
#endif
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
#ifdef UFE_UINODEGRAPHNODE_HAS_SIZE
    bool                      hasSize() const override;
    Ufe::Vector2f             getSize() const override;
    Ufe::UndoableCommand::Ptr setSizeCmd(const Ufe::Vector2f& size) override;
#endif
#ifdef UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR
    bool                      hasDisplayColor() const override;
    Ufe::Color3f              getDisplayColor() const override;
    Ufe::UndoableCommand::Ptr setDisplayColorCmd(const Ufe::Color3f& color) override;
#endif

private:
    enum class CoordType
    {
        Position,
        Size
    };

    class SetPosOrSizeCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
    {
    public:
        SetPosOrSizeCommand(
            CoordType              coordType,
            const PXR_NS::UsdPrim& prim,
            const Ufe::Vector2f&   newValue);

        void executeImplementation() override;

    private:
        const CoordType               _coordType;
        const PXR_NS::UsdStageWeakPtr _stage;
        const PXR_NS::SdfPath         _primPath;
        const PXR_NS::VtValue         _newValue;
    };

#ifdef UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR
    class SetDisplayColorCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
    {
    public:
        SetDisplayColorCommand(const PXR_NS::UsdPrim& prim, const Ufe::Color3f& newColor);

        void executeImplementation() override;

    private:
        const PXR_NS::UsdStageWeakPtr _stage;
        const PXR_NS::SdfPath         _primPath;
        const PXR_NS::VtValue         _newValue;
    };
#endif

    UsdSceneItem::Ptr fItem;

    bool          hasPosOrSize(CoordType coordType) const;
    Ufe::Vector2f getPosOrSize(CoordType coordType) const;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDUINODEGRAPHNODE_H
