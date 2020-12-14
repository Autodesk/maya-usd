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
#include "translatorMaterial.h"

#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/iterator.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (inputs)
    (varname)
);
// clang-format on

namespace {
// We want to know if this material is a specialization that was created to handle UV mappings on
// export. For details, see the _UVMappingManager class in ..\shading\shadingModeExporterContext.cpp
//
bool _IsMergeableMaterial(const UsdShadeMaterial& shadeMaterial)
{
    if (!shadeMaterial || !shadeMaterial.HasBaseMaterial()) {
        return false;
    }

    // Check for materials created by _UVMappingManager::getMaterial(). This code could probably be
    // expanded to be more generic and handle more complex composition arcs at a later stage.

    UsdPrimCompositionQuery query(shadeMaterial.GetPrim());
    if (query.GetCompositionArcs().size() != 2) {
        // Materials created by the _UVMappingManager have only 2 arcs:
        return false;
    }

    // This is a little more robust than grabbing a specific arc index.
    UsdPrimCompositionQuery::Filter filter;
    filter.arcTypeFilter = UsdPrimCompositionQuery::ArcTypeFilter::Specialize;
    query.SetFilter(filter);
    std::vector<UsdPrimCompositionQueryArc> arcs = query.GetCompositionArcs();
    const UsdPrimCompositionQueryArc        specializationArc = arcs.front();

    const SdfLayerHandle    layer = specializationArc.GetIntroducingLayer();
    const SdfPrimSpecHandle primSpec = layer->GetPrimAtPath(shadeMaterial.GetPath());

    // If the primSpec that specializes the base material introduces other
    // namespace children, it can't be merged.
    if (!primSpec->GetNameChildren().empty()) {
        return false;
    }

    // Check that the only properties authored are varname inputs.
    for (const SdfPropertySpecHandle& propSpec : primSpec->GetProperties()) {
        const SdfPath propPath = propSpec->GetPath();

        const std::vector<std::string> splitName = SdfPath::TokenizeIdentifier(propPath.GetName());
        // We allow only ["inputs", "<texture_name>", "varname"]
        if (splitName.size() != 3u || splitName[0u] != _tokens->inputs.GetString()
            || splitName[2u] != _tokens->varname.GetString()) {
            return false;
        }
    }

    return true;
}
} // namespace

/* static */
MObject UsdMayaTranslatorMaterial::Read(
    const UsdMayaJobImportArgs& jobArguments,
    const UsdShadeMaterial&     shadeMaterial,
    const UsdGeomGprim&         boundPrim,
    UsdMayaPrimReaderContext*   context)
{
    if (jobArguments.shadingModes.empty()) {
        return MObject();
    }

    UsdMayaShadingModeImportContext c(shadeMaterial, boundPrim, context);

    MObject shadingEngine;

    if (c.GetCreatedObject(shadeMaterial.GetPrim(), &shadingEngine)) {
        return shadingEngine;
    }

    if (_IsMergeableMaterial(shadeMaterial)) {
        // Use the base material instead
        return Read(jobArguments, shadeMaterial.GetBaseMaterial(), boundPrim, context);
    }

    UsdMayaJobImportArgs localArguments = jobArguments;
    for (const auto& shadingMode : jobArguments.shadingModes) {
        if (shadingMode.mode == UsdMayaShadingModeTokens->none) {
            break;
        }
        if (UsdMayaShadingModeImporter importer
            = UsdMayaShadingModeRegistry::GetImporter(shadingMode.mode)) {
            localArguments.shadingModes = UsdMayaJobImportArgs::ShadingModes(1, shadingMode);
            shadingEngine = importer(&c, localArguments);
        }

        if (!shadingEngine.isNull()) {
            c.AddCreatedObject(shadeMaterial.GetPrim(), shadingEngine);
            return shadingEngine;
        }
    }
    return shadingEngine;
}

