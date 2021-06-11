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

#include "layerEditorWidget.h"

#include "abstractCommandHook.h"
#include "dirtyLayersCountBadge.h"
#include "generatedIconButton.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "qtUtils.h"
#include "stageSelectorWidget.h"
#include "stringResources.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <maya/MGlobal.h>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QTimer>
#include <QtWidgets/QActionGroup>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <cstddef>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using namespace UsdLayerEditor;

// returns image_100.png" when you pass "image",
// using the DPI setting and also returns always 100 on mac, because Qt doesn't
// properly support high dpi with style sheets
QString getDPIPixmapName(QString baseName)
{
#ifdef Q_OS_DARWIN
    return baseName + "_100.png";
#else
    const auto scale = utils->dpiScale();
    if (scale >= 2.0)
        return baseName + "_200.png";
    else if (scale >= 1.5)
        return baseName + "_150.png";
    return baseName + "_100.png";
#endif
}

// setup a push button with DPI-appropriate regular, hover and pressed png in the
// autodesk human interface guideline style
static void setupButtonWithHIGBitmaps(QPushButton* button, QString baseName)
{
    button->setFlat(true);

    // regular size: 16px, pressed:24px
    // therefore, border is 4
    int     padding = DPIScale(4);
    QString cssTemplate(R"CSS(
    QPushButton {
        padding : %1px;
        background-image: url(%2);
        background-position: center center;
        background-repeat: no-repeat;
        border: 0px;
        background-origin: content;
        }
    QPushButton::hover {
            background-image: url(%3);
        }
    QPushButton::pressed {
        background-image: url(%4);
        border: 0px;
        padding: 0px;
        background-origin: content;
        })CSS");

    QString css = cssTemplate.arg(padding)
                      .arg(getDPIPixmapName(baseName))
                      .arg(getDPIPixmapName(baseName + "_hover"))
                      .arg(getDPIPixmapName(baseName + "_pressed"));

    button->setStyleSheet(css);

    // overkill, but used to generate the grayed out version
    auto effect = new QGraphicsOpacityEffect(button);
    button->setGraphicsEffect(effect);
}

void disableHIGButton(QPushButton* button, bool disable = true)
{
    button->setDisabled(disable);
    auto effect = dynamic_cast<QGraphicsOpacityEffect*>(button->graphicsEffect());
    effect->setOpacity(disable ? 0.4 : 1.0);
}

// create the default menus on the parent QMainWindow
void setupDefaultMenu(SessionState* in_sessionState, QMainWindow* in_parent)
{
    auto menuBar = in_parent->menuBar();
    // don't add menu twice -- this window is destroyed and re-created on new scene
    if (menuBar->actions().length() == 0) {
        auto createMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kCreate));
        auto aboutToShowCallback = [createMenu, in_sessionState]() {
            if (createMenu->actions().length() == 0) {
                in_sessionState->setupCreateMenu(createMenu);
            }
        };
        // we delay populating the create menu to first show, because in the python prototype, if
        // the layer editor was docked, the menu would get populated before the runtime commands had
        // the time to be created
        QObject::connect(createMenu, &QMenu::aboutToShow, in_parent, aboutToShowCallback);

        auto optionMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kOption));
        auto action = optionMenu->addAction(
            StringResources::getAsQString(StringResources::kAutoHideSessionLayer));
        QObject::connect(
            action, &QAction::toggled, in_sessionState, &SessionState::setAutoHideSessionLayer);
        action->setCheckable(true);
        action->setChecked(in_sessionState->autoHideSessionLayer());

        auto usdSaveMenu = optionMenu->addMenu(
            StringResources::getAsQString(StringResources::kUsdSaveFileFormat));

        // Add the save confirm existing file save checkbox
        static const MString kConfirmExistingFileSave
            = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
        auto confirmExistingFileSaveAct = optionMenu->addAction(
            StringResources::getAsQString(StringResources::kConfirmExistFileSave));

        confirmExistingFileSaveAct->setCheckable(true);

        QObject::connect(confirmExistingFileSaveAct, &QAction::toggled, in_parent, [](bool enable) {
            MGlobal::setOptionVarValue(kConfirmExistingFileSave, enable);
        });

        if (MGlobal::optionVarExists(kConfirmExistingFileSave)) {
            confirmExistingFileSaveAct->setChecked(
                MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0);
        } else {
            confirmExistingFileSaveAct->setChecked(true);
        }

        auto formatGroup = new QActionGroup(usdSaveMenu);
        formatGroup->setExclusive(true);
        auto formatBinary
            = formatGroup->addAction(StringResources::getAsQString(StringResources::kBinary));
        auto formatAscii
            = formatGroup->addAction(StringResources::getAsQString(StringResources::kAscii));
        auto grpAnn = StringResources::getAsQString(StringResources::kSaveLayerUsdFileFormatAnn);
        auto grpSbm = StringResources::getAsQString(StringResources::kSaveLayerUsdFileFormatSbm);
        formatBinary->setCheckable(true);
        formatBinary->setData(1);
        formatBinary->setToolTip(grpAnn);
        formatBinary->setStatusTip(grpSbm);
        formatAscii->setCheckable(true);
        formatAscii->setData(0);
        formatAscii->setToolTip(grpAnn);
        formatAscii->setStatusTip(grpSbm);
        usdSaveMenu->addAction(formatBinary);
        usdSaveMenu->addAction(formatAscii);
        bool isBinary = true;

        static const MString kSaveLayerFormatBinaryOption(
            MayaUsdOptionVars->SaveLayerFormatArgBinaryOption.GetText());
        if (MGlobal::optionVarExists(kSaveLayerFormatBinaryOption)) {
            isBinary = MGlobal::optionVarIntValue(kSaveLayerFormatBinaryOption) != 0;
        }
        isBinary ? formatBinary->setChecked(true) : formatAscii->setChecked(true);
        auto onFileFormatChanged = [=](QAction* action) {
            MGlobal::setOptionVarValue(kSaveLayerFormatBinaryOption, action->data().toInt());
        };
        QObject::connect(formatGroup, &QActionGroup::triggered, in_parent, onFileFormatChanged);

        auto helpMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kHelp));
        helpMenu->addAction(
            StringResources::getAsQString(StringResources::kHelpOnUSDLayerEditor),
            [in_sessionState]() { in_sessionState->commandHook()->showLayerEditorHelp(); });
    }
}

} // namespace

