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

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>

#include <ghc/filesystem.hpp>

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

bool isProxyShapeRelative(MayaUsdProxyShapeBase& proxyShape)
{
    MStatus           status;
    MFnDependencyNode depNode(proxyShape.thisMObject(), &status);
    if (!status)
        return false;

    MPlug filePathRelativePlug = depNode.findPlug(MayaUsdProxyShapeBase::filePathRelativeAttr);
    return filePathRelativePlug.asBool();
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

std::pair<TfToken, SdfPath> getReferencedPrimNameAndPath(const UsdStage& stage)
{
    // If there is a default prim, we use that as the root prim.
    if (stage.HasDefaultPrim())
        return std::make_pair(stage.GetDefaultPrim().GetName(), SdfPath());

    // Otherwise we use the first root prim of the stage.
    UsdPrimSiblingRange usdRootPrims = stage.GetPseudoRoot().GetChildren();
    for (UsdPrim const& prim : usdRootPrims)
        return std::make_pair(prim.GetName(), prim.GetPath());

    // Otherwise... there is nothing to reference.
    return {};
}

UsdPrim createOverForUsdRef(UsdStage& stage, const SdfPath& basePath, const TfToken& primName)
{
    SdfPath primWithRefPath = basePath.AppendChild(primName.IsEmpty() ? TfToken("Top") : primName);
    return stage.OverridePrim(primWithRefPath);
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

    if (isProxyShapeRelative(*proxyShape)) {
        std::string baseDir = UsdMayaUtilFileSystem::getDir(_GetExportArgs().file);
        refIdentifier = UsdMayaUtilFileSystem::makePathRelativeTo(refIdentifier, baseDir).first;
    }

    // Figure out what will be the root prim of the reference.
    TfToken rootPrimName;
    SdfPath rootPrimPath;
    std::tie(rootPrimName, rootPrimPath) = getReferencedPrimNameAndPath(*proxyStage);

    // Create the over that will contain the USD reference. It has the same name
    // as the root prim.
    UsdPrim primWithRef = createOverForUsdRef(*GetUsdStage(), GetUsdPath(), rootPrimName);

    // Create the USD reference.
    primWithRef.GetReferences().AddReference(refIdentifier, rootPrimPath);
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
