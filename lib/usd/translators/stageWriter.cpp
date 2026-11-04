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
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <set>
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

// Returns the root prims of the stage that need to be referenced: every root prim
// that is defined (not an over) and not abstract (not a class). Unloaded and inactive
// root prims are included: the reference carries their payload and active state.
std::vector<UsdPrim> getReferencedRootPrims(const UsdStage& stage)
{
    std::vector<UsdPrim> rootPrims;

    for (const UsdPrim& prim : stage.GetPseudoRoot().GetAllChildren()) {
        if (prim.IsDefined() && !prim.IsAbstract())
            rootPrims.push_back(prim);
    }

    return rootPrims;
}

UsdPrim createOverForUsdRef(UsdStage& stage, const SdfPath& basePath, const TfToken& primName)
{
    SdfPath primWithRefPath = basePath.AppendChild(primName.IsEmpty() ? TfToken("Top") : primName);
    return stage.OverridePrim(primWithRefPath);
}

// Returns the root prim ancestor of a prim or property path, or the empty path for
// the pseudo-root and the empty path.
SdfPath getRootPrimPath(const SdfPath& path)
{
    const SdfPath primPath = path.GetPrimPath();
    if (primPath.IsEmpty() || primPath.IsAbsoluteRootPath())
        return {};

    // GetPrefixes() lists the prefixes from the root down: the first one is the root prim.
    return primPath.GetPrefixes().front();
}

MString toMString(size_t value)
{
    MString str;
    str += static_cast<unsigned int>(value);
    return str;
}

// Since each root prim of the proxy stage is brought in by its own reference, USD
// can only remap relationship targets and attribute connections that stay inside a
// single root prim. Any target pointing under another root prim is outside the scope
// of its reference and is dropped (with a warning each time it is read).
//
// This class finds those cross-root relationships and connections in the proxy stage,
// where the targets are still valid, and authors overrides on the exported stage with
// the full list of targets remapped under the exported prim.
class CrossRootTargetsFixer
{
public:
    CrossRootTargetsFixer(
        UsdStage&                   exportedStage,
        const SdfPath&              basePath,
        const std::vector<UsdPrim>& referencedRoots)
        : _exportedStage(exportedStage)
        , _basePath(basePath)
    {
        for (const UsdPrim& root : referencedRoots)
            _referencedRoots.insert(root.GetPath());
    }

    void fix(const UsdStage& proxyStage)
    {
        // Instance proxies are traversed so that cross-root targets inside instances
        // are found. Prototypes are not traversed: they are not part of the namespace
        // of the stage and their content is reached through the instance proxies.
        const auto predicate = UsdTraverseInstanceProxies(UsdPrimDefaultPredicate);
        for (const SdfPath& rootPath : _referencedRoots) {
            const UsdPrim rootPrim = proxyStage.GetPrimAtPath(rootPath);
            if (!rootPrim)
                continue;

            for (const UsdPrim& prim : UsdPrimRange(rootPrim, predicate)) {
                if (prim.HasAuthoredPayloads() && !prim.IsLoaded())
                    _unloadedPrims.push_back(prim.GetPath());

                for (const UsdRelationship& rel : prim.GetAuthoredRelationships()) {
                    SdfPathVector targets;
                    rel.GetTargets(&targets);
                    SdfPathVector remapped;
                    if (!remapTargets(prim, targets, remapped))
                        continue;

                    UsdPrim overPrim = overrideExportedPrim(prim);
                    if (!overPrim)
                        continue;

                    // Explicit targets: they replace all the weaker, out-of-scope
                    // targets that come through the reference.
                    UsdRelationship overRel
                        = overPrim.CreateRelationship(rel.GetName(), rel.IsCustom());
                    if (overRel && overRel.SetTargets(remapped))
                        ++_nbOverrides;
                }

                for (const UsdAttribute& attr : prim.GetAuthoredAttributes()) {
                    if (!attr.HasAuthoredConnections())
                        continue;

                    SdfPathVector sources;
                    attr.GetConnections(&sources);
                    SdfPathVector remapped;
                    if (!remapTargets(prim, sources, remapped))
                        continue;

                    UsdPrim overPrim = overrideExportedPrim(prim);
                    if (!overPrim)
                        continue;

                    UsdAttribute overAttr = overPrim.CreateAttribute(
                        attr.GetName(), attr.GetTypeName(), attr.IsCustom(), attr.GetVariability());
                    if (overAttr && overAttr.SetConnections(remapped))
                        ++_nbOverrides;
                }
            }
        }
    }

