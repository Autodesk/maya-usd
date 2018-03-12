//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
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
///     gprim.GetMyTokenValuedAttr().Set(AL_USDMayaSchemasTokens->lock);
/// \endcode
struct AL_USDMayaSchemasTokensType {
    AL_USDMAYASCHEMAS_API AL_USDMayaSchemasTokensType();
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
    /// \brief "mayaNamespace"
    /// 
    /// AL_usd_MayaReference
    const TfToken mayaNamespace;
    /// \brief "mayaReference"
    /// 
    /// AL_usd_MayaReference
    const TfToken mayaReference;
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
