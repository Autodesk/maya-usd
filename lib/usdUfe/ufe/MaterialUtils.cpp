//
// Copyright 2026 Autodesk
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
#include "MaterialUtils.h"

#include <usdUfe/ufe/Utils.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <usdUfe/ufe/UsdShaderNodeDef.h>
#endif
#include <usdUfe/utils/Utils.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/schemaBase.h>
#include <pxr/usd/usdShade/material.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {
namespace {

bool isNodeTypeInList(
    const Ufe::SceneItem::Ptr&      sceneItem,
    const std::vector<std::string>& nodeTypeList,
    bool                            checkAllAncestors)
{
    const auto canonicalName = TfType::Find<UsdSchemaBase>().FindDerivedByName(
        UsdUfe::getSceneItemNodeType(sceneItem).c_str());

    if (canonicalName.IsUnknown()) {
        return false;
    }

    if (std::find(nodeTypeList.begin(), nodeTypeList.end(), canonicalName.GetTypeName().c_str())
        != nodeTypeList.end()) {
        return true;
    }

    const auto& ancestors = sceneItem->ancestorNodeTypes();
    for (const auto& ancestorType : ancestors) {
        const auto canonicalAncestorName
            = TfType::Find<UsdSchemaBase>().FindDerivedByName(ancestorType.c_str());

        if (canonicalAncestorName == canonicalName) {
            continue;
        }

        if (std::find(
                nodeTypeList.begin(),
                nodeTypeList.end(),
                canonicalAncestorName.GetTypeName().c_str())
            != nodeTypeList.end()) {
            return true;
        }

        if (!checkAllAncestors) {
            return false;
        }
    }

    return false;
}

#ifndef UFE_V4_FEATURES_AVAILABLE
void appendMaterialXMaterials(std::multimap<std::string, Ufe::ContextItem>& entries)
{
    // TODO: Replace hard-coded materials with dynamically generated list.
    static const std::vector<std::pair<std::string, std::string>> vettedSurfaces
        = { { "ND_standard_surface_surfaceshader", "Standard Surface" },
            { "ND_gltf_pbr_surfaceshader", "glTF PBR" },
            { "ND_UsdPreviewSurface_surfaceshader", "USD Preview Surface" },
            { "ND_open_pbr_surface_surfaceshader", "OpenPBR Surface" } };
    auto& sdrRegistry = SdrRegistry::GetInstance();
    for (auto&& info : vettedSurfaces) {
        auto shaderDef = sdrRegistry.GetShaderNodeByIdentifier(TfToken(info.first));
        if (!shaderDef) {
            continue;
        }
        entries.emplace("MaterialX", Ufe::ContextItem(info.first, info.second));
    }
}

void appendArnoldMaterials(std::multimap<std::string, Ufe::ContextItem>& entries)
{
    auto& sdrRegistry = SdrRegistry::GetInstance();
#if PXR_VERSION >= 2505
    const auto sourceTypes = sdrRegistry.GetAllShaderNodeSourceTypes();
#else
    const auto sourceTypes = sdrRegistry.GetAllNodeSourceTypes();
#endif
    const bool hasArnoldMaterials
        = std::find(sourceTypes.cbegin(), sourceTypes.cend(), TfToken("arnold"))
        != sourceTypes.cend();

    if (hasArnoldMaterials) {
        // TODO: Replace hard-coded materials with dynamically generated list.
        entries.emplace(
            "Arnold", Ufe::ContextItem("arnold:standard_surface", "AI Standard Surface"));
    }
}

void appendUsdMaterials(std::multimap<std::string, Ufe::ContextItem>& entries)
{
    entries.emplace("USD", Ufe::ContextItem("UsdPreviewSurface", "USD Preview Surface"));
}
#endif

} // namespace

std::multimap<std::string, Ufe::ContextItem> getMaterialsFromRenderers()
{
    // TODO: The list of returned materials is currently hard-coded and only for select,
    // known renderers. We should populate the material lists dynamically based on what the
    // installed renderers report as supported materials.

    std::multimap<std::string, Ufe::ContextItem> entries;

#ifdef UFE_V4_FEATURES_AVAILABLE
    const auto shaderNodeDefs = GetSurfaceShaderNodeDefs();
    for (const auto& nodeDef : shaderNodeDefs) {
        auto ufeNodeDef = UsdShaderNodeDef::create(nodeDef);
        entries.emplace(
            ufeNodeDef->classification(ufeNodeDef->nbClassifications() - 1),
            Ufe::ContextItem(
                nodeDef->GetIdentifier().GetString(), prettifyName(ufeNodeDef->classification(0))));
    }
#else
    appendUsdMaterials(entries);
    appendArnoldMaterials(entries);
    appendMaterialXMaterials(entries);
#endif

    return entries;
}

std::vector<SdfPath> getMaterialsInStage(const Ufe::Path& contextPath)
{
    std::vector<SdfPath> materials;
    if (auto stage = getStage(contextPath)) {
        for (const auto& prim : stage->Traverse()) {
            if (UsdShadeMaterial(prim)) {
                materials.emplace_back(prim.GetPath());
            }
        }
    }
    return materials;
}

bool canAssignMaterialToNodeType(const Ufe::SceneItem::Ptr& sceneItem)
{
    if (!sceneItem) {
        return false;
    }

    const std::vector<std::string> allowNodeTypes = { "UsdGeomImageable", "UsdGeomSubset" };
    const std::vector<std::string> rejectNodeTypes = { "MayaUsd_SchemasMayaReference",
                                                       "MayaUsd_SchemasALMayaReference",
                                                       "UsdGeomCamera",
                                                       "UsdMediaSpatialAudio",
                                                       "UsdProcGenerativeProcedural",
                                                       "UsdPhysicsJoint",
                                                       "UsdSkelRoot",
                                                       "UsdSkelSkeleton",
                                                       "UsdVolField3DAsset",
                                                       "UsdVolFieldAsset",
                                                       "UsdVolFieldBase",
                                                       "UsdVolOpenVDBAsset" };

    if (isNodeTypeInList(sceneItem, rejectNodeTypes, false)) {
        return false;
    }
    if (isNodeTypeInList(sceneItem, allowNodeTypes, true)) {
        return true;
    }
    return false;
}

} // namespace USDUFE_NS_DEF
