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
#include <mayaUsd/utils/exportStageEdits.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <usdUfe/utils/fileSystem.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <algorithm>
#include <functional>
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

// Returns whether the session layer of the stage holds any opinion.
bool hasSessionOpinions(const UsdStage& stage)
{
    const SdfLayerHandle sessionLayer = stage.GetSessionLayer();
    // IsEmpty() also checks the sublayer paths.
    return sessionLayer && !sessionLayer->IsEmpty();
}

// The exported references only bring in the root layer stack of the proxy stage, not
// its session layer. Returns a stage composed the same way, to decide what to reference
// and which relationships to fix: the proxy stage itself when its session layer is
// empty, otherwise a new stage on the same root layer, with the same load rules.
UsdStageRefPtr getReferencedStage(const UsdStageRefPtr& proxyStage)
{
    if (!hasSessionOpinions(*proxyStage))
        return proxyStage;

    UsdStageRefPtr stage = UsdStage::Open(
        proxyStage->GetRootLayer(), proxyStage->GetPathResolverContext(), UsdStage::LoadNone);
    if (!stage)
        return proxyStage;

    stage->SetLoadRules(proxyStage->GetLoadRules());
    return stage;
}

// Returns the layers used by the stage (outside its session layers) that the exported
// references can't see as they are in memory: layers with unsaved changes, and
// anonymous layers.
SdfLayerHandleVector getUnsavedLayers(const UsdStage& stage)
{
    // The layer stack is strength ordered: the session layers come first, then the
    // root layer and its sublayers.
    const SdfLayerHandleVector allLayers = stage.GetLayerStack(true);
    const SdfLayerHandleVector sessionLayers(
        allLayers.begin(), std::find(allLayers.begin(), allLayers.end(), stage.GetRootLayer()));

    SdfLayerHandleVector unsavedLayers;
    for (const SdfLayerHandle& layer : stage.GetUsedLayers(false)) {
        if (!layer || (!layer->IsDirty() && !layer->IsAnonymous()))
            continue;
        if (std::find(sessionLayers.begin(), sessionLayers.end(), layer) != sessionLayers.end())
            continue;
        unsavedLayers.push_back(layer);
    }

    return unsavedLayers;
}

UsdPrim createOverForUsdRef(UsdStage& stage, const SdfPath& basePath, const TfToken& primName)
{
    return stage.OverridePrim(basePath.AppendChild(primName));
}

MString toMString(size_t value)
{
    MString str;
    str += static_cast<unsigned int>(value);
    return str;
}

