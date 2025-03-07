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

#include "UsdHierarchy.h"

#include "LookdevXUfe/SceneItemUI.h"

#include <algorithm>
#include <iterator>

namespace LookdevXUsd
{

UsdHierarchy::~UsdHierarchy() = default;

Ufe::SceneItemList UsdHierarchy::filteredChildren(const ChildFilter& filter) const
{
    // Pass the filter unchanged to maya-usd, as it will not return anything on an unsupported filter.
    auto wrappedFiltered = m_wrappedUsdHierarchy->filteredChildren(filter);

    // Do extra post filtering here.
    Ufe::SceneItemList filtered;
    std::copy_if(wrappedFiltered.begin(), wrappedFiltered.end(), std::back_inserter(filtered), [](const auto& item) {
        const auto sceneItemUI = LookdevXUfe::SceneItemUI::sceneItemUI(item);
        return !sceneItemUI || !sceneItemUI->hidden();
    });
    return filtered;
}

} // namespace LookdevXUsd
