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
#include "AL/maya/test/testHelpers.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MCommonSystemUtils.h>
#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

#include <fstream>
#include <iostream>

using AL::maya::test::buildTempPath;
using AL::maya::test::compareTempPaths;

// UsdStageRefPtr ProxyShape::getUsdStage() const;
// UsdPrim ProxyShape::getRootPrim()
TEST(ProxyShape, basicProxyShapeSetUp)
{
    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        UsdGeomXform ltoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/rtoe1"));
        UsdGeomXform leg2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
        UsdGeomXform knee2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2"));
        UsdGeomXform ankle2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2"));
        UsdGeomXform rtoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/ltoe2"));
        UsdGeomXform ltoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/rtoe2"));

        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_basicProxyShapeSetUp.usda");
    const std::string temp_path2 = buildTempPath("AL_USDMayaTests_basicLayerSetUp.usda");
    const MString     temp_ma_path = buildTempPath("AL_USDMayaTests_basicProxyShapeSetUp.ma");
    std::string       sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        ASSERT_TRUE(stage);

        // should be composed of two layers
        SdfLayerHandle session = stage->GetSessionLayer();
        SdfLayerHandle root = stage->GetRootLayer();
        EXPECT_TRUE(session);
        EXPECT_TRUE(root);

        // make sure path is correct
        compareTempPaths(temp_path, root->GetRealPath());

        // UsdPrim ProxyShape::getRootPrim()
        UsdPrim rootPrim = proxy->getRootPrim();
        EXPECT_TRUE(rootPrim);
        EXPECT_TRUE(SdfPath("/") == rootPrim.GetPath());

        stage->SetEditTarget(session);

        // lets grab a prim, and then modify it a bit (this should leave us with a modification in
        // the session layer)
        UsdPrim rtoe1Prim = stage->GetPrimAtPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        EXPECT_TRUE(rtoe1Prim);
        UsdGeomXform rtoe1Geom(rtoe1Prim);

        // add some scale value to the geom (we can hunt this down later
        UsdGeomXformOp scaleOp = rtoe1Geom.AddScaleOp();
        GfVec3f        scale(1.0f, 2.0f, 3.0f);
        scaleOp.Set<GfVec3f>(scale);

        std::vector<UsdGeomXformOp> ordered;
        ordered.push_back(scaleOp);
        rtoe1Geom.SetXformOpOrder(ordered);

        EXPECT_TRUE(session->ExportToString(&sessionLayerContents));
        EXPECT_FALSE(sessionLayerContents.empty());

        // save the maya file (the modifications we made to the session layer should be present when
        // we reload)
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(temp_ma_path));

        // after saving, we should have a layerManager
        AL::usdmaya::nodes::LayerManager* layerManager
            = AL::usdmaya::nodes::LayerManager::findManager();
        ASSERT_TRUE(layerManager);
        SdfLayerHandle refoundExpectedLayer = layerManager->findLayer(session->GetIdentifier());
        EXPECT_TRUE(session->IsDirty());
        EXPECT_TRUE(refoundExpectedLayer);
        EXPECT_EQ(refoundExpectedLayer, session);

        // Because root layer isn't dirty, don't expect it to be saved out
        EXPECT_FALSE(root->IsDirty());
        refoundExpectedLayer = layerManager->findLayer(root->GetIdentifier());
        EXPECT_FALSE(refoundExpectedLayer);

        // please don't crash if I pass a NULL layer handle
        EXPECT_EQ(SdfLayerHandle(), layerManager->findLayer(""));

        {
            // please don't crash if I pass a valid layer, that isn't in any way involved in the
            // composed stage
            SdfLayerRefPtr temp = SdfLayer::CreateNew(temp_path2);
            EXPECT_EQ(nullptr, layerManager->findLayer(temp->GetIdentifier()));
        }
    }

    // nuke everything
    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::newFile(true));

    // inspect the sdf layer cache to make sure that has been cleared!
    {
        UsdStageCache& layerCache = AL::usdmaya::StageCache::Get();
        EXPECT_EQ(size_t(0), layerCache.Size());
    }
    {
        UsdStageCache& layerCache = AL::usdmaya::StageCache::Get();
        EXPECT_EQ(size_t(0), layerCache.Size());
    }

    // Now re-open the file, and re-check everything to make sure it restored correctly
    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::open(temp_ma_path, NULL, true));

    {
        // find the proxy shape node in the scene
        MSelectionList sl;
        EXPECT_EQ(MStatus(MS::kSuccess), sl.add(shapeName));
        MObject shape;
        EXPECT_EQ(MStatus(MS::kSuccess), sl.getDependNode(0, shape));
        MStatus    status;
        MFnDagNode fn(shape, &status);
        EXPECT_EQ(MStatus(MS::kSuccess), status);

        // grab ptr to proxy
        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        EXPECT_EQ(MString(temp_path.c_str()), proxy->filePathPlug().asString());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        ASSERT_TRUE(stage);

        // should be composed of two layers
        SdfLayerHandle session = stage->GetSessionLayer();
        SdfLayerHandle root = stage->GetRootLayer();
        EXPECT_TRUE(session);
        EXPECT_TRUE(root);

        // make sure path is correct
        compareTempPaths(temp_path, root->GetRealPath());

        AL::usdmaya::nodes::LayerManager* layerManager
            = AL::usdmaya::nodes::LayerManager::findManager();
        ASSERT_TRUE(layerManager);
        SdfLayerHandle refoundExpectedLayer = layerManager->findLayer(root->GetIdentifier());
        // Root wasn't dirty, shouldn't have been saved out
        EXPECT_FALSE(refoundExpectedLayer);

        // UsdPrim ProxyShape::getRootPrim()
        UsdPrim rootPrim = proxy->getRootPrim();
        EXPECT_TRUE(rootPrim);
        EXPECT_TRUE(SdfPath("/") == rootPrim.GetPath());

        // lets grab a prim, and then modify it a bit (this should leave us with a modification in
        // the session layer)
        UsdPrim rtoe1Prim = stage->GetPrimAtPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        EXPECT_TRUE(rtoe1Prim);
        UsdGeomXform rtoe1Geom(rtoe1Prim);

        std::vector<UsdGeomXformOp> ordered;
        bool                        foo = 0;
        ordered = rtoe1Geom.GetOrderedXformOps(&foo);

        ASSERT_EQ(size_t(1), ordered.size());

        // add some scale value to the geom (we can hunt this down later
        UsdGeomXformOp scaleOp = ordered[0];
        GfVec3f        scale2(1.0f, 2.0f, 3.0f);
        GfVec3f        scale;
        scaleOp.Get<GfVec3f>(&scale);
        EXPECT_EQ(scale2, scale);

        std::string contents;
        EXPECT_TRUE(session->ExportToString(&contents));
        EXPECT_FALSE(contents.empty());
        EXPECT_EQ(contents, sessionLayerContents);
    }
}

