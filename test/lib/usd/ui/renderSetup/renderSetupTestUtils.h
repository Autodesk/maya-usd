//
// Copyright 2026 Autodesk
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

#ifndef MAYAUSDUI_TEST_RENDERSETUP_TESTUTILS_H
#define MAYAUSDUI_TEST_RENDERSETUP_TESTUTILS_H

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <ufe/pathString.h>

#include <string>

namespace RenderSetupTest {

inline PXR_NS::UsdStageRefPtr createProxyShapeStage(const std::string& name = {})
{
    MString shapeName;
    MGlobal::executeCommand(
        name.empty() ? MString("createNode mayaUsdProxyShape")
                     : MString("createNode mayaUsdProxyShape -name ") + name.c_str(),
        shapeName);
    MGlobal::executeCommand("connectAttr time1.outTime " + shapeName + ".time");

    MStringArray longNames;
    MGlobal::executeCommand("ls -long " + shapeName, longNames);
    if (longNames.length() != 1) {
        return {};
    }
    const PXR_NS::UsdStageRefPtr stage
        = MayaUsd::ufe::getStage(Ufe::PathString::path(longNames[0].asChar()));

    // Adding a proxy shape rebuilds the UFE stage map, and that rebuild can
    // miss the proxy shapes created before it. A lookup by path adds them back.
    MStringArray allShapes;
    MGlobal::executeCommand("ls -long -type mayaUsdProxyShape", allShapes);
    for (const MString& shape : allShapes) {
        MayaUsd::ufe::getStage(Ufe::PathString::path(shape.asChar()));
    }
    return stage;
}

inline std::string undoName()
{
    MString name;
    MGlobal::executeCommand("undoInfo -q -undoName", name);
    return name.asChar();
}

// The undo queue is off while a command runs with undoEnabled false, the default.
inline void undo() { MGlobal::executeCommand("undo", false, true); }
inline void redo() { MGlobal::executeCommand("redo", false, true); }
inline void flushUndo() { MGlobal::executeCommand("flushUndo"); }

//! Starts from a new scene with an unlimited undo queue.
inline void resetScene()
{
    MGlobal::executeCommand("file -force -new");
    MGlobal::executeCommand("undoInfo -state on -infinity on", false, true);
    flushUndo();
}

} // namespace RenderSetupTest

#endif // MAYAUSDUI_TEST_RENDERSETUP_TESTUTILS_H
