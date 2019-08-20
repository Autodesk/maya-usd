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
#include "./MayaReference.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<AL_usd_MayaReference,
        TfType::Bases< UsdGeomXformable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ALMayaReference")
    // to find TfType<AL_usd_MayaReference>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, AL_usd_MayaReference>("ALMayaReference");
}

/* virtual */
AL_usd_MayaReference::~AL_usd_MayaReference()
{
}

/* static */
AL_usd_MayaReference
AL_usd_MayaReference::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_MayaReference();
    }
    return AL_usd_MayaReference(stage->GetPrimAtPath(path));
}

/* static */
AL_usd_MayaReference
AL_usd_MayaReference::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ALMayaReference");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_MayaReference();
    }
    return AL_usd_MayaReference(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType AL_usd_MayaReference::_GetSchemaType() const {
    return AL_usd_MayaReference::schemaType;
}

/* static */
const TfType &
AL_usd_MayaReference::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<AL_usd_MayaReference>();
    return tfType;
}

/* static */
bool 
AL_usd_MayaReference::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
AL_usd_MayaReference::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
AL_usd_MayaReference::GetMayaReferenceAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->mayaReference);
}

UsdAttribute
AL_usd_MayaReference::CreateMayaReferenceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->mayaReference,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
AL_usd_MayaReference::GetMayaNamespaceAttr() const
{
    return GetPrim().GetAttribute(AL_USDMayaSchemasTokens->mayaNamespace);
}

UsdAttribute
AL_usd_MayaReference::CreateMayaNamespaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(AL_USDMayaSchemasTokens->mayaNamespace,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
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
AL_usd_MayaReference::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        AL_USDMayaSchemasTokens->mayaReference,
        AL_USDMayaSchemasTokens->mayaNamespace,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomXformable::GetSchemaAttributeNames(true),
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
