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

#include "mayaRenderSetupHost.h"

#include <usdUfe/utils/Utils.h>

#include <maya/MAnimControl.h>
#include <maya/MQtUtil.h>
#include <maya/MTime.h>

#ifdef MAYA_HAS_USD_SETTINGS_NODES
#include <mayaUsd/nodes/sceneRenderDescription.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/pathString.h>

#include <string>
#endif

namespace MayaUsdRenderSetup {

double MayaRenderSetupHost::currentFrame() const
{
    return MAnimControl::currentTime().as(MTime::uiUnit());
}

AdskUsdRenderSetup::FrameRange MayaRenderSetupHost::timelineRange() const
{
    const double start = MAnimControl::minTime().as(MTime::uiUnit());
    const double end = MAnimControl::maxTime().as(MTime::uiUnit());
    return { start, end };
}

int MayaRenderSetupHost::dpiScaled(int logicalPixels) const
{
    return MQtUtil::dpiScale(logicalPixels);
}

std::string MayaRenderSetupHost::prettifyName(const std::string& name) const
{
    return UsdUfe::prettifyName(name);
}

#ifdef MAYA_HAS_USD_SETTINGS_NODES

AdskUsdRenderSetup::RenderDescription MayaRenderSetupHost::activeRenderDescription() const
{
    const std::string storedPath
        = MayaUsd::SceneRenderDescription::getActiveRenderDescriptionPath();
    if (storedPath.empty()) {
        return {};
    }

    Ufe::Path ufePath;
    try {
        ufePath = Ufe::PathString::path(storedPath);
    } catch (const std::exception&) {
        return {};
    }

    const Ufe::Path::Segments& segments = ufePath.getSegments();
    if (segments.size() < 2) {
        return {};
    }

    PXR_NS::UsdStageWeakPtr stage = MayaUsd::ufe::getStage(Ufe::Path(segments[0]));
    if (!stage) {
        return {};
    }

    const std::string primPath = segments[1].string();
    if (!PXR_NS::SdfPath::IsValidPathString(primPath)) {
        return {};
    }

    return { stage, PXR_NS::SdfPath(primPath) };
}

void MayaRenderSetupHost::setActiveRenderDescription(
    const AdskUsdRenderSetup::RenderDescription& description)
{
    if (description.isEmpty()) {
        MayaUsd::SceneRenderDescription::setActiveRenderDescriptionPath({});
        return;
    }

    const Ufe::Path gatewayPath = MayaUsd::ufe::stagePath(description.stage);
    if (gatewayPath.empty()) {
        return;
    }

    const Ufe::Path primPath = gatewayPath + UsdUfe::usdPathToUfePathSegment(description.path);
    MayaUsd::SceneRenderDescription::setActiveRenderDescriptionPath(
        Ufe::PathString::string(primPath));
}

#endif

} // namespace MayaUsdRenderSetup
