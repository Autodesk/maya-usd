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
#include "AL/maya/utils/Utils.h"

#include <maya/MFnDagNode.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
MDagPath getDagPath(const MObject& object)
{
    MFnDagNode fnDag(object);
    MDagPath   dagPath;
    fnDag.getPath(dagPath);
    return dagPath;
}

//----------------------------------------------------------------------------------------------------------------------
bool ensureMayaPluginIsLoaded(const MString& pluginName)
{
    if (MFnPlugin::findPlugin(pluginName) == MObject::kNullObj) {
        MGlobal::executeCommand(
            MString("catchQuiet( `loadPlugin -quiet \"") + pluginName + "\"`)", false, false);
        if (MFnPlugin::findPlugin(pluginName) == MObject::kNullObj) {
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
MObject findMayaObject(const MString& objectName)
{
    MSelectionList selList;
    MObject        mayaObj;
    if (selList.add(objectName) == MS::kSuccess
        && selList.getDependNode(0, mayaObj) == MS::kSuccess) {
        return mayaObj;
    }
    return MObject::kNullObj;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
