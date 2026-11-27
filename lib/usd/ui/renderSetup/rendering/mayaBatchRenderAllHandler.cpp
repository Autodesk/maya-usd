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

#include "mayaBatchRenderAllHandler.h"

#include "mayaBatchRender.h"
#include "mayaBatchRenderQueue.h"
#include "mayaBatchRenderResult.h"

#include <mayaUsdUI/ui/stringResources.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <AdskUsdRenderSetup/Host.h>
#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>

#include <filesystem>
#include <utility>

namespace MayaUsdRenderSetup {

std::string MayaBatchRenderAllHandler::displayName() const
{
    return GET_MAYAUSDUI_STR(kBatchRenderAll);
}

std::string MayaBatchRenderAllHandler::toolTip() const
{
    return GET_MAYAUSDUI_STR(kBatchRenderAllToolTip);
}

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaBatchRenderAllHandler::render() const
{
    MayaBatchRenderQueue& queue = MayaBatchRenderQueue::instance();
    if (queue.isRunning()) {
        if (!askToAbortCurrentRender()) {
            return {};
        }
        queue.cancel();
    }

    const AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance();
    const PXR_NS::UsdStageRefPtr    stage = host ? host->activeRenderDescription().stage : nullptr;
    if (!saveForBatchRender(stage)) {
        return {};
    }

    // Saving can give an anonymous render layer a file, so this must follow the save.
    const std::string    sceneCopy = getSceneCopyPath();
    std::vector<MString> commands = buildBatchRenderCommands(stage, sceneCopy);
    if (commands.empty()) {
        MGlobal::displayWarning(GET_MAYAUSDUI_MSTR(kBatchRenderNothingRenderable));
        return {};
    }

    // Each batchRender would otherwise write its own copy of the scene, and ask again about
    // saving the USD edits.
    if (!sceneCopy.empty() && !exportSceneCopy(sceneCopy)) {
        return {};
    }

    queue.start(std::move(commands), sceneCopy);
    return std::make_shared<MayaBatchRenderResult>();
}

/* static */
bool MayaBatchRenderAllHandler::askToAbortCurrentRender()
{
    QMessageBox dialog(
        QMessageBox::Warning,
        GET_MAYAUSDUI_QSTR(kBatchRenderAllRunningTitle),
        GET_MAYAUSDUI_QSTR(kBatchRenderAllRunning),
        QMessageBox::NoButton,
        MQtUtil::mainWindow());
    QPushButton* startNewButton
        = dialog.addButton(GET_MAYAUSDUI_QSTR(kBatchRenderAllStartNew), QMessageBox::AcceptRole);
    QPushButton* keepButton
        = dialog.addButton(GET_MAYAUSDUI_QSTR(kBatchRenderAllKeepRunning), QMessageBox::RejectRole);
    dialog.setDefaultButton(keepButton);
    dialog.setEscapeButton(keepButton);
    const int iconSize = dialog.style()->pixelMetric(QStyle::PM_MessageBoxIconSize) * 3 / 4;
    dialog.setIconPixmap(
        dialog.style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(iconSize));
    // QMessageBox sizes itself to its content and ignores resize(), so the message label sets
    // the size. The icon label gets the same height to stay centered beside the message, as
    // QMessageBox aligns it to the top.
    dialog.setStyleSheet(
        QString("QLabel#qt_msgbox_label { min-width: %1px; min-height: %2px; }"
                "QLabel#qt_msgboxex_icon_label { min-height: %2px; qproperty-alignment: "
                "AlignCenter; }")
            .arg(MQtUtil::dpiScale(380))
            .arg(MQtUtil::dpiScale(60)));
    dialog.exec();
    return dialog.clickedButton() == startNewButton;
}

/* static */
std::string MayaBatchRenderAllHandler::getSceneCopyPath()
{
    const std::filesystem::path scene(MFileIO::currentFile().asChar());
    std::string                 extension = scene.extension().string();
    if (extension.empty()) {
        extension = MFileIO::fileType() == "mayaBinary" ? ".mb" : ".ma";
    }
    int processId = 0;
    MGlobal::executeCommand("getpid", processId);

    const std::string path
        = (scene.parent_path()
           / (scene.stem().string() + "__" + std::to_string(processId) + extension))
              .generic_string();
    return canPassInMelString(path) ? path : std::string();
}

/* static */
bool MayaBatchRenderAllHandler::exportSceneCopy(const std::string& path)
{
    // Maya's own export callbacks check this flag to keep Render Setup nodes, AOVs and the
    // visible render layer in the file, as batchRender sets it for its scene copy. MEL putenv
    // also updates Python's os.environ, which those callbacks read.
    MGlobal::executeCommand("putenv \"MAYA_BATCH_RENDER_EXPORT\" \"1\"");
    // MFileIO::exportAll has no force option, and a copy left by a canceled render or a crashed
    // session would otherwise make Maya ask to overwrite it.
    const MStatus status = MGlobal::executeCommand(
        "file -force -exportAll -preserveReferences -type \"" + MFileIO::fileType() + "\" \""
        + MString(path.c_str()) + "\"");
    MGlobal::executeCommand("putenv \"MAYA_BATCH_RENDER_EXPORT\" \"\"");
    return status;
}

/* static */
std::vector<MString> MayaBatchRenderAllHandler::buildBatchRenderCommands(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            sceneFile)
{
    std::vector<MString> commands;
    if (!stage) {
        return commands;
    }

    for (const auto& layer :
         AdskUsdRenderSetup::RenderLayerManager::instance().getIncludedRenderLayers(stage)) {
        const std::string layerPath = sessionLayerPath(layer);
        const std::string activeOptions = activeRenderDescriptionOptions(stage, layer);
        MString           skipReason;
        if (layerPath.empty()) {
            skipReason = GET_MAYAUSDUI_MSTR(kBatchRenderLayerNotSaved);
        } else if (!canPassInMelString(layerPath)) {
            skipReason = GET_MAYAUSDUI_MSTR(kBatchRenderLayerPathHasQuote);
        } else if (activeOptions.empty()) {
            // Without its own pass, the layer would render whatever description the scene saved.
            skipReason = GET_MAYAUSDUI_MSTR(kBatchRenderLayerNoPass);
        }
        if (skipReason.length() > 0) {
            MString warning;
            warning.format(skipReason, MString(layer.name.c_str()));
            MGlobal::displayWarning(warning);
            continue;
        }
        // MayaHydra requires -activeRenderDescription before the other render settings flags.
        commands.push_back(batchRenderCommand(
            activeOptions + " " + sessionLayerOptions(stage, layerPath), sceneFile));
    }
    return commands;
}

} // namespace MayaUsdRenderSetup
