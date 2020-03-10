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

#include "./Api.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include "AL/usdmaya/PluginRegister.h" 
#include "AL/maya/test/testHarness.h"

using AL::maya::test::UnitTestHarness;

bool g_AL_usdmaya_ignoreLockPrims = false;

AL_USDMAYATEST_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
  MFnPlugin plugin(obj, "Animal Logic", "1.0", "Any");
  AL_REGISTER_COMMAND(plugin, UnitTestHarness);
  auto status = AL::usdmaya::registerPlugin(plugin);
  // make sure lockPrims are enabled prior to running tests. 
  // Store the value as a global so it can be restored when the plugin unloads. 
  g_AL_usdmaya_ignoreLockPrims = MGlobal::optionVarIntValue("AL_usdmaya_ignoreLockPrims");
  MGlobal::setOptionVarValue("AL_usdmaya_ignoreLockPrims", false);
  return status;
}

AL_USDMAYATEST_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
  MGlobal::setOptionVarValue("AL_usdmaya_ignoreLockPrims", g_AL_usdmaya_ignoreLockPrims);
  MFnPlugin plugin(obj);
  AL_UNREGISTER_COMMAND(plugin, UnitTestHarness);
  return AL::usdmaya::unregisterPlugin(plugin);
}

