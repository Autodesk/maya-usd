//
// Copyright 2020 Autodesk
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
#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Testing", "00Test", "Any");

    // Nothing to register Maya-side, but will load a few test translators
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj) { return MS::kSuccess; }
