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
#ifndef STAGESELECTORWIDGET_H
#define STAGESELECTORWIDGET_H

#include "sessionState.h"

#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QtWidgets>

namespace UsdLayerEditor {

/**
 * @brief Drop down list that allows selecting a stage. Owned by the LayerEditorWidget
 *
 */
class StageSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    StageSelectorWidget(SessionState* in_sessionState, QWidget* in_parent);

protected:
    void                           setSessionState(SessionState* in_sessionState);
    SessionState::StageEntry const selectedStage();

    // slot:
    void updateFromSessionState(
        SessionState::StageEntry const& entryToSelect = SessionState::StageEntry());
    void stageRenamed(SessionState::StageEntry const& renamedEntry);
    void stageReset(SessionState::StageEntry const& entry);
    void sessionStageChanged();
    void selectedIndexChanged(int index);

private:
    SessionState* _sessionState = nullptr;
    QComboBox*    _dropDown = nullptr;
    bool          _internalChange = false; // for notifications
};

} // namespace UsdLayerEditor

#endif // STAGESELECTORWIDGET_H
