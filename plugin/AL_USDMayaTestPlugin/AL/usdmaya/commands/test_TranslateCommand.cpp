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
#include "test_usdmaya.h"

#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MFileIO.h"


TEST(TranslateCommand, translateMeshPrim)
/*
 * Test translating a Mesh Prim via the command
 */
{
  AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();
  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);

  MStatus s = MGlobal::selectByName("pSphere1Shape");
  EXPECT_TRUE(s.statusCode() == MStatus::kSuccess);
}

TEST(TranslateCommand, translateMultipleMeshPrims)
/*
 * Test translating Mesh Prims multiple times via the command
 */
{
  AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMultipleMeshes();
  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1,/pSphere2,/pSphere3\" \"AL_usdmaya_ProxyShape1\"", false, false);
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
  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"",
                          false,
                          false);

  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"",
                          false,
                          false);

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

  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
  MStatus s = MGlobal::selectByName("pSphere1Shape");
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

  // call teardown on the prim
  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
  s = MGlobal::selectByName("pSphere1Shape");
  ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);

  MGlobal::executeCommand("AL_usdmaya_TranslatePrim -fi -ip \"/pSphere1\" \"AL_usdmaya_ProxyShape1\"", false, false);
  s = MGlobal::selectByName("pSphere1Shape");
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
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
  command.format(MString("AL_usdmaya_ProxyShapeImport -file \"^1s\""), (MString(AL_USDMAYA_TEST_DATA) + MString("/sphere2.usda")));
  MGlobal::executeCommand(command, false, false);

  MString importCommand("AL_usdmaya_TranslatePrim -fi -ip \"^1s\" \"AL_usdmaya_ProxyShape\"");
  MString teardownCommand("AL_usdmaya_TranslatePrim -tp \"^1s\" \"AL_usdmaya_ProxyShape\"");

	// import foofoo and verify it made it into maya
  MString importMesh1 = importCommand;
  importMesh1.format(importCommand, MString("/pSphere1/foofoo"));
  MGlobal::executeCommand(importMesh1, false, false);
  s = MGlobal::selectByName("foofoo");
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

  // import foofooforyou and verify it made it into maya
  MString importMesh2;
  importMesh2.format(importCommand, MString("/pSphere1/foofooforyou"));
  MGlobal::executeCommand(importMesh2, false, false);
  s = MGlobal::selectByName("foofooforyou");
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

  MGlobal::clearSelectionList();

  // Teardown foofoo and verify that foofooforyou is still there
  MString tearDownMesh1;
  tearDownMesh1.format(teardownCommand, MString("/pSphere1/foofoo"));
  MGlobal::executeCommand(tearDownMesh1, false, false);
  s = MGlobal::selectByName("foofoo");
  ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);
  s = MGlobal::selectByName("foofooforyou");
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
}

