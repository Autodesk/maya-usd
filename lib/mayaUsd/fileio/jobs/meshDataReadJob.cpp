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
#include "meshDataReadJob.h"

#include <mayaUsd/fileio/translators/translatorMesh.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MFnMatrixData.h>
#include <maya/MFnMeshData.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>

using namespace MAYAUSD_NS_DEF;

namespace {
void convertMatrix(const GfMatrix4d& inMatrix, MMatrix& outMatrix, MObject& outMatrixObj)
{
    double usdLocalTransformData[4u][4u];
    inMatrix.Get(usdLocalTransformData);

    outMatrix = MMatrix(usdLocalTransformData);

    MFnMatrixData matrixData;
    outMatrixObj = matrixData.create();
    matrixData.set(outMatrix);
}

GfMatrix4d getTransform(const UsdPrim& prim, const std::map<SdfPath, GfMatrix4d>& parentTransforms)
{
    auto transformable = UsdGeomXformable(prim);

    GfMatrix4d usdTransform(1.0);
    if (transformable) {
        bool reset = false;
        transformable.GetLocalTransformation(&usdTransform, &reset);
    }

    SdfPath parentPath = prim.GetPath().GetParentPath();
    while (parentPath != SdfPath::AbsoluteRootPath()) {
        auto found = parentTransforms.find(parentPath);

        if (found != parentTransforms.end()) {
            usdTransform *= found->second;
        }

        parentPath = parentPath.GetParentPath();
    }

    return usdTransform;
}

void getComponentTags(MFnMeshData& dataCreator, const UsdGeomMesh& mesh)
{
#if MAYA_API_VERSION >= 20220000

    std::vector<UsdMayaMeshReadUtils::ComponentTagData> componentTags;
    UsdMayaMeshReadUtils::getComponentTags(mesh, componentTags);

    for (auto& tag : componentTags) {
        auto& name = tag.first;
        auto& content = tag.second;

        if (!dataCreator.hasComponentTag(name)) {
            dataCreator.addComponentTag(name);
        }

        dataCreator.setComponentTagContents(name, content);
    }

#endif
}
} // namespace

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_MeshDataReadJob::UsdMaya_MeshDataReadJob(
    const MayaUsd::ImportData&  iImportData,
    const UsdMayaJobImportArgs& iArgs)
    : UsdMaya_ReadJob(iImportData, iArgs)
{
}

bool UsdMaya_MeshDataReadJob::OverridePrimReader(
    const UsdPrim&               usdRootPrim,
    const UsdPrim&               prim,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext&    readCtx,
    UsdPrimRange::iterator&      primIt)
{
    MStatus status = MS::kSuccess;

    const UsdPrim& primMesh = args.GetUsdPrim();
    UsdGeomMesh    mesh(primMesh);

    if (!mesh) {
        // Skip non mesh objects
        return true;
    }

    GfMatrix4d usdTransform = getTransform(prim, mTransforms);
    mTransforms[prim.GetPath()] = usdTransform;

    // Create an object of type MMeshData and use that as the parent in the
    // translator so that MFnMesh creates mesh data without a node.
    MFnMeshData dataCreator;
    MObject     meshData = dataCreator.create(&status);
    CHECK_MSTATUS_AND_RETURN(status, true);

    MObject stageNode;

    // Usd mesh translator to get mesh data from prim.
    MayaUsd::TranslatorMeshRead meshRead(
        mesh,
        prim,
        meshData,
        stageNode,
        args.GetTimeInterval(),
        args.GetUseAsAnimationCache(),
        &readCtx,
        &status);
    CHECK_MSTATUS_AND_RETURN(status, true);

    mMeshData.emplace_back();
    auto& newData = mMeshData.back();

    // Get the mesh transformation matrix.
    MMatrix matrix;
    convertMatrix(usdTransform, matrix, newData.matrix);

    // Set the matrix on the mesh object.
    dataCreator.setMatrix(matrix);

    // Get the mesh geometry.
    newData.geometry = meshData;

    // Get the mesh component tags.
    getComponentTags(dataCreator, mesh);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