// MObject makeUsdTransformChain(
//     const UsdPrim& usdPrim,
//     MDagModifier& modifier,
//     TransformReason reason,
//     MDGModifier* modifier2 = 0,
//     uint32_t* createCount = 0);
// void removeUsdTransformChain(
//     const UsdPrim& usdPrim,
//     MDagModifier& modifier,
//     TransformReason reason);
// inline bool isRequiredPath(const SdfPath& path) const;
// inline MObject findRequiredPath(const SdfPath& path) const;
TEST(ProxyShape, basicTransformChainOperations)
{
    MFileIO::newFile(true);
    auto constructTransformChain = [](std::vector<UsdGeomXform>& xforms) {
        GfVec3f scale(2.0f, 3.0f, 4.0f);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        UsdGeomXform ltoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/rtoe1"));
        UsdGeomXform leg2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
        UsdGeomXform knee2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2"));
        UsdGeomXform ankle2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2"));
        UsdGeomXform rtoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/ltoe2"));
        UsdGeomXform ltoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/rtoe2"));

        xforms.push_back(root);
        xforms.push_back(leg1);
        xforms.push_back(knee1);
        xforms.push_back(ankle1);
        xforms.push_back(rtoe1);
        xforms.push_back(ltoe1);
        xforms.push_back(leg2);
        xforms.push_back(knee2);
        xforms.push_back(ankle2);
        xforms.push_back(rtoe2);
        xforms.push_back(ltoe2);

        root.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        leg1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        knee1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ankle1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        rtoe1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ltoe1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        leg2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        knee2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ankle2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        rtoe2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ltoe2.AddScaleOp().Set<GfVec3f>(scale);

        return stage;
    };

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_basicTransformChainOperations.usda");

    std::vector<UsdGeomXform> xforms;
    std::string               sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain(xforms);
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        UsdPrim prim = stage->GetPrimAtPath(SdfPath("/root/hip1/knee1/ankle1/ltoe1"));

        // AL::usdmaya::nodes::ProxyShape::kSelection
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            uint32_t     createCount = 0;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransformChain(
                prim,
                modifier1,
                AL::usdmaya::nodes::ProxyShape::kSelection,
                &modifier2,
                &createCount);

            EXPECT_EQ(1u, proxy->selectedPaths().size());

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());
            EXPECT_EQ(5U, createCount);

            const GfVec3f scales[] = { GfVec3f(2.4f, 3.8f, 5.2f),
                                       GfVec3f(2.3f, 3.6f, 4.9f),
                                       GfVec3f(2.2f, 3.4f, 4.6f),
                                       GfVec3f(2.1f, 3.2f, 4.3f),
                                       GfVec3f(2.0f, 3.0f, 4.0f) };

            const MString paths[] = { MString("/root/hip1/knee1/ankle1/ltoe1"),
                                      MString("/root/hip1/knee1/ankle1"),
                                      MString("/root/hip1/knee1"),
                                      MString("/root/hip1"),
                                      MString("/root") };

            {
                MFnTransform fnx(leafNode);
                for (int i = 0; i < 5; ++i) {
                    // make sure we can read the scale value from the transform. It should
                    // *hopefully* match the values we set previously
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // these should report true
                    EXPECT_TRUE(proxy->isRequiredPath(SdfPath(paths[i].asChar())));
                    EXPECT_TRUE(
                        proxy->findRequiredPath(SdfPath(paths[i].asChar())) != MObject::kNullObj);

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // request again to construct a transform chain (using the same selection mode).
            // A second chain should not be constructed (cannot select the same node more than once)
            MDagModifier modifier1b;
            MDGModifier  modifier2b;
            createCount = 0;
            MObject leafNode2 = proxy->makeUsdTransformChain(
                prim,
                modifier1b,
                AL::usdmaya::nodes::ProxyShape::kSelection,
                &modifier2b,
                &createCount);
            EXPECT_EQ(1u, proxy->selectedPaths().size());

            // hopefully not much will happen this time!
            EXPECT_FALSE(leafNode2 == MObject::kNullObj);
            EXPECT_TRUE(leafNode == leafNode2);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1b.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2b.doIt());
            EXPECT_EQ(0U, createCount);

            // no real reason to do this. It shouldn't have changed anything.
            {
                MFnTransform fnx(leafNode2);
                for (int i = 0; i < 5; ++i) {
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // now lets' go and remove all of those transforms for fun!
            proxy->removeUsdTransformChain(
                prim, modifier1, AL::usdmaya::nodes::ProxyShape::kSelection);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(0u, proxy->selectedPaths().size());

            // having removed those chains, we shouldn't have any more transform nodes left
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }

        // AL::usdmaya::nodes::ProxyShape::kRequired
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            uint32_t     createCount = 0;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransformChain(
                prim,
                modifier1,
                AL::usdmaya::nodes::ProxyShape::kRequired,
                &modifier2,
                &createCount);

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());
            EXPECT_EQ(5U, createCount);

            const GfVec3f scales[] = { GfVec3f(2.4f, 3.8f, 5.2f),
                                       GfVec3f(2.3f, 3.6f, 4.9f),
                                       GfVec3f(2.2f, 3.4f, 4.6f),
                                       GfVec3f(2.1f, 3.2f, 4.3f),
                                       GfVec3f(2.0f, 3.0f, 4.0f) };

            const MString paths[] = { MString("/root/hip1/knee1/ankle1/ltoe1"),
                                      MString("/root/hip1/knee1/ankle1"),
                                      MString("/root/hip1/knee1"),
                                      MString("/root/hip1"),
                                      MString("/root") };

            {
                MFnTransform fnx(leafNode);
                for (int i = 0; i < 5; ++i) {
                    // make sure we can read the scale value from the transform. It should
                    // *hopefully* match the values we set previously
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // these should report true
                    EXPECT_TRUE(proxy->isRequiredPath(SdfPath(paths[i].asChar())));
                    EXPECT_TRUE(
                        proxy->findRequiredPath(SdfPath(paths[i].asChar())) == fnx.object());

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // request again to construct a transform chain (using the same selection mode).
            // Since the nodes already exist, then no new nodes should be created one would hope!
            MDagModifier modifier1b;
            MDGModifier  modifier2b;
            createCount = 0;
            MObject leafNode2 = proxy->makeUsdTransformChain(
                prim,
                modifier1b,
                AL::usdmaya::nodes::ProxyShape::kRequired,
                &modifier2b,
                &createCount);

            // hopefully not much will happen this time!
            EXPECT_FALSE(leafNode2 == MObject::kNullObj);
            EXPECT_TRUE(leafNode == leafNode2);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1b.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2b.doIt());
            EXPECT_EQ(0U, createCount);

            // no real reason to do this. It shouldn't have changed anything.
            {
                MFnTransform fnx(leafNode2);
                for (int i = 0; i < 5; ++i) {
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // now lets' go and remove all of those transforms for fun!
            proxy->removeUsdTransformChain(
                prim, modifier1, AL::usdmaya::nodes::ProxyShape::kRequired);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());

            // having removed those chains, we shouldn't have any more transform nodes left
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_FALSE(it.isDone());
            }

            // now lets' go and remove all of those transforms for fun!
            proxy->removeUsdTransformChain(
                prim, modifier1, AL::usdmaya::nodes::ProxyShape::kRequired);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());

            // having removed those chains, we shouldn't have any more transform nodes left
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }

        // AL::usdmaya::nodes::ProxyShape::kRequested
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            uint32_t     createCount = 0;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransformChain(
                prim,
                modifier1,
                AL::usdmaya::nodes::ProxyShape::kRequested,
                &modifier2,
                &createCount);

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());
            EXPECT_EQ(5U, createCount);

            const GfVec3f scales[] = { GfVec3f(2.4f, 3.8f, 5.2f),
                                       GfVec3f(2.3f, 3.6f, 4.9f),
                                       GfVec3f(2.2f, 3.4f, 4.6f),
                                       GfVec3f(2.1f, 3.2f, 4.3f),
                                       GfVec3f(2.0f, 3.0f, 4.0f) };

            const MString paths[] = { MString("/root/hip1/knee1/ankle1/ltoe1"),
                                      MString("/root/hip1/knee1/ankle1"),
                                      MString("/root/hip1/knee1"),
                                      MString("/root/hip1"),
                                      MString("/root") };

            {
                MFnTransform fnx(leafNode);
                for (int i = 0; i < 5; ++i) {
                    // make sure we can read the scale value from the transform. It should
                    // *hopefully* match the values we set previously
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // these should report false
                    EXPECT_TRUE(proxy->isRequiredPath(SdfPath(paths[i].asChar())));
                    EXPECT_TRUE(
                        proxy->findRequiredPath(SdfPath(paths[i].asChar())) != MObject::kNullObj);

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // request again to construct a transform chain (using the same selection mode).
            // Since the nodes already exist, then no new nodes should be created one would hope!
            MDagModifier modifier1b;
            MDGModifier  modifier2b;
            createCount = 0;
            MObject leafNode2 = proxy->makeUsdTransformChain(
                prim,
                modifier1b,
                AL::usdmaya::nodes::ProxyShape::kRequested,
                &modifier2b,
                &createCount);

            // hopefully not much will happen this time!
            EXPECT_FALSE(leafNode2 == MObject::kNullObj);
            EXPECT_TRUE(leafNode == leafNode2);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1b.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2b.doIt());
            EXPECT_EQ(0U, createCount);

            // no real reason to do this. It shouldn't have changed anything.
            {
                MFnTransform fnx(leafNode2);
                for (int i = 0; i < 5; ++i) {
                    double sc[3];
                    EXPECT_EQ(MStatus(MS::kSuccess), fnx.getScale(sc));
                    EXPECT_NEAR(scales[i][0], sc[0], 1e-5f);
                    EXPECT_NEAR(scales[i][1], sc[1], 1e-5f);
                    EXPECT_NEAR(scales[i][2], sc[2], 1e-5f);

                    MStatus status;
                    MPlug   primPathPlug = fnx.findPlug("primPath", &status);
                    EXPECT_EQ(MStatus(MS::kSuccess), status);

                    // make sure path is correct
                    EXPECT_EQ(paths[i], primPathPlug.asString());

                    // step up chain
                    fnx.setObject(fnx.parent(0));
                }
            }

            // now lets' go and remove all of those transforms for fun!
            proxy->removeUsdTransformChain(
                prim, modifier1, AL::usdmaya::nodes::ProxyShape::kRequested);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());

            // This one is slightly different. The requested transforms are ref counted, so we
            // *should* still have some plugin nodes
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_FALSE(it.isDone());
            }

            // So here the second call should nuke the nodes
            MDagModifier modifier1d;
            proxy->removeUsdTransformChain(
                prim, modifier1d, AL::usdmaya::nodes::ProxyShape::kRequested);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1d.doIt());

            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }

        // Now lets see what happens when we mix and match TM types.

        // AL::usdmaya::nodes::ProxyShape::kRequired
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            uint32_t     createCount = 0;

            // construct a chain of transform nodes
            EXPECT_EQ(0u, proxy->selectedPaths().size());
            MObject leafNode = proxy->makeUsdTransformChain(
                prim,
                modifier1,
                AL::usdmaya::nodes::ProxyShape::kSelection,
                &modifier2,
                &createCount);
            EXPECT_EQ(1u, proxy->selectedPaths().size());

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());
            EXPECT_EQ(5U, createCount);

            // construct a chain of transform nodes
            UsdPrim kneeprim = stage->GetPrimAtPath(SdfPath("/root/hip1/knee1"));
            createCount = 0;
            MObject kneeNode = proxy->makeUsdTransformChain(
                kneeprim,
                modifier1,
                AL::usdmaya::nodes::ProxyShape::kRequired,
                &modifier2,
                &createCount);
            EXPECT_EQ(1u, proxy->selectedPaths().size());

            // make sure we get some sane looking values.
            EXPECT_FALSE(kneeNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());
            EXPECT_EQ(0U, createCount);

            // now remove the selected transforms to the prim    {

            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_FALSE(it.isDone());
            }

            MDagModifier modifier1b;
            proxy->removeUsdTransformChain(
                prim, modifier1b, AL::usdmaya::nodes::ProxyShape::kSelection);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1b.doIt());
            EXPECT_EQ(0u, proxy->selectedPaths().size());

            // We should now only have 3 TM's left
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_FALSE(it.isDone());
                uint32_t count = 0;
                while (!it.isDone()) {
                    it.next();
                    count++;
                }
                EXPECT_EQ(3u, count);
            }

            {
                // we should be able to attach to this transform
                MStatus      status;
                MFnTransform fnx(kneeNode, &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                // and we *should* find it has zero children
                EXPECT_EQ(0u, fnx.childCount());
            }

            // now remove the last transforms
            MDagModifier modifier1c;
            proxy->removeUsdTransformChain(
                kneeprim, modifier1c, AL::usdmaya::nodes::ProxyShape::kRequired);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1c.doIt());

            // should have removed all of the transforms
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }
    }
}

