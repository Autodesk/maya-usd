//
// Copyright 2024 Autodesk
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
#ifndef USDUFE_UTILS_SCHEMAS_H
#define USDUFE_UTILS_SCHEMAS_H

#include <usdUfe/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/usd/prim.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

/// @brief Describe a single schema.
struct SchemaInfo
{
    std::string     pluginName;
    PXR_NS::TfType  schemaType;
    PXR_NS::TfToken schemaTypeName;
    bool            isMultiApply;
};

/// @brief The map of currently known single-apply and multiple-apply schemas indexed by type name.
using KnownSchemas = std::map<PXR_NS::TfToken, SchemaInfo>;

/// @brief Return the list of currently-known schemas.
/// @return the known schemas.
USDUFE_PUBLIC
KnownSchemas getKnownApplicableSchemas();

/// @brief finds a schema by its type name.
/// @param schemaTypeName the schema type to lookup.
/// @return the schema info of the schema, if found.
USDUFE_PUBLIC
std::shared_ptr<SchemaInfo> findSchemasByTypeName(const PXR_NS::TfToken& schemaTypeName);

/// @brief finds a schema by its type name.
/// @param schemaTypeName the schema type to lookup.
/// @param knownSchemas the list of known schemas.
/// @return the schema info of the schema, if found.
USDUFE_PUBLIC
std::shared_ptr<SchemaInfo>
findSchemasByTypeName(const PXR_NS::TfToken& schemaTypeName, const KnownSchemas& knownSchemas);

/// @brief apply the given single-apply schema type to the given prim.
/// @param prim the prim to receive the schema.
/// @param schemaType the schema type to apply.
/// @return true if the application succeeded.
USDUFE_PUBLIC
bool applySchemaToPrim(PXR_NS::UsdPrim& prim, const PXR_NS::TfType& schemaType);

/// @brief apply the given single-apply schema type to the given prim.
/// @param prim the prim to receive the schema.
/// @param info the schema info to apply.
/// @return true if the application succeeded.
USDUFE_PUBLIC
bool applySchemaInfoToPrim(PXR_NS::UsdPrim& prim, const SchemaInfo& info);

/// @brief apply the given multi-apply schema type to the given prim.
/// @param prim the prim to receive the schema.
/// @param schemaType the schema type to apply.
/// @param instanceName the unique name of the new schema application.
/// @return true if the application succeeded.
USDUFE_PUBLIC
bool applyMultiSchemaToPrim(
    PXR_NS::UsdPrim&       prim,
    const PXR_NS::TfType&  schemaType,
    const PXR_NS::TfToken& instanceName);

/// @brief apply the given multi-apply schema type to the given prim.
/// @param prim the prim to receive the schema.
/// @param schemaType the schema type to apply.
/// @param instanceName the unique name of the new schema application.
/// @return true if the application succeeded.
USDUFE_PUBLIC
bool applyMultiSchemaInfoToPrim(
    PXR_NS::UsdPrim&       prim,
    const SchemaInfo&      info,
    const PXR_NS::TfToken& instanceName);

/// @brief remove the given single-apply schema type from the given prim.
/// @param prim the prim to lose the schema.
/// @param schemaType the schema type to remove.
/// @return true if the removal succeeded.
USDUFE_PUBLIC
bool removeSchemaFromPrim(PXR_NS::UsdPrim& prim, const PXR_NS::TfType& schemaType);

/// @brief remove the given single-apply schema type from the given prim.
/// @param prim the prim to lose the schema.
/// @param info the schema info to remove.
/// @return true if the removal succeeded.
USDUFE_PUBLIC
bool removeSchemaInfoFromPrim(PXR_NS::UsdPrim& prim, const SchemaInfo& info);

/// @brief remove the given single-apply schema type from the given prim.
/// @param prim the prim to lose the schema.
/// @param schemaType the schema type to remove.
/// @param instanceName the unique name of the new schema application.
/// @return true if the removal succeeded.
USDUFE_PUBLIC
bool removeMultiSchemaFromPrim(
    PXR_NS::UsdPrim&       prim,
    const PXR_NS::TfType&  schemaType,
    const PXR_NS::TfToken& instanceName);

/// @brief remove the given single-apply schema type from the given prim.
/// @param prim the prim to lose the schema.
/// @param info the schema info to remove.
/// @param instanceName the unique name of the new schema application.
/// @return true if the removal succeeded.
USDUFE_PUBLIC
bool removeMultiSchemaInfoFromPrim(
    PXR_NS::UsdPrim&       prim,
    const SchemaInfo&      info,
    const PXR_NS::TfToken& instanceName);

/// @brief get all schemas that are applied to the given prim.
/// @return the list of applied schemas.
USDUFE_PUBLIC
std::vector<PXR_NS::TfToken> getPrimAppliedSchemas(const PXR_NS::UsdPrim& prim);

/// @brief get all (the union) the applied schemas that the given prims have.
/// @return the set of applied schemas of all the given prims.
USDUFE_PUBLIC
std::set<PXR_NS::TfToken> getPrimsAppliedSchemas(const std::vector<PXR_NS::UsdPrim>& prims);

} // namespace USDUFE_NS_DEF

#endif
