//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_INSTANCED_NODE_WRITER_H
#define PXRUSDMAYA_INSTANCED_NODE_WRITER_H

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// This is a "helper" prim writer used internally by UsdMayaWriteJobContext to
/// author nodes that are directly instanced in Maya.
class UsdMaya_InstancedNodeWriter : public UsdMayaPrimWriter
{
public:
    UsdMaya_InstancedNodeWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdInstancePath,
        UsdMayaWriteJobContext&  ctx);

    bool                                     ExportsGprims() const override;
    bool                                     ShouldPruneChildren() const override;
    const SdfPathVector&                     GetModelPaths() const override;
    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagToUsdPathMapping() const override;
    void                                     Write(const UsdTimeCode& usdTime) override;

private:
    UsdMayaWriteJobContext::_ExportAndRefPaths _masterPaths;

    // All of the data below is cached when we construct/obtain prim writers.
    bool                              _exportsGprims;
    std::vector<SdfPath>              _modelPaths;
    UsdMayaUtil::MDagPathMap<SdfPath> _dagToUsdPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
