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

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#if PXR_VERSION >= 2311
#include <pxr/usd/sdf/pathExpression.h>
#endif
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/propertySpec.h>
#include <pxr/usd/sdf/relationshipSpec.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <algorithm>
#include <functional>
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

// Returns whether the session layer of the stage holds any opinion.
bool hasSessionOpinions(const UsdStage& stage)
{
    const SdfLayerHandle sessionLayer = stage.GetSessionLayer();
    return sessionLayer && (!sessionLayer->IsEmpty() || !sessionLayer->GetSubLayerPaths().empty());
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
    const SdfLayerHandleVector nonSessionLayers = stage.GetLayerStack(false);
    const SdfLayerHandleVector allLayers = stage.GetLayerStack(true);

    SdfLayerHandleVector sessionLayers;
    for (const SdfLayerHandle& layer : allLayers) {
        if (std::find(nonSessionLayers.begin(), nonSessionLayers.end(), layer)
            == nonSessionLayers.end())
            sessionLayers.push_back(layer);
    }

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
    return stage.OverridePrim(basePath.AppendChild(primName));
}

// Returns the root prim ancestor of an absolute prim or property path, or the empty
// path for the pseudo-root, the empty path and relative paths.
SdfPath getRootPrimPath(const SdfPath& path)
{
    if (!path.IsAbsolutePath())
        return {};

    SdfPath primPath = path.GetPrimPath();
    while (!primPath.IsEmpty() && !primPath.IsRootPrimPath())
        primPath = primPath.GetParentPath();

    return primPath;
}

MString toMString(size_t value)
{
    MString str;
    str += static_cast<unsigned int>(value);
    return str;
}

// Since each root prim of the proxy stage is brought in by its own reference, USD
// can only remap relationship targets, attribute connections and path expressions
// that stay inside a single root prim. Anything pointing under another root prim is
// outside the scope of its reference and is dropped (with a warning each time it is
// read, for relationships and connections).
//
// This class finds those cross-root relationships, connections and path expressions
// in the source stage, where they are still valid, and authors overrides on the
// exported layer with everything remapped under the exported prim.
//
// The overrides are authored with the Sdf API in a single change block: authoring
// through the UsdStage API would recompose the exported stage, which already
// contains the whole referenced content, after each edit.
class CrossRootTargetsFixer
{
public:
    CrossRootTargetsFixer(
        const SdfLayerHandle&       exportedLayer,
        const SdfPath&              basePath,
        const std::vector<UsdPrim>& referencedRoots)
        : _exportedLayer(exportedLayer)
        , _basePath(basePath)
    {
        for (const UsdPrim& root : referencedRoots)
            _referencedRoots.insert(root.GetPath());
    }

    void fix(const UsdStage& sourceStage)
    {
        if (!_exportedLayer)
            return;

        SdfChangeBlock changeBlock;

        // Instance proxies are traversed so that cross-root targets inside instances
        // are found. Prototypes are not traversed: they are not part of the namespace
        // of the stage and their content is reached through the instance proxies.
        // Unloaded prims are visited (their children are not populated) to report them.
        const auto predicate
            = UsdTraverseInstanceProxies(UsdPrimIsActive && UsdPrimIsDefined && !UsdPrimIsAbstract);
        for (const SdfPath& rootPath : _referencedRoots) {
            const UsdPrim rootPrim = sourceStage.GetPrimAtPath(rootPath);
            if (!rootPrim)
                continue;

            for (const UsdPrim& prim : UsdPrimRange(rootPrim, predicate)) {
                if (!prim.IsLoaded())
                    _unloadedPrims.push_back(prim.GetPath());

                fixRelationships(prim);
                fixAttributes(prim);
            }
        }
    }

    void report(const MDagPath& dagPath) const
    {
        if (_nbOverrides > 0) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships, connections or "
                "path expressions between its root prims. ^2s of them were copied as overrides "
                "in the exported file and won't follow later changes to the source file.\n",
                dagPath.fullPathName(),
                toMString(_nbOverrides));
            MGlobal::displayInfo(msg);
        }

        if (!_deinstancedPrims.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has instances with relationships or "
                "connections to another root prim. ^2s instances (including nested ones) were "
                "made non-instanceable in the exported file to keep these relationships, "
                "starting with '^3s' in the source stage.\n",
                dagPath.fullPathName(),
                toMString(_deinstancedPrims.size()),
                _deinstancedPrims.begin()->GetText());
            MGlobal::displayWarning(msg);
        }

        if (!_unresolvedTargets.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships, connections or "
                "path expressions to prims that aren't exported (for example, under a class, "
                "only in the session layer, or missing). ^2s of these targets were dropped, "
                "starting with '^3s' in the source stage.\n",
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

        if (!_firstOverrideOutsideVariants.IsEmpty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships or connections "
                "between root prims under variant sets that come through other composition "
                "arcs, starting with '^2s' in the source stage. They were copied for the "
                "current variant selections: switching variants in the exported file won't "
                "update them.\n",
                dagPath.fullPathName(),
                _firstOverrideOutsideVariants.GetText());
            MGlobal::displayWarning(msg);
        }

        if (!_failedOverrides.empty()) {
            MString msg;
            msg.format(
                "The stage in the proxy shape at path '^1s' has relationships or connections "
                "between root prims that couldn't be written to the exported file, starting "
                "with '^2s' in the source stage.\n",
                dagPath.fullPathName(),
                _failedOverrides.front().GetText());
            MGlobal::displayWarning(msg);
        }
    }

