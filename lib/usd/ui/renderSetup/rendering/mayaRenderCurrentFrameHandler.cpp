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

#include "mayaRenderCurrentFrameHandler.h"

#include "mayaRenderCurrentFrameResult.h"

#include <maya/MGlobal.h>

// For current renderer
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <sstream>

namespace {

MString currentRenderer()
{
    MSelectionList list;
    if (list.add("defaultRenderGlobals") != MS::kSuccess)
        return {};

    MObject obj;
    if (list.getDependNode(0, obj) != MS::kSuccess)
        return {};

    MFnDependencyNode renderGlobals(obj);

    MStatus status;
    MPlug   plug = renderGlobals.findPlug("currentRenderer", true, &status);
    if (!status)
        return {};

    MString renderer;
    if (plug.getValue(renderer) != MS::kSuccess)
        return {};

    return renderer;
}

} // namespace

namespace MayaUsdRenderSetup {

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaRenderCurrentFrameHandler::render() const
{
    // We assume that if the USD render setup window is being shown, that the
    // renderer must be Hydra-based, and therefore that the hydraRender command
    // is appropriate.
    auto rendererName = currentRenderer();

    if (rendererName.isEmpty()) {
        return std::make_shared<MayaRenderCurrentFrameResult>("Renderer could not be found.");
    }

    // renderWindowPanel.mel has very rich functionality to render the current
    // frame and invoke the render view (potentially renderer-specific).  At
    // time of writing we cannot match this functionality and simply call for a
    // single-frame render on the current frame.
    //
    // Render current frame would be simpler if we could call hydraRender
    // without a renderer name (command would read it).  PPT, 10-Aug-2026.
    MGlobal::displayInfo(MString("MayaRenderCurrentFrameHandler::render() called."));
    std::ostringstream cmdStr;
    cmdStr << "hydraRender -r " << rendererName.asChar() << " -cf";
    MGlobal::executeCommand(MString(cmdStr.str().c_str()));
    cmdStr << " called.";
    MGlobal::displayInfo(MString(cmdStr.str().c_str()));
    return std::make_shared<MayaRenderCurrentFrameResult>();
}

} // namespace MayaUsdRenderSetup
