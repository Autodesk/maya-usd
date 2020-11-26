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
#ifndef PXRUSDMAYA_MODEL_KIND_PROCESSOR_H
#define PXRUSDMAYA_MODEL_KIND_PROCESSOR_H

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primWriter.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// This class encapsulates all of the logic for writing model kinds from
/// UsdMaya_WriteJob. It is a "black box" that reads each newly-written prim, one
/// by one, saving information that is used to determine model hierarchy at the
/// end of writing to the USD stage.
class UsdMaya_ModelKindProcessor
{
public:
    UsdMaya_ModelKindProcessor(const UsdMayaJobExportArgs& args);

    /// Processes the given prim in order to collect model hierarchy data.
    /// This should be called after the prim has been written with the given
    /// prim writer.
    /// Note: this assumes DFS traversal, i.e. parent prims should be traversed
    /// before child prims.
    void OnWritePrim(const UsdPrim& prim, const UsdMayaPrimWriterSharedPtr& primWriter);

    /// Writes model hierarchy for the given stage based on the information
    /// collected by OnWritePrim.
    /// Existing model hierarchy information will be verified if it already
    /// exists, and computed where it does not already exist.
    ///
    /// \returns true if the model hierarchy was written successfully, or
    ///          false if there was a problem verifying or writing model kinds
    bool MakeModelHierarchy(UsdStageRefPtr& stage);

private:
    typedef std::unordered_map<SdfPath, std::vector<SdfPath>, SdfPath::Hash> _PathVectorMap;
    typedef std::unordered_map<SdfPath, bool, SdfPath::Hash>                 _PathBoolMap;
    typedef std::unordered_set<SdfPath, SdfPath::Hash>                       _PathSet;

    UsdMayaJobExportArgs _args;

    // Precomputes whether _args.rootKind IsA assembly.
    bool _rootIsAssembly;

    // Paths on at which we added USD references or authored kind.
    std::vector<SdfPath> _pathsThatMayHaveKind;

    // Maps root paths to list of exported gprims under the root path.
    // The keys in this map are only root prim paths that are assemblies
    // (either because _args.rootKind=assembly or kind=assembly was previously
    // authored).
    // These are just used for error messages.
    _PathVectorMap _pathsToExportedGprimsMap;

    // Set of all root paths that contain exported gprims.
    _PathSet _pathsWithExportedGprims;

    bool _AuthorRootPrimKinds(UsdStageRefPtr& stage, _PathBoolMap& rootPrimIsComponent);

    bool _FixUpPrimKinds(UsdStageRefPtr& stage, const _PathBoolMap& rootPrimIsComponent);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