// void makeUsdTransforms(
//     const UsdPrim& usdPrim,
//     MDagModifier& modifier,
//     TransformReason reason,
//     MDGModifier* modifier2 = 0);
// void removeUsdTransforms(
//     const UsdPrim& usdPrim,
//     MDagModifier& modifier,
//     TransformReason reason);
//
TEST(ProxyShape, basicTransformChainOperations2)
{
    MFileIO::newFile(true);
    auto constructTransformChain = [](std::vector<UsdGeomXform>& xforms) {
        GfVec3f scale(2.0f, 3.0f, 4.0f);

        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
        UsdGeomXform knee1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));
        UsdGeomXform ankle1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1"));
        UsdGeomXform rtoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/ltoe1"));
        UsdGeomXform ltoe1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1/ankle1/rtoe1"));
        UsdGeomXform leg2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
        UsdGeomXform knee2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2"));
        UsdGeomXform ankle2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2"));
        UsdGeomXform rtoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/ltoe2"));
        UsdGeomXform ltoe2 = UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee2/ankle2/rtoe2"));

        xforms.push_back(root);
        xforms.push_back(leg1);
        xforms.push_back(knee1);
        xforms.push_back(ankle1);
        xforms.push_back(rtoe1);
        xforms.push_back(ltoe1);
        xforms.push_back(leg2);
        xforms.push_back(knee2);
        xforms.push_back(ankle2);
        xforms.push_back(rtoe2);
        xforms.push_back(ltoe2);

        root.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        leg1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        knee1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ankle1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        rtoe1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ltoe1.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        leg2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        knee2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ankle2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        rtoe2.AddScaleOp().Set<GfVec3f>(scale);
        scale[0] += 0.1f;
        scale[1] += 0.2f;
        scale[2] += 0.3f;
        ltoe2.AddScaleOp().Set<GfVec3f>(scale);

        return stage;
    };

    std::vector<UsdGeomXform> xforms;
    const std::string         temp_path
        = buildTempPath("AL_USDMayaTests_basicTransformChainOperations2.usda");
    std::string sessionLayerContents;

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain(xforms);
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        UsdPrim kneePrim = stage->GetPrimAtPath(SdfPath("/root/hip1/knee1"));

        // AL::usdmaya::nodes::ProxyShape::kSelection
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            MDagModifier modifier3;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransforms(
                kneePrim, modifier1, AL::usdmaya::nodes::ProxyShape::kSelection, &modifier2);

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

            {
                // we should be able to attach to this transform
                MStatus      status;
                MFnTransform fnx(leafNode, &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                // we should have one child here (the ankle1)
                EXPECT_EQ(0u, fnx.childCount());
            }

            // construct a chain of transform nodes
            proxy->removeUsdTransforms(
                kneePrim, modifier3, AL::usdmaya::nodes::ProxyShape::kSelection);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier3.doIt());

            // should have removed all of the transforms
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }

        // AL::usdmaya::nodes::ProxyShape::kRequested
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            MDagModifier modifier3;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransforms(
                kneePrim, modifier1, AL::usdmaya::nodes::ProxyShape::kRequested, &modifier2);

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

            {
                // we should be able to attach to this transform
                MStatus      status;
                MFnTransform fnx(leafNode, &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                // we should have one child here (the ankle1)
                EXPECT_EQ(1u, fnx.childCount());

                MFnTransform fnAnkle(fnx.child(0), &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                // we should have two children here (ltoe1, rtoe1)
                EXPECT_EQ(2u, fnAnkle.childCount());

                MFnTransform fnLToe(fnAnkle.child(0), &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                MFnTransform fnRToe(fnAnkle.child(1), &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);
            }

            // construct a chain of transform nodes
            proxy->removeUsdTransforms(
                kneePrim, modifier3, AL::usdmaya::nodes::ProxyShape::kRequested);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier3.doIt());

            // should have removed all of the transforms
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }

        // AL::usdmaya::nodes::ProxyShape::kRequired
        {
            MDagModifier modifier1;
            MDGModifier  modifier2;
            MDagModifier modifier3;

            // construct a chain of transform nodes
            MObject leafNode = proxy->makeUsdTransforms(
                kneePrim, modifier1, AL::usdmaya::nodes::ProxyShape::kRequired, &modifier2);

            // make sure we get some sane looking values.
            EXPECT_FALSE(leafNode == MObject::kNullObj);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier1.doIt());
            EXPECT_EQ(MStatus(MS::kSuccess), modifier2.doIt());

            {
                // we should be able to attach to this transform
                MStatus      status;
                MFnTransform fnx(leafNode, &status);
                EXPECT_EQ(MStatus(MS::kSuccess), status);

                // we should have one child here (the ankle1)
                EXPECT_EQ(0u, fnx.childCount());
            }

            // construct a chain of transform nodes
            proxy->removeUsdTransforms(
                kneePrim, modifier3, AL::usdmaya::nodes::ProxyShape::kRequired);
            EXPECT_EQ(MStatus(MS::kSuccess), modifier3.doIt());

            // should have removed all of the transforms
            {
                MItDependencyNodes it(MFn::kPluginTransformNode);
                EXPECT_TRUE(it.isDone());
            }
        }
    }
}