private:
    void fixRelationships(const UsdPrim& prim)
    {
        for (const UsdRelationship& rel : prim.GetAuthoredRelationships()) {
            SdfPathVector targets;
            rel.GetTargets(&targets);
            SdfPathVector remapped;
            if (!remapTargets(prim, targets, remapped))
                continue;

            SdfRelationshipSpecHandle relSpec;
            if (SdfPrimSpecHandle primSpec = overrideExportedPrim(prim, rel)) {
                const SdfPath specPath = primSpec->GetPath().AppendProperty(rel.GetName());
                relSpec = _exportedLayer->GetRelationshipAtPath(specPath);
                if (!relSpec)
                    relSpec = SdfRelationshipSpec::New(primSpec, rel.GetName(), rel.IsCustom());
            }

            if (!relSpec) {
                _failedOverrides.push_back(rel.GetPath());
                continue;
            }

            // Explicit targets: they replace all the weaker, out-of-scope targets that
            // come through the reference.
            relSpec->GetTargetPathList().ClearEditsAndMakeExplicit();
            relSpec->GetTargetPathList().GetExplicitItems() = remapped;
            ++_nbOverrides;
        }
    }

    void fixAttributes(const UsdPrim& prim)
    {
        for (const UsdAttribute& attr : prim.GetAuthoredAttributes()) {
            if (attr.HasAuthoredConnections()) {
                SdfPathVector sources;
                attr.GetConnections(&sources);
                SdfPathVector remapped;
                if (remapTargets(prim, sources, remapped)) {
                    if (SdfAttributeSpecHandle attrSpec = overrideExportedAttribute(prim, attr)) {
                        attrSpec->GetConnectionPathList().ClearEditsAndMakeExplicit();
                        attrSpec->GetConnectionPathList().GetExplicitItems() = remapped;
                        ++_nbOverrides;
                    }
                }
            }

#if PXR_VERSION >= 2311
            if (attr.GetTypeName() == SdfValueTypeNames->PathExpression) {
                SdfPathExpression expression;
                if (!attr.Get(&expression))
                    continue;
                SdfPathExpression remapped;
                if (!remapExpression(prim, expression, remapped))
                    continue;
                if (SdfAttributeSpecHandle attrSpec = overrideExportedAttribute(prim, attr)) {
                    attrSpec->SetDefaultValue(VtValue(remapped));
                    ++_nbOverrides;
                }
            }
#endif
        }
    }

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

#if PXR_VERSION >= 2311
    // Remaps a path expression of the given prim. Returns false if all its paths stay
    // inside the root prim of the given prim.
    bool remapExpression(
        const UsdPrim&           prim,
        const SdfPathExpression& expression,
        SdfPathExpression&       remapped)
    {
        const SdfPath           primRoot = getRootPrimPath(prim.GetPath());
        const SdfPathExpression absolute = expression.MakeAbsolute(prim.GetPath());

        // TfFunctionRef only binds to lvalues, so the callbacks need names.
        bool hasCrossRoot = false;
        auto onLogic = [](SdfPathExpression::Op, int) {};
        auto onReference = [](const SdfPathExpression::ExpressionReference&) {};
        auto onPattern = [&](const SdfPathExpression::PathPattern& pattern) {
            const SdfPath patternRoot = getRootPrimPath(pattern.GetPrefix());
            if (patternRoot != primRoot)
                hasCrossRoot = true;
            if (!patternRoot.IsEmpty() && _referencedRoots.count(patternRoot) == 0)
                _unresolvedTargets.insert(pattern.GetPrefix());
        };
        absolute.Walk(onLogic, onReference, onPattern);

        if (!hasCrossRoot)
            return false;

        // Patterns under prims that aren't referenced are remapped too: they match
        // nothing in the exported stage, as they would through the reference.
        remapped = absolute.ReplacePrefix(SdfPath::AbsoluteRootPath(), _basePath);
        return true;
    }
