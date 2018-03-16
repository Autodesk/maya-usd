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
  ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
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