// Reports what UsdUfe::authorCrossRootOverrides() did for the stage of the proxy shape.
void reportCrossRootOverrides(
    const UsdUfe::CrossRootOverridesResult& result,
    const MDagPath&                         dagPath)
{
    if (result.nbOverrides > 0) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has relationships, connections or "
            "path expressions between its root prims. ^2s of them were copied as overrides "
            "in the exported file and won't follow later changes to the source file.\n",
            dagPath.fullPathName(),
            toMString(result.nbOverrides));
        MGlobal::displayInfo(msg);
    }

    if (!result.deinstancedPrims.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has instances with relationships or "
            "connections to another root prim. ^2s instances (including nested ones) were "
            "made non-instanceable in the exported file to keep these relationships, "
            "starting with '^3s' in the source stage.\n",
            dagPath.fullPathName(),
            toMString(result.deinstancedPrims.size()),
            result.deinstancedPrims.begin()->GetText());
        MGlobal::displayWarning(msg);
    }

    if (!result.unresolvedTargets.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has relationships, connections or "
            "path expressions to prims that aren't exported (for example, under a class, "
            "only in the session layer, or missing). ^2s of these targets were dropped, "
            "starting with '^3s' in the source stage.\n",
            dagPath.fullPathName(),
            toMString(result.unresolvedTargets.size()),
            result.unresolvedTargets.begin()->GetText());
        MGlobal::displayWarning(msg);
    }

    if (!result.unloadedPrims.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has unloaded payloads, starting "
            "with '^2s'. Relationships or connections between root prims inside them "
            "couldn't be checked and may be lost in the exported file.\n",
            dagPath.fullPathName(),
            result.unloadedPrims.front().GetText());
        MGlobal::displayWarning(msg);
    }

    if (!result.overridesOutsideVariants.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has relationships or connections "
            "between root prims under variant sets that come through other composition "
            "arcs, starting with '^2s' in the source stage. They were copied for the "
            "current variant selections: switching variants in the exported file won't "
            "update them.\n",
            dagPath.fullPathName(),
            result.overridesOutsideVariants.front().GetText());
        MGlobal::displayWarning(msg);
    }

    if (!result.failedOverrides.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has relationships or connections "
            "between root prims that couldn't be written to the exported file, starting "
            "with '^2s' in the source stage.\n",
            dagPath.fullPathName(),
            result.failedOverrides.front().GetText());
        MGlobal::displayWarning(msg);
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

    // Per design, the transform prim above the reference is a component. (It becomes
    // a group when several root prims are referenced, see Write().)
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

    // Figure out the file path to the USD reference. When the stage has edits that
    // are only in memory, they are written next to the exported file, and the
    // references point at a layer that brings them in on top of the files on disk.
    std::string    refIdentifier;
    UsdStageRefPtr referencedStage;
    if (exportStageEdits(*proxyShape, proxyStage, refIdentifier)) {
        // The references bring in the whole stage, including its session layer.
        referencedStage = proxyStage;
    } else {
        refIdentifier = getUsdRefIdentifier(*proxyStage, GetDagPath());
        if (refIdentifier.size() <= 0) {
            return;
        }

        if (MayaUsd::utils::isProxyShapePathRelative(*proxyShape)) {
            std::string baseDir = UsdUfe::FileSystem::getDir(_GetExportArgs().file);
            refIdentifier = UsdUfe::FileSystem::makePathRelativeTo(refIdentifier, baseDir).first;
        }

        // The references point at the files on disk, without the session layer of the
        // proxy stage: decide what to reference from a stage composed the same way.
        referencedStage = getReferencedStage(proxyStage);

        if (!_GetExportArgs().exportStageEdits && referencedStage != proxyStage) {
            reportWarning(
                "The stage in the proxy shape at path '^1s' has opinions in its session layer. "
                "The exported file doesn't include them. Turn on the option to include the "
                "unsaved stage edits to export them.\n",
                GetDagPath());
        }
    }

    const SdfLayerHandleVector unsavedLayers
        = _checkedStageEdits ? SdfLayerHandleVector() : getUnsavedLayers(*referencedStage);
    if (!unsavedLayers.empty()) {
        MString msg;
        msg.format(
            "The stage in the proxy shape at path '^1s' has USD layers that aren't saved, "
            "starting with '^2s'. The exported file references the files on disk, so it doesn't "
            "include these changes. Save the USD layers before exporting, or turn on the option "
            "to include the unsaved stage edits.\n",
            GetDagPath().fullPathName(),
            unsavedLayers.front()->GetIdentifier().c_str());
        MGlobal::displayWarning(msg);
    }

    // Create a reference for each root prim of the stage. A single reference to the
    // default prim would only bring in that prim, while the proxy shape shows all the
    // root prims of the stage.
    const std::vector<UsdPrim> rootPrims = UsdUfe::getRootPrimsToReference(*referencedStage);

    // When the default prim is the only root prim to reference, it holds the whole
    // content of the file (classes aside, which inherits still reach). Reference the
    // file without a prim path then, so that the reference follows the default prim
    // of the file, as for any well-structured USD asset.
    const UsdPrim defaultPrim = referencedStage->GetDefaultPrim();
    const bool    referenceDefaultPrim = rootPrims.size() == 1 && defaultPrim
        && defaultPrim.GetPath() == rootPrims.front().GetPath();

    for (const UsdPrim& rootPrim : rootPrims) {
        // Create the over that will contain the USD reference. It has the same name
        // as the root prim.
        UsdPrim primWithRef = createOverForUsdRef(*GetUsdStage(), GetUsdPath(), rootPrim.GetName());

        // Create the USD reference.
        primWithRef.GetReferences().AddReference(
            refIdentifier, referenceDefaultPrim ? SdfPath() : rootPrim.GetPath());

        _referencedRootNames.push_back(rootPrim.GetName());
    }

    // Relationships, connections and path expressions that cross root prim boundaries
    // (e.g. material bindings from one root to a material under another root) are dropped
    // by the per-root references. Fix them by authoring local overrides on the exported layer.
    if (rootPrims.size() > 1) {
        // The referenced root prims are often models (e.g. components) themselves, which
        // can't be under a component. A group keeps the model hierarchy valid.
        if (_usdPrim)
            UsdModelAPI(_usdPrim).SetKind(KindTokens->group);

        const UsdUfe::CrossRootOverridesResult result = UsdUfe::authorCrossRootOverrides(
            *referencedStage, rootPrims, GetUsdStage()->GetEditTarget().GetLayer(), GetUsdPath());
        reportCrossRootOverrides(result, GetDagPath());
    }
}

