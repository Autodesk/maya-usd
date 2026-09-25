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

#include "mayaBatchRenderHandler.h"

#include "../mayaRenderSetupHost.h"
#include "mayaBatchRenderResult.h"

#include <maya/MGlobal.h>

namespace {
bool isStageDirty(pxr::UsdStageRefPtr stage)
{
    if (!stage)
        return false;

    for (auto layer : stage->GetUsedLayers()) {
        if (layer->IsDirty()) {
            return true;
        }
    }

    return false;
}
} // namespace

namespace MayaUsdRenderSetup {

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaBatchRenderHandler::render() const
{
    if (AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance()) {
        AdskUsdRenderSetup::RenderDescription desc = host->activeRenderDescription();
        if (isStageDirty(desc.stage)) {
            int     saveDialogResult = 0;
            MStatus status
                = MGlobal::executeCommand("saveChanges(\"file -save\")", saveDialogResult);
            if (!status || !saveDialogResult) {
                MGlobal::displayInfo(MString("MayaBatchRenderHandler::render() canceled by user."));
                return {};
            }
        }
    }

    MGlobal::displayInfo(MString("MayaBatchRenderHandler::render() called."));
    MGlobal::executeCommand(MString("mayaBatchRender"));
    return std::make_shared<MayaBatchRenderResult>();
}

} // namespace MayaUsdRenderSetup
