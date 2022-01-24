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
#ifndef MAYAUSD_SCHEMAS_TOKENS_H
#define MAYAUSD_SCHEMAS_TOKENS_H

/// \file mayaUsd_Schemas/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
//
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include <mayaUsd_Schemas/api.h>

#include <pxr/base/tf/staticData.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class MayaUsd_SchemasTokensType
///
/// \link MayaUsd_SchemasTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// MayaUsd_SchemasTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use MayaUsd_SchemasTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(MayaUsd_SchemasTokens->mayaAutoEdit);
/// \endcode
struct MayaUsd_SchemasTokensType
{
    MAYAUSD_SCHEMAS_API MayaUsd_SchemasTokensType();
    /// \brief "mayaAutoEdit"
    ///
    /// MayaUsd_SchemasMayaReference
    const TfToken mayaAutoEdit;
    /// \brief "mayaNamespace"
    ///
    /// MayaUsd_SchemasMayaReference
    const TfToken mayaNamespace;
    /// \brief "mayaReference"
    ///
    /// MayaUsd_SchemasMayaReference
    const TfToken mayaReference;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var MayaUsd_SchemasTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa MayaUsd_SchemasTokensType
extern MAYAUSD_SCHEMAS_API TfStaticData<MayaUsd_SchemasTokensType> MayaUsd_SchemasTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
