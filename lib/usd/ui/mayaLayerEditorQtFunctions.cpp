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
#include "mayaLayerEditorQtFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <maya/MQtUtil.h>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <stringResources.h>

#include <mayaUsdUI/ui/editForwardDialog.h>

#include <pxr/usd/usd/stage.h>

#include <QtCore/QPointer>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
namespace {
QPointer<UsdEditForwardConfig::EditForwardDialog> g_editForwardDialog;
} // namespace
#endif

namespace UsdLayerEditor {

void registerLayerEditorQtFunctions()
{
    // Read-modify-write so the Qt-free registrations from
    // registerLayerEditorDCCFunctions() are preserved.
    auto environment = layerEditorDCCFunctions().environment;
    environment.mainWindowParent = []() -> QWidget* { return MQtUtil::mainWindow(); };
    setEnvironmentFns(environment);

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    auto editForwarding = layerEditorDCCFunctions().editForwarding;
    editForwarding.openEditForwardDialog = [](const UsdStageRefPtr& stage) {
        // Reuse a single dialog instance; QPointer auto-nulls if it is destroyed.
        if (!g_editForwardDialog) {
            g_editForwardDialog = new UsdEditForwardConfig::EditForwardDialog(
                StringResources::getAsQString(StringResources::kConfigureEditForwardingTitle),
                MQtUtil::mainWindow());
        }
        g_editForwardDialog->setActiveStage(stage);
        g_editForwardDialog->show();
        g_editForwardDialog->raise();
        g_editForwardDialog->activateWindow();
    };
    editForwarding.isEditForwardDialogOpen
        = []() { return g_editForwardDialog && g_editForwardDialog->isVisible(); };
    setEditForwardingFns(editForwarding);
#endif
}

} // namespace UsdLayerEditor