// Make sure that if we make a brand new layer, make it the edit target, then
// change it away, then save, the layer is saved
TEST(ProxyShape, editTargetChangeAndSave)
{
    const MString     mayaAsciiPath = buildTempPath("AL_USDMayaTests_editTargetChangeAndSave.ma");
    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_ProxyShape_editTargetChangeAndSave.usda");
    const SdfPath dirtiestPrimPath = SdfPath("/world/dirtiestPrim");

    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/world"));
        return stage;
    };

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    MString shapeName;
    {
        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        auto newLayer = SdfLayer::New(
            SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id),
            buildTempPath("AL_USDMayaTests_fresh_layer.usda"));

        stage->GetSessionLayer()->InsertSubLayerPath(newLayer->GetIdentifier());
        // At the time newLayer is made the edit target, it shouldn't be dirty!
        stage->SetEditTarget(newLayer);
        // Now make edits to the stage, which should go to newLayer, making it dirty...
        stage->DefinePrim(dirtiestPrimPath);
        // Now change edit target away again
        stage->SetEditTarget(stage->GetRootLayer());

        // save the maya file
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(mayaAsciiPath));
    }

    {
        // reopen - the stage should have the world's dirtiest prim!
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::open(mayaAsciiPath, NULL, true));

        MSelectionList sl;
        EXPECT_EQ(MStatus(MS::kSuccess), sl.add(shapeName));
        MObject shape;
        EXPECT_EQ(MStatus(MS::kSuccess), sl.getDependNode(0, shape));
        MStatus    status;
        MFnDagNode fn(shape, &status);
        EXPECT_EQ(MStatus(MS::kSuccess), status);

        // grab ptr to proxy
        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        EXPECT_EQ(MString(temp_path.c_str()), proxy->filePathPlug().asString());

        auto stage = proxy->getUsdStage();

        // stage should be valid
        ASSERT_TRUE(stage);

        // world's dirtiest prim should exist!
        auto dirtyPrim = stage->GetPrimAtPath(dirtiestPrimPath);
        ASSERT_TRUE(dirtyPrim.IsValid());
    }
}

