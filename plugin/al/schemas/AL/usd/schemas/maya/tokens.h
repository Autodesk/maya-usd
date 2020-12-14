//
// Copyright 2016 Pixar
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
#ifndef AL_USDMAYASCHEMAS_TOKENS_H
#define AL_USDMAYASCHEMAS_TOKENS_H

/// \file AL_USDMayaSchemas/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
//
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "api.h"

#include <pxr/base/tf/staticData.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class AL_USDMayaSchemasTokensType
///
/// \link AL_USDMayaSchemasTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// AL_USDMayaSchemasTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use AL_USDMayaSchemasTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(AL_USDMayaSchemasTokens->animationEndFrame);
/// \endcode
struct AL_USDMayaSchemasTokensType
{
    AL_USDMAYASCHEMAS_API AL_USDMayaSchemasTokensType();
    /// \brief "animationEndFrame"
    ///
    /// AL_usd_FrameRange
    const TfToken animationEndFrame;
    /// \brief "animationStartFrame"
    ///
    /// AL_usd_FrameRange
    const TfToken animationStartFrame;
    /// \brief "currentFrame"
    ///
    /// AL_usd_FrameRange
    const TfToken currentFrame;
    /// \brief "endFrame"
    ///
    /// AL_usd_FrameRange
    const TfToken endFrame;
    /// \brief "al_usdmaya_lock"
    ///
    /// Stores the lock state of corresponding Maya objects of the prims
    const TfToken lock;
    /// \brief "inherited"
    ///
    /// State which makes the Prim inherit its parent's lock state
    const TfToken lock_inherited;
    /// \brief "transform"
    ///
    /// State which makes transform attributes of Maya objects (including children) locked
    const TfToken lock_transform;
    /// \brief "unlocked"
    ///
    /// State which makes the Prim unlocked regardless of its parent's state
    const TfToken lock_unlocked;
    /// \brief "al_usdmaya_mergedTransform"
    ///
    /// State which makes the prim merged
    const TfToken mergedTransform;
    /// \brief "unmerged"
    ///
    /// State which makes the prim unmerged
    const TfToken mergedTransform_unmerged;
    /// \brief "al_usdmaya_selectability"
    ///
    /// Stores the state of the prims selectability
    const TfToken selectability;
    /// \brief "inherited"
    ///
    /// State which makes the Prim inherit it's selectability
    const TfToken selectability_inherited;
    /// \brief "selectable"
    ///
    /// State which makes the Prim selectable
    const TfToken selectability_selectable;
    /// \brief "unselectable"
    ///
    /// State which makes the Prim unselectable
    const TfToken selectability_unselectable;
    /// \brief "startFrame"
    ///
    /// AL_usd_FrameRange
    const TfToken startFrame;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var AL_USDMayaSchemasTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa AL_USDMayaSchemasTokensType
extern AL_USDMAYASCHEMAS_API TfStaticData<AL_USDMayaSchemasTokensType> AL_USDMayaSchemasTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
