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
#ifndef EDITFORWARDDIALOG_H
#define EDITFORWARDDIALOG_H

#include <mayaUsd/utils/mayaNodeTypeObserver.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MMessage.h>
#include <ufe/observer.h>

#include <QtWidgets/QDialog>

#include <memory>
#include <vector>

namespace AdskUsdEditForwardUi {
class ForwardWidget;
}

namespace UsdEditForwardConfig {

class EditForwardDialog
    : public QDialog
    , private MayaUsd::MayaNodeTypeObserver::Listener
{
    Q_OBJECT
public:
    explicit EditForwardDialog(const QString& title, QWidget* parent);
    ~EditForwardDialog() override;

    void handleSelectionChanged();
    void setActiveStage(PXR_NS::UsdStageRefPtr const& stage);

private:
    void refreshStages();

    void        processNodeAdded(MObject& node) override;
    void        processNodeRemoved(MObject& node) override;
    static void onSceneChangedCB(void* clientData);

    AdskUsdEditForwardUi::ForwardWidget* _forwardWidget { nullptr };
    std::shared_ptr<Ufe::Observer>       _selectionObserver;
    std::vector<MCallbackId>             _sceneCallbackIds;
};

} // namespace UsdEditForwardConfig

#endif // EDITFORWARDDIALOG_H
