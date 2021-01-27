

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

#include "mayaLayerEditorWindow.h"

#include "layerEditorWidget.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "mayaSessionState.h"
#include "qtUtils.h"
#include "sessionState.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/utils/query.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QtCore/QPointer>

#include <map>
#include <vector>

using namespace MAYAUSD_NS_DEF;

namespace {
using namespace UsdLayerEditor;

class LayerEditorWindowCreator : public AbstractLayerEditorCreator
{
public:
    LayerEditorWindowCreator() { ; };
    virtual ~LayerEditorWindowCreator() { }

    AbstractLayerEditorWindow* createWindow(const char* panelName) override;
    AbstractLayerEditorWindow* getWindow(const char* panelName) const override;
    std::vector<std::string>   getAllPanelNames() const override;

private:
    // it's very important that this be a QPointer, so that it gets
    // automatically nulled if the window gets closed
    typedef std::map<std::string, QPointer<MayaLayerEditorWindow>> EditorsMap;

    static EditorsMap _editors;
} g_layerEditorWindowCreator;

LayerEditorWindowCreator::EditorsMap LayerEditorWindowCreator::_editors;

AbstractLayerEditorWindow* LayerEditorWindowCreator::createWindow(const char* panelName)
{
    auto _workspaceControl = MQtUtil::getCurrentParent();
    auto editorWindow = new MayaLayerEditorWindow(panelName, nullptr);
    _editors[panelName] = editorWindow;

    // Add UI as a child of the workspace control
    MQtUtil::addWidgetToMayaLayout(editorWindow, _workspaceControl);
    return editorWindow;
}

AbstractLayerEditorWindow* LayerEditorWindowCreator::getWindow(const char* panelName) const
{
    auto window = _editors.find(panelName);
    return (window == _editors.end()) ? nullptr : window->second;
}

AbstractLayerEditorCreator::PanelNamesList LayerEditorWindowCreator::getAllPanelNames() const
{
    PanelNamesList result;
    for (auto entry : _editors) {
        result.push_back(entry.first);
    }
    return result;
}

class MayaQtUtils : public QtUtils
{

    double dpiScale() override { return MQtUtil::dpiScale(1.0f); }
    QIcon  createIcon(const char* iconName) override
    {
        QIcon* icon = MQtUtil::createIcon(iconName);
        QIcon  copy;
        if (icon) {
            copy = QIcon(*icon);
        }
        delete icon;
        return copy;
    }

    QPixmap createPixmap(QString const& pixmapName, int width, int height) override
    {
        QPixmap* pixmap = MQtUtil::createPixmap(pixmapName.toStdString().c_str());
        if (pixmap != nullptr) {
            QPixmap copy(*pixmap);
            delete pixmap;
            return copy;
        }
        return QPixmap();
    }
} g_mayaQtUtils;

} // namespace

namespace UsdLayerEditor {

MayaLayerEditorWindow::MayaLayerEditorWindow(const char* panelName, QWidget* parent)
    : PARENT_CLASS(parent)
    , AbstractLayerEditorWindow(panelName)
    , _panelName(panelName)
{
    if (!UsdLayerEditor::utils) {
        UsdLayerEditor::utils = &g_mayaQtUtils;
    }

    onCreateUI();

    connect(
        &_sessionState,
        &MayaSessionState::clearUIOnSceneResetSignal,
        this,
        &MayaLayerEditorWindow::onClearUIOnSceneReset);
}

MayaLayerEditorWindow::~MayaLayerEditorWindow() { _sessionState.unregisterNotifications(); }

void MayaLayerEditorWindow::onClearUIOnSceneReset()
{
    // I'm not sure if I need this, but in earlier prototypes it was
    // safer to delete the entire UI and re-recreate it on scene changes
    // to release all the proxies
    LayerTreeModel::suspendUsdNotices(true);
    _sessionState.unregisterNotifications();
    setCentralWidget(nullptr);
    delete _layerEditor;

    QTimer::singleShot(2000, this, &MayaLayerEditorWindow::onCreateUI);
}

void MayaLayerEditorWindow::onCreateUI()
{
    LayerTreeModel::suspendUsdNotices(false);
    _layerEditor = new LayerEditorWidget(_sessionState, this);
    setCentralWidget(_layerEditor);
    _layerEditor->show();
    _sessionState.registerNotifications();

    connect(
        treeView(),
        &QWidget::customContextMenuRequested,
        this,
        &MayaLayerEditorWindow::onShowContextMenu);
}

LayerTreeView* MayaLayerEditorWindow::treeView() { return _layerEditor->layerTree(); }

int MayaLayerEditorWindow::selectionLength()
{
    auto selection = treeView()->getSelectedLayerItems();
    return static_cast<int>(selection.size());
}

#define CALL_CURRENT_ITEM(method)               \
    auto item = treeView()->currentLayerItem(); \
    return (item == nullptr) ? false : item->method()

bool MayaLayerEditorWindow::isInvalidLayer() { CALL_CURRENT_ITEM(isInvalidLayer); }
bool MayaLayerEditorWindow::isSessionLayer() { CALL_CURRENT_ITEM(isSessionLayer); }
bool MayaLayerEditorWindow::isLayerDirty() { CALL_CURRENT_ITEM(isDirty); }
bool MayaLayerEditorWindow::isSubLayer() { CALL_CURRENT_ITEM(isSublayer); }
bool MayaLayerEditorWindow::isAnonymousLayer() { CALL_CURRENT_ITEM(isAnonymous); }
bool MayaLayerEditorWindow::layerNeedsSaving() { CALL_CURRENT_ITEM(needsSaving); }
bool MayaLayerEditorWindow::layerAppearsMuted() { CALL_CURRENT_ITEM(appearsMuted); }
bool MayaLayerEditorWindow::layerIsMuted() { CALL_CURRENT_ITEM(isMuted); }

void MayaLayerEditorWindow::removeSubLayer()
{
    QString name = "Remove";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::removeSubLayer);
}

