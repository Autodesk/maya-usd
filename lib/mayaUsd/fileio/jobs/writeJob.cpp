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
#include "writeJob.h"

#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/layer.h>

#include <maya/MAnimControl.h>
#include <maya/MComputation.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnRenderLayer.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MObjectArray.h>
#include <maya/MStatus.h>
#include <maya/MUuid.h>

#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <set>

// Needed for directly removing a UsdVariant via Sdf
//   Remove when UsdVariantSet::RemoveVariant() is exposed
//   XXX [bug 75864]
#include <mayaUsd/fileio/chaser/exportChaser.h>
#include <mayaUsd/fileio/chaser/exportChaserRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/modelKindProcessor.h>
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/transformWriter.h>
#include <mayaUsd/fileio/translators/translatorMaterial.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/autoUndoCommands.h>
#include <mayaUsd/utils/progressBarScope.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdf/variantSetSpec.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdUtils/dependencies.h>
#include <pxr/usd/usdUtils/pipeline.h>

#if PXR_VERSION >= 2505
#include <pxr/usd/usdUI/accessibilityAPI.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_WriteJob::UsdMaya_WriteJob(
    const UsdMayaJobExportArgs& iArgs,
    const std::string&          fileName,
    bool                        append)
    : _fileName(fileName)
    , _appendToFile(append)
    , mJobCtx(iArgs)
    , _modelKindProcessor(new UsdMaya_ModelKindProcessor(iArgs))
{
}

UsdMaya_WriteJob::~UsdMaya_WriteJob() { }

SdfPath UsdMaya_WriteJob::MapDagPathToSdfPath(const MDagPath& dagPath) const
{
    SdfPath usdPrimPath;
    TfMapLookup(mDagPathToUsdPathMap, dagPath, &usdPrimPath);

    return usdPrimPath;
}

/// Generates a name for a temporary usdc file in \p dir.
/// Unless you are very, very unlucky, the stage name is unique because it's
/// generated from a UUID.
static std::string _MakeTmpStageName(const std::string& dir)
{
    MUuid uuid;
    uuid.generate();

    const std::string fileName = TfStringPrintf(
        "tmp-%s.%s",
        uuid.asString().asChar(),
        UsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText());
    return TfStringCatPaths(dir, fileName);
}

/// Chooses the fallback extension based on the compatibility profile, e.g.
/// ARKit-compatible files should be usdz's by default.
static TfToken _GetFallbackExtension(const TfToken& compatibilityMode)
{
    if (compatibilityMode == UsdMayaJobExportArgsTokens->appleArKit) {
        return UsdMayaTranslatorTokens->UsdFileExtensionPackage;
    }
    return UsdMayaTranslatorTokens->UsdFileExtensionDefault;
}

/// Converts export option tokens to metersPerUnit values used in USD metadata.
static double _WantedUSDMetersPerUnit(const TfToken& unitOption)
{
    if (unitOption == UsdMayaJobExportArgsTokens->mayaPrefs) {
        return UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(MDistance::uiUnit());
    }

    static const std::map<TfToken, double> unitsConversionMap
        = { { UsdMayaJobExportArgsTokens->nm, UsdGeomLinearUnits::nanometers },
            { UsdMayaJobExportArgsTokens->um, UsdGeomLinearUnits::micrometers },
            { UsdMayaJobExportArgsTokens->mm, UsdGeomLinearUnits::millimeters },
            { UsdMayaJobExportArgsTokens->cm, UsdGeomLinearUnits::centimeters },
            // Note: there is no official USD decimeter units, we have to roll our own.
            { UsdMayaJobExportArgsTokens->dm, 0.1 },
            { UsdMayaJobExportArgsTokens->m, UsdGeomLinearUnits::meters },
            { UsdMayaJobExportArgsTokens->km, UsdGeomLinearUnits::kilometers },
            { UsdMayaJobExportArgsTokens->lightyear, UsdGeomLinearUnits::lightYears },
            { UsdMayaJobExportArgsTokens->inch, UsdGeomLinearUnits::inches },
            { UsdMayaJobExportArgsTokens->foot, UsdGeomLinearUnits::feet },
            { UsdMayaJobExportArgsTokens->yard, UsdGeomLinearUnits::yards },
            { UsdMayaJobExportArgsTokens->mile, UsdGeomLinearUnits::miles } };

    const auto iter = unitsConversionMap.find(unitOption);
    return (iter == unitsConversionMap.end()) ? UsdGeomLinearUnits::centimeters : iter->second;
}

/// Converts upAxis export option tokens to USD upAxis tokens.
static TfToken _WantedUSDUpAxis(const TfToken& upAxisOption)
{
    if (upAxisOption == UsdMayaJobExportArgsTokens->mayaPrefs)
        return MGlobal::isZAxisUp() ? UsdGeomTokens->z : UsdGeomTokens->y;

    if (upAxisOption == UsdMayaJobExportArgsTokens->z)
        return UsdGeomTokens->z;

    return UsdGeomTokens->y;
}

static MObject _WantedRenderLayerNode(const TfToken& renderLayerMode)
{
    // Return the wanted render layer node based on the requested render layer mode:
    //     defaultLayer    - Switch to the default render layer before exporting,
    //                       then switch back afterwards (no layer switching if
    //                       the current layer IS the default layer).
    //     currentLayer    - No layer switching before or after exporting. Just
    //                       use whatever is the current render layer for export.
    //     modelingVariant - Switch to the default render layer before exporting,
    //                       and export each render layer in the scene as a
    //                       modeling variant, then switch back afterwards (no
    //                       layer switching if the current layer IS the default
    //                       layer). The default layer will be made the default
    //                       modeling variant.
    if (renderLayerMode == UsdMayaJobExportArgsTokens->currentLayer) {
        return MFnRenderLayer::currentLayer();
    }
    return MFnRenderLayer::defaultRenderLayer();
}

