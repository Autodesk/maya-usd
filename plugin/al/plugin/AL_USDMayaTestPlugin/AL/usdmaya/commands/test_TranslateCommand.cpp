//
// Copyright 2018 Animal Logic
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
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

using AL::maya::test::buildTempPath;

TEST(TranslateCommand, translateMeshPrim)
/*
 * Test translating a Mesh Prim via the command
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();
    ASSERT_TRUE(proxyShape != 0);
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        false);

    MStatus s = MGlobal::selectByName("pSphereShape1");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, translateMergedMeshPrim)
/*
 * Test translating a Mesh Prim via the command
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMergedMesh();
    ASSERT_TRUE(proxyShape != 0);
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);

    MStatus s = MGlobal::selectByName("pSphere1Shape");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, doNotCreateTransform)
/*
 * Make sure we don't create a transform chain when not force importing
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();
    ASSERT_TRUE(proxyShape != 0);
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);

    MStatus s = MGlobal::selectByName("pSphere1");
    EXPECT_FALSE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, forceDefaultRead)
/*
 * Import a prim authored with one time sample only (no default value).
 * We expect to get an empty mesh with the "fd" flag because there's no default value.
 */
{
    MString importCommand
        = "AL_usdmaya_ProxyShapeImport -f \"" + MString(AL_USDMAYA_TEST_DATA) + "/cube.usda\"";

    // With "-fd" flag
    MFileIO::newFile(true);
    MStatus s;

    s = MGlobal::executeCommand(importCommand);
    EXPECT_TRUE(s == MStatus::kSuccess);
    s = MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -fd -ip \"/cube\" \"AL_usdmaya_ProxyShape\"");
    EXPECT_TRUE(s == MStatus::kSuccess);

    MIntArray ia;
    s = MGlobal::executeCommand("polyEvaluate -v cubeShape", ia, false, false);
    EXPECT_TRUE(s == MStatus::kSuccess);
    EXPECT_TRUE(ia[0] == 0);

    // Without "-fd" flag
    MFileIO::newFile(true);

    s = MGlobal::executeCommand(importCommand);
    EXPECT_TRUE(s == MStatus::kSuccess);
    s = MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/cube\" \"AL_usdmaya_ProxyShape\"");
    EXPECT_TRUE(s == MStatus::kSuccess);

    s = MGlobal::executeCommand("polyEvaluate -v cubeShape", ia, false, false);
    EXPECT_TRUE(s == MStatus::kSuccess);
    EXPECT_TRUE(ia[0] == 8);
}

TEST(TranslateCommand, translateMultipleMeshPrims)
/*
 * Test translating Mesh Prims multiple times via the command
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMultipleMeshes();
    ASSERT_TRUE(proxyShape != 0);
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1,/pSphere2,/pSphere3\" "
        "\"AL_usdmaya_ProxyShape1\"",
        false,
        false);
    MStatus s;

    s = MGlobal::selectByName("pSphere1Shape");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);

    s = MGlobal::selectByName("pSphere2Shape");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);

    s = MGlobal::selectByName("pSphere3Shape");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, translateMultipleTimes)
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMultipleMeshes();
    ASSERT_TRUE(proxyShape != 0);
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);

    MStatus s;
    s = MGlobal::selectByName("pSphere1Shape");
    EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);

    s = MGlobal::selectByName("pSphere1Shape1");
    EXPECT_FALSE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, roundTripMeshPrim)
/*
 * Test translating a Mesh Prim via the command
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();
    ASSERT_TRUE(proxyShape != 0);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        false);
    MStatus s = MGlobal::selectByName("pSphereShape1");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

    // call teardown on the prim
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -tp \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        false);
    s = MGlobal::selectByName("pSphereShape1");
    ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape1\"",
        false,
        false);
    s = MGlobal::selectByName("pSphereShape1");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, roundTripMeshMergedPrim)
/*
 * Test translating a Mesh Prim via the command
 */
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMergedMesh();
    ASSERT_TRUE(proxyShape != 0);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
    MStatus s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

    // call teardown on the prim
    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -tp \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
    s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
    s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, translateFromUnmergedFile)
{
    MFileIO::newFile(true);

    const MString command = MString("AL_usdmaya_ProxyShapeImport -file \"")
        + MString(AL_USDMAYA_TEST_DATA) + MString("/sphere.usda\"");
    MGlobal::executeCommand(command);
    MStatus s = MGlobal::selectByName("pSphereShape1");
    EXPECT_FALSE(s == MStatus::kSuccess);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape\"",
        false,
        false);
    s = MGlobal::selectByName("pSphereShape1");
    EXPECT_TRUE(s == MStatus::kSuccess);

    MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -tp \"/pSphere1/pSphereShape1\" \"AL_usdmaya_ProxyShape\"",
        false,
        false);
    s = MGlobal::selectByName("pSphereShape1");
    EXPECT_FALSE(s == MStatus::kSuccess);

    // Make sure it's also torn down the parent node
    s = MGlobal::selectByName("pSphere1");

    /// TODO: This test needs to pass prior to release!!
    EXPECT_FALSE(s == MStatus::kSuccess);
}

TEST(TranslateCommand, translateMultiplePrimsFromUnmergedFile)
/*
 * Test, in the UnMerged Case, the case where if there are multiple shape's that are siblings
 * that if one of the shape's get "tearDown" called on it, that the other sibling survives
 */
{
    MFileIO::newFile(true);
    MStatus s;
    MString command;
    command.format(
        MString("AL_usdmaya_ProxyShapeImport -file \"^1s\""),
        (MString(AL_USDMAYA_TEST_DATA) + MString("/sphere2.usda")));
    EXPECT_TRUE(MGlobal::executeCommand(command, false, false));

    MString importCommand("AL_usdmaya_TranslatePrim -fi -ip \"^1s\" \"AL_usdmaya_ProxyShape\"");
    MString teardownCommand("AL_usdmaya_TranslatePrim -tp \"^1s\" \"AL_usdmaya_ProxyShape\"");

    // import foofoo and verify it made it into maya
    MString importMesh1 = importCommand;
    EXPECT_TRUE(importMesh1.format(importCommand, MString("/pSphere1/pSphereShape1")));
    EXPECT_TRUE(MGlobal::executeCommand(importMesh1, false, false));
    s = MGlobal::selectByName("pSphereShape1");
    EXPECT_TRUE(s == MStatus::kSuccess);

    // import foofooforyou and verify it made it into maya
    MString importMesh2;
    EXPECT_TRUE(importMesh2.format(importCommand, MString("/pSphere1/pSphereShape2")));
    EXPECT_TRUE(MGlobal::executeCommand(importMesh2, false, false));
    s = MGlobal::selectByName("pSphereShape2");
    EXPECT_TRUE(s == MStatus::kSuccess);

    MGlobal::clearSelectionList();

    // Teardown foofoo and verify that foofooforyou is still there
    MString tearDownMesh1;
    EXPECT_TRUE(tearDownMesh1.format(teardownCommand, MString("/pSphere1/pSphereShape1")));
    EXPECT_TRUE(MGlobal::executeCommand(tearDownMesh1, false, false));
    s = MGlobal::selectByName("pSphereShape1");
    EXPECT_FALSE(s == MStatus::kSuccess);
    s = MGlobal::selectByName("pSphereShape2");
    EXPECT_TRUE(s == MStatus::kSuccess);
}