namespace {
using _UVBindings = std::map<TfToken, TfToken>;

static _UVBindings
_GetUVBindingsFromMaterial(const UsdShadeMaterial& material, UsdMayaPrimReaderContext* context)
{
    _UVBindings retVal;

    if (!material || !context) {
        return retVal;
    }
    const bool isMergeable = _IsMergeableMaterial(material);
    // Find out the nodes requiring mapping. This code has deep knowledge of how the mappings are
    // exported. See the _UVMappingManager class in ..\shading\shadingModeExporterContext.cpp for
    // details.
    for (const UsdShadeInput& input : material.GetInputs()) {
        const UsdAttribute&      usdAttr = input.GetAttr();
        std::vector<std::string> splitName = usdAttr.SplitName();
        if (splitName.size() != 3 || splitName[2] != _tokens->varname.GetString()) {
            continue;
        }
        VtValue val;
        usdAttr.Get(&val);
        if (!val.IsHolding<TfToken>()) {
            continue;
        }
        SdfPath nodePath = isMergeable
            ? material.GetBaseMaterial().GetPath().AppendChild(TfToken(splitName[1]))
            : material.GetPath().AppendChild(TfToken(splitName[1]));
        MObject           mayaNode = context->GetMayaNode(nodePath, false);
        MStatus           status;
        MFnDependencyNode depFn(mayaNode, &status);
        if (!status) {
            continue;
        }
        retVal[val.UncheckedGet<TfToken>()] = TfToken(depFn.name().asChar());
    }

    return retVal;
}

static void _BindUVs(const MDagPath& shapeDagPath, const _UVBindings& uvBindings)
{
    if (uvBindings.empty()) {
        return;
    }

    MStatus status;
    MFnMesh meshFn(shapeDagPath, &status);
    if (!status) {
        return;
    }

    MStringArray uvSets;
    meshFn.getUVSetNames(uvSets);

    // We explicitly skip uvSet[0] since it is the default in Maya and does not require explicit
    // linking:
    for (unsigned int uvSetIndex = 1; uvSetIndex < uvSets.length(); uvSetIndex++) {
        TfToken                     uvSetName(uvSets[uvSetIndex].asChar());
        _UVBindings::const_iterator iter = uvBindings.find(uvSetName);
        if (iter == uvBindings.cend()) {
            continue;
        }
        MString uvLinkCommand("uvLink -make -uvs \"");
        uvLinkCommand += shapeDagPath.fullPathName();
        uvLinkCommand += ".uvSet[";
        uvLinkCommand += uvSetIndex;
        uvLinkCommand += "].uvSetName\" -t \"";
        uvLinkCommand += iter->second.GetText(); // texture name
        uvLinkCommand += "\";";
        status = MGlobal::executeCommand(uvLinkCommand);
    }
}
} // namespace

static bool _AssignMaterialFaceSet(
    const MObject&     shadingEngine,
    const MDagPath&    shapeDagPath,
    const VtIntArray&  faceIndices,
    const _UVBindings& faceUVBindings)
{
    MStatus status;

    // Create component object using single indexed
    // components, i.e. face indices.
    MFnSingleIndexedComponent compFn;
    MObject                   faceComp = compFn.create(MFn::kMeshPolygonComponent, &status);
    if (!status) {
        TF_RUNTIME_ERROR("Failed to create face component.");
        return false;
    }

    MIntArray mFaces;
    TF_FOR_ALL(fIdxIt, faceIndices) { mFaces.append(*fIdxIt); }
    compFn.addElements(mFaces);

    MFnSet seFnSet(shadingEngine, &status);
    if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
        status = seFnSet.addMember(shapeDagPath, faceComp);
        if (!status) {
            TF_RUNTIME_ERROR(
                "Could not add component to shadingEngine %s.", seFnSet.name().asChar());
            return false;
        }
        _BindUVs(shapeDagPath, faceUVBindings);
    }

    return true;
}