    void report(const MDagPath& dagPath) const
    {
        if (_nbOverrides > 0) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships or connections "
                "between its root prims. ^2s of them were copied as overrides in the exported "
                "file and won't follow later changes to the source file.\n",
                dagPath.fullPathName(),
                toMString(_nbOverrides));
            MGlobal::displayInfo(msg);
        }

        if (!_deinstancedPrims.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has instances with relationships or "
                "connections to another root prim. ^2s instances were made non-instanceable in "
                "the exported file to keep these relationships, starting with '^3s'.\n",
                dagPath.fullPathName(),
                toMString(_deinstancedPrims.size()),
                _deinstancedPrims.begin()->GetText());
            MGlobal::displayWarning(msg);
        }

        if (!_unresolvedTargets.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships or connections "
                "to prims that aren't exported (for example, under a class). ^2s of these "
                "targets were dropped, starting with '^3s'.\n",
                dagPath.fullPathName(),
                toMString(_unresolvedTargets.size()),
                _unresolvedTargets.begin()->GetText());
            MGlobal::displayWarning(msg);
        }

        if (!_unloadedPrims.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has unloaded payloads, starting "
                "with '^2s'. Relationships or connections between root prims inside them "
                "couldn't be checked and may be lost in the exported file.\n",
                dagPath.fullPathName(),
                _unloadedPrims.front().GetText());
            MGlobal::displayWarning(msg);
        }
    }

private:
    SdfPath toExportedPath(const SdfPath& path) const
    {
        return path.ReplacePrefix(SdfPath::AbsoluteRootPath(), _basePath);
    }

    // Remaps the targets of a relationship or connection of the given prim. Returns
    // false if no target points outside the root prim of the given prim: then the
    // reference remaps everything already and no override is needed.
    bool remapTargets(const UsdPrim& prim, const SdfPathVector& targets, SdfPathVector& remapped)
    {
        const SdfPath primRoot = getRootPrimPath(prim.GetPath());

        bool hasCrossRoot = false;
        for (const SdfPath& target : targets) {
            if (getRootPrimPath(target) != primRoot) {
                hasCrossRoot = true;
                break;
            }
        }

        if (!hasCrossRoot)
            return false;

        // All the targets must be written, even those within the same root prim,
        // since the explicit list replaces the targets coming from the reference.
        remapped.clear();
        remapped.reserve(targets.size());
        for (const SdfPath& target : targets) {
            if (_referencedRoots.count(getRootPrimPath(target)) == 0) {
                // No reference brings in the target, so there is nothing valid to
                // point to in the exported stage.
                _unresolvedTargets.insert(target);
                continue;
            }
            remapped.push_back(toExportedPath(target));
        }

        return true;
    }

    // Returns the prim to author the overrides on in the exported stage. USD ignores
    // opinions on descendants of instances, so the enclosing instances are first made
    // non-instanceable.
    UsdPrim overrideExportedPrim(const UsdPrim& prim)
    {
        if (prim.IsInstanceProxy()) {
            // De-instance from the outermost instance down: nested instances are only
            // reachable once their parent instance is de-instanced.
            SdfPathVector instances;
            for (UsdPrim parent = prim.GetParent(); parent && !parent.IsPseudoRoot();
                 parent = parent.GetParent()) {
                if (parent.IsInstance())
                    instances.push_back(parent.GetPath());
            }

            for (auto it = instances.rbegin(); it != instances.rend(); ++it) {
                if (!_deinstancedPrims.insert(*it).second)
                    continue;
                UsdPrim overInstance = _exportedStage.OverridePrim(toExportedPath(*it));
                if (overInstance)
                    overInstance.SetInstanceable(false);
            }
        }

        // An instance can carry overrides itself, only its descendants can't.
        return _exportedStage.OverridePrim(toExportedPath(prim.GetPath()));
    }

    UsdStage&         _exportedStage;
    const SdfPath     _basePath;
    std::set<SdfPath> _referencedRoots;
    std::set<SdfPath> _deinstancedPrims;
    std::set<SdfPath> _unresolvedTargets;
    SdfPathVector     _unloadedPrims;
    size_t            _nbOverrides = 0;
};
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

    // Create a reference for each root prim of the stage. A single reference to the
    // default prim would only bring in that prim, while the proxy shape shows all the
    // root prims of the stage.
    const std::vector<UsdPrim> rootPrims = getReferencedRootPrims(*proxyStage);
    for (const UsdPrim& rootPrim : rootPrims) {
        // Create the over that will contain the USD reference. It has the same name
        // as the root prim.
        UsdPrim primWithRef = createOverForUsdRef(*GetUsdStage(), GetUsdPath(), rootPrim.GetName());

        // Create the USD reference.
        primWithRef.GetReferences().AddReference(refIdentifier, rootPrim.GetPath());
    }

    // Relationships and connections that cross root prim boundaries (e.g. material
    // bindings from one root to a material under another root) are dropped by the
    // per-root references. Fix them by authoring local overrides on the exported stage.
    if (rootPrims.size() > 1) {
        CrossRootTargetsFixer fixer(*GetUsdStage(), GetUsdPath(), rootPrims);
        fixer.fix(*proxyStage);
        fixer.report(GetDagPath());
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
