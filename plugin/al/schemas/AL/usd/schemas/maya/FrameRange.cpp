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
#include "FrameRange.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usd/typed.h>

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<AL_usd_FrameRange, TfType::Bases<UsdTyped>>();

    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ALFrameRange")
    // to find TfType<AL_usd_FrameRange>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, AL_usd_FrameRange>("ALFrameRange");
}

/* virtual */
AL_usd_FrameRange::~AL_usd_FrameRange() { }

/* static */
AL_usd_FrameRange AL_usd_FrameRange::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_FrameRange();
    }
    return AL_usd_FrameRange(stage->GetPrimAtPath(path));
}

/* static */
AL_usd_FrameRange AL_usd_FrameRange::Define(const UsdStagePtr& stage, const SdfPath& path)
{
    static TfToken usdPrimTypeName("ALFrameRange");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_FrameRange();
    }
    return AL_usd_FrameRange(stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType AL_usd_FrameRange::_GetSchemaType() const { return AL_usd_FrameRange::schemaType; }

/* static */
const TfType& AL_usd_FrameRange::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<AL_usd_FrameRange>();
    return tfType;
}

/* static */
bool AL_usd_FrameRange::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType& AL_usd_FrameRange::_GetTfType() const { return _GetStaticTfType(); }

UsdAttribute AL_usd_FrameRange::GetAnimationStartFrameAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->animationStartFrame);
}

UsdAttribute AL_usd_FrameRange::CreateAnimationStartFrameAttr(
    VtValue const& defaultValue,
    bool           writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        AL_USDMayaSchemasTokens->animationStartFrame,
        SdfValueTypeNames->Double,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute AL_usd_FrameRange::GetStartFrameAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->startFrame);
}

UsdAttribute
AL_usd_FrameRange::CreateStartFrameAttr(VtValue const& defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        AL_USDMayaSchemasTokens->startFrame,
        SdfValueTypeNames->Double,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute AL_usd_FrameRange::GetEndFrameAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->endFrame);
}

UsdAttribute
AL_usd_FrameRange::CreateEndFrameAttr(VtValue const& defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        AL_USDMayaSchemasTokens->endFrame,
        SdfValueTypeNames->Double,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute AL_usd_FrameRange::GetAnimationEndFrameAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->animationEndFrame);
}

UsdAttribute AL_usd_FrameRange::CreateAnimationEndFrameAttr(
    VtValue const& defaultValue,
    bool           writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        AL_USDMayaSchemasTokens->animationEndFrame,
        SdfValueTypeNames->Double,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute AL_usd_FrameRange::GetCurrentFrameAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->currentFrame);
}

UsdAttribute
AL_usd_FrameRange::CreateCurrentFrameAttr(VtValue const& defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        AL_USDMayaSchemasTokens->currentFrame,
        SdfValueTypeNames->Double,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left, const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
} // namespace

/*static*/
const TfTokenVector& AL_usd_FrameRange::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        AL_USDMayaSchemasTokens->animationStartFrame,
        AL_USDMayaSchemasTokens->startFrame,
        AL_USDMayaSchemasTokens->endFrame,
        AL_USDMayaSchemasTokens->animationEndFrame,
        AL_USDMayaSchemasTokens->currentFrame,
    };
    static TfTokenVector allNames
        = _ConcatenateAttributeNames(UsdTyped::GetSchemaAttributeNames(true), localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
