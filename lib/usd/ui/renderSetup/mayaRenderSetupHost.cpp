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

#include <maya/MAnimControl.h>
#include <maya/MQtUtil.h>
#include <maya/MTime.h>

#include <usdUfe/utils/Utils.h>

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

} // namespace MayaUsdRenderSetup
