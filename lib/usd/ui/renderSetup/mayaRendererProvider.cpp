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

#include "mayaRendererProvider.h"

#include <maya/MCommandResult.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MStringArray.h>

namespace MayaUsdRenderSetup {

namespace {
//! \return the label Maya's own Render Settings dialog shows for `rendererName`.
MString rendererUIName(const MString& rendererName)
{
    MString cmd;
    cmd.format("renderer -query -rendererUIName \"^1s\"", rendererName);
    MString uiName;
    MGlobal::executeCommand(cmd, uiName);
    return uiName;
}
} // namespace

std::vector<AdskUsdRenderSetup::RendererInfo> MayaRendererProvider::availableRenderers() const
{
    MStringArray allRenderers;
    MStatus      status
        = MGlobal::executeCommand("renderer -query -namesOfAvailableRenderers", allRenderers);
    if (!status) {
        MGlobal::displayWarning("Unable to retrieve available renderers.");
        return {};
    }

    std::vector<AdskUsdRenderSetup::RendererInfo> renderers;
    renderers.reserve(allRenderers.length());
    for (const auto& rendererName : allRenderers) {
        AdskUsdRenderSetup::RendererInfo info;
        info.name = rendererName.asChar();
        info.displayName = rendererUIName(rendererName).asChar();
        info.isHydra = isHydraCapable(info.name);
        renderers.push_back(info);
    }
    return renderers;
}

std::string MayaRendererProvider::currentRenderer() const
{
    MString result;
    MGlobal::executeCommand("currentRenderer()", result);
    return result.asChar();
}

bool MayaRendererProvider::isHydraCapable(const std::string& rendererName) const
{
    MString cmd;
    cmd.format("renderer -query -capability \"isHydra\" \"^1s\"", MString(rendererName.c_str()));

    // Renderers that don't report the "isHydra" capability at all return no
    // value (MCommandResult::kInvalid) rather than a boolean false.
    MCommandResult result;
    MStatus        status = MGlobal::executeCommand(cmd, result);
    if (!status) {
        MGlobal::displayWarning(
            MString("\"isHydra\" capability query failed for renderer \"") + rendererName.c_str()
            + "\": " + status.errorString());
        return false;
    }

    switch (result.resultType()) {
    case MCommandResult::kString: {
        MString value;
        result.getResult(value);
        return value == "true" || value == "1";
    }
    default: return false;
    }
}

void MayaRendererProvider::switchRenderer(const std::string& next)
{
    MString cmd;
    cmd.format("setCurrentRenderer(\"^1s\")", MString(next.c_str()));
    MGlobal::executeCommand(cmd);
}

} // namespace MayaUsdRenderSetup