///////////////////////////////////////////////////////////////////////////////

namespace UsdLayerEditor {
LayerEditorWidget::LayerEditorWidget(SessionState& in_sessionState, QMainWindow* in_parent)
    : QWidget(in_parent)
    , _sessionState(in_sessionState)
{
    setupLayout();
    ::setupDefaultMenu(&in_sessionState, in_parent);
}

// helper for setupLayout
QLayout* LayerEditorWidget::setupLayout_toolbar()
{
    auto buttonSize = DPIScale(24);
    auto margin = DPIScale(12);
    auto toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(margin, 0, 0, 0);
    auto buttonAlignment = Qt::AlignLeft | Qt::AlignRight;

    auto addButton = [toolbar, buttonAlignment](const char* iconName, const QString& tooltip) {
        auto icon = utils->createIcon(iconName);
        auto button = new GeneratedIconButton(nullptr, icon);
        button->setToolTip(tooltip);
        toolbar->addWidget(button, 0, buttonAlignment);
        return button;
    };

    _buttons._newLayer = addButton(
        ":/RS_create_layer.png", StringResources::getAsQString(StringResources::kAddNewLayer));
    // clicked callback
    connect(
        _buttons._newLayer,
        &QAbstractButton::clicked,
        this,
        &LayerEditorWidget::onNewLayerButtonClicked);
    // update layer button on stage change
    connect(
        _treeView->model(),
        &QAbstractItemModel::modelReset,
        this,
        &LayerEditorWidget::updateNewLayerButton);
    // update layer button if muted state changes
    connect(
        _treeView->model(),
        &QAbstractItemModel::dataChanged,
        this,
        &LayerEditorWidget::updateNewLayerButton);
    // update layer button on selection change
    connect(
        _treeView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &LayerEditorWidget::updateNewLayerButton);

    _buttons._loadLayer = addButton(
        ":/RS_import_layer.png",
        StringResources::getAsQString(StringResources::kLoadExistingLayer));
    // clicked callback
    connect(
        _buttons._loadLayer,
        &QAbstractButton::clicked,
        this,
        &LayerEditorWidget::onLoadLayersButtonClicked);

    toolbar->addStretch();

    { // save stage button: contains a push button and a "badge" widget
        auto saveButtonParent = new QWidget();
        auto saveButtonYOffset = DPIScale(4);
        auto saveButtonSize = QSize(buttonSize + DPIScale(12), buttonSize + saveButtonYOffset);
        saveButtonParent->setFixedSize(saveButtonSize);
        auto saveStageBtn = new QPushButton(saveButtonParent);
        saveStageBtn->move(0, saveButtonYOffset);
        setupButtonWithHIGBitmaps(saveStageBtn, ":/UsdLayerEditor/LE_save_all");
        saveStageBtn->setFixedSize(buttonSize, buttonSize);
        saveStageBtn->setToolTip(
            StringResources::getAsQString(StringResources::kSaveAllEditsInLayerStack));
        connect(
            saveStageBtn,
            &QAbstractButton::clicked,
            this,
            &LayerEditorWidget::onSaveStageButtonClicked);

        auto dirtyCountBadge = new DirtyLayersCountBadge(saveButtonParent);
        dirtyCountBadge->setFixedSize(saveButtonSize);
        toolbar->addWidget(saveButtonParent, 0, buttonAlignment);

        _buttons._saveStageButton = saveStageBtn;
        _buttons._dirtyCountBadge = dirtyCountBadge;

        // update dirty count on stage change
        connect(
            _treeView->model(),
            &QAbstractItemModel::modelReset,
            this,
            &LayerEditorWidget::updateDirtyCountBadgeOnIdle);
        // update dirty count on dirty notfication
        connect(
            _treeView->model(),
            &QAbstractItemModel::dataChanged,
            this,
            &LayerEditorWidget::updateDirtyCountBadgeOnIdle);
    }

    return toolbar;
}

void LayerEditorWidget::setupLayout()
{
    _treeView = new LayerTreeView(&_sessionState, this);

    auto mainVLayout = new QVBoxLayout();
    mainVLayout->setSpacing(DPIScale(4));

    auto stageSelector = new StageSelectorWidget(&_sessionState, this);
    mainVLayout->addWidget(stageSelector);

    auto toolbarLayout = setupLayout_toolbar();
    mainVLayout->addLayout(toolbarLayout);

    mainVLayout->addWidget(_treeView);

    setLayout(mainVLayout);

    updateNewLayerButton();
    updateDirtyCountBadge();
}

void LayerEditorWidget::updateDirtyCountBadgeOnIdle()
{
    if (!_updateDirtyCountOnIdle) {
        _updateDirtyCountOnIdle = true;
        QTimer::singleShot(0, this, &LayerEditorWidget::updateDirtyCountBadge);
    }
}

void LayerEditorWidget::updateNewLayerButton()
{
    bool disabled = _treeView->model()->rowCount() == 0;
    if (!disabled) {
        auto selectionModel = _treeView->selectionModel();
        // enabled if no or one layer selected
        disabled = selectionModel->selectedRows().size() > 1;

        // also disable if the selected layer is muted or invalid
        if (!disabled) {
            auto item = _treeView->currentLayerItem();
            if (item) {
                disabled = item->isInvalidLayer() || item->appearsMuted();
            }
        }
    }
    _buttons._newLayer->setDisabled(disabled);
    _buttons._loadLayer->setDisabled(disabled);
}

void LayerEditorWidget::updateDirtyCountBadge()
{
    const auto layers = _treeView->layerTreeModel()->getAllNeedsSavingLayers();
    int        count = static_cast<int>(layers.size());
    _buttons._dirtyCountBadge->updateCount(count);

    bool disable = count == 0;
    disableHIGButton(_buttons._saveStageButton, disable);
    _updateDirtyCountOnIdle = false;
}

void LayerEditorWidget::onNewLayerButtonClicked()
{
    // if nothing or root selected, add new layer to top of root
    auto model = _treeView->layerTreeModel();
    auto selectionModel = _treeView->selectionModel();
    auto selection = selectionModel->selectedRows();
    bool addToRoot = false;

    LayerTreeItem* layerTreeItem;
    if (selection.size() == 0) {
        addToRoot = true;
        layerTreeItem = model->layerItemFromIndex(model->rootLayerIndex());
    } else {
        layerTreeItem = model->layerItemFromIndex(selection[0]);
        // this test catches both root layer and session layer
        if (layerTreeItem->parent() == nullptr) {
            addToRoot = true;
        }
    }

    if (addToRoot) {
        layerTreeItem->addAnonymousSublayer();
    } else {
        // add a sibling to the selection
        UndoContext context(_sessionState.commandHook(), "Add Anonymous Layer");
        auto        parentItem = layerTreeItem->parentLayerItem();
        int         rowToInsert = layerTreeItem->row();
        auto        newLayer = parentItem->addAnonymousSublayerAndReturn();
        // move it to the right place, if it's not top
        if (rowToInsert > 0) {
            context.hook()->removeSubLayerPath(parentItem->layer(), newLayer->GetIdentifier());
            context.hook()->insertSubLayerPath(
                parentItem->layer(), newLayer->GetIdentifier(), rowToInsert);
            model->selectUsdLayerOnIdle(newLayer);
        }
    }
}

void LayerEditorWidget::onLoadLayersButtonClicked()
{
    auto           model = _treeView->layerTreeModel();
    auto           selectionModel = _treeView->selectionModel();
    auto           selection = selectionModel->selectedRows();
    LayerTreeItem* layerTreeItem;
    if (selection.size() == 0) {
        layerTreeItem = model->layerItemFromIndex(model->rootLayerIndex());
    } else {
        layerTreeItem = model->layerItemFromIndex(selection[0]);
    }
    layerTreeItem->loadSubLayers(this);
}

void LayerEditorWidget::onSaveStageButtonClicked() { _treeView->layerTreeModel()->saveStage(this); }

} // namespace UsdLayerEditor
