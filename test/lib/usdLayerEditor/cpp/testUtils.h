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
#pragma once

#include "layerContentsWidget.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

#include <initializer_list>

namespace UsdLayerEditor {
namespace TestUtils {

// Find a QPushButton by case-insensitive substring of its tooltip.
inline QPushButton* findButtonByTooltip(QWidget* root, const QString& tooltip)
{
    for (auto* btn : root->findChildren<QPushButton*>()) {
        if (btn->toolTip().contains(tooltip, Qt::CaseInsensitive))
            return btn;
    }
    return nullptr;
}

// Find a QPushButton by its Qt object name.
inline QPushButton* findButtonByObjectName(QWidget* root, const QString& name)
{
    return root->findChild<QPushButton*>(name);
}

// Find a QPushButton whose visible text contains any of the given substrings
// (case-insensitive). Useful for dialog buttons identified by label.
inline QPushButton* findButtonByText(QWidget* root, std::initializer_list<QString> texts)
{
    for (auto* btn : root->findChildren<QPushButton*>()) {
        for (const auto& text : texts) {
            if (btn->text().contains(text, Qt::CaseInsensitive))
                return btn;
        }
    }
    return nullptr;
}

inline QPushButton* findButtonByText(QWidget* root, const QString& text)
{
    return findButtonByText(root, { text });
}

// Locate the LayerContentsWidget inside a LayerEditorWidget hierarchy.
inline LayerContentsWidget* findContentsWidget(QWidget* root)
{
    return root->findChild<LayerContentsWidget*>(QString(), Qt::FindChildrenRecursively);
}

// Stage with one anonymous sublayer already inserted at index 0.
inline PXR_NS::UsdStageRefPtr makeStageWithSublayer(const std::string& sublayerName = "sub")
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto sub   = PXR_NS::SdfLayer::CreateAnonymous(sublayerName);
    stage->GetRootLayer()->InsertSubLayerPath(sub->GetIdentifier(), 0);
    return stage;
}

// Mark the root layer dirty by setting a comment.
inline void makeDirty(const PXR_NS::UsdStageRefPtr& stage)
{
    stage->GetRootLayer()->SetComment("dirty");
}

// Lock a layer by revoking edit permission directly (no DCC attr update).
inline void lockLayerDirect(const PXR_NS::SdfLayerRefPtr& layer)
{
    layer->SetPermissionToEdit(false);
}

// Unlock a layer by restoring edit permission.
inline void unlockLayerDirect(const PXR_NS::SdfLayerRefPtr& layer)
{
    layer->SetPermissionToEdit(true);
}

// Schedule closing any active modal dialog after `ms` milliseconds.
inline void dismissNextModal(int ms = 200)
{
    QTimer::singleShot(ms, []() {
        QWidget* modal = QApplication::activeModalWidget();
        if (modal)
            modal->close();
    });
}

// Find a named action in a menu (searches recursively into submenus).
inline QAction* findAction(QMenu* menu, const QString& text)
{
    if (!menu)
        return nullptr;
    for (QAction* action : menu->actions()) {
        if (action->text() == text)
            return action;
        if (action->menu()) {
            QAction* found = findAction(action->menu(), text);
            if (found)
                return found;
        }
    }
    return nullptr;
}

} // namespace TestUtils
} // namespace UsdLayerEditor
