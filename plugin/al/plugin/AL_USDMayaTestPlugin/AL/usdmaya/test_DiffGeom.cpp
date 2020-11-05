//
// Copyright 2019 Animal Logic
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
#include "AL/usdmaya/utils/DiffPrimVar.h"

#include <maya/MFileIO.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MIntArray.h>

#include <gtest/gtest.h>

static const float P[][4] = {
    { 0, 0, 0, 1 }, { 1, 0, 0, 1 }, { 1, 1, 0, 1 }, { 0, 1, 0, 1 },
    { 0, 0, 0, 1 }, { 2, 0, 0, 1 }, { 2, 2, 0, 1 }, { 0, 2, 0, 1 },
};
static const int        FC[] = { 4, 4 };
static const int        FV[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static constexpr size_t numP = sizeof(P) / (sizeof(float) * 4);
static constexpr size_t numFC = sizeof(FC) / sizeof(int);
static constexpr size_t numFV = sizeof(FV) / sizeof(int);

// make sure the geom diffing works for point arrays of differing sizes
TEST(DiffGeom, pointTests)
{
    MFileIO::newFile(true);

    // test that the points report a change if the maya mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();
        MObject      oMesh = fnMesh.create(
            numP,
            numFC,
            MFloatPointArray(P, numP),
            MIntArray(FC, numFC),
            MIntArray(FV, numFV),
            oTransform);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> vfv, vfc;
        vfv.assign(FV, FV + numFV);
        vfc.assign(FC, FC + numFC);
        // shrink arrays - should hopefully retain the previous memory allocation (and therefore
        // values!)
        vfv.resize(numFV / 2);
        vfc.resize(numFC / 2);
        geom.CreateFaceVertexIndicesAttr(VtValue(vfv));
        geom.CreateFaceVertexCountsAttr(VtValue(vfc));

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        points.resize(numP / 2);
        geom.CreatePointsAttr(VtValue(points));

        EXPECT_EQ(
            AL::usdmaya::utils::kPoints,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kPoints));
    }

    // test that the points report a change if the usd mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();

        MFloatPointArray vp(P, numP);
        MIntArray        vfc(FC, numFC);
        MIntArray        vfv(FV, numFV);

        vp.setLength(numP / 2);
        vfc.setLength(numFC / 2);
        vfv.setLength(numFV / 2);
        MObject oMesh = fnMesh.create(numP / 2, numFC / 2, vp, vfc, vfv, oTransform);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> svfv, svfc;
        svfv.assign(FV, FV + numFV);
        svfc.assign(FC, FC + numFC);
        geom.CreateFaceVertexIndicesAttr(VtValue(svfv));
        geom.CreateFaceVertexCountsAttr(VtValue(svfc));

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        geom.CreatePointsAttr(VtValue(points));

        EXPECT_EQ(
            AL::usdmaya::utils::kPoints,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kPoints));
    }
}

static const float N[][3] = {
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 },
};
static const int        NF[] = { 0, 0, 0, 0, 1, 1, 1, 1 };
static constexpr size_t numN = sizeof(N) / (sizeof(float) * 3);

