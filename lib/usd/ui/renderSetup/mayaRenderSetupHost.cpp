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

#include <mayaUsd/ufe/MayaUsdUndoRenameCommand.h>
#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsdUI/ui/undoChunkUtils.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoDeleteCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/Utils.h>

#include <maya/MAnimControl.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MTime.h>
#include <ufe/pathComponent.h>
#include <ufe/undoableCommandMgr.h>

#include <exception>

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

namespace {

// The Render Setup window also lists the Maya Settings stage owned by
// UsdSceneSettingsManager, which has no proxy shape and therefore no UFE path.
// Returning null is how the callers below decide to take the non-UFE route.
UsdUfe::UsdSceneItem::Ptr sceneItemFor(const PXR_NS::UsdPrim& prim)
{
    const Ufe::Path stageUfePath = UsdUfe::stagePath(prim.GetStage());
    if (stageUfePath.empty()) {
        return nullptr;
    }
    return UsdUfe::UsdSceneItem::create(
        stageUfePath + UsdUfe::usdPathToUfePathSegment(prim.GetPath()), prim);
}

} // namespace

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

bool MayaRenderSetupHost::deletePrim(const PXR_NS::UsdPrim& prim)
{
    if (!prim.IsValid()) {
        return false;
    }

    try {
        if (sceneItemFor(prim)) {
            // executeCmd is what puts the command on the undo queue; calling
            // execute() directly would edit the stage with no way back.
            Ufe::UndoableCommandMgr::instance().executeCmd(
                UsdUfe::UsdUndoDeleteCommand::create(prim));
            return true;
        }

        const MayaUsdUI::UndoChunkGuard undoChunkGuard("Delete " + prim.GetName().GetString());
        MayaUsd::MayaUsdUndoBlock       block;
        return Host::deletePrim(prim);
    } catch (const std::exception& ex) {
        // The command throws on a locked layer or a disallowed edit, and the
        // message names the reason, which is the useful half for the user.
        MGlobal::displayError(ex.what());
        return false;
    }
}

PXR_NS::SdfPath
MayaRenderSetupHost::renamePrim(const PXR_NS::UsdPrim& prim, const std::string& newName)
{
    if (!prim.IsValid() || newName.empty()) {
        return {};
    }

    try {
        if (const UsdUfe::UsdSceneItem::Ptr sceneItem = sceneItemFor(prim)) {
            const auto cmd = MayaUsd::ufe::MayaUsdUndoRenameCommand::create(
                sceneItem, Ufe::PathComponent(newName));
            if (!cmd) {
                return {};
            }
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);

            // Maya sanitizes and uniquifies, so the name that landed is
            // routinely not the one asked for. The seam exists to report it.
            const UsdUfe::UsdSceneItem::Ptr renamedItem = cmd->renamedItem();
            return renamedItem ? renamedItem->prim().GetPath() : PXR_NS::SdfPath();
        }

        const MayaUsdUI::UndoChunkGuard undoChunkGuard("Rename " + prim.GetName().GetString());
        MayaUsd::MayaUsdUndoBlock       block;
        return Host::renamePrim(prim, newName);
    } catch (const std::exception& ex) {
        MGlobal::displayError(ex.what());
        return {};
    }
}

} // namespace MayaUsdRenderSetup
