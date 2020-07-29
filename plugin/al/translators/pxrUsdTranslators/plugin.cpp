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

// This plugin exists solely because:
//    a) we need to load the code in ProxyShapeTranslator.cpp, so it's PXRUSDMAYA_DEFINE_WRITER is
//    run b) we want this library separate from the main AL plugins / libraries, so they don't have
//    to link
//       against Pixar's usdMaya
//    c) the plugin finding/loading mechanism for UsdMayaPrimWriterRegistry doesn't actually ever
//    call
//       PlugPlugin.Load() on the plugins registered with it - it just uses the plugins to inspect
//       their metadata, find a maya plugin, and load that. While that works, it means you need this
//       extra boilerplate to create a maya plugin...

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>

MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, "Animal Logic", "1.0", "Any", &status);
    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    return MStatus::kSuccess;
}
