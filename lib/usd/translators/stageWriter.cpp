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
#include "stageWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>

#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

void reportError(const char* msg, const MDagPath& dagPath)
{
    MString formatted;
    formatted.format(msg, dagPath.fullPathName());
    MGlobal::displayError(formatted);
}

void reportWarning(const char* msg, const MDagPath& dagPath)
{
    MString formatted;
    formatted.format(msg, dagPath.fullPathName());
    MGlobal::displayWarning(formatted);
}

std::string getUsdRefIdentifier(const UsdStage& stage, const MDagPath& dagPath)
{
    SdfLayerRefPtr rootLayer = stage.GetRootLayer();
    if (!rootLayer) {
        reportWarning(
            "Cannot export the stage in the proxy shape at path '^1s': no root layer\n", dagPath);
        return {};
    }

    if (rootLayer->IsAnonymous()) {
        reportWarning(
            "Cannot export the stage in the proxy shape at path '^1s': the root layer is "
            "anonymous\n",
            dagPath);
        return {};
    }

    return rootLayer->GetRealPath();
}

std::vector<std::pair<TfToken, SdfPath>> getReferencedRootPrims(const UsdStage& stage)
{
    std::vector<std::pair<TfToken, SdfPath>> rootPrims;

    UsdPrimSiblingRange usdRootPrims = stage.GetPseudoRoot().GetChildren();
    for (UsdPrim const& prim : usdRootPrims)
        rootPrims.emplace_back(prim.GetName(), prim.GetPath());

    return rootPrims;
}

UsdPrim createOverForUsdRef(UsdStage& stage, const SdfPath& basePath, const TfToken& primName)
{
    SdfPath primWithRefPath = basePath.AppendChild(primName.IsEmpty() ? TfToken("Top") : primName);
    return stage.OverridePrim(primWithRefPath);
}

// Returns the name of the root prim ancestor for a given path, or empty token if pseudo-root.
TfToken getRootPrimName(const SdfPath& path)
{
    if (path.IsEmpty() || path == SdfPath::AbsoluteRootPath())
        return {};

    // Walk up to find the root-level ancestor (child of the absolute root).
    SdfPath current = path;
    while (!current.GetParentPath().IsEmpty()
           && current.GetParentPath() != SdfPath::AbsoluteRootPath()) {
        current = current.GetParentPath();
    }
    return current.GetNameToken();
}

// After references are authored, relationships whose targets live under a different
// root prim will be broken because each root prim is in its own reference arc.
// This function finds such cross-root relationships in the proxy stage and authors
// local opinion overrides on the exported stage so those targets resolve correctly.
void fixCrossRootRelationships(
    UsdStage&      proxyStage,
    UsdStage&      exportedStage,
    const SdfPath& basePath)
{
    for (const UsdPrim& prim : proxyStage.Traverse()) {
        const SdfPath& primPath = prim.GetPath();
        TfToken        primRoot = getRootPrimName(primPath);
        if (primRoot.IsEmpty())
            continue;

        for (const UsdRelationship& rel : prim.GetRelationships()) {
            SdfPathVector targets;
            rel.GetTargets(&targets);

            bool            hasCrossRoot = false;
            SdfPathVector   remappedTargets;
            remappedTargets.reserve(targets.size());

            for (const SdfPath& target : targets) {
                TfToken targetRoot = getRootPrimName(target);
                if (!targetRoot.IsEmpty() && targetRoot != primRoot) {
                    hasCrossRoot = true;
                }
                // Remap: /RootX/... -> {basePath}/RootX/...
                remappedTargets.push_back(
                    target.ReplacePrefix(SdfPath::AbsoluteRootPath(), basePath));
            }

            if (!hasCrossRoot)
                continue;

            // Create an override prim at the corresponding exported path and
            // author the relationship with remapped targets.
            SdfPath overPath = primPath.ReplacePrefix(SdfPath::AbsoluteRootPath(), basePath);
            UsdPrim overPrim = exportedStage.OverridePrim(overPath);
            if (!overPrim)
                continue;

            UsdRelationship overRel = overPrim.CreateRelationship(rel.GetName());
            overRel.SetTargets(remappedTargets);
        }
    }
}
} // namespace

PXRUSDMAYA_REGISTER_WRITER(mayaUsdProxyShape, PxrUsdTranslators_StageWriter);

