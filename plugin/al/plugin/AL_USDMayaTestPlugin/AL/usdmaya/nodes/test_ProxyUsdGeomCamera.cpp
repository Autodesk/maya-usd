//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/ProxyUsdGeomCamera.h"
#include "test_usdmaya.h"

#include <AL/maya/utils/MayaHelperMacros.h>
#include <mayaUsd/nodes/stageData.h>

#include <pxr/usd/usd/api.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDistance.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MTypes.h>

using AL::usdmaya::nodes::ProxyShape;
using AL::usdmaya::nodes::ProxyUsdGeomCamera;

using AL::maya::test::buildTempPath;
using AL::maya::test::compareTempPaths;

TEST(ProxyUsdGeomCamera, cameraProxyReadWriteAttributes)
{
    MStatus status;
    MFileIO::newFile(true);

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_cameraProxyReadWriteAttributes.usda");

    UsdStageRefPtr saveStage = UsdStage::CreateInMemory();

    UsdGeomXform::Define(saveStage, SdfPath("/root"));
    UsdGeomCamera::Define(saveStage, SdfPath("/root/cam"));

    saveStage->Export(temp_path, false);

    MFnDependencyNode fnNode;
    MObject           proxyNode = fnNode.create("AL_usd_ProxyUsdGeomCamera", &status);
    EXPECT_EQ(status, MStatus::kSuccess);
    ProxyUsdGeomCamera* proxyCamera = (ProxyUsdGeomCamera*)fnNode.userNode(&status);
    EXPECT_EQ(status, MStatus::kSuccess);
    MString proxyCameraName = proxyCamera->name();

    MFnDagNode  fnDag;
    MObject     xform = fnDag.create("transform");
    MObject     shape = fnDag.create("AL_usdmaya_ProxyShape", xform);
    ProxyShape* proxyShape = (ProxyShape*)fnDag.userNode(&status);
    EXPECT_EQ(status, MStatus::kSuccess);
    MString proxyShapeName = proxyShape->name();

    // force the stage to load
    proxyShape->filePathPlug().setString(temp_path.c_str());

    auto stage = proxyShape->getUsdStage();

    // stage should be valid
    ASSERT_TRUE(stage);

    MGlobal::executeCommand(
        "connectAttr \"" + proxyShapeName + ".outStageData\" \"" + proxyCameraName + ".stage\";");
    proxyCamera->pathPlug().setString("/root/cam");

    SdfLayerHandle root = stage->GetRootLayer();
    EXPECT_TRUE(root);

    // make sure path is correct
    compareTempPaths(temp_path, root->GetRealPath());

    UsdPrim cameraPrim = stage->GetPrimAtPath(SdfPath("/root/cam"));
    EXPECT_TRUE(cameraPrim);
    UsdGeomCamera camera(cameraPrim);

    UsdTimeCode usdTime(0);

    // Maya -> USD
    proxyCamera->projectionPlug().setInt(
        static_cast<uint16_t>(ProxyUsdGeomCamera::Projection::Perspective));
    TfToken projection;
    camera.GetProjectionAttr().Get(&projection, usdTime);
    EXPECT_EQ(UsdGeomTokens->perspective, projection);
    EXPECT_EQ(false, proxyCamera->orthographicPlug().asBool());

    proxyCamera->projectionPlug().setInt(
        static_cast<uint16_t>(ProxyUsdGeomCamera::Projection::Orthographic));
    camera.GetProjectionAttr().Get(&projection, usdTime);
    EXPECT_EQ(UsdGeomTokens->orthographic, projection);
    EXPECT_EQ(true, proxyCamera->orthographicPlug().asBool());

    // USD -> Maya
    camera.GetProjectionAttr().Set(UsdGeomTokens->perspective, usdTime);
    EXPECT_EQ(
        static_cast<uint16_t>(ProxyUsdGeomCamera::Projection::Perspective),
        proxyCamera->projectionPlug().asShort());
    EXPECT_EQ(false, proxyCamera->orthographicPlug().asBool());

    camera.GetProjectionAttr().Set(UsdGeomTokens->orthographic, usdTime);
    EXPECT_EQ(
        static_cast<uint16_t>(ProxyUsdGeomCamera::Projection::Orthographic),
        proxyCamera->projectionPlug().asShort());
    EXPECT_EQ(true, proxyCamera->orthographicPlug().asBool());

    // Maya -> USD
    proxyCamera->stereoRolePlug().setInt(
        static_cast<uint16_t>(ProxyUsdGeomCamera::StereoRole::Left));
    TfToken stereoRole;
    camera.GetStereoRoleAttr().Get(&stereoRole, usdTime);
    EXPECT_EQ(UsdGeomTokens->left, stereoRole);

    proxyCamera->stereoRolePlug().setInt(
        static_cast<uint16_t>(ProxyUsdGeomCamera::StereoRole::Mono));
    camera.GetStereoRoleAttr().Get(&stereoRole, usdTime);
    EXPECT_EQ(UsdGeomTokens->mono, stereoRole);

    // USD -> Maya
    camera.GetStereoRoleAttr().Set(UsdGeomTokens->left, UsdTimeCode::Default());
    EXPECT_EQ(
        static_cast<uint16_t>(ProxyUsdGeomCamera::StereoRole::Left),
        proxyCamera->stereoRolePlug().asShort());

    camera.GetStereoRoleAttr().Set(UsdGeomTokens->mono, UsdTimeCode::Default());
    EXPECT_EQ(
        static_cast<uint16_t>(ProxyUsdGeomCamera::StereoRole::Mono),
        proxyCamera->stereoRolePlug().asShort());

    // Maya -> USD
    proxyCamera->fStopPlug().setFloat(8.0f);
    float fStop;
    camera.GetFStopAttr().Get(&fStop, usdTime);
    EXPECT_EQ(8.0f, fStop);

    // USD -> Maya
    camera.GetFStopAttr().Set(5.6f, usdTime);
    EXPECT_EQ(5.6f, proxyCamera->fStopPlug().asFloat());

    // Maya -> USD
    proxyCamera->focusDistancePlug().setFloat(10.0f);
    float focusDistance;
    camera.GetFocusDistanceAttr().Get(&focusDistance, usdTime);
    EXPECT_EQ(10.0f, focusDistance);

    // USD -> Maya
    camera.GetFocusDistanceAttr().Set(100.0f, usdTime);
    EXPECT_EQ(100.0f, proxyCamera->focusDistancePlug().asFloat());

    // Maya -> USD
    proxyCamera->focalLengthPlug().setFloat(200.0f);
    float focalLength;
    camera.GetFocalLengthAttr().Get(&focalLength, usdTime);
    EXPECT_EQ(200.0f, focalLength);

    // USD -> Maya
    camera.GetFocalLengthAttr().Set(50.0f, usdTime);
    EXPECT_EQ(50.0f, proxyCamera->focalLengthPlug().asFloat());

    // Maya -> USD
    proxyCamera->shutterOpenPlug().setDouble(200.0);
    double shutterOpen;
    camera.GetShutterOpenAttr().Get(&shutterOpen, usdTime);
    EXPECT_EQ(200.0, shutterOpen);

    // USD -> Maya
    camera.GetShutterOpenAttr().Set(50.0, usdTime);
    EXPECT_EQ(50.0, proxyCamera->shutterOpenPlug().asDouble());

    // Maya -> USD
    proxyCamera->shutterClosePlug().setDouble(200.0);
    double shutterClose;
    camera.GetShutterCloseAttr().Get(&shutterClose, usdTime);
    EXPECT_EQ(200.0, shutterClose);

    // USD -> Maya
    camera.GetShutterCloseAttr().Set(50.0, usdTime);
    EXPECT_EQ(50.0, proxyCamera->shutterClosePlug().asDouble());

    // Maya -> USD
    proxyCamera->horizontalAperturePlug().setFloat(18.0f);
    float horizontalAperture;
    camera.GetHorizontalApertureAttr().Get(&horizontalAperture, usdTime);
    EXPECT_EQ(18.0f, horizontalAperture);

    // USD -> Maya
    camera.GetHorizontalApertureAttr().Set(36.0f, usdTime);
    EXPECT_EQ(36.0f, proxyCamera->horizontalAperturePlug().asFloat());

    // Maya -> USD
    proxyCamera->verticalAperturePlug().setFloat(12.0f);
    float verticalAperture;
    camera.GetVerticalApertureAttr().Get(&verticalAperture, usdTime);
    EXPECT_EQ(12.0f, verticalAperture);

    // USD -> Maya
    camera.GetVerticalApertureAttr().Set(24.0f, usdTime);
    EXPECT_EQ(24.0f, proxyCamera->verticalAperturePlug().asFloat());

    // Maya -> USD
    proxyCamera->horizontalApertureOffsetPlug().setFloat(3.0f);
    float horizontalApertureOffset;
    camera.GetHorizontalApertureOffsetAttr().Get(&horizontalApertureOffset, usdTime);
    EXPECT_EQ(3.0f, horizontalApertureOffset);

    // USD -> Maya
    camera.GetHorizontalApertureOffsetAttr().Set(6.0f, usdTime);
    EXPECT_EQ(6.0f, proxyCamera->horizontalApertureOffsetPlug().asFloat());

    // Maya -> USD
    proxyCamera->verticalApertureOffsetPlug().setFloat(2.0f);
    float verticalApertureOffset;
    camera.GetVerticalApertureOffsetAttr().Get(&verticalApertureOffset, usdTime);
    EXPECT_EQ(2.0f, verticalApertureOffset);

    // USD -> Maya
    camera.GetVerticalApertureOffsetAttr().Set(4.0f, usdTime);
    EXPECT_EQ(4.0f, proxyCamera->verticalApertureOffsetPlug().asFloat());

    // Maya -> USD
    proxyCamera->nearClipPlanePlug().setFloat(0.1f);
    proxyCamera->farClipPlanePlug().setFloat(1000.0f);
    GfVec2f clippingRange;
    camera.GetClippingRangeAttr().Get(&clippingRange, usdTime);
    EXPECT_EQ(0.1f, clippingRange[0]);
    EXPECT_EQ(1000.0f, clippingRange[1]);

    // USD -> Maya
    camera.GetClippingRangeAttr().Set(GfVec2f(1.0f, 10000.0f), usdTime);
    EXPECT_EQ(1.0f, proxyCamera->nearClipPlanePlug().asFloat());
    EXPECT_EQ(10000.0f, proxyCamera->farClipPlanePlug().asFloat());
}
