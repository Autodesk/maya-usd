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
#include "AL/usdmaya/PluginRegister.h"
#include "Api.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

using AL::maya::test::UnitTestHarness;

AL_USDMAYATEST_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Animal Logic", "1.0", "Any");
    AL_REGISTER_COMMAND(plugin, UnitTestHarness);
    return AL::usdmaya::registerPlugin(plugin);
}

AL_USDMAYATEST_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    AL_UNREGISTER_COMMAND(plugin, UnitTestHarness);
    return AL::usdmaya::unregisterPlugin(plugin);
}