// Test translating a Mesh Prim via the command
TEST(ManualTranslate, importMeshPrim)
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();

    AL::usdmaya::fileio::translators::TranslatorParameters param;
    param.setForcePrimImport(true);

    SdfPathVector importPaths;
    SdfPath       meshPath("/pSphere1/pSphereShape1");
    importPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), param);

    // Select the shape, if it's there, it worked
    MObjectHandle translatedObject;
    MStatus       s = MGlobal::selectByName("pSphereShape1");
    ASSERT_TRUE(s == MStatus::kSuccess);
}

// Test translating a Mesh Prim via the command
TEST(ManualTranslate, importMergedMeshPrim)
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMergedMesh();

    AL::usdmaya::fileio::translators::TranslatorParameters param;
    param.setForcePrimImport(true);

    SdfPathVector importPaths;
    SdfPath       meshPath("/pSphere1");
    importPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), param);

    // Select the shape, if it's there, it worked
    MObjectHandle translatedObject;
    MStatus       s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_TRUE(s == MStatus::kSuccess);
}

//// Test translating a Mesh Prim via the command
TEST(ManualTranslate, roundtripMeshPrim)
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMesh();
    SdfPath                         meshPath("/pSphere1/pSphereShape1");

    AL::usdmaya::fileio::translators::TranslatorParameters tp;
    tp.setForcePrimImport(true);

    // Import Mesh, test that it actually got imported
    SdfPathVector importPaths;
    importPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), tp);
    MStatus s = MGlobal::selectByName("pSphereShape1");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

    // Tear down Mesh
    SdfPathVector teardownPaths;
    teardownPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(SdfPathVector(), teardownPaths, tp);
    s = MGlobal::selectByName("pSphereShape1");
    ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);

    // Import Mesh, test that it actually got imported
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), tp);
    s = MGlobal::selectByName("pSphereShape1");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
}

