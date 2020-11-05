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
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerTree.h>
#include <pxr/usd/sdf/path.h>
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

TF_DEFINE_PRIVATE_TOKENS(_tokens, (inputs)(varname));

namespace {
// We want to know if this material is a specialization that was created to handle UV mappings on
// export. For details, see the _UVMappingManager class in ..\shading\shadingModeExporterContext.cpp
//
// If the root layer contains anything that is not a varname input, we do not consider it a
// mergeable material.
bool _IsMergeableMaterial(const UsdShadeMaterial& shadeMaterial)
{
    if (!shadeMaterial || !shadeMaterial.HasBaseMaterial()) {
        return false;
    }
    const PcpPrimIndex&   primIndex = shadeMaterial.GetPrim().GetPrimIndex();
    PcpNodeRef            primRoot = primIndex.GetRootNode();
    SdfPath               primPath = primIndex.GetPath();
    const SdfLayerHandle& layer = primRoot.GetLayerStack()->GetLayerTree()->GetLayer();

    bool retVal = true;
    auto testFunction = [&](const SdfPath& path) {
        // If we traverse anything that is not a varname specialization, we return false.
        if (path == primPath) {
            return;
        }
        if (path.GetParentPath() != primPath || !path.IsPrimPropertyPath()) {
            retVal = false;
            return;
        }
        std::vector<std::string> splitName = SdfPath::TokenizeIdentifier(path.GetName());
        // We allow only ["inputs", "XXX", "varname"]
        if (splitName.size() != 3 || splitName[0] != _tokens->inputs.GetString()
            || splitName[2] != _tokens->varname.GetString()) {
            retVal = false;
        }
    };

    layer->Traverse(primPath, testFunction);

    return retVal;
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
}

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
    MFnMesh meshFn(shapeDagPath.node(), &status);
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
