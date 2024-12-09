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

#include <ufe/attribute.h>
#include <ufe/sceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/attribute.h>

#include <string>
#include <vector>

/**
 * Namespace for LookdevXUsd utility functions.
 */
namespace LookdevXUsdUtils
{
    using Strings = std::vector<std::string>;

PXR_NS::SdfLayerRefPtr getSessionLayer(const Ufe::SceneItem::Ptr& item);

// Tries to find the shader source of the item the attribute belongs to (e.g. arnold, mtlx).
// The attribute connection is recursively traced through compounds until it can reach a valid node definition.
PXR_NS::TfToken getShaderSourceType(const Ufe::Attribute::Ptr& attr);

// Check if the src and dst attributes are connected.
bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr);

/// Splits a string based on a list of separators. Splits on the first separator found.
Strings splitString(const std::string& str, const std::string& separators);

} // namespace LookdevXUsdUtils
