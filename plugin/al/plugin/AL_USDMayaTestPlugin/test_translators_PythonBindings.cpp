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
#include "AL/usd/schemas/mayatest/ExamplePolyCubeNode.h"
#include "test_usdmaya.h"

#include <pxr/usd/usd/stage.h>

#include <maya/MFnDagNode.h>

using namespace AL::usdmaya::fileio::translators;
using AL::maya::test::buildTempPath;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test translators python bindings
//----------------------------------------------------------------------------------------------------------------------

// test manufacturing of a TranslatorTest translator
// its instantiation looks for a TranslatorTestType TfType
TEST(translators_PythonBindings, import)
{
    auto filepath = buildTempPath("examplepolycube.usda");
    auto stage = UsdStage::CreateNew(filepath);
    auto prim = AL_usd_ExamplePolyCubeNode::Define(stage, SdfPath { "/examplecube" });
    prim.GetWidthAttr().Set(0.5f);
    prim.GetHeightAttr().Set(1.2f);
    prim.GetDepthAttr().Set(3.4f);
    stage->Save();

    auto pythonscript
        = MString("'") + AL_USDMAYA_TEST_DATA + MString("/../test_data/examplecubetranslator.py'");

    MString pyExecCmd;
    pyExecCmd.format(
        "file = ^1s;\n"
        "globals = {'__file__': ^1s, '__name__': '__main__'};\n"
        "exec(compile(open(file, 'rb').read(), file, 'exec'), globals);\n",
        pythonscript);

    auto status = MGlobal::executePythonCommand(pyExecCmd);
    ASSERT_TRUE(status);

    MFnDagNode fnd;
    auto       xform = fnd.create("transform");
    auto       shape = fnd.create("AL_usdmaya_ProxyShape", xform);
    auto*      proxy = (AL::usdmaya::nodes::ProxyShape*)fnd.userNode();
    // force the stage to load
    proxy->filePathPlug().setString(filepath);

    status = MGlobal::selectByName("myrender", MGlobal::kReplaceList);
    ASSERT_TRUE(status);

    MSelectionList sl;
    MObject        obj;
    MGlobal::getActiveSelectionList(sl);
    sl.getDependNode(0, obj);
    MFnDependencyNode renderBoxDep { obj };
    ASSERT_EQ(renderBoxDep.findPlug("sizeX").asFloat(), 0.5f);
    ASSERT_EQ(renderBoxDep.findPlug("sizeY").asFloat(), 1.2f);
    ASSERT_EQ(renderBoxDep.findPlug("sizeZ").asFloat(), 3.4f);

    TranslatorManufacture::clearPythonTranslators();
}

TEST(translators_PythonBindings, unknownType)
{
    auto pythonscript = MString("'") + AL_USDMAYA_TEST_DATA + MString("/unknowntypetranslator.py'");

    MString pyExecCmd;
    pyExecCmd.format(
        "file = ^1s;\n"
        "globals = {'__file__': ^1s, '__name__': '__main__'};\n"
        "exec(compile(open(file, 'rb').read(), file, 'exec'), globals);\n",
        pythonscript);

    auto status = MGlobal::executePythonCommand(pyExecCmd);
    ASSERT_TRUE(status);

    auto pythonTranslators = TranslatorManufacture::getPythonTranslators();

    ASSERT_EQ(pythonTranslators.size(), 1u);

    TranslatorManufacture::clearPythonTranslators();
}
