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

#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsdUI/ui/undoChunkUtils.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoUtils.h>
#include <usdUfe/utils/Utils.h>

#ifdef MAYA_HAS_USD_SETTINGS_NODES
#include <mayaUsd/nodes/sceneRenderDescription.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/weakPtr.h>
#include <pxr/usd/sdf/path.h>

#include <ufe/path.h>
#include <ufe/pathString.h>

#include <AdskUsdRenderSetup/RenderSetupUtils.h>

#include <string>
#include <utility>
#endif

#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MQtUtil.h>
#include <maya/MTime.h>
#include <ufe/pathComponent.h>
#include <ufe/sceneItemOps.h>
#include <ufe/undoableCommandMgr.h>

#include <exception>

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

#ifdef MAYA_HAS_USD_SETTINGS_NODES
MayaRenderSetupHost::MayaRenderSetupHost()
{
    // Registered with no sender, so every stage's changes arrive here.
    PXR_NS::TfWeakPtr<MayaRenderSetupHost> me(this);
    _objectsChangedKey = PXR_NS::TfNotice::Register(me, &MayaRenderSetupHost::onObjectsChanged);
}

MayaRenderSetupHost::~MayaRenderSetupHost() { PXR_NS::TfNotice::Revoke(_objectsChangedKey); }

void MayaRenderSetupHost::onObjectsChanged(const PXR_NS::UsdNotice::ObjectsChanged& notice)
{
    if (!notice.GetResyncedPaths().empty()) {
        _cameraPathsByStage.erase(notice.GetStage());
    }
}
#endif

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
        if (const UsdUfe::UsdSceneItem::Ptr sceneItem = sceneItemFor(prim)) {
            const Ufe::SceneItemOps::Ptr    ops = Ufe::SceneItemOps::sceneItemOps(sceneItem);
            const Ufe::UndoableCommand::Ptr cmd = ops ? ops->deleteItemCmdNoExecute() : nullptr;
            if (!cmd) {
                return false;
            }
            // executeCmd is what puts the command on the undo queue; calling
            // execute() directly would edit the stage with no way back.
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);

            // A restricted delete is refused without throwing, leaving the prim.
            return !prim.IsValid();
        }

        UsdUfe::trackStagesEditTargets({ prim.GetStage() });
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
            const Ufe::SceneItemOps::Ptr ops = Ufe::SceneItemOps::sceneItemOps(sceneItem);
            const Ufe::SceneItemResultUndoableCommand::Ptr cmd
                = ops ? ops->renameItemCmdNoExecute(Ufe::PathComponent(newName)) : nullptr;
            if (!cmd) {
                return {};
            }
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);

            // Maya sanitizes and uniquifies, so the name that landed is
            // routinely not the one asked for. The seam exists to report it.
            const UsdUfe::UsdSceneItem::Ptr renamedItem = UsdUfe::downcast(cmd->sceneItem());
            return renamedItem ? renamedItem->prim().GetPath() : PXR_NS::SdfPath();
        }

        UsdUfe::trackStagesEditTargets({ prim.GetStage() });
        const MayaUsdUI::UndoChunkGuard undoChunkGuard("Rename " + prim.GetName().GetString());
        MayaUsd::MayaUsdUndoBlock       block;
        return Host::renamePrim(prim, newName);
    } catch (const std::exception& ex) {
        MGlobal::displayError(ex.what());
        return {};
    }
}

std::vector<AdskUsdRenderSetup::ExternalCamera>
MayaRenderSetupHost::externalCameras(const PXR_NS::UsdStageRefPtr& editedStage) const
{
    std::vector<AdskUsdRenderSetup::ExternalCamera> cameras;

    for (MItDag dagIt(MItDag::kDepthFirst, MFn::kCamera); !dagIt.isDone(); dagIt.next()) {
        MDagPath shapePath;
        dagIt.getPath(shapePath);
        if (MFnDagNode(shapePath).isIntermediateObject()) {
            continue;
        }

        // Named by transform to match SceneRenderDescription's default ("|persp").
        MDagPath transformPath(shapePath);
        transformPath.pop();
        cameras.push_back(
            { transformPath.fullPathName().asChar(), transformPath.partialPathName().asChar() });
    }

#ifdef MAYA_HAS_USD_SETTINGS_NODES
    // find() must gate getUsdStage(), which creates the node on first call.
    if (!MAYAUSD_NS_DEF::SceneRenderDescription::find().empty()
        && editedStage == MAYAUSD_NS_DEF::SceneRenderDescription::getUsdStage()) {
        // Rebuilt rather than updated in place, so stages that are gone are
        // dropped along the way.
        decltype(_cameraPathsByStage) cameraPathsByStage;

        for (const auto& stage : MayaUsd::ufe::getAllStages()) {
            const Ufe::Path   proxyShapePath = MayaUsd::ufe::stagePath(stage);
            const std::string groupLabel = proxyShapePath.back().string();

            const auto            cached = _cameraPathsByStage.find(stage);
            PXR_NS::SdfPathVector primPaths = cached != _cameraPathsByStage.end()
                ? std::move(cached->second)
                : AdskUsdRenderSetup::RenderSetupUtils::GetAllCameraPaths(stage);

            for (const PXR_NS::SdfPath& primPath : primPaths) {
                cameras.push_back({ Ufe::PathString::string(
                                        proxyShapePath + UsdUfe::usdPathToUfePathSegment(primPath)),
                                    primPath.GetString(),
                                    groupLabel });
            }

            cameraPathsByStage.emplace(stage, std::move(primPaths));
        }

        _cameraPathsByStage = std::move(cameraPathsByStage);
    }
#endif

    return cameras;
}

} // namespace MayaUsdRenderSetup