//// Test translating a Mesh Prim via the command
TEST(ManualTranslate, roundtripMergedMeshPrim)
{
    AL::usdmaya::nodes::ProxyShape* proxyShape = SetupProxyShapeWithMergedMesh();
    SdfPath                         meshPath("/pSphere1");

    AL::usdmaya::fileio::translators::TranslatorParameters tp;
    tp.setForcePrimImport(true);

    // Import Mesh, test that it actually got imported
    SdfPathVector importPaths;
    importPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), tp);
    MStatus s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);

    // Tear down Mesh
    SdfPathVector teardownPaths;
    teardownPaths.push_back(meshPath);
    proxyShape->translatePrimPathsIntoMaya(SdfPathVector(), teardownPaths, tp);
    s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_FALSE(s.statusCode() == MStatus::kSuccess);

    // Import Mesh, test that it actually got imported
    proxyShape->translatePrimPathsIntoMaya(importPaths, SdfPathVector(), tp);
    s = MGlobal::selectByName("pSphere1Shape");
    ASSERT_TRUE(s.statusCode() == MStatus::kSuccess);
}

// void destroyTransformReferences()
TEST(ProxyShape, destroyTransformReferences) { AL_USDMAYA_UNTESTED; }

// MBoundingBox boundingBox() const override;
TEST(ProxyShape, boundingBox) { AL_USDMAYA_UNTESTED; }