// make sure the geom diffing works for point arrays of differing sizes
TEST(DiffGeom, normalTests)
{
    MFileIO::newFile(true);

    // test that the points report a change if the maya mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();
        MObject      oMesh = fnMesh.create(
            numP,
            numFC,
            MFloatPointArray(P, numP),
            MIntArray(FC, numFC),
            MIntArray(FV, numFV),
            oTransform);
        MVectorArray mvn(N, numN);
        MIntArray    nff(NF, numN);
        MIntArray    mfv(FV, numFV);
        fnMesh.setFaceVertexNormals(mvn, nff, mfv);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> vfv, vfc;
        vfv.assign(FV, FV + numFV);
        vfc.assign(FC, FC + numFC);
        // shrink arrays - should hopefully retain the previous memory allocation (and therefore
        // values!)
        vfv.resize(numFV / 2);
        vfc.resize(numFC / 2);
        geom.CreateFaceVertexIndicesAttr(VtValue(vfv));
        geom.CreateFaceVertexCountsAttr(VtValue(vfc));
        geom.SetNormalsInterpolation(UsdGeomTokens->vertex);

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        points.resize(numP / 2);

        geom.CreatePointsAttr(VtValue(points));

        VtArray<GfVec3f> normals(numN);
        for (size_t i = 0; i < numN; ++i) {
            normals[i][0] = N[i][0];
            normals[i][1] = N[i][1];
            normals[i][2] = N[i][2];
        }
        normals.resize(numN / 2);

        geom.CreateNormalsAttr(VtValue(normals));

        EXPECT_EQ(
            AL::usdmaya::utils::kNormals,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kNormals));
    }

    MFileIO::newFile(true);

    // test that the points report a change if the usd mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();

        MFloatPointArray vp(P, numP);
        MIntArray        vfc(FC, numFC);
        MIntArray        vfv(FV, numFV);
        MVectorArray     vn(N, numN);
        MIntArray        nf(NF, numN);

        vp.setLength(numP / 2);
        vfc.setLength(numFC / 2);
        vfv.setLength(numFV / 2);
        vn.setLength(numN / 2);
        nf.setLength(numN / 2);
        MObject oMesh = fnMesh.create(numP / 2, numFC / 2, vp, vfc, vfv, oTransform);
        fnMesh.setFaceVertexNormals(vn, nf, vfv);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> svfv, svfc;
        svfv.assign(FV, FV + numFV);
        svfc.assign(FC, FC + numFC);
        geom.CreateFaceVertexIndicesAttr(VtValue(svfv));
        geom.CreateFaceVertexCountsAttr(VtValue(svfc));
        geom.SetNormalsInterpolation(UsdGeomTokens->vertex);

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        geom.CreatePointsAttr(VtValue(points));

        VtArray<GfVec3f> normals(numN);
        for (size_t i = 0; i < numN; ++i) {
            normals[i][0] = N[i][0];
            normals[i][1] = N[i][1];
            normals[i][2] = N[i][2];
        }
        geom.CreateNormalsAttr(VtValue(normals));

        EXPECT_EQ(
            AL::usdmaya::utils::kNormals,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kNormals));
    }

    MFileIO::newFile(true);

    // test that the points report a change if the maya mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();
        MObject      oMesh = fnMesh.create(
            numP,
            numFC,
            MFloatPointArray(P, numP),
            MIntArray(FC, numFC),
            MIntArray(FV, numFV),
            oTransform);
        MVectorArray mvn(N, numN);
        MIntArray    nff(NF, numN);
        MIntArray    mfv(FV, numFV);
        fnMesh.setFaceVertexNormals(mvn, nff, mfv);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> vfv, vfc;
        vfv.assign(FV, FV + numFV);
        vfc.assign(FC, FC + numFC);
        // shrink arrays - should hopefully retain the previous memory allocation (and therefore
        // values!)
        vfv.resize(numFV / 2);
        vfc.resize(numFC / 2);
        geom.CreateFaceVertexIndicesAttr(VtValue(vfv));
        geom.CreateFaceVertexCountsAttr(VtValue(vfc));
        geom.SetNormalsInterpolation(UsdGeomTokens->faceVarying);

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        points.resize(numP / 2);

        geom.CreatePointsAttr(VtValue(points));

        VtArray<GfVec3f> normals(numN);
        for (size_t i = 0; i < numN; ++i) {
            normals[i][0] = N[i][0];
            normals[i][1] = N[i][1];
            normals[i][2] = N[i][2];
        }
        normals.resize(numN / 2);

        geom.CreateNormalsAttr(VtValue(normals));

        EXPECT_EQ(
            AL::usdmaya::utils::kNormals,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kNormals));
    }

    MFileIO::newFile(true);

    // test that the points report a change if the usd mesh has an additional polygon
    {
        MFnTransform fnTM;
        MFnMesh      fnMesh;
        MObject      oTransform = fnTM.create();

        MFloatPointArray vp(P, numP);
        MIntArray        vfc(FC, numFC);
        MIntArray        vfv(FV, numFV);
        MVectorArray     vn(N, numN);
        MIntArray        nf(NF, numN);

        vp.setLength(numP / 2);
        vfc.setLength(numFC / 2);
        vfv.setLength(numFV / 2);
        vn.setLength(numN / 2);
        nf.setLength(numN / 2);
        MObject oMesh = fnMesh.create(numP / 2, numFC / 2, vp, vfc, vfv, oTransform);
        fnMesh.setFaceVertexNormals(vn, nf, vfv);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ASSERT_TRUE(stage);

        auto         geom = UsdGeomMesh::Define(stage, SdfPath("/mesh"));
        VtArray<int> svfv, svfc;
        svfv.assign(FV, FV + numFV);
        svfc.assign(FC, FC + numFC);
        geom.CreateFaceVertexIndicesAttr(VtValue(svfv));
        geom.CreateFaceVertexCountsAttr(VtValue(svfc));
        geom.SetNormalsInterpolation(UsdGeomTokens->faceVarying);

        VtArray<GfVec3f> points(numP);
        for (size_t i = 0; i < numP; ++i) {
            points[i][0] = P[i][0];
            points[i][1] = P[i][1];
            points[i][2] = P[i][2];
        }
        geom.CreatePointsAttr(VtValue(points));

        VtArray<GfVec3f> normals(numN);
        for (size_t i = 0; i < numN; ++i) {
            normals[i][0] = N[i][0];
            normals[i][1] = N[i][1];
            normals[i][2] = N[i][2];
        }
        geom.CreateNormalsAttr(VtValue(normals));

        EXPECT_EQ(
            AL::usdmaya::utils::kNormals,
            AL::usdmaya::utils::diffGeom(
                geom, fnMesh, UsdTimeCode::Default(), AL::usdmaya::utils::kNormals));
    }
}
