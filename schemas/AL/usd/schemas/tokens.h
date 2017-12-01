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
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define AL_USDMAYASCHEMAS_TOKENS \
    ((lock, "al_usdmaya_lock")) \
    ((lock_none, "none")) \
    ((lock_transform, "transform")) \
    (mayaNamespace) \
    (mayaReference) \
    ((selectability, "al_usdmaya_selectability")) \
    ((selectability_inherited, "inherited")) \
    ((selectability_selectable, "selectable")) \
    ((selectability_unselectable, "unselectable"))

/// \anchor AL_USDMayaSchemasTokens
///
/// <b>AL_USDMayaSchemasTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// AL_USDMayaSchemasTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use AL_USDMayaSchemasTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(AL_USDMayaSchemasTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>lock</b> - Stores the lock state of corresponding Maya objects of the prims
/// \li <b>lock_none</b> - State which makes the Prim unlock regardless of its parent state
/// \li <b>lock_transform</b> - State which makes transform attributes of Maya objects (including children) locked
/// \li <b>mayaNamespace</b> - AL_usd_MayaReference
/// \li <b>mayaReference</b> - AL_usd_MayaReference
/// \li <b>selectability</b> - Stores the state of the prims selectability
/// \li <b>selectability_inherited</b> - State which makes the Prim inherit it's selectability
/// \li <b>selectability_selectable</b> - State which makes the Prim selectable
/// \li <b>selectability_unselectable</b> - State which makes the Prim unselectable
TF_DECLARE_PUBLIC_TOKENS(AL_USDMayaSchemasTokens, AL_USDMAYASCHEMAS_API, AL_USDMAYASCHEMAS_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