bool UsdMayaTranslatorMaterial::AssignMaterial(
    const UsdMayaJobImportArgs& jobArguments,
    const UsdGeomGprim&         primSchema,
    MObject                     shapeObj,
    UsdMayaPrimReaderContext*   context)
{
    // if we don't have a valid context, we make one temporarily.  This is to
    // make sure we don't duplicate shading nodes within a material.
    UsdMayaPrimReaderContext::ObjectRegistry tmpRegistry;
    UsdMayaPrimReaderContext                 tmpContext(&tmpRegistry);
    if (!context) {
        context = &tmpContext;
    }

    MDagPath shapeDagPath;
    MFnDagNode(shapeObj).getPath(shapeDagPath);

    MStatus                          status;
    const UsdShadeMaterialBindingAPI bindingAPI(primSchema.GetPrim());
    UsdShadeMaterial                 meshMaterial = bindingAPI.ComputeBoundMaterial();
    _UVBindings                      uvBindings;
    MObject                          shadingEngine
        = UsdMayaTranslatorMaterial::Read(jobArguments, meshMaterial, primSchema, context);

    if (shadingEngine.isNull()) {
        status = UsdMayaUtil::GetMObjectByName("initialShadingGroup", shadingEngine);
        if (status != MS::kSuccess) {
            return false;
        }
    } else {
        uvBindings = _GetUVBindingsFromMaterial(meshMaterial, context);
    }

    // If the gprim does not have a material faceSet which represents per-face
    // shader assignments, assign the shading engine to the entire gprim.
    const std::vector<UsdGeomSubset> faceSubsets
        = UsdShadeMaterialBindingAPI(primSchema.GetPrim()).GetMaterialBindSubsets();

    if (faceSubsets.empty()) {
        MFnSet seFnSet(shadingEngine, &status);
        if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
            status = seFnSet.addMember(shapeObj);
            if (!status) {
                TF_RUNTIME_ERROR(
                    "Could not add shadingEngine for '%s'.", shapeDagPath.fullPathName().asChar());
            }
            _BindUVs(shapeDagPath, uvBindings);
        }

        return true;
    }

    if (!faceSubsets.empty()) {

        int               faceCount = 0;
        const UsdGeomMesh mesh(primSchema);
        if (mesh) {
            VtIntArray faceVertexCounts;
            mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
            faceCount = faceVertexCounts.size();
        }

        if (faceCount == 0) {
            TF_RUNTIME_ERROR(
                "Unable to get face count for gprim at path <%s>.", primSchema.GetPath().GetText());
            return false;
        }

        std::string reasonWhyNotPartition;

        const bool validPartition = UsdGeomSubset::ValidateSubsets(
            faceSubsets, faceCount, UsdGeomTokens->partition, &reasonWhyNotPartition);
        if (!validPartition) {
            TF_WARN(
                "Face-subsets on <%s> don't form a valid partition: %s",
                primSchema.GetPath().GetText(),
                reasonWhyNotPartition.c_str());

            VtIntArray unassignedIndices
                = UsdGeomSubset::GetUnassignedIndices(faceSubsets, faceCount);
            if (!_AssignMaterialFaceSet(
                    shadingEngine, shapeDagPath, unassignedIndices, uvBindings)) {
                return false;
            }
        }

        for (const auto& subset : faceSubsets) {
            const UsdShadeMaterialBindingAPI subsetBindingAPI(subset.GetPrim());
            const UsdShadeMaterial boundMaterial = subsetBindingAPI.ComputeBoundMaterial();
            if (boundMaterial) {
                MObject faceSubsetShadingEngine = UsdMayaTranslatorMaterial::Read(
                    jobArguments, boundMaterial, UsdGeomGprim(), context);

                _UVBindings faceUVBindings;
                if (faceSubsetShadingEngine.isNull()) {
                    status = UsdMayaUtil::GetMObjectByName(
                        "initialShadingGroup", faceSubsetShadingEngine);
                    if (status != MS::kSuccess) {
                        return false;
                    }
                } else {
                    faceUVBindings = _GetUVBindingsFromMaterial(boundMaterial, context);
                }

                // Only transfer the first timeSample or default indices, if
                // there are no time-samples.
                VtIntArray indices;
                subset.GetIndicesAttr().Get(&indices, UsdTimeCode::EarliestTime());

                if (!_AssignMaterialFaceSet(
                        faceSubsetShadingEngine, shapeDagPath, indices, faceUVBindings)) {
                    return false;
                }
            }
        }
    }

    return true;
}

/* static */
void UsdMayaTranslatorMaterial::ExportShadingEngines(
    UsdMayaWriteJobContext&                  writeJobContext,
    const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap)
{
    const TfToken& shadingMode = writeJobContext.GetArgs().shadingMode;
    if (shadingMode == UsdMayaShadingModeTokens->none) {
        return;
    }

    if (auto exporterCreator = UsdMayaShadingModeRegistry::GetExporter(shadingMode)) {
        if (auto exporter = exporterCreator()) {
            exporter->DoExport(writeJobContext, dagPathToUsdMap);
        }
    } else {
        TF_RUNTIME_ERROR("No shadingMode '%s' found.", shadingMode.GetText());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
