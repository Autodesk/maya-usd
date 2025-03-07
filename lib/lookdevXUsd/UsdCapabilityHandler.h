//*****************************************************************************
// Copyright (c) 2023 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#pragma once

#include <LookdevXUfe/CapabilityHandler.h>

namespace LookdevXUsd
{

/**
 * @brief Provides an interface for the USD runtime specific capabilities.
 */
class UsdCapabilityHandler : public LookdevXUfe::CapabilityHandler
{
public:
    using Ptr = std::shared_ptr<UsdCapabilityHandler>;

    static LookdevXUfe::CapabilityHandler::Ptr create()
    {
        return std::make_shared<UsdCapabilityHandler>();
    }

    [[nodiscard]] bool hasCapability(Capability capability) const override;
};

} // namespace LookdevXUsd
