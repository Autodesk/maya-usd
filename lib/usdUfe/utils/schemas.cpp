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

#include "schemas.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/usd/usd/schemaRegistry.h>

namespace USDUFE_NS_DEF {

namespace {

} // namespace

KnownSchemas getKnownApplicableSchemas()
{
    KnownSchemas knownSchemas;

    PXR_NS::UsdSchemaRegistry& schemaReg = PXR_NS::UsdSchemaRegistry::GetInstance();
    PXR_NS::PlugRegistry&      plugReg = PXR_NS::PlugRegistry::GetInstance();

    std::set<PXR_NS::TfType> allSchemaTypes;
    PXR_NS::TfType::FindByName("UsdAPISchemaBase").GetAllDerivedTypes(&allSchemaTypes);

    for (const PXR_NS::TfType& schemaType : allSchemaTypes) {
        if (!schemaReg.IsAppliedAPISchema(schemaType))
            continue;

        PXR_NS::PlugPluginPtr plugin = plugReg.GetPluginForType(schemaType);
        if (!plugin)
            continue;

        const PXR_NS::TfToken typeName = schemaReg.GetAPISchemaTypeName(schemaType);
        const bool            isMulti = schemaReg.IsMultipleApplyAPISchema(schemaType);

        SchemaInfo info = { plugin->GetName(), schemaType, typeName, isMulti };
        knownSchemas[typeName] = std::move(info);
    }

    return knownSchemas;
}

std::shared_ptr<SchemaInfo>
findSchemasByTypeName(const PXR_NS::TfToken& schemaTypeName, const KnownSchemas& knownSchemas)
{
    const auto iter = knownSchemas.find(schemaTypeName);
    if (iter == knownSchemas.end())
        return std::shared_ptr<SchemaInfo>();

    return std::make_shared<SchemaInfo>(iter->second);
}

std::shared_ptr<SchemaInfo> findSchemasByTypeName(const PXR_NS::TfToken& schemaTypeName)
{
    return findSchemasByTypeName(schemaTypeName, UsdUfe::getKnownApplicableSchemas());
}

bool applySchemaToPrim(PXR_NS::UsdPrim& prim, const PXR_NS::TfType& schemaType)
{
    return prim.ApplyAPI(schemaType);
}

bool applySchemaToPrim(PXR_NS::UsdPrim& prim, const SchemaInfo& info)
{
    return applySchemaToPrim(prim, info.schemaType);
}

bool applyMultiSchemaToPrim(
    PXR_NS::UsdPrim&       prim,
    const PXR_NS::TfType&  schemaType,
    const PXR_NS::TfToken& instanceName)
{
    return prim.ApplyAPI(schemaType, instanceName);
}

bool applyMultiSchemaToPrim(
    PXR_NS::UsdPrim&       prim,
    const SchemaInfo&      info,
    const PXR_NS::TfToken& instanceName)
{
    return applyMultiSchemaToPrim(prim, info.schemaType, instanceName);
}

std::vector<PXR_NS::TfToken> getPrimAppliedSchemas(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::UsdPrimTypeInfo& info = prim.GetPrimTypeInfo();
    return info.GetAppliedAPISchemas();
}

std::set<PXR_NS::TfToken> getPrimsAppliedSchemas(const std::vector<PXR_NS::UsdPrim>& prims)
{
    std::set<PXR_NS::TfToken> allSchemas;

    for (const PXR_NS::UsdPrim& prim : prims)
        for (const PXR_NS::TfToken& schema : getPrimAppliedSchemas(prim))
            allSchemas.insert(schema);

    return allSchemas;
}

} // namespace USDUFE_NS_DEF