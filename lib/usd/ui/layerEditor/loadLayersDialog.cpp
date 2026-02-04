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

#include "loadLayersDialog.h"

#include "generatedIconButton.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "pathChecker.h"
#include "qtUtils.h"
#include "stringResources.h"

#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <maya/MQtUtil.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>

#include <cassert>

namespace {
using namespace UsdLayerEditor;

class MyLineEdit : public QLineEdit
{
public:
    MyLineEdit(QWidget* in_parent)
        : QLineEdit(in_parent)
    {
    }
    QSize sizeHint() const override
    {
        auto hint = QLineEdit::sizeHint();
        hint.setWidth(DPIScale(300));
        return hint;
    }
};

} // namespace

namespace UsdLayerEditor {

LayerPathRow::LayerPathRow(LoadLayersDialog* in_parent)
    : QWidget(in_parent)
    , _parent(in_parent)
{
    auto gridLayout = new QGridLayout();
    QtUtils::initLayoutMargins(gridLayout);
    _parentPath = _parent->findDirectoryToUse("");

    _label = new QLabel(StringResources::getAsQString(StringResources::kLayerPath));
    gridLayout->addWidget(_label, 0, 0);

    _pathEdit = new MyLineEdit(this);
    gridLayout->addWidget(_pathEdit, 0, 1);

    QIcon icon;

    icon = utils->createIcon(":/fileOpen.png");
    _openBrowser = new GeneratedIconButton(this, icon);
    gridLayout->addWidget(_openBrowser, 0, 2);
    connect(_openBrowser, &QAbstractButton::clicked, in_parent, &LoadLayersDialog::onOpenBrowser);

    icon = utils->createIcon(":/trash.png");
    _trashIcon = new GeneratedIconButton(this, icon);
    connect(_trashIcon, &QAbstractButton::clicked, in_parent, &LoadLayersDialog::onDeleteRow);
    _trashIcon->setToolTip(StringResources::getAsQString(StringResources::kRemoveSublayer));
    gridLayout->addWidget(_trashIcon, 0, 3);

    icon = utils->createIcon(":/UsdLayerEditor/addCreateGeneric.png");
    _addPathIcon = new GeneratedIconButton(this, icon);
    _addPathIcon->setVisible(false);
    _addPathIcon->setToolTip(StringResources::getAsQString(StringResources::kAddSublayer));
    connect(_addPathIcon, &QAbstractButton::clicked, in_parent, &LoadLayersDialog::onAddRow);
    gridLayout->addWidget(_addPathIcon, 0, 3);

    setLayout(gridLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
}

std::string LayerPathRow::pathToUse() const { return _pathEdit->text().toStdString(); }

void LayerPathRow::setPathToUse(const std::string& path)
{
    _pathEdit->setText(path.c_str());
    _pathEdit->setEnabled(true);
}

void LayerPathRow::setAsRowInserter(bool setIt)
{
    bool enabled = !setIt;
    _label->setEnabled(enabled);
    _pathEdit->setEnabled(enabled);
    _openBrowser->setEnabled(enabled);

    _trashIcon->setVisible(!setIt);
    _addPathIcon->setVisible(setIt);
}

LoadLayersDialog::LoadLayersDialog(LayerTreeItem* in_treeItem, QWidget* in_parent)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _treeItem(in_treeItem)
{
    MString title;
    title.format(
        StringResources::getAsMString(StringResources::kLoadSublayersTo),
        MQtUtil::toMString(_treeItem->text()));
    setWindowTitle(MQtUtil::toQString(title));
    auto rowsLayout = new QVBoxLayout();
    int  margin = DPIScale(5) + DPIScale(20);
    rowsLayout->setContentsMargins(margin, margin, margin, 0);
    rowsLayout->setSpacing(DPIScale(8));

    auto firstRow = new LayerPathRow(this);
    rowsLayout->addWidget(firstRow);
    auto inserter = new LayerPathRow(this);
    inserter->setAsRowInserter(true);
    rowsLayout->addWidget(inserter);

    _rowsLayout = rowsLayout;

    // Ok/Cancel button area
    auto buttonsLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(buttonsLayout, DPIScale(20));
    buttonsLayout->addStretch();
    auto okButton
        = new QPushButton(StringResources::getAsQString(StringResources::kLoadSublayers), this);
    connect(okButton, &QPushButton::clicked, this, &LoadLayersDialog::onOk);
    okButton->setDefault(true);
    auto cancelButton
        = new QPushButton(StringResources::getAsQString(StringResources::kCancel), this);
    connect(cancelButton, &QPushButton::clicked, this, &LoadLayersDialog::onCancel);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    auto mainVLayout = new QVBoxLayout();
    QtUtils::initLayoutMargins(mainVLayout);
    mainVLayout->setAlignment(Qt::AlignTop);
    mainVLayout->addLayout(rowsLayout);

    // setup a scroll area
    auto dialogContentParent = new QWidget();
    dialogContentParent->setLayout(mainVLayout);

    auto scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setWidget(dialogContentParent);
    scrollArea->setWidgetResizable(true);
    _scrollArea = scrollArea;

    // add that scroll area as our single child
    auto topLayout = new QVBoxLayout();
    QtUtils::initLayoutMargins(topLayout);
    topLayout->setSpacing(0);
    topLayout->addWidget(scrollArea);

    // then add the buttons
    auto buttonArea = new QWidget(this);
    buttonArea->setLayout(buttonsLayout);
    buttonArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    topLayout->addWidget(buttonArea);

    setLayout(topLayout);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}

void LoadLayersDialog::onOk()
{

    for (int i = 0, count = _rowsLayout->count(); i < count; ++i) {
        auto row = dynamic_cast<LayerPathRow*>(_rowsLayout->itemAt(i)->widget());
        if (!row->pathToUse().empty()) {
            if (!checkIfPathIsSafeToAdd(
                    this,
                    StringResources::getAsQString(StringResources::kLoadSublayersError),
                    _treeItem,
                    row->pathToUse())) {
                return; // abort the OK
            }
            _pathToLoad.push_back(row->pathToUse());
        }
    }
    accept();
}

void LoadLayersDialog::onCancel() { reject(); }

LayerPathRow* LoadLayersDialog::getLastRow() const
{
    return dynamic_cast<LayerPathRow*>(_rowsLayout->itemAt(_rowsLayout->count() - 1)->widget());
}

void LoadLayersDialog::appendInserterRow()
{
    auto inserter = new LayerPathRow(this);
    inserter->setAsRowInserter(true);
    _rowsLayout->addWidget(inserter);
    adjustScrollArea();
    scrollToEnd();
}

void LoadLayersDialog::scrollToEnd()
{
    QTimer::singleShot(0, this, [this]() { _scrollArea->ensureWidgetVisible(getLastRow()); });
}

void LoadLayersDialog::adjustScrollArea()
{
    auto lastRow = getLastRow();
    auto rowCount = _rowsLayout->count();
    auto maxRows = 8;
    if (rowCount > maxRows)
        rowCount = maxRows;

    auto height = lastRow->sizeHint().height();
    auto spacing = _rowsLayout->spacing();
    height = height * rowCount + (rowCount - 1) * spacing;

    auto margins = _rowsLayout->contentsMargins();
    height = height + margins.top() + margins.bottom();

    _scrollArea->setMinimumHeight(height);
}

void LoadLayersDialog::onAddRow()
{
    auto lastRow = getLastRow();
    lastRow->setAsRowInserter(false);
    appendInserterRow();
}

void LoadLayersDialog::onDeleteRow()
{
    auto row = dynamic_cast<LayerPathRow*>(sender()->parent());
    _rowsLayout->removeWidget(row);
    row->deleteLater();
    adjustScrollArea();
}

std::string LoadLayersDialog::findDirectoryToUse(const std::string& rowText) const
{
    auto path = rowText;
    if (path.empty()) {
        auto item = _treeItem;
        while (item != nullptr) {
            if (!item->isAnonymous()) {
                path = item->layer()->GetRealPath();
                break;
            }
            item = item->parentLayerItem();
        }
    }

    if (path.empty()) {
        path = UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar();
    }

    if (!path.empty()) {
        QFileInfo fileInfo(QString::fromStdString(path));
        path = fileInfo.path().toStdString();
    }
    return path;
}

void LoadLayersDialog::onOpenBrowser()
{
    using namespace PXR_NS::UsdMayaUtilFileSystem;

    const bool useSceneFileForRoot = false;
    const auto parentLayer = _treeItem->layer();
    prepareLayerSaveUILayer(parentLayer, useSceneFileForRoot);

    LayerPathRow*     row = dynamic_cast<LayerPathRow*>(sender()->parent());
    const std::string defaultPath = findDirectoryToUse(row->pathToUse());

    auto files = _treeItem->parentModel()->sessionState()->loadLayersUI(windowTitle(), defaultPath);
    if (files.size() == 0)
        return;

    // Replace selected filenames with relative ones if enabled.
    if (requireUsdPathsRelativeToParentLayer() && parentLayer && !parentLayer->IsAnonymous()) {
        for (std::string& fileName : files) {
            fileName = getPathRelativeToLayerFile(fileName, parentLayer);
        }
    }

    row->setPathToUse(files[0]);

    // insert new rows if more than one file selected
    if (files.size() > 1) {
        // find where to insert new rows
        int index = 0;
        int rowCount = _rowsLayout->count();
        for (int i = 0; i < rowCount; i++) {
            auto thisRow = dynamic_cast<LayerPathRow*>(_rowsLayout->itemAt(i)->widget());
            if (thisRow == row) {
                index = i + 1;
                break;
            }
        }
        assert(index != 0);

        // note: starts at 1
        for (size_t i = 1; i < files.size(); i++) {
            auto newRow = new LayerPathRow(this);
            _rowsLayout->insertWidget(index, newRow);
            newRow->setPathToUse(files[i]);
            ++index;
        }

        adjustScrollArea();
        scrollToEnd();
    }
}

} // namespace UsdLayerEditor
