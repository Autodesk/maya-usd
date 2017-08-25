//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/CameraTranslator.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"

#include "maya/MFnDagNode.h"
#include "maya/MDagModifier.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/camera.h"

using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;
using AL::usdmaya::fileio::translators::CameraTranslator;
using AL::usdmaya::fileio::AnimationTranslator;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the CameraTranslator.
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_CameraTranslator, io)
{
  CameraTranslator::registerType();
  for(int i = 0; i < 100; ++i)
  {
    MDagModifier mod, mod2;
    MObject xform = mod.createNode("transform");
    MObject node = mod.createNode("camera", xform);
    MObject xformB = mod.createNode("transform");
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    const char* const attributeNames[] = {
        "orthographic",
        "horizontalFilmAperture",
        "verticalFilmAperture",
        "horizontalFilmOffset",
        "verticalFilmOffset",
        "focalLength",
        "focusDistance",
        "nearClipPlane",
        "farClipPlane",
        "fStop",
        //"lensSqueezeRatio"
    };
    const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

    randomNode(node, attributeNames, numAttributes);

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomCamera camera = UsdGeomCamera::Define(stage, SdfPath("/hello"));
    UsdPrim prim = camera.GetPrim();

    ExporterParams eparams;
    ImporterParams iparams;
    CameraTranslator xlator;
    EXPECT_EQ(MStatus(MS::kSuccess), CameraTranslator::copyAttributes(node, prim, eparams));
    MObject nodeB = xlator.createNode(prim, xformB, "camera", iparams);

    // now make sure the imported node matches the one we started with
    compareNodes(node, nodeB, attributeNames, numAttributes, true);

    mod2.deleteNode(nodeB);
    mod2.deleteNode(xformB);
    mod2.deleteNode(node);
    mod2.deleteNode(xform);
    mod2.doIt();
  }
}

TEST(translators_CameraTranslator, animated_io)
{
  const double startFrame = 1.0;
  const double endFrame = 20.0;

  CameraTranslator::registerType();
  for(int i = 0; i < 100; ++i)
  {
    MDagModifier mod;
    MObject xform = mod.createNode("transform");
    MObject node = mod.createNode("camera", xform);
    MObject xformB = mod.createNode("transform");
    MObject nodeB = mod.createNode("camera", xformB);
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    const char* const attributeNames[] = {
        "orthographic",
        "horizontalFilmAperture",
        "verticalFilmAperture",
        "horizontalFilmOffset",
        "verticalFilmOffset",
        "focalLength",
        "focusDistance",
        "nearClipPlane",
        "farClipPlane",
        "fStop",
        //"lensSqueezeRatio"
    };
    const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

    randomAnimatedNode(node, attributeNames, numAttributes, startFrame, endFrame);

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomCamera camera = UsdGeomCamera::Define(stage, SdfPath("/hello"));

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export animation
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    ExporterParams eparams;
    eparams.m_minFrame = startFrame;
    eparams.m_maxFrame = endFrame;
    eparams.m_animation = true;
    eparams.m_animTranslator = new AnimationTranslator;

    UsdPrim prim = camera.GetPrim();
    EXPECT_EQ(MStatus(MS::kSuccess), CameraTranslator::copyAttributes(node, prim, eparams));
    eparams.m_animTranslator->exportAnimation(eparams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Import animation
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    ImporterParams iparams;
    CameraTranslator xlator;
    EXPECT_EQ(MStatus(MS::kSuccess), xlator.copyAttributes(prim, nodeB, iparams));

    // now make sure the imported node matches the one we started with
    for(double t = eparams.m_minFrame, e = eparams.m_maxFrame + 1e-3f; t < e; t += 1.0)
    {
      MGlobal::viewFrame(t);
      compareNodes(node, nodeB, attributeNames, numAttributes, true);
    }

    EXPECT_EQ(MStatus(MS::kSuccess), mod.undoIt());
  }
}

