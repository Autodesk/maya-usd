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

#include "editForwardDialog.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MMessage.h>
#include <maya/MQtUtil.h>
#include <maya/MSceneMessage.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/selection.h>
#include <ufe/selectionNotification.h>

#include <AdskUsdEditForwardUi/ForwardWidget.h>
#include <AdskUsdEditForwardUi/StageEntry.h>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtWidgets/QVBoxLayout>

#include <vector>

namespace UsdEditForwardConfig {

namespace {

std::vector<AdskUsdEditForwardUi::StageEntry> findAllStages()
{
    std::vector<AdskUsdEditForwardUi::StageEntry> result;
    for (const auto& dagPath : MayaUsd::ufe::ProxyShapeHandler::getAllNames()) {
        PXR_NS::UsdStageWeakPtr stage = MayaUsd::ufe::ProxyShapeHandler::dagPathToStage(dagPath);
        if (!stage)
            continue;
        AdskUsdEditForwardUi::StageEntry out;
        out.name = dagPath.substr(dagPath.rfind('|') + 1); // npos+1==0, so safe when no '|'
        out.stage = stage;
        result.push_back(std::move(out));
    }
    // Sort alphabetically by name, matching the layer editor's allStages() ordering.
    std::sort(
        result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    return result;
}

struct SelectionObserver : Ufe::Observer
{
    QPointer<EditForwardDialog> dialog;
    explicit SelectionObserver(EditForwardDialog* d)
        : dialog(d)
    {
    }
    void operator()(const Ufe::Notification& n) override
    {
        if (dialog && dynamic_cast<const Ufe::SelectionChanged*>(&n))
            dialog->handleSelectionChanged();
    }
};

} // namespace

EditForwardDialog::EditForwardDialog(const QString& title, QWidget* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(title);
    resize(
        static_cast<int>(MQtUtil::dpiScale(1250.0f)), static_cast<int>(MQtUtil::dpiScale(1000.0f)));

    _forwardWidget = new AdskUsdEditForwardUi::ForwardWidget(this);
    _forwardWidget->setSourceLayerDefault(
        AdskUsdEditForwardUi::ForwardWidget::SourceLayerDefault::SessionLayer);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_forwardWidget);

    refreshStages();

    // Observe proxy shape node additions and removals directly.
    PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver().addTypeListener(*this);

    // Refresh after scene open or new scene.
    _sceneCallbackIds.push_back(
        MSceneMessage::addCallback(MSceneMessage::kAfterOpen, onSceneChangedCB, this));
    _sceneCallbackIds.push_back(
        MSceneMessage::addCallback(MSceneMessage::kAfterNew, onSceneChangedCB, this));

    // Register as UFE observer to listen for selection changes.
    _selectionObserver = std::make_shared<SelectionObserver>(this);
    const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();
    if (ufeSelection) {
        ufeSelection->addObserver(_selectionObserver);
    }
}

EditForwardDialog::~EditForwardDialog()
{
    PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver().removeTypeListener(*this);

    for (auto id : _sceneCallbackIds) {
        MMessage::removeCallback(id);
    }

    const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();
    if (ufeSelection && _selectionObserver) {
        ufeSelection->removeObserver(_selectionObserver);
    }
}

void EditForwardDialog::processNodeAdded(MObject& /*node*/)
{
    QTimer::singleShot(0, this, &EditForwardDialog::refreshStages);
}

void EditForwardDialog::processNodeRemoved(MObject& /*node*/)
{
    QTimer::singleShot(0, this, &EditForwardDialog::refreshStages);
}

void EditForwardDialog::onSceneChangedCB(void* clientData)
{
    auto* self = static_cast<EditForwardDialog*>(clientData);
    QTimer::singleShot(0, self, &EditForwardDialog::refreshStages);
}

void EditForwardDialog::handleSelectionChanged()
{
    const Ufe::GlobalSelection::Ptr& ufeGlobalSelection = Ufe::GlobalSelection::get();
    if (!ufeGlobalSelection)
        return;

    for (const auto& item : *ufeGlobalSelection) {
        auto* proxy = MayaUsd::ufe::getProxyShapeFromItemOrChildren(item);
        if (!proxy)
            continue;

        MFnDagNode dagNode(proxy->thisMObject());
        MDagPath   dagPath;
        dagNode.getPath(dagPath);
        PXR_NS::UsdStageWeakPtr stage
            = MayaUsd::ufe::ProxyShapeHandler::dagPathToStage(dagPath.fullPathName().asChar());
        if (stage) {
            setActiveStage(stage);
            return;
        }
    }
}

void EditForwardDialog::refreshStages()
{
    _forwardWidget->setStages(findAllStages());
    handleSelectionChanged();
}

void EditForwardDialog::setActiveStage(PXR_NS::UsdStageRefPtr const& stage)
{
    _forwardWidget->setActiveStage(stage);
}

} // namespace UsdEditForwardConfig
