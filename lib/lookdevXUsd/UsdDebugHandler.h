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
#pragma once

#include <LookdevXUfe/DebugHandler.h>

#include <ufe/sceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <string>

namespace LookdevXUsd
{

class UsdDebugHandler : public LookdevXUfe::DebugHandler
{
public:
    /**
     * @brief Export the data for a given scene item to a string.
     *
     * @param sceneItem  scene item we want the data model information for
     * @return string representation for the given scene item
     */
    std::string exportToString(Ufe::SceneItem::Ptr sceneItem) override;

    /**
     * @brief Run arbitrary commands in the runtime for debug/prototype purposes.
     *
     * @param command command identifier.
     * @param args key/value pairs for optional arguments of the command.
     */
    void runCommand(const std::string& command, const std::unordered_map<std::string, std::any>& args = {}) override;

    static LookdevXUfe::DebugHandler::Ptr create()
    {
        return std::make_shared<UsdDebugHandler>();
    }

    [[nodiscard]] bool hasViewportSupport(const Ufe::NodeDef::Ptr& nodeDef) const override;

private:
    /**
     * @brief Dump the data to a string for a given USD primitive.
     *
     * @param prim  primitive we want to get a string representation for
     * @return string representation for the given primitive
     */
    static std::string dumpPrim(const PXR_NS::UsdPrim& prim);
};

} // namespace LookdevXUsd
