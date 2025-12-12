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
#ifndef PXRUSDMAYA_WRITE_JOB_H
#define PXRUSDMAYA_WRITE_JOB_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/chaser/exportChaser.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/pxr.h>

#include <maya/MObjectHandle.h>

#include <memory>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMaya_ModelKindProcessor;
class UsdMaya_WriteJobImpl;

class UsdMaya_WriteJob
{
public:
    /// Constructs a job that will write the Maya stage to the given USD file name \p fileName,
    /// If \p append is \c true, adds to an existing stage. Otherwise, replaces any existing file.
    MAYAUSD_CORE_PUBLIC
    UsdMaya_WriteJob(
        const UsdMayaJobExportArgs& iArgs,
        const std::string&          fileName,
        bool                        append = false);

    MAYAUSD_CORE_PUBLIC
    ~UsdMaya_WriteJob();

    /// Writes the Maya stage to the associated USD file.
    /// This will write the entire frame range specified by the export args.
    /// Returns \c true if successful, or \c false if an error was encountered.
    MAYAUSD_CORE_PUBLIC
    bool Write();

    MAYAUSD_CORE_PUBLIC
    SdfPath MapDagPathToSdfPath(const MDagPath& dagPath) const;

    MAYAUSD_CORE_PUBLIC
    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagPathToUsdPathMap() const;

    // Retrieve all exported material paths.
    MAYAUSD_CORE_PUBLIC
    const std::vector<SdfPath>& GetMaterialPaths() const { return mJobCtx.GetMaterialPaths(); }

    // Cached prims paths from chasers
    MAYAUSD_CORE_PUBLIC
    const std::vector<SdfPath>& GetExtraPrimsPaths() const { return _extrasPrimsPaths; }

private:
    friend class UsdMaya_WriteJobImpl;

    /// Begins constructing the USD stage, writing out the values at the default
    /// time. Returns \c true if the stage can be created successfully.
    bool _BeginWriting();

    /// Writes the stage values at the given frame.
    /// Warning: this function must be called with non-decreasing frame numbers.
    /// If you call WriteFrame() with a frame number lower than a previous
    /// WriteFrame() call, internal code may generate errors.
    bool _WriteFrame(double iFrame);

    /// Runs any post-export processes.
    bool _PostExport();

    /// Closes the USD stage, and writes it out to disk.
    void _FinishWriting();

    /// Writes the root prim variants based on the Maya render layers.
    TfToken _WriteVariants(const UsdPrim& usdRootPrim);

    /// Remove empty xform and scope recursively if the options to include them is off.
    void _PruneEmpties();

    /// Hides the source data in the Maya scene.
    void _HideSourceData();

    /// Creates a usdz package from the write job's current USD stage.
    void _CreatePackage() const;

    void _PerFrameCallback(double iFrame);
    void _PostCallback();

    bool _CheckNameClashes(const SdfPath& path, const MDagPath& dagPath);

    void _AddDefaultPrimAccessibility();

    // Name of the destination USD file.
    std::string _fileName;

    /// Should content be appended to an existing stage, or replace any existing USD file.
    bool _appendToFile = false;

    // Name of the real written USD file. It will be a temporary file if _fileName is a package.
    std::string _realFilename;

    // Name of destination packaged archive.
    std::string _packageName;

    // List of renderLayerObjects. Currently used for variants
    MObjectArray mRenderLayerObjs;

    UsdMayaUtil::MDagPathMap<SdfPath> mDagPathToUsdPathMap;

    // Array to track any extra prims created chasers
    std::vector<SdfPath> _extrasPrimsPaths;

    // Currently only used if stripNamespaces is on, to ensure we don't have clashes
    TfHashMap<SdfPath, MDagPath, SdfPath::Hash> mUsdPathToDagPathMap;

    UsdMayaExportChaserRefPtrVector mChasers;

    UsdMayaWriteJobContext mJobCtx;

    std::unique_ptr<UsdMaya_ModelKindProcessor> _modelKindProcessor;
};

/// This class queues several independent `UsdMaya_WriteJob`s, each writing to a **different**
/// output stage/file. It aims to optimize the export of multiple USD stages from an animated
/// Maya scene by reducing redundant evaluations to a single timeline pass.
class UsdMaya_WriteJobBatch
{
public:
    /// Add a job at the end of this batch with \p iArgs args. The stage to the given USD
    /// \p fileName.
    MAYAUSD_CORE_PUBLIC
    void AddJob(
        const UsdMayaJobExportArgs& args,
        const std::string&          fileName,
        bool                        appendToFile = false);

    /// Get the job at \p index.
    MAYAUSD_CORE_PUBLIC
    const UsdMaya_WriteJob& JobAt(std::size_t index) const;

    /// Runs all the write jobs in the batch, writing the Maya stages to their respective USD files.
    /// The Maya animation evaluation is optimized by performing a single timeline pass through all
    /// frames needed by the jobs in this batch, each frame beeing evaluated only once.
    /// In case of error, none of the destination USD files will be written to disk.
    /// Returns \c true if successful, or \c false if an error was encountered.
    MAYAUSD_CORE_PUBLIC
    bool Write();

private:
    std::vector<std::unique_ptr<UsdMaya_WriteJob>> m_jobs;
};

/// Return the scaling conversion factor to apply to distances when writing
/// from Maya to USD according to the given export args and the current Maya
/// internal units preference.
MAYAUSD_CORE_PUBLIC
double GetJobScalingConversionFactor(const UsdMayaJobExportArgs& exportArgs);

/// Convert the unit token from the export arguments to a metersPerUnit value.
MAYAUSD_CORE_PUBLIC
double ConvertExportArgUnitToMetersPerUnit(const TfToken& unitOption);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
