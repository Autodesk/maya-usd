#pragma once

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

#include "layerTreeItem.h"
#include "layerTreeView.h"
#include "stringResources.h"

#include <QtWidgets/QMenu>

#include <functional>

namespace UsdLayerEditor {

namespace Detail {

// The session layer's sublayers count as session layers too.
inline bool isInSessionLayerTree(const LayerTreeItem* item)
{
    for (; item != nullptr; item = item->parentLayerItem()) {
        if (item->isSessionLayer())
            return true;
    }
    return false;
}

} // namespace Detail

/**
 * @brief Fills the layer editor context menu for the current selection.
 *
 * @tparam Window any layer editor window exposing the layer queries and actions.
 * @param addDCCItems appends DCC-specific items after "Select Prims With Spec".
 */
template <class Window>
void populateLayerContextMenu(
    QMenu&                               menu,
    Window&                              window,
    LayerTreeView&                       treeView,
    const std::function<void(QMenu&)>&   addDCCItems = {})
{
    auto addAction = [&menu](const QString& label, bool enabled, std::function<void()> callback) {
        QAction* action = menu.addAction(label);
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [callback]() { callback(); });
    };

    // Without a current layer there is nothing to act on, so the menu stays empty.
    if (treeView.currentLayerItem() == nullptr)
        return;

    if (window.isInvalidLayer()) {
        // Nothing else is supported on invalid layers.
        addAction(GET_LE_QSTR(kMenuRemove), true, [&window]() { window.removeSubLayer(); });
        return;
    }

    const bool isSession = window.isSessionLayer();
    const bool isAnonymous = window.isAnonymousLayer();
    const bool needsSaving = window.layerNeedsSaving();
    const bool appearsMuted = window.layerAppearsMuted();
    const bool isSublayer = window.isSubLayer();
    const bool isMuted = window.layerIsMuted();
    const bool isReadOnly = window.layerIsReadOnly();
    const bool appearsLocked = window.layerAppearsLocked();
    const bool isLocked = window.layerIsLocked();
    const bool appearsSystemLocked = window.layerAppearsSystemLocked();
    const bool isSystemLocked = window.layerIsSystemLocked();
    const bool hasSublayers = window.layerHasSubLayers();
    const bool isActiveRenderLayer = window.layerIsActiveRenderLayer();
    // Merging into or clearing the session layer would interfere with an active render layer.
    const bool isSessionWithRenderLayer = isSession && window.stageHasActiveRenderLayer();

    const int  selectedCount = window.selectionLength();
    const bool singleSelect = selectedCount == 1;
    const bool multiSelect = selectedCount > 1;

    if (!isSession) {
        // An anonymous layer whose parent is locked or system locked cannot be saved.
        bool enabled = singleSelect && needsSaving && !isSystemLocked;
        if (isAnonymous)
            enabled = enabled && !appearsLocked && !appearsSystemLocked;
        addAction(
            isAnonymous ? GET_LE_QSTR(kMenuSaveAs) : GET_LE_QSTR(kMenuSaveEdits),
            enabled,
            [&window]() { window.saveEdits(); });
    }

    if (!isAnonymous)
        addAction(GET_LE_QSTR(kMenuReload), true, [&window]() { window.discardEdits(); });

    menu.addSeparator();

    // The active render layer is not a place to add layers to or around.
    if (!isActiveRenderLayer) {
        addAction(
            GET_LE_QSTR(kMenuAddSublayer),
            !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked,
            [&window]() { window.addAnonymousSublayer(); });

        addAction(
            GET_LE_QSTR(kMenuAddParentLayer),
            isSublayer && !appearsMuted && !isReadOnly && !appearsLocked && !appearsSystemLocked,
            [&window]() { window.addParentLayer(); });

        addAction(
            GET_LE_QSTR(kMenuLoadSublayers),
            singleSelect && !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked,
            [&window]() { window.loadSubLayers(); });
    }

    if (multiSelect) {
        bool hasSession = false;
        bool hasNonSession = false;
        for (const LayerTreeItem* item : treeView.getSelectedLayerItems()) {
            if (Detail::isInSessionLayerTree(item))
                hasSession = true;
            else
                hasNonSession = true;
        }
        if (!(hasSession && hasNonSession)) {
            addAction(
                GET_LE_QSTR(kMenuStitchLayers),
                !isReadOnly && !isLocked && !appearsSystemLocked && !appearsMuted,
                [&window]() { window.stitchLayers(); });
        }
    }

    if (hasSublayers) {
        addAction(
            GET_LE_QSTR(kMenuMergeWithSublayers),
            !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked
                && !isSessionWithRenderLayer,
            [&window]() { window.mergeWithSublayers(); });
    }

    menu.addSeparator();

    if (isSublayer) {
        addAction(
            isMuted ? GET_LE_QSTR(kMenuUnmute) : GET_LE_QSTR(kMenuMute),
            true,
            [&window]() { window.muteLayer(); });
    }

    if (!isSession) {
        addAction(
            isLocked ? GET_LE_QSTR(kMenuUnlock) : GET_LE_QSTR(kMenuLock),
            !isSystemLocked,
            [&window]() { window.lockLayer(); });
    }

    if (!isSession && hasSublayers) {
        addAction(
            isLocked ? GET_LE_QSTR(kMenuUnlockLayerAndSublayers)
                     : GET_LE_QSTR(kMenuLockLayerAndSublayers),
            !isSystemLocked,
            [&window]() { window.lockLayerAndSubLayers(); });
    }

    addAction(GET_LE_QSTR(kMenuPrintToScriptEditor), true, [&window]() { window.printLayer(); });

    menu.addSeparator();

    addAction(
        GET_LE_QSTR(kMenuSelectPrimsWithSpec), true, [&window]() { window.selectPrimsWithSpec(); });

    if (addDCCItems)
        addDCCItems(menu);

    if (isSublayer || !isAnonymous)
        menu.addSeparator();

    if (isSublayer) {
        addAction(
            GET_LE_QSTR(kMenuRemove),
            !isReadOnly && !appearsLocked && !appearsSystemLocked,
            [&window]() { window.removeSubLayer(); });
    }

    addAction(
        GET_LE_QSTR(kMenuClear),
        !isReadOnly && !isLocked && !isSystemLocked && !isSessionWithRenderLayer,
        [&window]() { window.clearLayer(); });
}

} // namespace UsdLayerEditor
