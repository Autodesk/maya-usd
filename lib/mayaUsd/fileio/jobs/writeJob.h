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
#include <mayaUsd/fileio/chaser/chaser.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/pxr.h>

#include <maya/MObjectHandle.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMaya_ModelKindProcessor;

class UsdMaya_WriteJob
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMaya_WriteJob(const UsdMayaJobExportArgs& iArgs);

    MAYAUSD_CORE_PUBLIC
    ~UsdMaya_WriteJob();

    /// Writes the Maya stage to the given USD file name, If \p append is
    /// \c true, adds to an existing stage. Otherwise, replaces any existing
    /// file.
    /// This will write the entire frame range specified by the export args.
    /// Returns \c true if successful, or \c false if an error was encountered.
    MAYAUSD_CORE_PUBLIC
    bool Write(const std::string& fileName, bool append);

private:
    /// Begins constructing the USD stage, writing out the values at the default
    /// time. Returns \c true if the stage can be created successfully.
    bool _BeginWriting(const std::string& fileName, bool append);

    /// Writes the stage values at the given frame.
    /// Warning: this function must be called with non-decreasing frame numbers.
    /// If you call WriteFrame() with a frame number lower than a previous
    /// WriteFrame() call, internal code may generate errors.
    bool _WriteFrame(double iFrame);

    /// Runs any post-export processes, closes the USD stage, and writes it out
    /// to disk.
    bool _FinishWriting();

    /// Writes the root prim variants based on the Maya render layers.
    TfToken _WriteVariants(const UsdPrim& usdRootPrim);

    /// Creates a usdz package from the write job's current USD stage.
    void _CreatePackage() const;

    void _PerFrameCallback(double iFrame);
    void _PostCallback();

    bool _CheckNameClashes(const SdfPath& path, const MDagPath& dagPath);

    // Name of the created/appended USD file
    std::string _fileName;

    // Name of destination packaged archive.
    std::string _packageName;

    // Name of current layer since it should be restored after looping over them
    MString mCurrentRenderLayerName;

    // List of renderLayerObjects. Currently used for variants
    MObjectArray mRenderLayerObjs;

    UsdMayaUtil::MDagPathMap<SdfPath> mDagPathToUsdPathMap;

    // Currently only used if stripNamespaces is on, to ensure we don't have clashes
    TfHashMap<SdfPath, MDagPath, SdfPath::Hash> mUsdPathToDagPathMap;

    UsdMayaChaserRefPtrVector mChasers;

    UsdMayaWriteJobContext mJobCtx;

    std::unique_ptr<UsdMaya_ModelKindProcessor> _modelKindProcessor;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
