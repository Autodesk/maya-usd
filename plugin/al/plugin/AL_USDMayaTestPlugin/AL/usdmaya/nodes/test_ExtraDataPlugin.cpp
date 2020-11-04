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
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/fileio/translators/TestExtraDataPlugin.h"
#include "AL/usdmaya/fileio/translators/TranslatorTestType.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

#include <fstream>

using namespace AL::usdmaya::fileio::translators;
using AL::maya::test::buildTempPath;

TEST(ExtraDataPlugin, ExtraDataPlugin)
{
    const std::string filePath = buildTempPath("AL_USDMayaTests_extraData.usda");
    {
        // create a TranslatorTestType usd prim
        UsdStageRefPtr     m_stage = UsdStage::CreateInMemory();
        TranslatorTestType testPrim = TranslatorTestType::Define(m_stage, SdfPath("/testPrim"));
        UsdPrim            m_prim = testPrim.GetPrim();
        m_stage->GetRootLayer()->Export(filePath);
    }

    MFileIO::newFile(true);
    auto status = MGlobal::executeCommand(
        MString("AL_usdmaya_ProxyShapeImport -file \"") + filePath.c_str() + MString("\""));
    EXPECT_EQ(MS::kSuccess, status);

    // select the shape
    MSelectionList sl;
    EXPECT_TRUE(sl.add("AL_usdmaya_ProxyShape"));
    MObject node;
    sl.getDependNode(0, node);
    ASSERT_TRUE(status);

    // grab pointer to it
    MFnDependencyNode fn(node, &status);
    ASSERT_TRUE(status);
    AL::usdmaya::nodes::ProxyShape* shape = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    ASSERT_TRUE(shape);

    // now we can access the translator context

    auto manufacture = shape->translatorManufacture();
    auto context = shape->context();
    ASSERT_TRUE(context);

    MFnDagNode fnDag;
    MObject    mayaTM = fnDag.create("transform");
    MObject    mayaObject = fnDag.create("distanceDimShape", mayaTM);
    EXPECT_TRUE(mayaObject.hasFn(MFn::kDistance));

    auto dataPlugins = manufacture.getExtraDataPlugins(mayaObject);
    ASSERT_TRUE(!dataPlugins.empty());
    EXPECT_EQ(1u, dataPlugins.size());

    // ensure correct extra data plugin returned
    auto first = dataPlugins[0];
    EXPECT_EQ(MFn::kDistance, first->getFnType());

    typedef TfRefPtr<AL::usdmaya::fileio::translators::TestExtraDataPlugin> TestExtraDataPluginPtr;
    auto pptr = TfStatic_cast<TestExtraDataPluginPtr>(first);

    EXPECT_TRUE(pptr->importCalled);
    EXPECT_TRUE(pptr->postImportCalled);
    EXPECT_TRUE(pptr->initialiseCalled);
    EXPECT_FALSE(pptr->exportObjectCalled);
    EXPECT_FALSE(pptr->preTearDownCalled);
    EXPECT_FALSE(pptr->updateCalled);

    pptr->importCalled = false;
    pptr->postImportCalled = false;
    pptr->initialiseCalled = false;

    // grab the stage and deactivate the prim
    auto stage = shape->usdStage();
    auto prim = stage->GetPrimAtPath(SdfPath("/testPrim"));
    prim.SetActive(false);

    EXPECT_FALSE(pptr->importCalled);
    EXPECT_FALSE(pptr->postImportCalled);
    EXPECT_FALSE(pptr->initialiseCalled);
    EXPECT_FALSE(pptr->exportObjectCalled);
    EXPECT_TRUE(pptr->preTearDownCalled);
    EXPECT_FALSE(pptr->updateCalled);

    pptr->initialiseCalled = false;
    pptr->preTearDownCalled = false;
    prim.SetActive(true);

    EXPECT_TRUE(pptr->importCalled);
    EXPECT_TRUE(pptr->postImportCalled);
    EXPECT_FALSE(pptr->initialiseCalled);
    EXPECT_FALSE(pptr->exportObjectCalled);
    EXPECT_FALSE(pptr->preTearDownCalled);
    EXPECT_FALSE(pptr->updateCalled);
    prim.SetActive(false);
    pptr->importCalled = false;
    pptr->postImportCalled = false;
    pptr->preTearDownCalled = false;

    sl.clear();
    sl.add(mayaTM);
    MGlobal::setActiveSelectionList(sl);

    // lets see if we can export the schema plugin
    const std::string temp_path = buildTempPath("AL_USDMayaTests_extraData2.usda");
    MString           command = "file -force -options "
                      "\"Merge_Transforms=1;\" -typ \"AL usdmaya export\" -pr -ea \"";
    command += temp_path.c_str();
    command += "\";";
    MGlobal::executeCommand(command);

    // we can't test the translators to see if export has been called, since the export would use a
    // different context, and so the extra data plugin used will be a new instance :( As a result,
    // load the usda file that was exported, and see if the extra data plugin has been applied to
    // the prim
    auto stg = UsdStage::Open(temp_path);
    ASSERT_TRUE(stg);
    prim = stg->GetPrimAtPath(SdfPath("/transform1"));
    ASSERT_TRUE(prim);

    auto v = prim.GetAttribute(TfToken("exported"));
    ASSERT_TRUE(v);
}