MStatus _ActivateRenderLayer(const MObject& renderLayerNode)
{
    static const MString cmdFmt("editRenderLayerGlobals -currentRenderLayer ^1s");
    if (renderLayerNode != MFnRenderLayer::currentLayer()) {
        MStatus        status;
        MFnRenderLayer renderLayerFn(renderLayerNode, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString cmd;
        cmd.format(cmdFmt, renderLayerFn.name());
        status = MGlobal::executeCommand(cmd, /*displayEnabled=*/false, /*undoEnabled=*/false);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    return MS::kSuccess;
}

/// RAII class to backup and restore the current render layer.
class CurrentRenderLayerGuard
{
public:
    CurrentRenderLayerGuard()
        : _prevLayer(MFnRenderLayer::currentLayer())
    {
    }
    ~CurrentRenderLayerGuard() { _ActivateRenderLayer(_prevLayer); }

private:
    const MObject _prevLayer;
};

/// Class to automatically change and restore the up-axis and units of the Maya scene.
class AutoUpAxisAndUnitsChanger : public MayaUsd::AutoUndoCommands
{
public:
    /// Constructs AutoUndoCommands that changes optionally maya upAxis or metersPerUnit
    AutoUpAxisAndUnitsChanger(const TfToken* upAxis, const double* metersPerUnit)
        : AutoUndoCommands("change up-axis and units", _prepareCommands(upAxis, metersPerUnit))
    {
    }

private:
    static std::string _prepareUnitsCommands(double metersPerUnit)
    {
        const double mayaMetersPerUnit
            = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(MDistance::internalUnit());

        // If the Maya data unit is already the right one, we dont have to modify the Maya scene.
        if (mayaMetersPerUnit == metersPerUnit)
            return {};

        static const char scalingCommandsFormat[]
            = "scale -relative -pivot 0 0 0 -scaleXYZ %f %f %f $groupName;\n";

        const double requiredScale = mayaMetersPerUnit / metersPerUnit;

        return TfStringPrintf(scalingCommandsFormat, requiredScale, requiredScale, requiredScale);
    }

    static std::string _prepareUpAxisCommands(const TfToken& upAxis)
    {
        // If the Maya up-axis is already the right one, we dont have to modify the Maya scene.
        if (upAxis == _WantedUSDUpAxis(UsdMayaJobExportArgsTokens->mayaPrefs))
            return {};

        static const char rotationCommandsFormat[] =
            // Rotate the group to align with the desired axis.
            //
            //    - Use relative rotation since we want to rotate the group as it is already
            //    positioned
            //    - Use -euler to make the angle be relative to the current angle
            //    - Use forceOrderXYZ to force the rotation to be relative to world
            //    - Use -pivot to make sure we are rotating relative to the origin
            //      (The group is positioned at the center of all sub-object, so we need to
            //      specify the pivot)
            "rotate -relative -euler -pivot 0 0 0 -forceOrderXYZ %d 0 0 $groupName;\n";

        const int angleYtoZ = 90;
        const int angleZtoY = -90;
        const int rotationAngle = upAxis == UsdGeomTokens->z ? angleYtoZ : angleZtoY;

        return TfStringPrintf(rotationCommandsFormat, rotationAngle);
    }

    static std::string _prepareCommands(const TfToken* upAxis, const double* metersPerUnit)
    {
        // These commands wrap the scene-changing commands by providing:
        //
        //     - the list of root names as the variable $rootNodeNames
        //     - a group containing all those nodes named $groupName
        //     -
        //
        // The scene-changing commands should mofify the group, so that ungrouping
        // these node while preserving transform changes were done on the group will
        // modify each root node individually.

        static const char scriptPrefix[] =
            // Preserve the selection. Grouping and ungrouping changes it.
            "string $selection[] = `ls -selection`;\n"
            // Find all root nodes.
            "string $rootNodeNames[] = `ls -assemblies`;\n"
            // Group all root node under a new group:
            //
            //    - Use -absolute to keep the grouped node world positions
            //    - Use -world to create the group under the root ofthe scene
            //      if the import was done at the root of the scene
            //    - Capture the new group name in a MEL variable called $groupName
            "string $groupName = `group -absolute -world $rootNodeNames`;\n";

        static const char scriptSuffix[] =
            // Apply the transformations to avoid accumulating transforms on ungroup.
            "makeIdentity -apply true -rotate true -scale true -normal 0 -preserveNormals true "
            "$groupName;\n"
            // Ungroup while preserving the rotation.
            "ungroup -absolute $groupName;\n"
            // Restore the selection.
            "select -replace $selection;\n";

        std::string commands;

        // If the user don't want to author the up-axis, we won't need to change the Maya up-axis.
        if (upAxis != nullptr)
            commands += _prepareUpAxisCommands(*upAxis);

        // If the user don't want to author the unit, we won't need to change the Maya unit.
        if (metersPerUnit != nullptr)
            commands += _prepareUnitsCommands(*metersPerUnit);

        // If both are empty, we don't need to do anything.
        if (commands.empty())
            return {};

        const std::string fullScript = scriptPrefix + commands + scriptSuffix;
        return fullScript;
    }
};

class UsdMaya_WriteJobImpl
{
public:
    /// Helper for Write methods.
    static bool WriteJobs(const std::vector<UsdMaya_WriteJob*>&);

private:
    using TimeSamples = std::vector<double>;

    /// Helper class to write a job sequence of frames,
    /// WriteFrameIfNeeded must be called for all timeSamples of the job in non-decreasing order.
    class JobFramesWriter
    {
    public:
        JobFramesWriter() = delete;
        explicit JobFramesWriter(UsdMaya_WriteJob* job);

        bool Finished() const { return _next == _end; }
        auto AllSamples() const -> const TimeSamples&;
        bool WriteFrameIfNeeded(double frame);

    private:
        UsdMaya_WriteJob*           _job;
        TimeSamples::const_iterator _next;
        TimeSamples::const_iterator _end;
    };
    using JobFramesWriters = std::vector<JobFramesWriter>;

    /// Computes the ordered union of all \p writers time-samples.
    static TimeSamples _UnionTimeSamples(const JobFramesWriters& writers);
};

UsdMaya_WriteJobImpl::JobFramesWriter::JobFramesWriter(UsdMaya_WriteJob* job)
    : _job(job)
    , _next(job->mJobCtx.GetArgs().timeSamples.cbegin())
    , _end(job->mJobCtx.GetArgs().timeSamples.cend())
{
}

bool UsdMaya_WriteJobImpl::JobFramesWriter::WriteFrameIfNeeded(double frame)
{
    if (Finished())
        return false;

    if (frame != *_next)
        return true;

    ++_next;
    return _job->_WriteFrame(frame);
}

auto UsdMaya_WriteJobImpl::JobFramesWriter::AllSamples() const -> const TimeSamples&
{
    return _job->mJobCtx.GetArgs().timeSamples;
}

// static
auto UsdMaya_WriteJobImpl::_UnionTimeSamples(const JobFramesWriters& writers) -> TimeSamples
{
    if (writers.empty())
        return {};

    TimeSamples unioned = writers.front().AllSamples();

    TimeSamples storage;
    for (auto itr = writers.cbegin() + 1; itr != writers.cend(); ++itr)
        UsdMayaWriteUtil::UpdateTimeSamples(unioned, itr->AllSamples(), &storage);

    return unioned;
}

// static
bool UsdMaya_WriteJobImpl::WriteJobs(const std::vector<UsdMaya_WriteJob*>& jobs)
{
    if (jobs.empty())
        return true;

    // Get the wanted upAxis, unit and renderLayer node.
    const auto* firstJob = jobs.front();
    const auto& firstArgs = firstJob->mJobCtx.GetArgs();

    const auto usdUpAxis = _WantedUSDUpAxis(firstArgs.upAxis);
    const auto usdMetersPerUnit = _WantedUSDMetersPerUnit(firstArgs.unit);
    const auto renderLayer = _WantedRenderLayerNode(firstArgs.renderLayerMode);

    // Validate that multiple jobs can be exported all together.
    if (jobs.size() > 1) {
        auto argsAreCompatible = [&](const UsdMayaJobExportArgs& args) -> bool {
            return (_WantedUSDUpAxis(args.upAxis) == usdUpAxis)
                && (_WantedUSDMetersPerUnit(args.unit) == usdMetersPerUnit)
                && (_WantedRenderLayerNode(args.renderLayerMode) == renderLayer);
        };

        std::set<std::string> seenFileNames;
        for (auto& job : jobs) {
            // Verify that we are not writing twice to the same filename.
            if (!seenFileNames.insert(job->_fileName).second) {
                MGlobal::displayError("Cannot write twice to the same whole USD file.");
                return false;
            }
            // Verify that all job upAxis and unit are compatible.
            if (job != firstJob && !argsAreCompatible(job->mJobCtx.GetArgs())) {
                MGlobal::displayError("Cannot write two USD files with different upAxes or units.");
                return false;
            }
        }
    }

    // Collect timeSampled jobs for animation export, and determine which timeSamples to evaluate.
    JobFramesWriters jobFramesWriters;
    jobFramesWriters.reserve(jobs.size());

    for (auto& job : jobs) {
        JobFramesWriter framesWriter(job);
        if (!framesWriter.Finished())
            jobFramesWriters.push_back(std::move(framesWriter));
    }

    auto unionedSamples
        = (jobFramesWriters.size() > 1) ? _UnionTimeSamples(jobFramesWriters) : TimeSamples();
    const auto& timeSamples
        = (jobFramesWriters.size() == 1) ? jobFramesWriters.front().AllSamples() : unionedSamples;

    // Non-animated export doesn't show progress.
    const bool showProgress = !timeSamples.empty();

    // Animated export shows frame-by-frame progress.
    int                       nbSteps = (jobs.size() * 3) + timeSamples.size() + 2;
    MayaUsd::ProgressBarScope progressBar(showProgress, true /*interruptible */, nbSteps, "");

    // Temporarily tweak the Maya scene for export if needed.
    const AutoUpAxisAndUnitsChanger unitsChanger(
        firstArgs.upAxis == UsdMayaJobExportArgsTokens->none ? nullptr : &usdUpAxis,
        firstArgs.unit == UsdMayaJobExportArgsTokens->none ? nullptr : &usdMetersPerUnit);

    const CurrentRenderLayerGuard currentLayerGuard;
    if (!_ActivateRenderLayer(renderLayer)) {
        return false;
    }

    progressBar.advance();

    // Default-time exports.
    for (auto& job : jobs) {
        if (!job->_BeginWriting())
            return false;
        progressBar.advance();
    }

    // Time-sampled exports.
    if (!timeSamples.empty()) {
        const MTime oldCurTime = MAnimControl::currentTime();

        for (double t : timeSamples) {
            MGlobal::viewFrame(t);
            progressBar.advance();

            // Process per frame data.
            for (auto itr = jobFramesWriters.begin(); itr != jobFramesWriters.end(); /**/) {
                if (!itr->WriteFrameIfNeeded(t)) {
                    MGlobal::viewFrame(oldCurTime);
                    return false;
                }
                itr = itr->Finished() ? jobFramesWriters.erase(itr) : std::next(itr);
            }

            // Allow user cancellation.
            if (progressBar.isInterruptRequested()) {
                break;
            }
        }

        // Set the time back.
        MGlobal::viewFrame(oldCurTime);
    }

    // Finalize the exports.
    for (auto& job : jobs) {
        if (!job->_PostExport())
            return false;
        progressBar.advance();
    }

    for (auto& job : jobs) {
        job->_FinishWriting();
        progressBar.advance();
    }

    return true;
}

// static
bool UsdMaya_WriteJob::Write()
{
    std::vector<UsdMaya_WriteJob*> oneJob { this };
    return UsdMaya_WriteJobImpl::WriteJobs(oneJob);
}

static MStringArray GetExportDefaultPrimCandidates(const UsdMayaJobExportArgs& exportArgs)
{
    MStringArray roots;

    // If the user provided a root prim, used it as the default prim.
    if (!exportArgs.rootPrim.IsEmpty()) {
        roots.append(exportArgs.rootPrim.GetName().c_str());
        return roots;
    }

    // If the user provided export roots, used them to select the default prim.
    if (exportArgs.exportRoots.size() > 0) {
        for (const std::string& root : exportArgs.exportRoots) {
            if (root.empty())
                continue;
            roots.append(root.c_str());
        }
        if (roots.length() > 0)
            return roots;
    }

    // Note: we reuse the same logic used for the UI so that the logic stay in sync.
    //       This is called only once during an export, so calling a Python command
    //       is not an issue in regard to performance.
    MString cmd;

    static const MString getAllRoots("updateDefaultPrimCandidates");
    static const MString getSelRoots("updateDefaultPrimCandidatesFromSelection");

    const MString getRoots = exportArgs.exportSelected ? getSelRoots : getAllRoots;

    static const MString pyTrue("True");
    static const MString pyFalse("False");

    // Note: the booleans all represent exclusion while the job arguments are all inclusion,
    //       so we pass False when something is included.
    const MString excludeMesh = exportArgs.isExportingMeshes() ? pyFalse : pyTrue;
    const MString excludeLight = exportArgs.isExportingLights() ? pyFalse : pyTrue;
    const MString excludeCamera = exportArgs.isExportingCameras() ? pyFalse : pyTrue;
    const MString excludeStage = exportArgs.exportStagesAsRefs ? pyFalse : pyTrue;

    cmd.format(
        "import mayaUsd_exportHelpers; mayaUsd_exportHelpers.^1s(^2s, ^3s, ^4s, ^5s)",
        getRoots,
        excludeMesh,
        excludeLight,
        excludeCamera,
        excludeStage);

    MGlobal::executePythonCommand(cmd, roots);

    return roots;
}

bool UsdMaya_WriteJob::_BeginWriting()
{
    MayaUsd::ProgressBarScope progressBar(7);

    // If no default prim for the exported root layer was given, select one from
    // the available root nodes of the Maya scene in order for materials to be
    // parented correctly. We take into account the excluded node types based on
    // the export job arguments. This is not required if using the legacy
    // material scope.
    if (!mJobCtx.mArgs.legacyMaterialScope) {
        if (mJobCtx.mArgs.defaultPrim.empty()) {
            MStringArray roots = GetExportDefaultPrimCandidates(mJobCtx.mArgs);
            if (roots.length() > 0) {
                mJobCtx.mArgs.defaultPrim = roots[0].asChar();
            }
        }
    }

    if (!mJobCtx.mArgs.defaultPrim.empty()) {
        mJobCtx.mArgs.defaultPrim = UsdMayaUtil::MayaNodeNameToPrimName(
            mJobCtx.mArgs.defaultPrim, mJobCtx.mArgs.stripNamespaces);
    }

    // Check for DAG nodes that are a child of an already specified DAG node to export
    // if that's the case, report the issue and skip the export
    UsdMayaUtil::MDagPathSet::const_iterator m, n;
    UsdMayaUtil::MDagPathSet::const_iterator endPath = mJobCtx.mArgs.dagPaths.end();
    for (m = mJobCtx.mArgs.dagPaths.begin(); m != endPath;) {
        MDagPath path1 = *m;
        m++;
        for (n = m; n != endPath; n++) {
            MDagPath path2 = *n;
            if (UsdMayaUtil::isAncestorDescendentRelationship(path1, path2)) {
                TF_RUNTIME_ERROR(
                    "%s and %s are ancestors or descendants of each other. "
                    "Please specify export DAG paths that don't overlap. "
                    "Exiting.",
                    path1.fullPathName().asChar(),
                    path2.fullPathName().asChar());
                return false;
            }
        } // for n
    }     // for m
    progressBar.advance();

    // Make sure the file name is a valid one with a proper USD extension.
    TfToken     fileExt(TfGetExtension(_fileName));
    std::string fileNameWithExt;
    if (!(SdfLayer::IsAnonymousLayerIdentifier(_fileName)
          || fileExt == UsdMayaTranslatorTokens->UsdFileExtensionDefault
          || fileExt == UsdMayaTranslatorTokens->UsdFileExtensionASCII
          || fileExt == UsdMayaTranslatorTokens->UsdFileExtensionCrate
          || fileExt == UsdMayaTranslatorTokens->UsdFileExtensionPackage)) {
        // No extension; get fallback extension based on compatibility profile.
        fileExt = _GetFallbackExtension(mJobCtx.mArgs.compatibility);
        fileNameWithExt = TfStringPrintf("%s.%s", _fileName.c_str(), fileExt.GetText());
    } else {
        // Has correct extension; use as-is.
        fileNameWithExt = _fileName;
    }
    progressBar.advance();

    // Setup file structure for export based on whether we are doing a
    // "standard" flat file export or a "packaged" export to usdz.
    if (fileExt == UsdMayaTranslatorTokens->UsdFileExtensionPackage) {
        if (_appendToFile) {
            TF_RUNTIME_ERROR("Cannot append to USDZ packages");
            return false;
        }

        // We don't write to fileNameWithExt directly; instead, we write to
        // a temp stage file.
        _realFilename = _MakeTmpStageName(TfGetPathName(fileNameWithExt));
        if (TfPathExists(_realFilename)) {
            // This shouldn't happen (since we made the temp stage name from
            // a UUID). Don't try to recover.
            TF_RUNTIME_ERROR("Temporary stage '%s' already exists", _realFilename.c_str());
            return false;
        }

        // The packaged file gets written to fileNameWithExt.
        _packageName = fileNameWithExt;
    } else {
        _realFilename = fileNameWithExt;
        _packageName = std::string();
    }
    progressBar.advance();

    TF_STATUS("Opening layer '%s' for writing", _realFilename.c_str());
    if (mJobCtx.mArgs.renderLayerMode == UsdMayaJobExportArgsTokens->modelingVariant) {
        // Handle usdModelRootOverridePath for USD Variants
        MFnRenderLayer::listAllRenderLayers(mRenderLayerObjs);
        if (mRenderLayerObjs.length() > 1) {
            if (!mJobCtx.mArgs.rootMapFunction.IsNull()) {
                MGlobal::displayError("Export roots can't be used together with export to modeling"
                                      " variant; export aborting");
                return false;
            }

            mJobCtx.mArgs.usdModelRootOverridePath = SdfPath("/_BaseModel_");
        }
    }

    if (!mJobCtx._OpenFile(_realFilename, _appendToFile)) {
        return false;
    }
    progressBar.advance();

    // Set time range for the USD file if we're exporting animation.
    if (!mJobCtx.mArgs.timeSamples.empty()) {
        mJobCtx.mStage->SetStartTimeCode(mJobCtx.mArgs.timeSamples.front());
        mJobCtx.mStage->SetEndTimeCode(mJobCtx.mArgs.timeSamples.back());
        mJobCtx.mStage->SetTimeCodesPerSecond(UsdMayaUtil::GetSceneMTimeUnitAsDouble());
        mJobCtx.mStage->SetFramesPerSecond(UsdMayaUtil::GetSceneMTimeUnitAsDouble());
    }

    // Author USD units and up axis if requested.
    if (mJobCtx.mArgs.unit != UsdMayaJobExportArgsTokens->none) {
        UsdGeomSetStageMetersPerUnit(mJobCtx.mStage, _WantedUSDMetersPerUnit(mJobCtx.mArgs.unit));
    }
    if (mJobCtx.mArgs.upAxis != UsdMayaJobExportArgsTokens->none) {
        UsdGeomSetStageUpAxis(mJobCtx.mStage, _WantedUSDUpAxis(mJobCtx.mArgs.upAxis));
    }
    // Set the customLayerData on the layer
    if (!mJobCtx.mArgs.customLayerData.empty()) {
        mJobCtx.mStage->GetRootLayer()->SetCustomLayerData(mJobCtx.mArgs.customLayerData);
    }

    // Pre-process the argument dagPath path names into two sets. One set
    // contains just the arg dagPaths, and the other contains all parents of
    // arg dagPaths all the way up to the world root. Partial path names are
    // enough because Maya guarantees them to still be unique, and they require
    // less work to hash and compare than full path names.
    TfHashSet<std::string, TfHash>           argDagPaths;
    TfHashSet<std::string, TfHash>           argDagPathParents;
    UsdMayaUtil::MDagPathSet::const_iterator end = mJobCtx.mArgs.dagPaths.end();
    for (UsdMayaUtil::MDagPathSet::const_iterator it = mJobCtx.mArgs.dagPaths.begin(); it != end;
         ++it) {
        MDagPath curDagPath = *it;
        MStatus  status;
        bool     curDagPathIsValid = curDagPath.isValid(&status);
        if (status != MS::kSuccess || !curDagPathIsValid) {
            continue;
        }

        std::string curDagPathStr(curDagPath.partialPathName(&status).asChar());
        if (status != MS::kSuccess) {
            continue;
        }

        argDagPaths.insert(curDagPathStr);

        status = curDagPath.pop();
        if (status != MS::kSuccess) {
            continue;
        }
        curDagPathIsValid = curDagPath.isValid(&status);

        while (status == MS::kSuccess && curDagPathIsValid) {
            curDagPathStr = curDagPath.partialPathName(&status).asChar();
            if (status != MS::kSuccess) {
                break;
            }

            if (argDagPathParents.find(curDagPathStr) != argDagPathParents.end()) {
                // We've already traversed up from this path.
                break;
            }
            argDagPathParents.insert(curDagPathStr);

            status = curDagPath.pop();
            if (status != MS::kSuccess) {
                break;
            }
            curDagPathIsValid = curDagPath.isValid(&status);
        }
    }
    progressBar.advance();

    // We are entering a loop here, so count the number of dag objects
    // so we can have a better progress bar status.
    // Note: Maya does the same thing during its write.
    uint32_t numberDagObjects { 0 };
    {
        for (MItDag itDag(MItDag::kDepthFirst, MFn::kInvalid); !itDag.isDone(); itDag.next()) {
            ++numberDagObjects;
        }
    }

    // Now do a depth-first traversal of the Maya DAG from the world root.
    // We keep a reference to arg dagPaths as we encounter them.
    MayaUsd::ProgressBarLoopScope dagObjLoop(numberDagObjects);
    MDagPath                      curLeafDagPath;
    for (MItDag itDag(MItDag::kDepthFirst, MFn::kInvalid); !itDag.isDone(); itDag.next()) {
        MDagPath curDagPath;
        itDag.getPath(curDagPath);
        std::string curDagPathStr(curDagPath.partialPathName().asChar());

        if (argDagPathParents.find(curDagPathStr) != argDagPathParents.end()) {
            // This dagPath is a parent of one of the arg dagPaths. It should
            // be included in the export, but not necessarily all of its
            // children should be, so we continue to traverse down.
        } else if (argDagPaths.find(curDagPathStr) != argDagPaths.end()) {
            // This dagPath IS one of the arg dagPaths. It AND all of its
            // children should be included in the export.
            curLeafDagPath = curDagPath;
        } else if (!MFnDagNode(curDagPath).hasParent(curLeafDagPath.node())) {
            // This dagPath is not a child of one of the arg dagPaths, so prune
            // it and everything below it from the traversal.
            itDag.prune();
            continue;
        }

        if (!mJobCtx._NeedToTraverse(curDagPath) && curDagPath.length() > 0) {
            // This dagPath and all of its children should be pruned.
            itDag.prune();
        } else {
            const MFnDagNode           dagNodeFn(curDagPath);
            UsdMayaPrimWriterSharedPtr primWriter = mJobCtx.CreatePrimWriter(dagNodeFn);

            if (primWriter) {
                mJobCtx.mMayaPrimWriterList.push_back(primWriter);

                // Write out data (non-animated/default values).
                if (const auto& usdPrim = primWriter->GetUsdPrim()) {
                    if (!_CheckNameClashes(usdPrim.GetPath(), primWriter->GetDagPath())) {
                        return false;
                    }

                    primWriter->Write(UsdTimeCode::Default());

                    const UsdMayaUtil::MDagPathMap<SdfPath>& mapping
                        = primWriter->GetDagToUsdPathMapping();
                    mDagPathToUsdPathMap.insert(mapping.begin(), mapping.end());

                    _modelKindProcessor->OnWritePrim(usdPrim, primWriter);
                }

                if (primWriter->ShouldPruneChildren()) {
                    itDag.prune();
                }
            }
        }
        dagObjLoop.loopAdvance();
    }

    if (!mJobCtx.mArgs.rootMapFunction.IsNull()) {
        // Check if there was no intersection between export roots and given selection.
        // We achieve this by checking if any valid prim writer was executed and populated
        // the mDagPathToUsdPathMap map.
        if (mDagPathToUsdPathMap.size() == 0) {
            MGlobal::displayError("Given export root was neither a parent or child of"
                                  " any of the items to export; export aborting");
            return false;
        }
    }

    // Writing Materials/Shading
    UsdMayaTranslatorMaterial::ExportShadingEngines(mJobCtx, mDagPathToUsdPathMap);
    progressBar.advance();

    // Perform post-processing for instances, skel, etc.
    // We shouldn't be creating new instance masters after this point, and we
    // want to cleanup the MayaExportedInstanceSources prim before writing model hierarchy.
    if (!mJobCtx._PostProcess()) {
        return false;
    }
    progressBar.advance();

    if (!_modelKindProcessor->MakeModelHierarchy(mJobCtx.mStage)) {
        return false;
    }

    // now we populate the chasers and run export default
    mChasers.clear();
    UsdMayaExportChaserRegistry::FactoryContext ctx(
        mJobCtx.mStage, mDagPathToUsdPathMap, mJobCtx.mArgs);
    MayaUsd::ProgressBarLoopScope chaserNamesLoop(mJobCtx.mArgs.chaserNames.size());
    for (const std::string& chaserName : mJobCtx.mArgs.chaserNames) {
        if (UsdMayaExportChaserRefPtr fn
            = UsdMayaExportChaserRegistry::GetInstance().Create(chaserName, ctx)) {
            mChasers.push_back(fn);
        } else {
            TF_RUNTIME_ERROR("Failed to create chaser: %s", chaserName.c_str());
        }
        chaserNamesLoop.loopAdvance();
    }

    MayaUsd::ProgressBarLoopScope chasersLoop(mChasers.size());
    for (const UsdMayaExportChaserRefPtr& chaser : mChasers) {
        if (!chaser->ExportDefault()) {
            return false;
        }
        chasersLoop.loopAdvance();
    }

    return true;
}

bool UsdMaya_WriteJob::_WriteFrame(double iFrame)
{
    const UsdTimeCode usdTime(iFrame);

    for (const UsdMayaPrimWriterSharedPtr& primWriter : mJobCtx.mMayaPrimWriterList) {
        const UsdPrim& usdPrim = primWriter->GetUsdPrim();
        if (usdPrim) {
            primWriter->Write(usdTime);
        }
    }

    for (UsdMayaExportChaserRefPtr& chaser : mChasers) {
        if (!chaser->ExportFrame(iFrame)) {
            return false;
        }
    }

    _PerFrameCallback(iFrame);

    return true;
}

bool UsdMaya_WriteJob::_PostExport()
{
    MayaUsd::ProgressBarScope progressBar(5);

    UsdPrimSiblingRange usdRootPrims = mJobCtx.mStage->GetPseudoRoot().GetChildren();

    // Write Variants (to first root prim path)
    UsdPrim usdRootPrim;
    TfToken defaultPrim;

    if (!usdRootPrims.empty()) {
        usdRootPrim = *usdRootPrims.begin();
        defaultPrim = usdRootPrim.GetName();
    }

    if (usdRootPrim && mRenderLayerObjs.length() > 1
        && !mJobCtx.mArgs.usdModelRootOverridePath.IsEmpty()) {
        // Get RenderLayers
        //   mArgs.usdModelRootOverridePath:
        //     Require mArgs.usdModelRootOverridePath to be set so that
        //     the variants are put under a UsdPrim that references a BaseModel
        //     prim that has all of the geometry, transforms, and other details.
        //     This needs to be done since "local" values have stronger precedence
        //     than "variant" values, but "referencing" will cause the variant values
        //     to take precedence.
        defaultPrim = _WriteVariants(usdRootPrim);
    }
    progressBar.advance();

    // XXX Currently all distance values are written directly to USD, and will
    // be in centimeters (Maya's internal unit) despite what the users UIUnit
    // preference is.
    // Some conversion does take place but this is experimental
    MDistance::Unit mayaInternalUnit = MDistance::internalUnit();
    auto            mayaInternalUnitLinear
        = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(mayaInternalUnit);
    if (mJobCtx.mArgs.metersPerUnit != mayaInternalUnitLinear) {
        TF_WARN(
            "Support for Distance unit conversion is evolving. "
            "All distance units will be written in %s except where conversion is supported "
            "and if enabled.",
            MDistance::Unit_EnumDef::raw_name(mayaInternalUnit)
                + sizeof(char)); // skip the k character
    }

    if (mJobCtx.mArgs.exportDistanceUnit) {
        UsdGeomSetStageMetersPerUnit(mJobCtx.mStage, mJobCtx.mArgs.metersPerUnit);
    }

    if (!mJobCtx.mArgs.defaultPrim.empty()) {
        defaultPrim = TfToken(mJobCtx.mArgs.defaultPrim);
        if (defaultPrim != TfToken("None")) {
            mJobCtx.mStage->GetRootLayer()->SetDefaultPrim(defaultPrim);
        }
    } else if (usdRootPrim) {
        // We have already decided above that 'usdRootPrim' is the important
        // prim for the export... usdVariantRootPrimPath
        mJobCtx.mStage->GetRootLayer()->SetDefaultPrim(defaultPrim);
    }
    progressBar.advance();

    // Running post export function on all the prim writers.
    const int                     loopSize = mJobCtx.mMayaPrimWriterList.size();
    MayaUsd::ProgressBarLoopScope primWriterLoop(loopSize);
    for (auto& primWriter : mJobCtx.mMayaPrimWriterList) {
        primWriter->PostExport();
        primWriterLoop.loopAdvance();
    }

    _extrasPrimsPaths.clear();

    _AddDefaultPrimAccessibility();
    progressBar.advance();

    // Run post export function on the chasers.
    MayaUsd::ProgressBarLoopScope chasersLoop(mChasers.size());
    for (const UsdMayaExportChaserRefPtr& chaser : mChasers) {
        if (!chaser->PostExport()) {
            return false;
        }

        // Collect extra prims paths from chasers
        _extrasPrimsPaths.insert(
            _extrasPrimsPaths.end(),
            chaser->GetExtraPrimsPaths().begin(),
            chaser->GetExtraPrimsPaths().end());

        chasersLoop.loopAdvance();
    }

    // Collect extra prim paths from the instanced prims.
    {
        std::vector<SdfPath> masterPaths = mJobCtx.GetAlInstanceMasterPaths();
        _extrasPrimsPaths.insert(_extrasPrimsPaths.end(), masterPaths.begin(), masterPaths.end());
    }

    _PostCallback();
    progressBar.advance();

    _PruneEmpties();
    progressBar.advance();

    _HideSourceData();
    progressBar.advance();

    return true;
}

void UsdMaya_WriteJob::_FinishWriting()
{
    MayaUsd::ProgressBarScope progressBar(2);

    TF_STATUS("Saving stage");
    if (mJobCtx.mStage->GetRootLayer()->PermissionToSave()) {
        mJobCtx.mStage->GetRootLayer()->Save();
    }

    // If we are making a usdz archive, invoke the packaging API and then clean
    // up the non-packaged stage file.
    if (!_packageName.empty()) {
        TF_STATUS("Packaging USDZ file");
        _CreatePackage();
    }
    progressBar.advance();

    mJobCtx.mStage = UsdStageRefPtr();
    mJobCtx.mMayaPrimWriterList.clear(); // clear this so that no stage references are left around

    // In the usdz case, the layer at _realFilename was just a temp file, so
    // clean it up now. Do this after mJobCtx.mStage is reset to ensure
    // there are no outstanding handles to the file, which will cause file
    // access issues on Windows.
    if (!_packageName.empty()) {
        TfDeleteFile(_realFilename);
    }
    progressBar.advance();
}

TfToken UsdMaya_WriteJob::_WriteVariants(const UsdPrim& usdRootPrim)
{
    // Some notes about the expected structure that this function will create:

    // Suppose we have a maya scene, that, with no rootPrim path, and
    // without renderLayerMode='modelingVariant', would give these prims:
    //
    //  /mayaRoot
    //  /mayaRoot/Geom
    //  /mayaRoot/Geom/Cube1
    //  /mayaRoot/Geom/Cube2
    //
    // If you have rootPrim='foo', you would instead get:
    //
    //  /foo/mayaRoot
    //  /foo/mayaRoot/Geom
    //  /foo/mayaRoot/Geom/Cube1
    //  /foo/mayaRoot/Geom/Cube2
    //
    // If you have renderLayerMode='modelingVariant', and no parent scope, you
    // will have:
    //
    //  /_BaseModel_
    //  /_BaseModel_/Geom
    //  /_BaseModel_/Geom/Cube1
    //  /_BaseModel_/Geom/Cube2
    //
    //  /mayaRoot [reference to => /_BaseModel_]
    //     [variants w/ render layer overrides]
    //
    // If you have both rootPrim='foo' and renderLayerMode='modelingVariant',
    // then you will get:
    //
    //  /_BaseModel_
    //  /_BaseModel_/mayaRoot
    //  /_BaseModel_/mayaRoot/Geom
    //  /_BaseModel_/mayaRoot/Geom/Cube1
    //  /_BaseModel_/mayaRoot/Geom/Cube2
    //
    //  /foo [reference to => /_BaseModel_]
    //     [variants w/ render layer overrides]

    // Init parameters for filtering and setting the active variant
    std::string defaultModelingVariant;

    SdfPath usdVariantRootPrimPath;
    if (mJobCtx.mRootPrimPath.IsEmpty()) {
        // Get the usdVariantRootPrimPath (optionally filter by renderLayer prefix)
        UsdMayaPrimWriterSharedPtr firstPrimWriterPtr = *mJobCtx.mMayaPrimWriterList.begin();
        std::string                firstPrimWriterPathStr(
            firstPrimWriterPtr->GetDagPath().fullPathName().asChar());
        std::replace(firstPrimWriterPathStr.begin(), firstPrimWriterPathStr.end(), '|', '/');
        std::replace(
            firstPrimWriterPathStr.begin(),
            firstPrimWriterPathStr.end(),
            ':',
            '_'); // replace namespace ":" with "_"
        usdVariantRootPrimPath = SdfPath(firstPrimWriterPathStr).GetPrefixes()[0];
    } else {
        // If they passed a rootPrim, then use that for our new top-level
        // variant-switcher prim
        usdVariantRootPrimPath = mJobCtx.mRootPrimPath;
    }

    // Create a new usdVariantRootPrim and reference the Base Model UsdRootPrim
    //   This is done for reasons as described above under mArgs.usdModelRootOverridePath
    UsdPrim usdVariantRootPrim = mJobCtx.mStage->DefinePrim(usdVariantRootPrimPath);
    TfToken defaultPrim = usdVariantRootPrim.GetName();
    usdVariantRootPrim.GetReferences().AddInternalReference(usdRootPrim.GetPath());
    usdVariantRootPrim.SetActive(true);
    usdRootPrim.SetActive(false);

    // Loop over all the renderLayers
    for (const auto& renderLayerNode : mRenderLayerObjs) {
        SdfPathTable<bool> tableOfActivePaths;
        MFnRenderLayer     renderLayerFn(renderLayerNode);
        std::string        variantName(renderLayerFn.name().asChar());
        // Determine default variant. Currently unsupported
        // MPlug renderLayerDisplayOrderPlug = renderLayerFn.findPlug("displayOrder", true);
        // int renderLayerDisplayOrder = renderLayerDisplayOrderPlug.asShort();

        // The Maya default RenderLayer is also the default modeling variant
        if (renderLayerNode == MFnRenderLayer::defaultRenderLayer()) {
            defaultModelingVariant = variantName;
        }

        // Make the renderlayer being looped the current one
        _ActivateRenderLayer(renderLayerNode);

        // == ModelingVariants ==
        // Identify prims to activate
        // Put prims and parent prims in a SdfPathTable
        // Then use that membership to determine if a prim should be Active.
        // It has to be done this way since SetActive(false) disables access to all child prims.
        MObjectArray renderLayerMemberObjs;
        renderLayerFn.listMembers(renderLayerMemberObjs);
        std::vector<SdfPath> activePaths;
        for (unsigned int im = 0; im < renderLayerMemberObjs.length(); ++im) {
            MFnDagNode dagFn(renderLayerMemberObjs[im]);
            MDagPath   dagPath;
            dagFn.getPath(dagPath);
            dagPath.extendToShape();
            SdfPath usdPrimPath;
            if (!TfMapLookup(mDagPathToUsdPathMap, dagPath, &usdPrimPath)) {
                continue;
            }
            usdPrimPath = usdPrimPath.ReplacePrefix(
                usdPrimPath.GetPrefixes()[0],
                usdVariantRootPrimPath); // Convert base to variant usdPrimPath
            tableOfActivePaths[usdPrimPath] = true;
            activePaths.push_back(usdPrimPath);
            // UsdPrim usdPrim = mStage->GetPrimAtPath(usdPrimPath);
            // usdPrim.SetActive(true);
        }
        if (!tableOfActivePaths.empty()) {
            { // == BEG: Scope for Variant EditContext
                // Create the variantSet and variant
                UsdVariantSet modelingVariantSet
                    = usdVariantRootPrim.GetVariantSets().AddVariantSet("modelingVariant");
                modelingVariantSet.AddVariant(variantName);
                modelingVariantSet.SetVariantSelection(variantName);
                // Set the Edit Context
                UsdEditTarget  editTarget = modelingVariantSet.GetVariantEditTarget();
                UsdEditContext editContext(mJobCtx.mStage, editTarget);

                // == Activate/Deactivate UsdPrims
                UsdPrimRange         rng = UsdPrimRange::AllPrims(mJobCtx.mStage->GetPseudoRoot());
                std::vector<UsdPrim> primsToDeactivate;
                for (auto it = rng.begin(); it != rng.end(); ++it) {
                    UsdPrim usdPrim = *it;
                    // For all xformable usdPrims...
                    if (usdPrim && usdPrim.IsA<UsdGeomXformable>()) {
                        bool isActive = false;
                        for (const auto& activePath : activePaths) {
                            // primPathD.HasPrefix(primPathA);
                            if (usdPrim.GetPath().HasPrefix(activePath)
                                || activePath.HasPrefix(usdPrim.GetPath())) {
                                isActive = true;
                                break;
                            }
                        }
                        if (!isActive) {
                            primsToDeactivate.push_back(usdPrim);
                            it.PruneChildren();
                        }
                    }
                }
                // Now deactivate the prims (done outside of the UsdPrimRange
                // so not to modify the iterator while in the loop)
                for (UsdPrim const& prim : primsToDeactivate) {
                    prim.SetActive(false);
                }
            } // == END: Scope for Variant EditContext
        }
    } // END: RenderLayer iterations

    // Set the default modeling variant
    UsdVariantSet modelingVariantSet = usdVariantRootPrim.GetVariantSet("modelingVariant");
    if (modelingVariantSet.IsValid()) {
        modelingVariantSet.SetVariantSelection(defaultModelingVariant);
    }
    return defaultPrim;
}

namespace {
bool isEmptyPrim(const UsdPrim& prim)
{
    // Note: prim might have been removed previously.
    if (!prim.IsValid())
        return false;

    static std::set<TfToken> emptyTypes = std::set<TfToken>({ TfToken("Xform"), TfToken("Scope") });
    TfToken                  primTypeName = prim.GetTypeName();
    if (emptyTypes.count(primTypeName) == 0)
        return false;

    if (!prim.GetAllChildren().empty())
        return false;

    if (prim.HasAuthoredPayloads())
        return false;

    if (prim.HasAuthoredReferences())
        return false;

    return true;
}

bool isEmptyPrim(const UsdStageRefPtr& stage, const SdfPath& path)
{
    return isEmptyPrim(stage->GetPrimAtPath(path));
}

std::vector<SdfPath>
removeEmptyPrims(const UsdStageRefPtr& stage, const std::vector<SdfPath>& toRemove)
{
    // Once we start removing empties, we need to re-check their parents.
    std::vector<SdfPath> toRecheck;

    for (const SdfPath& path : toRemove) {
        stage->RemovePrim(path);
        toRecheck.emplace_back(path.GetParentPath());
    }
    return toRecheck;
}
} // namespace

void UsdMaya_WriteJob::_PruneEmpties()
{
    if (mJobCtx.mArgs.includeEmptyTransforms)
        return;

    SdfPath defaultPrimPath;
    if (mJobCtx.mArgs.defaultPrim.size() > 0) {
        if (mJobCtx.mArgs.defaultPrim[0] != '/')
            defaultPrimPath = SdfPath(std::string("/") + mJobCtx.mArgs.defaultPrim);
        else
            defaultPrimPath = SdfPath(mJobCtx.mArgs.defaultPrim);
    }

    std::vector<SdfPath> toRemove;

    for (UsdPrim prim : mJobCtx.mStage->Traverse()) {
        if (defaultPrimPath != prim.GetPath())
            if (isEmptyPrim(prim))
                toRemove.emplace_back(prim.GetPath());
    }

    while (toRemove.size()) {
        std::vector<SdfPath> toRecheck = removeEmptyPrims(mJobCtx.mStage, toRemove);
        toRemove.clear();

        for (const SdfPath& path : toRecheck)
            if (defaultPrimPath != path)
                if (isEmptyPrim(mJobCtx.mStage, path))
                    toRemove.emplace_back(path);
    }
}

void UsdMaya_WriteJob::_HideSourceData()
{
    if (!mJobCtx.mArgs.hideSourceData)
        return;

    MString hideCommand("hide");

    const auto end = mJobCtx.mArgs.dagPaths.end();
    for (auto iter = mJobCtx.mArgs.dagPaths.begin(); iter != end; ++iter) {
        MDagPath curDagPath = *iter;
        if (!curDagPath.isValid())
            continue;

        MString curDagPathStr(curDagPath.partialPathName());
        if (curDagPathStr.length() <= 0)
            continue;

        hideCommand += " ";
        hideCommand += curDagPathStr;
    }

    bool displayEnabled = false;
    bool undoEnabled = true;
    MGlobal::executeCommand(hideCommand, displayEnabled, undoEnabled);
}

void UsdMaya_WriteJob::_CreatePackage() const
{
    // Since we're packaging a temporary stage file that has an
    // auto-generated name, create a nicer name for the root layer from
    // the package layer name specified by the user.
    // (Otherwise, the name inside the package will be a random string!)
    const std::string firstLayerBaseName = TfStringGetBeforeSuffix(TfGetBaseName(_packageName));
    const std::string firstLayerName = TfStringPrintf(
        "%s.%s", firstLayerBaseName.c_str(), TfGetExtension(_realFilename).c_str());

    if (mJobCtx.mArgs.compatibility == UsdMayaJobExportArgsTokens->appleArKit) {
        // If exporting with compatibility=appleArKit, there are additional
        // requirements on the usdz file to make it compatible with Apple's usdz
        // support in macOS Mojave/iOS 12.
        // UsdUtilsCreateNewARKitUsdzPackage will automatically flatten and
        // enforce that the first layer has a .usdc extension.
        if (!UsdUtilsCreateNewARKitUsdzPackage(
                SdfAssetPath(_realFilename), _packageName, firstLayerName)) {
            TF_RUNTIME_ERROR(
                "Could not create package '%s' from temporary stage '%s'",
                _packageName.c_str(),
                _realFilename.c_str());
        }
    } else {
        // No compatibility options (standard).
        if (!UsdUtilsCreateNewUsdzPackage(
                SdfAssetPath(_realFilename), _packageName, firstLayerName)) {
            TF_RUNTIME_ERROR(
                "Could not create package '%s' from temporary stage '%s'",
                _packageName.c_str(),
                _realFilename.c_str());
        }
    }
}

void UsdMaya_WriteJob::_PerFrameCallback(double /*iFrame*/)
{
    // XXX Should we be passing the frame number into the callback?
    // Unfortunately, we need to be careful that we don't affect existing
    // callbacks that don't take a frame.

    if (!mJobCtx.mArgs.melPerFrameCallback.empty()) {
        MGlobal::executeCommand(mJobCtx.mArgs.melPerFrameCallback.c_str(), true);
    }

    if (!mJobCtx.mArgs.pythonPerFrameCallback.empty()) {
        MGlobal::executePythonCommand(mJobCtx.mArgs.pythonPerFrameCallback.c_str(), true);
    }
}

// write the frame ranges and statistic string on the root
// Also call the post callbacks
void UsdMaya_WriteJob::_PostCallback()
{
    if (!mJobCtx.mArgs.melPostCallback.empty()) {
        MGlobal::executeCommand(mJobCtx.mArgs.melPostCallback.c_str(), true);
    }

    if (!mJobCtx.mArgs.pythonPostCallback.empty()) {
        MGlobal::executePythonCommand(mJobCtx.mArgs.pythonPostCallback.c_str(), true);
    }
}

bool UsdMaya_WriteJob::_CheckNameClashes(const SdfPath& path, const MDagPath& dagPath)
{
    if (!mJobCtx.mArgs.stripNamespaces) {
        return true;
    }
    auto foundPair = mUsdPathToDagPathMap.find(path);
    if (foundPair != mUsdPathToDagPathMap.end()) {
        if (mJobCtx.mArgs.mergeTransformAndShape) {
            // Shape should not conflict with xform
            MDagPath other = foundPair->second;
            MDagPath self = dagPath;
            other.extendToShape();
            self.extendToShape();
            if (other == self) {
                return true;
            }
        }
        TF_RUNTIME_ERROR(
            "Multiple dag nodes map to the same prim "
            "path after stripping namespaces: %s - %s",
            foundPair->second.fullPathName().asChar(),
            dagPath.fullPathName().asChar());
        return false;
    }
    // Note that mUsdPathToDagPathMap is _only_ used for
    // stripping namespaces, so we only need to populate it
    // when stripping namespaces. (This is different from
    // mDagPathToUsdPathMap!)
    mUsdPathToDagPathMap[path] = dagPath;
    return true;
}

void UsdMaya_WriteJob::_AddDefaultPrimAccessibility()
{
    auto defaultPrim = mJobCtx.mStage->GetDefaultPrim();
    if (!defaultPrim) {
        return;
    }

    auto accessibilityLabel = mJobCtx.mArgs.accessibilityLabel;
    auto accessibilityDescription = mJobCtx.mArgs.accessibilityDescription;
    if (accessibilityLabel.empty() && accessibilityDescription.empty()) {
        return;
    }

    /* The USD AccessibilityAPI is only available from OpenUSD 25.5 onwards.
     * We support writing the data with ad-hoc attributes on pre-25.5 versions,
     * and use the actual API for 25.5 and beyond.
     */
#if PXR_VERSION >= 2505
    auto defaultAPI = UsdUIAccessibilityAPI::ApplyDefaultAPI(defaultPrim);
    if (!accessibilityLabel.empty()) {
        defaultAPI.CreateLabelAttr(VtValue(accessibilityLabel));
    }
    if (!accessibilityDescription.empty()) {
        defaultAPI.CreateDescriptionAttr(VtValue(accessibilityDescription));
    }
#else
    defaultPrim.AddAppliedSchema(TfToken("AccessibilityAPI:default"));
    if (!accessibilityLabel.empty()) {
        auto labelAttr = defaultPrim.CreateAttribute(
            TfToken("accessibility:default:label"),
            SdfValueTypeNames->String,
            false,
            SdfVariabilityVarying);
        labelAttr.Set(accessibilityLabel);
    }

    if (!accessibilityDescription.empty()) {
        auto descriptionAttr = defaultPrim.CreateAttribute(
            TfToken("accessibility:default:description"),
            SdfValueTypeNames->String,
            false,
            SdfVariabilityVarying);
        descriptionAttr.Set(accessibilityDescription);
    }
#endif
}

const UsdMayaUtil::MDagPathMap<SdfPath>& UsdMaya_WriteJob::GetDagPathToUsdPathMap() const
{
    return mDagPathToUsdPathMap;
}

void UsdMaya_WriteJobBatch::AddJob(
    const UsdMayaJobExportArgs& args,
    const std::string&          fileName,
    bool                        appendToFile)
{
    m_jobs.emplace_back(std::make_unique<UsdMaya_WriteJob>(args, fileName, appendToFile));
}

const UsdMaya_WriteJob& UsdMaya_WriteJobBatch::JobAt(std::size_t index) const
{
    return *m_jobs.at(index);
}

bool UsdMaya_WriteJobBatch::Write()
{
    std::vector<UsdMaya_WriteJob*> jobs(m_jobs.size());
    std::transform(m_jobs.begin(), m_jobs.end(), jobs.begin(), [](auto& p) { return p.get(); });
    return UsdMaya_WriteJobImpl::WriteJobs(jobs);
}

PXR_NAMESPACE_CLOSE_SCOPE