// std::vector<UsdPrim> huntForNativeNodesUnderPrim(const MDagPath& proxyTransformPath, SdfPath
// startPath);
TEST(ProxyShape, huntForNativeNodesUnderPrim) { AL_USDMAYA_UNTESTED; }

// void createSelectionChangedCallback();
// void destroySelectionChangedCallback();
TEST(ProxyShape, createSelectionChangedCallback) { AL_USDMAYA_UNTESTED; }

// void unloadMayaReferences();
TEST(ProxyShape, unloadMayaReferences) { AL_USDMAYA_UNTESTED; }

// void serialiseTranslatorContext();
// void deserialiseTranslatorContext();
TEST(ProxyShape, serialiseTranslatorContext) { AL_USDMAYA_UNTESTED; }

// SdfPathVector& selectedPaths()
TEST(ProxyShape, selectedPaths) { AL_USDMAYA_UNTESTED; }

// void findExcludedGeometry();
TEST(ProxyShape, findExcludedGeometry) { AL_USDMAYA_UNTESTED; }

namespace {

bool prepareBootstrapUSDA(MString& dirString, MString& bootstrapFullPath)
{
    dirString = buildTempPath("usdMayaEmptyScene");

    MStatus stats = MCommonSystemUtils::makeDirectory(dirString);
    if (!stats)
        return false;

    constexpr char content[] = "#usda 1.0";
    bootstrapFullPath = (dirString + AL_PATH_CHAR "bootstrap.usda");

    std::ofstream fileObj;
    fileObj.open(bootstrapFullPath.asChar(), std::ios::out);
    if (fileObj.is_open()) {
        fileObj << content;
        fileObj.close();
        return true;
    } else {
        return false;
    }
}

void checkStageAndRootLayer(UsdStageRefPtr stage, const MString& expectedPath)
{
    ASSERT_TRUE(stage);

    SdfLayerHandle root = stage->GetRootLayer();
    EXPECT_TRUE(root);

    // make sure path is correct
    compareTempPaths(root->GetRealPath(), expectedPath.asChar());
}
} // namespace

