//
// Copyright 2022 Autodesk
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
#ifndef PXRUSDMAYA_MESHDATAREAD_JOB_H
#define PXRUSDMAYA_MESHDATAREAD_JOB_H

#include <mayaUsd/fileio/jobs/readJob.h>

#include <maya/MObject.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Custom class to prevent read job from creating new nodes and to access
// geometric data directly.
class UsdMaya_MeshDataReadJob : public UsdMaya_ReadJob
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMaya_MeshDataReadJob(
        const MayaUsd::ImportData&  iImportData,
        const UsdMayaJobImportArgs& iArgs);

protected:
    // Override prim reader to create a mesh without a node and store the created mesh
    // to be retrieved later.
    bool OverridePrimReader(
        const UsdPrim&               usdRootPrim,
        const UsdPrim&               prim,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext&    readCtx,
        UsdPrimRange::iterator&      primIt) override;

public:
    struct Data
    {
        MObject geometry;
        MObject matrix;
    };

    std::vector<Data> mMeshData;

private:
    std::map<SdfPath, GfMatrix4d> mTransforms;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
