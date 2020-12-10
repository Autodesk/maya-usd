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

#include <pxr/usd/usd/common.h>

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

Q_DECLARE_METATYPE(pxr::UsdStageRefPtr);

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
    updateFromSessionState();
    connect(
        _sessionState,
        &SessionState::currentStageChangedSignal,
        this,
        &StageSelectorWidget::sessionStageChanged);
    connect(
        _sessionState, &SessionState::stageRenamedSignal, this, &StageSelectorWidget::stageRenamed);
    updateFromSessionState();
}

pxr::UsdStageRefPtr StageSelectorWidget::selectedStage()
{
    if (_dropDown->currentIndex() != -1) {
        auto const& data = _dropDown->currentData();
        return data.value<pxr::UsdStageRefPtr>();
    }

    return pxr::UsdStageRefPtr();
}
// repopulates the combo based on the session stage list
void StageSelectorWidget::updateFromSessionState(pxr::UsdStageRefPtr const& stageToSelect)
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

void StageSelectorWidget::stageRenamed(std::string const& name, pxr::UsdStageRefPtr const& stage)
{
    auto index = _dropDown->findData(QVariant::fromValue(stage));
    if (index != -1) {
        _dropDown->setItemText(index, name.c_str());
    }
}
} // namespace UsdLayerEditor