PxrUsdTranslators_StageWriter::PxrUsdTranslators_StageWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!GetDagPath().isValid()) {
        return;
    }

    _usdPrim = GetUsdStage()->DefinePrim(GetUsdPath(), TfToken("Xform"));
    if (!_usdPrim) {
        reportError("Could not create Xform for the stage at path '^1s'\n", GetDagPath());
        return;
    }

    // Per design, the transform prim above the reference is a component.
    UsdModelAPI modelPrim(_usdPrim);
    modelPrim.SetKind(KindTokens->component);
}

/* virtual */
void PxrUsdTranslators_StageWriter::Write(const UsdTimeCode& usdTime)
{
    // If export stages as USD references is off, don't export anything.
    // The base prim will be removed by pruning if left empty.
    if (!_GetExportArgs().exportStagesAsRefs)
        return;

    // Only create the USD reference at the default time. USD refs is a metadata
    // that does *not* change with time.
    if (usdTime != UsdTimeCode::Default())
        return;

    // This write the transform node.
    UsdMayaPrimWriter::Write(usdTime);

    // Retrieve some data we will need for the export.
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    MayaUsdProxyShapeBase* proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    if (!proxyShape) {
        reportError("Could not access the proxy shape at path '^1s'\n", GetDagPath());
        return;
    }

    UsdStageRefPtr proxyStage = proxyShape->getUsdStage();
    if (!proxyStage) {
        reportError("The proxy shape at path '^1s' has no USD stage\n", GetDagPath());
        return;
    }

    // Figure out the file path to the USD reference.
    std::string refIdentifier = getUsdRefIdentifier(*proxyStage, GetDagPath());
    if (refIdentifier.size() <= 0) {
        return;
    }

    if (MayaUsd::utils::isProxyShapePathRelative(*proxyShape)) {
        std::string baseDir = UsdMayaUtilFileSystem::getDir(_GetExportArgs().file);
        refIdentifier = UsdMayaUtilFileSystem::makePathRelativeTo(refIdentifier, baseDir).first;
    }

    // Create a reference for each root prim in the stage.
    std::vector<std::pair<TfToken, SdfPath>> rootPrims = getReferencedRootPrims(*proxyStage);
    for (const auto& rootPrim : rootPrims) {
        const TfToken& rootPrimName = rootPrim.first;
        const SdfPath& rootPrimPath = rootPrim.second;

        // Create the over that will contain the USD reference. It has the same name
        // as the root prim.
        UsdPrim primWithRef = createOverForUsdRef(*GetUsdStage(), GetUsdPath(), rootPrimName);

        // Create the USD reference.
        primWithRef.GetReferences().AddReference(refIdentifier, rootPrimPath);
    }

    // Relationships that cross root prim boundaries (e.g. material bindings from
    // one root referencing a material under another root) are broken by the per-root
    // reference arcs. Fix them by authoring local overrides on the exported stage.
    if (rootPrims.size() > 1) {
        fixCrossRootRelationships(*proxyStage, *GetUsdStage(), GetUsdPath());
    }
}

/* virtual */
void PxrUsdTranslators_StageWriter::PostExport()
{
    // Retrieve data needed for post-export.
    if (!_usdPrim)
        return;

    UsdStageRefPtr exportedStage = GetUsdStage();
    if (!exportedStage)
        return;

    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    MayaUsdProxyShapeBase* proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    if (!proxyShape)
        return;

    UsdStageRefPtr proxyStage = proxyShape->getUsdStage();
    if (!proxyStage)
        return;

    // The export stages as USD references flag is off or the stage
    // is anonymous then we must delete the prim and the parent transform.
    // Otherwise we have nothing to do.
    SdfLayerRefPtr root = proxyStage->GetRootLayer();
    bool deleteTransform = !root || root->IsAnonymous() || !_GetExportArgs().exportStagesAsRefs;
    if (!deleteTransform)
        return;

    // If the prim has children because the user created Maya children
    // on the proxy node, then we will keep the prim.
    if (!_usdPrim.GetAllChildren().empty())
        return;

    UsdPrim parentPrim = _usdPrim.GetParent();
    exportedStage->RemovePrim(_usdPrim.GetPath());

    // Remove the parent transform if we are not merging the shape and transform.
    MDagPath parentPath = GetDagPath();
    parentPath.pop();
    if (!_writeJobCtx.IsMergedTransform(parentPath)) {
        // If the parent prim has other children because the user created
        // Maya children on the proxy node, then we will keep the parent prim.
        if (parentPrim.GetAllChildren().empty()) {
            exportedStage->RemovePrim(parentPrim.GetPath());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
