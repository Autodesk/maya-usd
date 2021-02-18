//
// Copyright 2020 Autodesk
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
#include "stageSelectorWidget.h"

#include "qtUtils.h"
#include "stringResources.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

PXR_NAMESPACE_USING_DIRECTIVE

Q_DECLARE_METATYPE(UsdStageRefPtr);

namespace UsdLayerEditor {

StageSelectorWidget::StageSelectorWidget(SessionState* in_sessionState, QWidget* in_parent)
    : QWidget(in_parent)
{
    auto mainHLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(mainHLayout);
    mainHLayout->setSpacing(DPIScale(4));
    mainHLayout->addWidget(QtUtils::fixedWidget(
        new QLabel(StringResources::getAsQString(StringResources::kUsdStage))));

    _dropDown = new QComboBox();
    mainHLayout->addWidget(_dropDown);
    connect(
        _dropDown,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this,
        &StageSelectorWidget::selectedIndexChanged);

    setLayout(mainHLayout);

    setSessionState(in_sessionState);
}

void StageSelectorWidget::setSessionState(SessionState* in_sessionState)
{
    _sessionState = in_sessionState;
    connect(
        _sessionState,
        &SessionState::stageListChangedSignal,
        this,
        &StageSelectorWidget::updateFromSessionState);
    connect(
        _sessionState,
        &SessionState::currentStageChangedSignal,
        this,
        &StageSelectorWidget::sessionStageChanged);
    connect(
        _sessionState, &SessionState::stageRenamedSignal, this, &StageSelectorWidget::stageRenamed);
    connect(_sessionState, &SessionState::stageResetSignal, this, &StageSelectorWidget::stageReset);

    updateFromSessionState();
}

UsdStageRefPtr StageSelectorWidget::selectedStage()
{
    if (_dropDown->currentIndex() != -1) {
        auto const& data = _dropDown->currentData();
        return data.value<UsdStageRefPtr>();
    }

    return UsdStageRefPtr();
}
// repopulates the combo based on the session stage list
void StageSelectorWidget::updateFromSessionState(UsdStageRefPtr const& stageToSelect)
{
    QSignalBlocker blocker(_dropDown);
    _dropDown->clear();
    auto allStages = _sessionState->allStages();
    for (auto const& stage : allStages) {
        _dropDown->addItem(QString(stage._displayName.c_str()), QVariant::fromValue(stage._stage));
    }
    if (!stageToSelect) {
        const auto& newStage = selectedStage();
        if (newStage != _sessionState->stage()) {
            _sessionState->setStage(newStage);
        }
    } else {
        _sessionState->setStage(stageToSelect);
    }
}

// called when combo value is changed by user
void StageSelectorWidget::selectedIndexChanged(int index)
{
    _internalChange = true;
    _sessionState->setStage(selectedStage());
    _internalChange = false;
}

// handles when someone else changes the current stage
// but also called when when do it ourselves
void StageSelectorWidget::sessionStageChanged()
{
    if (!_internalChange) {
        auto index = _dropDown->findData(QVariant::fromValue(_sessionState->stage()));
        if (index != -1) {
            QSignalBlocker blocker(_dropDown);
            _dropDown->setCurrentIndex(index);
        }
    }
}

void StageSelectorWidget::stageRenamed(std::string const& name, UsdStageRefPtr const& stage)
{
    auto index = _dropDown->findData(QVariant::fromValue(stage));
    if (index != -1) {
        _dropDown->setItemText(index, name.c_str());
    }
}

void StageSelectorWidget::stageReset(const std::string& proxyPath, UsdStageRefPtr const& stage)
{
    // Individual combo box entries have a short display name and a reference to a stage,
    // which is not a unique combination.  By construction the combo box indices do line
    // up with the SessionState StageEntry vector though, so in the case of resetting
    // a proxy we will find the matching full proxy path in that vector and use its index
    // to update the combo box.
    auto count = _dropDown->count();
    if (count <= 0) {
        return;
    }

    std::vector<SessionState::StageEntry> allStages = _sessionState->allStages();
    auto                                  it = std::find_if(
        allStages.begin(), allStages.end(), [proxyPath](SessionState::StageEntry entry) {
            return (proxyPath == entry._proxyShapePath);
        });

    if (it != allStages.end()) {
        auto index = (it - allStages.begin());
        if (index < count) {
            if (_dropDown->itemText(index) == QString::fromStdString((*it)._displayName)) {
                _dropDown->setItemData(index, QVariant::fromValue(stage));
            }
        }
    }
}

} // namespace UsdLayerEditor
