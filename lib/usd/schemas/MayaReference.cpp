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
#include "MayaReference.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usd/typed.h>

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaUsd_SchemasMayaReference, TfType::Bases<UsdGeomXformable>>();

    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("MayaReference")
    // to find TfType<MayaUsd_SchemasMayaReference>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, MayaUsd_SchemasMayaReference>("MayaReference");
}

/* virtual */
MayaUsd_SchemasMayaReference::~MayaUsd_SchemasMayaReference() { }

/* static */
MayaUsd_SchemasMayaReference
MayaUsd_SchemasMayaReference::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return MayaUsd_SchemasMayaReference();
    }
    return MayaUsd_SchemasMayaReference(stage->GetPrimAtPath(path));
}

/* static */
MayaUsd_SchemasMayaReference
MayaUsd_SchemasMayaReference::Define(const UsdStagePtr& stage, const SdfPath& path)
{
    static TfToken usdPrimTypeName("MayaReference");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return MayaUsd_SchemasMayaReference();
    }
    return MayaUsd_SchemasMayaReference(stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind MayaUsd_SchemasMayaReference::_GetSchemaKind() const
{
    return MayaUsd_SchemasMayaReference::schemaKind;
}

/* static */
const TfType& MayaUsd_SchemasMayaReference::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<MayaUsd_SchemasMayaReference>();
    return tfType;
}

/* static */
bool MayaUsd_SchemasMayaReference::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType& MayaUsd_SchemasMayaReference::_GetTfType() const { return _GetStaticTfType(); }

UsdAttribute MayaUsd_SchemasMayaReference::GetMayaReferenceAttr() const
{
    return GetPrim().GetAttribute(MayaUsd_SchemasTokens->mayaReference);
}

UsdAttribute MayaUsd_SchemasMayaReference::CreateMayaReferenceAttr(
    VtValue const& defaultValue,
    bool           writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        MayaUsd_SchemasTokens->mayaReference,
        SdfValueTypeNames->Asset,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute MayaUsd_SchemasMayaReference::GetMayaNamespaceAttr() const
{
    return GetPrim().GetAttribute(MayaUsd_SchemasTokens->mayaNamespace);
}

UsdAttribute MayaUsd_SchemasMayaReference::CreateMayaNamespaceAttr(
    VtValue const& defaultValue,
    bool           writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        MayaUsd_SchemasTokens->mayaNamespace,
        SdfValueTypeNames->String,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute MayaUsd_SchemasMayaReference::GetMayaAutoEditAttr() const
{
    return GetPrim().GetAttribute(MayaUsd_SchemasTokens->mayaAutoEdit);
}

UsdAttribute MayaUsd_SchemasMayaReference::CreateMayaAutoEditAttr(
    VtValue const& defaultValue,
    bool           writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        MayaUsd_SchemasTokens->mayaAutoEdit,
        SdfValueTypeNames->Bool,
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
const TfTokenVector& MayaUsd_SchemasMayaReference::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        MayaUsd_SchemasTokens->mayaReference,
        MayaUsd_SchemasTokens->mayaNamespace,
        MayaUsd_SchemasTokens->mayaAutoEdit,
    };
    static TfTokenVector allNames
        = _ConcatenateAttributeNames(UsdGeomXformable::GetSchemaAttributeNames(true), localNames);

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
