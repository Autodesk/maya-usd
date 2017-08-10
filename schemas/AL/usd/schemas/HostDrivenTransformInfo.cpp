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
#include "./HostDrivenTransformInfo.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<AL_usd_HostDrivenTransformInfo,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ALHostDrivenTransformInfo")
    // to find TfType<AL_usd_HostDrivenTransformInfo>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, AL_usd_HostDrivenTransformInfo>("ALHostDrivenTransformInfo");
}

/* virtual */
AL_usd_HostDrivenTransformInfo::~AL_usd_HostDrivenTransformInfo()
{
}

/* static */
AL_usd_HostDrivenTransformInfo
AL_usd_HostDrivenTransformInfo::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_HostDrivenTransformInfo();
    }
    return AL_usd_HostDrivenTransformInfo(stage->GetPrimAtPath(path));
}

/* static */
AL_usd_HostDrivenTransformInfo
AL_usd_HostDrivenTransformInfo::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ALHostDrivenTransformInfo");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_HostDrivenTransformInfo();
    }
    return AL_usd_HostDrivenTransformInfo(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
AL_usd_HostDrivenTransformInfo::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<AL_usd_HostDrivenTransformInfo>();
    return tfType;
}

/* static */
bool 
AL_usd_HostDrivenTransformInfo::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
AL_usd_HostDrivenTransformInfo::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetTransformSourceNodesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->transformSourceNodes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateTransformSourceNodesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->transformSourceNodes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetScaleAttributesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->scaleAttributes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateScaleAttributesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->scaleAttributes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetScaleAttributeIndicesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->scaleAttributeIndices);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateScaleAttributeIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->scaleAttributeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetRotateAttributesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->rotateAttributes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateRotateAttributesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->rotateAttributes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetRotateAttributeIndicesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->rotateAttributeIndices);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateRotateAttributeIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->rotateAttributeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetTranslateAttributesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->translateAttributes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateTranslateAttributesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->translateAttributes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetTranslateAttributeIndicesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->translateAttributeIndices);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateTranslateAttributeIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->translateAttributeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetRotateOrderAttributesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->rotateOrderAttributes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateRotateOrderAttributesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->rotateOrderAttributes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetRotateOrderAttributeIndicesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->rotateOrderAttributeIndices);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateRotateOrderAttributeIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->rotateOrderAttributeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetVisibilitySourceNodesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->visibilitySourceNodes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateVisibilitySourceNodesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->visibilitySourceNodes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetVisibilityAttributesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->visibilityAttributes);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateVisibilityAttributesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->visibilityAttributes,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::GetVisibilityAttributeIndicesAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->visibilityAttributeIndices);
}

UsdAttribute
AL_usd_HostDrivenTransformInfo::CreateVisibilityAttributeIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->visibilityAttributeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
AL_usd_HostDrivenTransformInfo::GetTargetTransformsRel() const
{
    return GetPrim().GetRelationship(AL_USDMayaSchemasTokens->targetTransforms);
}

UsdRelationship
AL_usd_HostDrivenTransformInfo::CreateTargetTransformsRel() const
{
    return GetPrim().CreateRelationship(AL_USDMayaSchemasTokens->targetTransforms,
                       /* custom = */ false);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
AL_usd_HostDrivenTransformInfo::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        AL_USDMayaSchemasTokens->transformSourceNodes,
        AL_USDMayaSchemasTokens->scaleAttributes,
        AL_USDMayaSchemasTokens->scaleAttributeIndices,
        AL_USDMayaSchemasTokens->rotateAttributes,
        AL_USDMayaSchemasTokens->rotateAttributeIndices,
        AL_USDMayaSchemasTokens->translateAttributes,
        AL_USDMayaSchemasTokens->translateAttributeIndices,
        AL_USDMayaSchemasTokens->rotateOrderAttributes,
        AL_USDMayaSchemasTokens->rotateOrderAttributeIndices,
        AL_USDMayaSchemasTokens->visibilitySourceNodes,
        AL_USDMayaSchemasTokens->visibilityAttributes,
        AL_USDMayaSchemasTokens->visibilityAttributeIndices,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
            localNames);

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
