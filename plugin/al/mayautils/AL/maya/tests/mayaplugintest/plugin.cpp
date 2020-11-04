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
#include "AL/maya/test/testHarness.h"
#include "AL/maya/tests/mayaplugintest/utils/CommandGuiHelperTest.h"
#include "AL/maya/tests/mayaplugintest/utils/NodeHelperUnitTest.h"
#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/maya/utils/MenuBuilder.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

using AL::maya::test::UnitTestHarness;

MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, "Animal Logic", "1.0", "Any");

    AL_REGISTER_DEPEND_NODE(plugin, AL::maya::tests::utils::NodeHelperUnitTest);
    AL_REGISTER_COMMAND(plugin, AL::maya::tests::utils::CommandGuiHelperTestCMD);
    AL_REGISTER_COMMAND(plugin, UnitTestHarness);
    AL::maya::tests::utils::CommandGuiHelperTestCMD::makeGUI();

    AL_REGISTER_COMMAND(plugin, AL::maya::utils::CommandGuiListGen);
    CHECK_MSTATUS(AL::maya::utils::MenuBuilder::generatePluginUI(plugin, "mayaplugintest"));
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    AL_UNREGISTER_NODE(plugin, AL::maya::tests::utils::NodeHelperUnitTest);
    AL_UNREGISTER_COMMAND(plugin, AL::maya::tests::utils::CommandGuiHelperTestCMD);
    AL_UNREGISTER_COMMAND(plugin, UnitTestHarness);

    MStatus status;
    AL_UNREGISTER_COMMAND(plugin, AL::maya::utils::CommandGuiListGen);
    return status;
}