#endif

    SdfAttributeSpecHandle overrideExportedAttribute(const UsdPrim& prim, const UsdAttribute& attr)
    {
        SdfAttributeSpecHandle attrSpec;
        if (SdfPrimSpecHandle primSpec = overrideExportedPrim(prim, attr)) {
            const SdfPath specPath = primSpec->GetPath().AppendProperty(attr.GetName());
            attrSpec = _exportedLayer->GetAttributeAtPath(specPath);
            if (!attrSpec) {
                attrSpec = SdfAttributeSpec::New(
                    primSpec,
                    attr.GetName(),
                    attr.GetTypeName(),
                    attr.GetVariability(),
                    attr.IsCustom());
            }
        }

        if (!attrSpec)
            _failedOverrides.push_back(attr.GetPath());
        return attrSpec;
    }

    // Returns the prim spec to author the override of the given property on in the
    // exported layer. USD ignores opinions on descendants of instances, so the enclosing
    // instances are made non-instanceable.
    SdfPrimSpecHandle overrideExportedPrim(const UsdPrim& prim, const UsdProperty& prop)
    {
        if (prim.IsInstanceProxy()) {
            for (UsdPrim parent = prim.GetParent(); parent && !parent.IsPseudoRoot();
                 parent = parent.GetParent()) {
                if (!parent.IsInstance() || !_deinstancedPrims.insert(parent.GetPath()).second)
                    continue;
                if (SdfPrimSpecHandle instanceSpec
                    = SdfCreatePrimInLayer(_exportedLayer, toExportedPath(parent.GetPath())))
                    instanceSpec->SetInstanceable(false);
            }
        }

        // An instance can carry overrides itself, only its descendants can't.
        return SdfCreatePrimInLayer(_exportedLayer, getOverridePrimPath(prim, prop));
    }

    static bool isUnderVariantSet(const UsdPrim& prim)
    {
        for (UsdPrim current = prim; current && !current.IsPseudoRoot();
             current = current.GetParent()) {
            if (current.HasVariantSets())
                return true;
        }
        return false;
    }

    // Returns the path of the prim spec to author the override of the given property on.
    //
    // When the strongest opinion of the property is inside variants, the override goes
    // inside the same variants of the exported prim: it only applies to the current
    // variant selections, and the other variants keep their own opinions from the
    // source file. Otherwise it goes on the exported prim itself.
    SdfPath getOverridePrimPath(const UsdPrim& prim, const UsdProperty& prop)
    {
        const SdfPath exportedPath = toExportedPath(prim.GetPath());

        // Most prims aren't under variant sets: skip the property stack for them.
        if (!isUnderVariantSet(prim))
            return exportedPath;

        // Only the strongest opinion matters, to know which variants it comes from.
        const SdfPropertySpecHandleVector stack = prop.GetPropertyStack(UsdTimeCode::Default());
        if (!stack.empty()) {
            const SdfPath specPrimPath = stack.front()->GetPath().GetParentPath();

            // An opinion outside variants is stronger than any variant opinion of the
            // same layer stack: the plain override is right for every variant selection.
            if (!specPrimPath.ContainsPrimVariantSelection())
                return exportedPath;

            // The spec path is the stage path with variant selections when the opinion
            // comes from the variants of the referenced layer stack itself, rather than
            // through another composition arc (e.g. the class of an instance).
            if (specPrimPath.StripAllVariantSelections() == prim.GetPath())
                return toExportedPath(specPrimPath);
        }

        // Fall back to the exported prim: the override then applies to all variants.
        if (_firstOverrideOutsideVariants.IsEmpty())
            _firstOverrideOutsideVariants = prop.GetPath();
        return exportedPath;
    }

    SdfLayerHandle    _exportedLayer;
    const SdfPath     _basePath;
    std::set<SdfPath> _referencedRoots;
    std::set<SdfPath> _deinstancedPrims;
    std::set<SdfPath> _unresolvedTargets;
    SdfPathVector     _unloadedPrims;
    SdfPathVector     _failedOverrides;
    SdfPath           _firstOverrideOutsideVariants;
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
    const std::vector<UsdPrim> rootPrims = getReferencedRootPrims(*referencedStage);

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

        CrossRootTargetsFixer fixer(
            GetUsdStage()->GetEditTarget().GetLayer(), GetUsdPath(), rootPrims);
        fixer.fix(*referencedStage);
        fixer.report(GetDagPath());
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

    // Each proxy shape gets its own folder, named after its DAG path.
    const std::string exportPath = exportLayer->GetRealPath();
    const std::string proxyName = TfMakeValidIdentifier(GetDagPath().partialPathName().asChar());
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
    const auto relative = UsdUfe::FileSystem::makePathRelativeTo(
        edits.referencedLayer, UsdUfe::FileSystem::getDir(exportPath));
    refIdentifier = relative.first;
    if (relative.second && !TfStringStartsWith(refIdentifier, "./")
        && !TfStringStartsWith(refIdentifier, "../"))
        refIdentifier = "./" + refIdentifier;

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
