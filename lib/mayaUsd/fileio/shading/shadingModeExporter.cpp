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
#include "shadingModeExporter.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/collectionAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdUtils/authoring.h>

#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>

#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((materialNamespace, "material:"))
);
// clang-format on

UsdMayaShadingModeExporter::UsdMayaShadingModeExporter() { }

/* virtual */
UsdMayaShadingModeExporter::~UsdMayaShadingModeExporter() { }

static TfToken _GetCollectionName(const UsdShadeMaterial& mat)
{
    return TfToken(_tokens->materialNamespace.GetString() + mat.GetPrim().GetName().GetString());
}

// Returns the set of root prim paths present in the given path-set.
static SdfPathSet _GetRootPaths(const SdfPathSet& paths)
{
    SdfPathSet result;
    for (const auto& p : paths) {
        const std::string& pathString = p.GetString();
        // Skip pseudo-root.
        if (!TF_VERIFY(pathString.size() > 1u, "Invalid path '%s'", pathString.c_str())) {
            continue;
        }

        // This should be faster than calling GetPrefixes()[0].
        SdfPath rootPath(pathString.substr(0, pathString.find('/', 1)));
        result.insert(rootPath);
    }
    return result;
}

void UsdMayaShadingModeExporter::DoExport(
    UsdMayaWriteJobContext&                  writeJobContext,
    const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap)
{
    const UsdMayaJobExportArgs& exportArgs = writeJobContext.GetArgs();
    const UsdStageRefPtr&       stage = writeJobContext.GetUsdStage();

    const SdfPath& materialCollectionsPath = exportArgs.exportMaterialCollections
        ? exportArgs.materialCollectionsPath
        : SdfPath::EmptyPath();

    UsdPrim materialCollectionsPrim;
    if (!materialCollectionsPath.IsEmpty()) {
        materialCollectionsPrim = stage->OverridePrim(materialCollectionsPath);
        if (!materialCollectionsPrim) {
            TF_WARN(
                "Error: could not override prim at path <%s>. One of the "
                "ancestors of the path must be inactive or an instance root. "
                "Not exporting material collections!",
                materialCollectionsPath.GetText());
        }
    }

    UsdMayaShadingModeExportContext context(MObject(), writeJobContext, dagPathToUsdMap);

    PreExport(&context);

    using MaterialAssignments = std::vector<std::pair<TfToken, SdfPathSet>>;
    MaterialAssignments matAssignments;

    std::vector<UsdShadeMaterial> exportedMaterials;

    MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);
    for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
        MObject shadingEngine(shadingEngineIter.thisNode());
        context.SetShadingEngine(shadingEngine);

        UsdShadeMaterial mat;
        SdfPathSet       boundPrimPaths;
        Export(context, &mat, &boundPrimPaths);

        if (mat && !boundPrimPaths.empty()) {
            exportedMaterials.push_back(mat);
            matAssignments.push_back(std::make_pair(_GetCollectionName(mat), boundPrimPaths));
        }
    }

    context.SetShadingEngine(MObject());
    PostExport(context);

    if ((materialCollectionsPrim || exportArgs.exportCollectionBasedBindings)
        && !matAssignments.empty()) {
        if (!materialCollectionsPrim) {
            // Find a place to export the material collections. The collections
            // can live anywhere in the scene, but the collection-based bindings
            // must live at or above the prims being bound.
            //
            // This computes the first root prim below which a material has
            // been exported.
            SdfPath rootPrimPath = exportedMaterials[0].GetPath().GetPrefixes()[0];
            materialCollectionsPrim = stage->GetPrimAtPath(rootPrimPath);
            TF_VERIFY(
                materialCollectionsPrim,
                "Could not get prim at path <%s>. Not exporting material "
                "collections / bindings.",
                rootPrimPath.GetText());
            return;
        }

        std::vector<UsdCollectionAPI> collections
            = UsdUtilsCreateCollections(matAssignments, materialCollectionsPrim);

        if (exportArgs.exportCollectionBasedBindings) {
            for (size_t i = 0u; i < exportedMaterials.size(); ++i) {
                const UsdShadeMaterial& mat = exportedMaterials[i];
                const UsdCollectionAPI& coll = collections[i];

                // If the all the paths are under the prim with the materialBind
                // collections, export the binding on the prim.
                const SdfPathSet& paths = matAssignments[i].second;
                if (std::all_of(
                        paths.begin(), paths.end(), [materialCollectionsPrim](const SdfPath& p) {
                            return p.HasPrefix(materialCollectionsPrim.GetPath());
                        })) {

                    // Materials are named uniquely in maya, so we can
                    // skip passing in the 'bindingName' param.
                    UsdShadeMaterialBindingAPI bindingAPI
                        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<
                            UsdShadeMaterialBindingAPI>(materialCollectionsPrim);
                    bindingAPI.Bind(coll, mat);
                    continue;
                }

                // If all the paths are not under materialCollectionsPrim, then
                // figure out the set of root paths at which to export the
                // collection-based bindings.
                const SdfPathSet rootPaths = _GetRootPaths(matAssignments[i].second);
                for (auto& rootPath : rootPaths) {
                    auto rootPrim = stage->GetPrimAtPath(rootPath);
                    if (!TF_VERIFY(
                            rootPrim, "Could not get prim at path <%s>", rootPath.GetText())) {
                        continue;
                    }

                    UsdShadeMaterialBindingAPI bindingAPI
                        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<
                            UsdShadeMaterialBindingAPI>(rootPrim);
                    bindingAPI.Bind(coll, mat);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