void MayaLayerEditorWindow::saveEdits()
{
    auto item = treeView()->currentLayerItem();
    if (item) {
        bool shouldSaveEdits = true;

        // the layer is already saved on disk.
        // ask the user a confirmation before overwrite it.
        if (!item->isAnonymous()) {
            const MString titleFormat
                = StringResources::getAsMString(StringResources::kSaveLayerWarnTitle);
            const MString msgFormat
                = StringResources::getAsMString(StringResources::kSaveLayerWarnMsg);

            MString title;
            title.format(titleFormat, item->displayName().c_str());

            MString msg;
            msg.format(msgFormat, item->layer()->GetRealPath().c_str());

            const QString okButtonText = StringResources::getAsQString(StringResources::kSave);

            shouldSaveEdits = confirmDialog(
                MQtUtil::toQString(title),
                MQtUtil::toQString(msg),
                nullptr /*bulletList*/,
                &okButtonText);
        }

        if (shouldSaveEdits) {
            QString name = item->isAnonymous() ? "Save As..." : "Save Edits";
            treeView()->callMethodOnSelection(name, &LayerTreeItem::saveEdits);
        }
    }
}

void MayaLayerEditorWindow::discardEdits()
{
    QString name = "Discard Edits";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::discardEdits);
}

void MayaLayerEditorWindow::addAnonymousSublayer()
{
    QString name = "Add Sublayer";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::addAnonymousSublayer);
}

void MayaLayerEditorWindow::addParentLayer()
{
    QString name = "Add Parent Layer";
    treeView()->onAddParentLayer(name);
}

void MayaLayerEditorWindow::loadSubLayers()
{
    auto item = treeView()->currentLayerItem();
    if (item) {
        item->loadSubLayers(this);
    }
}

void MayaLayerEditorWindow::muteLayer()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        QString name = item->isMuted() ? "Unmute" : "Mute";
        treeView()->onMuteLayer(name);
    }
}

void MayaLayerEditorWindow::printLayer()
{
    QString name = "Print to Script Editor";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::printLayer);
}

void MayaLayerEditorWindow::clearLayer()
{
    QString name = "Clear";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::clearLayer);
}

void MayaLayerEditorWindow::selectPrimsWithSpec()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        _sessionState.commandHook()->selectPrimsWithSpec(item->layer());
    }
}

void MayaLayerEditorWindow::selectProxyShape(const char* shapePath)
{
    auto prim = UsdMayaQuery::GetPrim(shapePath);
    if (prim) {
        auto stage = prim.GetStage();
        if (stage != nullptr) {
            _sessionState.setStage(stage);
        }
    }
}

void MayaLayerEditorWindow::onShowContextMenu(const QPoint& pos)
{
    QMenu contextMenu;

    MString menuName = "UsdLayerEditorContextMenu";
    contextMenu.setObjectName(menuName.asChar());
    contextMenu.setSeparatorsCollapsible(false); // elimitates a maya gltich with divider

    MString setParent;
    setParent.format("setParent -menu ^1s;", menuName);

    MString command;
    command.format("mayaUsdMenu_layerEditorContextMenu(\"^1s\");", _panelName.c_str());

    MGlobal::executeCommand(
        setParent + command,
        /*display*/ false,
        /*undo*/ false);

    contextMenu.exec(treeView()->mapToGlobal(pos));
}

} // namespace UsdLayerEditor
