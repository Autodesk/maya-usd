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
#include "ALMayaReference.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usd/typed.h>

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaUsd_SchemasALMayaReference, TfType::Bases<MayaUsd_SchemasMayaReference>>();

    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ALMayaReference")
    // to find TfType<MayaUsd_SchemasALMayaReference>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, MayaUsd_SchemasALMayaReference>("ALMayaReference");
}

/* virtual */
MayaUsd_SchemasALMayaReference::~MayaUsd_SchemasALMayaReference() { }

/* static */
MayaUsd_SchemasALMayaReference
MayaUsd_SchemasALMayaReference::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return MayaUsd_SchemasALMayaReference();
    }
    return MayaUsd_SchemasALMayaReference(stage->GetPrimAtPath(path));
}

/* static */
MayaUsd_SchemasALMayaReference
MayaUsd_SchemasALMayaReference::Define(const UsdStagePtr& stage, const SdfPath& path)
{
    static TfToken usdPrimTypeName("ALMayaReference");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return MayaUsd_SchemasALMayaReference();
    }
    return MayaUsd_SchemasALMayaReference(stage->DefinePrim(path, usdPrimTypeName));
}

#if PXR_VERSION >= 2108
/* virtual */
UsdSchemaKind MayaUsd_SchemasALMayaReference::_GetSchemaKind() const
{
    return MayaUsd_SchemasALMayaReference::schemaKind;
}
#else
/* virtual */
UsdSchemaType MayaUsd_SchemasALMayaReference::_GetSchemaType() const
{
    return MayaUsd_SchemasALMayaReference::schemaType;
}
#endif

/* static */
const TfType& MayaUsd_SchemasALMayaReference::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<MayaUsd_SchemasALMayaReference>();
    return tfType;
}

/* static */
bool MayaUsd_SchemasALMayaReference::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType& MayaUsd_SchemasALMayaReference::_GetTfType() const { return _GetStaticTfType(); }

/*static*/
const TfTokenVector& MayaUsd_SchemasALMayaReference::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames = MayaUsd_SchemasMayaReference::GetSchemaAttributeNames(true);

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
