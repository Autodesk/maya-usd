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

#include "UsdCapabilityHandler.h"

namespace LookdevXUsd
{

bool UsdCapabilityHandler::hasCapability(Capability capability) const
{
    switch (capability)
    {
    case Capability::kCanPromoteToMaterial:
    case Capability::kCanPromoteInputAtTopLevel:
    case Capability::kCanHaveNestedNodeGraphs:
        return true;
    case Capability::kCanUseCreateShaderCommandForComponentConnections:
        return false;
    }
    return false;
}

} // namespace LookdevXUsd