// void resolveRelativePathWithinMayaContext();
TEST(ProxyShape, relativePathSupport)
{
    MString tempDirString, bootstrapFullPath;
    EXPECT_TRUE(prepareBootstrapUSDA(tempDirString, bootstrapFullPath));

    // Test the relative USD bootstrap file path support:
    MFileIO::newFile(true);
    MStatus status;

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

    // Test it right away:
    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    // force the stage to load
    proxy->filePathPlug().setString("." AL_PATH_CHAR "bootstrap.usda");

    // Before testing we need to save the maya scene first, since the relative path is resolved
    // primarily with current maya scene directory.
    MString mayaFileName = tempDirString + (AL_PATH_CHAR "emptyscene.ma");
    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(mayaFileName, NULL, true));

    // Now, reopen maya scene and test again:
    MFileIO::newFile(true);
    MFileIO::open(mayaFileName, NULL, true);

    UsdStageCache&              cache = AL::usdmaya::StageCache::Get();
    std::vector<UsdStageRefPtr> stages = cache.GetAllStages();
    EXPECT_TRUE(!stages.empty());

    auto stage = stages[0];
    checkStageAndRootLayer(stage, bootstrapFullPath);

    // If the proxy shape is not referenced, the relative file path will be resolved using the
    // referenced maya scene directory:
    MFileIO::newFile(true);
    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::reference(mayaFileName, false, false, "ref"));

    const MString outerFileName = buildTempPath("AL_USDMayaTests_usdMayaTestRefEmptyScene.ma");
    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(outerFileName, NULL, true));

    // Now, reopen maya scene and test again:
    MFileIO::newFile(true);
    MFileIO::open(outerFileName, NULL, true);

    cache = AL::usdmaya::StageCache::Get();
    stages = cache.GetAllStages();
    EXPECT_TRUE(!stages.empty());

    stage = stages[0];
    checkStageAndRootLayer(stage, bootstrapFullPath);

    // Clear out the scene to avoid crashing in proxy shape code during idle
    // redraw.
    MFileIO::newFile(true);
}

// duplication
TEST(ProxyShape, duplication)
{
    MFileIO::newFile(true);

    MString tempDirString, bootstrapFullPath;
    EXPECT_TRUE(prepareBootstrapUSDA(tempDirString, bootstrapFullPath));

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

    auto proxy = reinterpret_cast<AL::usdmaya::nodes::ProxyShape*>(fn.userNode());
    // force the stage to load
    proxy->filePathPlug().setString(bootstrapFullPath.asChar());

    // Add the proxy shape to the selection, then duplicate the selection.
    MSelectionList sl;
    sl.add(shape);
    EXPECT_TRUE(MGlobal::setActiveSelectionList(sl));
    EXPECT_TRUE(MGlobal::executeCommand("duplicate"));

    // Get the newly-created proxy shape from the selection.
    sl.clear();
    EXPECT_TRUE(MGlobal::getActiveSelectionList(sl));
    MObject dupShape;
    EXPECT_TRUE(sl.getDependNode(0, dupShape));

    MFnDagNode dupFn(dupShape);
    auto       dupProxy = reinterpret_cast<AL::usdmaya::nodes::ProxyShape*>(dupFn.userNode());

    // The duplicate has the same USD file set in its file path plug.
    EXPECT_EQ(dupProxy->filePathPlug().asString(), bootstrapFullPath);

    // Its stage must not be null.
    EXPECT_NE(dupProxy->getUsdStage(), nullptr);

    // Clear out the scene to avoid crashing in proxy shape code during idle
    // redraw.
    MFileIO::newFile(true);
}

//
// funcs that aren't easily testable:
//
// bool getRenderAttris(void* attribs, const MHWRender::MFrameContext& frameContext, const MDagPath&
// dagPath); void printRefCounts() const; void constructGLImagingEngine(); inline
// UsdImagingGLHdEngine* engine() const nodes::SchemaNodeRefDB& schemaDB()
