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

#ifndef RENDERLAYERTESTDIALOG_H
#define RENDERLAYERTESTDIALOG_H

#if defined(ADSK_ABI) && ADSK_ABI >= 2027

#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace UsdLayerEditor {

//! Developer-only UI for exercising the render layer API against a proxy shape.
//! Authors scene content directly, so it is not shipped functionality.
class RenderLayerTestDialog : public QDialog
{
public:
    explicit RenderLayerTestDialog(QWidget* in_parent = nullptr);

private:
    //! Creates a proxy shape holding a small scene plus a default /Render hierarchy.
    void createSetup();
    void createRenderLayer();
    void refreshLayers();

    //! Activates the double-clicked layer, or deactivates it when it is already active.
    void toggleActiveRenderLayer(QListWidgetItem* in_item);

    //! Stage of the first proxy shape in the scene, or null after warning. The dialog
    //! discovers it rather than taking a name, so Create Setup is all the setup there is.
    PXR_NS::UsdStageRefPtr stage() const;

    QLabel*      _stageLabel { nullptr };
    QLineEdit*   _nameField { nullptr };
    QListWidget* _layerList { nullptr };
};

} // namespace UsdLayerEditor

#endif // ADSK_ABI >= 2027

#endif // RENDERLAYERTESTDIALOG_H
