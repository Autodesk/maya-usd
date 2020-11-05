//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once
//----------------------------------------------------------------------------------------------------------------------
/// \file   Metadata.h
/// \brief  This file contains the tokens for the USDMaya metadata.
//----------------------------------------------------------------------------------------------------------------------

#include "AL/usdmaya/Api.h"

#include <pxr/base/tf/token.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

/// \brief  The MetaData tokens we attach to various prims
struct Metadata
{
    /// MetaData token that allows the overriding of the transform type from the default
    /// AL_usdmaya_Transform on import.
    AL_USDMAYA_PUBLIC
    static const TfToken transformType;

    /// MetaData token that can be applied to a prim which will add it to the UsdImaging ignore list
    /// (so that it will not be rendered in Hydra).
    AL_USDMAYA_PUBLIC
    static const TfToken excludeFromProxyShape;

    /// MetaData token that controls whether a prim will be imported as Maya geometry
    AL_USDMAYA_PUBLIC
    static const TfToken importAsNative;

    /// Name of the property that determines if the prim is selectable or not
    AL_USDMAYA_PUBLIC
    static const TfToken selectability;

    /// Value used in the selectibility property that tags the prim as selectable
    AL_USDMAYA_PUBLIC
    static const TfToken selectable;

    /// Value used in the selectibility property that tags the prim as unselectable
    AL_USDMAYA_PUBLIC
    static const TfToken unselectable;

    AL_USDMAYA_PUBLIC
    static const TfToken mergedTransform;
    AL_USDMAYA_PUBLIC
    static const TfToken unmerged;

    /// Name of the property that determines if attributes on corresponding Maya object of the prim
    /// are locked or not.
    AL_USDMAYA_PUBLIC
    static const TfToken locked;

    /// Value used in the lock property that tags transform (including children) attributes are
    /// locked
    AL_USDMAYA_PUBLIC
    static const TfToken lockTransform;

    /// Value used in the lock property that tags prim inherits its parent state
    AL_USDMAYA_PUBLIC
    static const TfToken lockInherited;

    /// Value used in the lock property that tags prim unlocked regardless of its parent state
    AL_USDMAYA_PUBLIC
    static const TfToken lockUnlocked;

    /// This represents the medata key that plugins can be registered against as an alternative to
    /// typed schemas
    AL_USDMAYA_PUBLIC
    static const TfToken assetType;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
