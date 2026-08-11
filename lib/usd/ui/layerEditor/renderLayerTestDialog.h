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

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/sdf/notice.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace UsdLayerEditor {

//! Developer-only UI for exercising the render layer API against a proxy shape.
//! Authors scene content directly, so it is not shipped functionality.
class RenderLayerTestDialog
    : public QDialog
    , public PXR_NS::TfWeakBase
{
public:
    explicit RenderLayerTestDialog(QWidget* in_parent = nullptr);
    ~RenderLayerTestDialog() override;

private:
    void onLayerDirtinessChanged(const PXR_NS::SdfNotice::LayerDirtinessChanged& notice);

    //! Coalesces a burst of notices into one refresh on the event loop.
    void refreshLayersOnIdle();

    //! Creates a proxy shape holding a small scene plus a default /Render hierarchy.
    void createSetup();
    void createRenderLayer();
    void refreshLayers();

    //! Activates the double-clicked layer, or deactivates it when it is already active.
    void toggleActiveRenderLayer(QListWidgetItem* in_item);

    //! Builds a list row: an active icon, a tracked icon, then the layer description.
    //! The row is transparent to mouse events so the view still sees the double-click.
    QWidget* createLayerRow(bool isActive, bool isTracked, const QString& description);

    //! Stage of the first proxy shape in the scene, or null after warning. The dialog
    //! discovers it rather than taking a name, so Create Setup is all the setup there is.
    PXR_NS::UsdStageRefPtr stage() const;

    QLabel*      _stageLabel { nullptr };
    QLineEdit*   _nameField { nullptr };
    QListWidget* _layerList { nullptr };

    PXR_NS::TfNotice::Key _dirtinessNoticeKey;
    bool                  _refreshPending { false };
};

} // namespace UsdLayerEditor

#endif // ADSK_ABI >= 2027

#endif // RENDERLAYERTESTDIALOG_H