bool PxrUsdTranslators_StageWriter::exportStageEdits(
    MayaUsdProxyShapeBase& proxyShape,
    const UsdStageRefPtr&  proxyStage,
    std::string&           refIdentifier)
{
    if (!_GetExportArgs().exportStageEdits)
        return false;

    // The edits are written next to the exported file, which must be on disk.
    // (Exports to memory, e.g. merge to USD, don't export stages.)
    const SdfLayerHandle exportLayer = GetUsdStage()->GetRootLayer();
    if (!exportLayer || exportLayer->IsAnonymous() || exportLayer->GetRealPath().empty())
        return false;

    // Each proxy shape gets its own folder, named after its DAG path. Names that had to
    // be changed into valid names (e.g. "ns:shape" and "ns_shape") could collide: make
    // them unique with a hash of the full DAG path.
    const std::string exportPath = exportLayer->GetRealPath();
    const std::string dagName = GetDagPath().partialPathName().asChar();
    std::string       proxyName = TfMakeValidIdentifier(dagName);
    if (proxyName != dagName) {
        const size_t hash = TfHash()(std::string(GetDagPath().fullPathName().asChar()));
        proxyName += TfStringPrintf("_%08x", static_cast<unsigned int>(hash & 0xffffffff));
    }
    const std::string destFolder
        = TfStringCatPaths(MayaUsd::getStageEditsFolder(exportPath), proxyName);

    const MayaUsd::ExportedStageEdits edits = MayaUsd::exportStageEdits(
        proxyStage,
        destFolder,
        _GetExportArgs().defaultUSDFormat.GetString(),
        MayaUsd::utils::isProxyShapePathRelative(proxyShape));
    _checkedStageEdits = true;

    for (const std::string& warning : edits.warnings) {
        MString msg;
        msg.format(
            "Exporting the unsaved edits of the stage in the proxy shape at path '^1s': ^2s\n",
            GetDagPath().fullPathName(),
            MString(warning.c_str()));
        MGlobal::displayWarning(msg);
    }

    if (edits.referencedLayer.empty())
        return false;

    // The edits are next to the exported file: always reference them relatively.
    refIdentifier = UsdUfe::FileSystem::makePathRelativeTo(
                        edits.referencedLayer, UsdUfe::FileSystem::getDir(exportPath))
                        .first;

    _exportedStageEdits = true;
    return true;
}

void PxrUsdTranslators_StageWriter::warnAboutNameClashes(const UsdStage& exportedStage) const
{
    const SdfLayerHandle layer = exportedStage.GetEditTarget().GetLayer();
    if (!layer)
        return;

    // The prims holding the references, and the overrides authored under them, are all
    // untyped overs. A def or typed prim spec in there was written by another writer
    // that defined prims at the same paths. (Defining an already defined prim may not
    // change the spec of the prim itself, hence the check of the whole subtree.)
    std::function<bool(const SdfPrimSpecHandle&)> hasDefinedSpec;
    hasDefinedSpec = [&hasDefinedSpec](const SdfPrimSpecHandle& spec) {
        if (spec->GetSpecifier() != SdfSpecifierOver || !spec->GetTypeName().IsEmpty())
            return true;
        for (const SdfPrimSpecHandle& child : spec->GetNameChildren()) {
            if (hasDefinedSpec(child))
                return true;
        }
        return false;
    };

    MString clashingNames;
    for (const TfToken& name : _referencedRootNames) {
        const SdfPrimSpecHandle spec = layer->GetPrimAtPath(GetUsdPath().AppendChild(name));
        if (!spec || !hasDefinedSpec(spec))
            continue;

        if (clashingNames.length() > 0)
            clashingNames += ", ";
        clashingNames += name.GetText();
    }

    if (clashingNames.length() == 0)
        return;

    MString msg;
    msg.format(
        "The stage in the proxy shape at path '^1s' has root prims with the same names as "
        "prims exported from Maya under the same parent, such as the materials scope: ^2s. "
        "Their contents are merged in the exported file. Change the materials scope name or "
        "the default prim of the export, or rename the Maya nodes.\n",
        GetDagPath().fullPathName(),
        clashingNames);
    MGlobal::displayWarning(msg);
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

    warnAboutNameClashes(*exportedStage);

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
    // is anonymous (and its edits were not exported) then we must delete
    // the prim and the parent transform.
    // Otherwise we have nothing to do.
    SdfLayerRefPtr root = proxyStage->GetRootLayer();

    bool deleteTransform = !root || (root->IsAnonymous() && !_exportedStageEdits)
        || !_GetExportArgs().exportStagesAsRefs;
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
