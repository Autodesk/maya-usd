//
// Copyright 2023 Autodesk
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

#include "saveLayersDialog.h"

#include "componentSaveWidget.h"
#include "generatedIconButton.h"
#include "layerEditorDCCFunctions.h"
#include "layerLocking.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "pathChecker.h"
#include "sessionState.h"
#include "stringResources.h"
#include "utilFileSystem.h"
#include "utilQT.h"
#include "utilSerialization.h"
#include "utilString.h"
#include "warningDialogs.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/layer.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGridLayout>
#include <ghc/fs_std.hpp>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

template <typename T> void moveAppendVector(std::vector<T>& src, std::vector<T>& dst)
{
    if (dst.empty()) {
        dst = std::move(src);
    } else {
        dst.reserve(dst.size() + src.size());
        std::move(std::begin(src), std::end(src), std::back_inserter(dst));
        src.clear();
    }
}

using namespace UsdLayerEditor;

void getDialogMessages(
    const int nbStages,
    const int nbAnonLayers,
    QString&  msg1,
    QString&  msg2,
    QString&  msg3,
    bool      isExporting)
{
    std::string strNbStages = std::to_string(nbStages);
    std::string strNbAnonLayers = std::to_string(nbAnonLayers);

    auto msgRes = isExporting ? StringResources::kToExportTheStageSaveAnonym
                              : StringResources::kToSaveTheStageSaveAnonym;
    auto msg = String::format(msgRes.value, strNbStages, strNbAnonLayers);
    msg1 = QString::fromStdString(msg);

    msgRes = isExporting ? StringResources::kToExportTheStageSaveFiles
                         : StringResources::kToSaveTheStageSaveFiles;
    msg = String::format(msgRes.value, strNbStages);
    msg2 = QString::fromStdString(msg);

    msgRes = isExporting ? StringResources::kToExportTheStageSaveComponents
                         : StringResources::kToSaveTheStageSaveComponents;
    msg = String::format(msgRes.value, strNbStages);
    msg3 = QString::fromStdString(msg);
}

class AnonLayerPathEdit : public QLineEdit
{
public:
    AnonLayerPathEdit(QWidget* in_parent)
        : QLineEdit(in_parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }

    QSize sizeHint() const override
    {
        auto hint = QLineEdit::sizeHint();
        if (!text().isEmpty()) {
            QFontMetrics appFont = QApplication::fontMetrics();
            int          pathWidth = appFont.boundingRect(text()).width();
            hint.setWidth(pathWidth + DPIScale(100));
        }
        return hint;
    }
};

} // namespace

namespace UsdLayerEditor {
class SaveLayersDialog;
}

class SaveLayerPathRow : public QWidget
{
public:
    using LayerInfo = UsdLayerEditor::Serialization::LayerInfo;
    using LayerInfos = UsdLayerEditor::Serialization::LayerInfos;

    SaveLayerPathRow(
        SaveLayersDialog* in_parent,
        QGridLayout*      gridLayout,
        int               gridRow,
        const LayerInfo&  in_layerInfo);

    QString layerDisplayName() const;

    QString getAbsolutePath() const;

    void setSaveAsRelative(bool relative);
    bool needToSaveAsRelative() const;

    void setPathToSaveAs(const std::string& absolutePath, bool saveAsRelative);

    std::string calculateParentLayerDir() const;

protected:
    void onOpenBrowser();
    void onTextChanged(const QString& text);
    void onRelativeChanged();
    void postUpdate();

public:
    fs::filesystem::path _absolutePath;
    fs::filesystem::path _relativeAnchor;

    SaveLayersDialog* _parent { nullptr };
    LayerInfo         _layerInfo;
    QLabel*           _label { nullptr };
    QLineEdit*        _pathEdit { nullptr };
    QAbstractButton*  _openBrowser { nullptr };
    QCheckBox*        _relative { nullptr };
    bool              _suppressUserInputCallbacks { false };
};

SaveLayerPathRow::SaveLayerPathRow(
    SaveLayersDialog* in_parent,
    QGridLayout*      gridLayout,
    int               gridRow,
    const LayerInfo&  in_layerInfo)
    : QWidget(in_parent)
    , _parent(in_parent)
    , _layerInfo(in_layerInfo)
{

    // Since this is an anonymous layer, it should only be associated with a single stage.
    std::string stageName;
    const auto& stageLayers = in_parent->stageLayers();
    if (TF_VERIFY(1 == stageLayers.count(_layerInfo.layer))) {
        auto search = stageLayers.find(_layerInfo.layer);
        stageName = search->second;
    }

    QString displayName = _layerInfo.layer->GetDisplayName().c_str();
    _label = new QLabel(displayName);
    _label->setToolTip(in_parent->buildTooltipForLayer(_layerInfo.layer));
    gridLayout->addWidget(_label, gridRow, 0);

    _pathEdit = new AnonLayerPathEdit(this);
    connect(_pathEdit, &QLineEdit::textChanged, this, &SaveLayerPathRow::onTextChanged);
    gridLayout->addWidget(_pathEdit, gridRow, 1);

    auto  utils = getQtUtils();
    QIcon icon = utils->createIcon(":/UsdLayerEditor/LE_fileOpen.png");
    _openBrowser = new GeneratedIconButton(this, icon);
    gridLayout->addWidget(_openBrowser, gridRow, 2);
    connect(_openBrowser, &QAbstractButton::clicked, this, &SaveLayerPathRow::onOpenBrowser);

    QString     checkBoxTitle = _layerInfo.parent._layerParent
            ? StringResources::getAsQString(StringResources::kBatchSaveRelativeToParent)
            : StringResources::getAsQString(StringResources::kBatchSaveRelativeToScene);
    std::string checkBoxTooltip;
    if (_layerInfo.parent._layerParent) {
        checkBoxTooltip = String::format(
            StringResources::kBatchSaveRelativeToLayerTooltip.value,
            _layerInfo.parent._layerParent->GetDisplayName().c_str());
    } else {
        checkBoxTooltip = StringResources::kBatchSaveRelativeToSceneTooltip.value;
    }

    _relative = new QCheckBox(checkBoxTitle, this);
    _relative->setToolTip(QString::fromStdString(checkBoxTooltip));
    connect(_relative, &QCheckBox::stateChanged, this, &SaveLayerPathRow::onRelativeChanged);
    gridLayout->addWidget(_relative, gridRow, 3);

    QString pathToSaveAs
        = Serialization::generateUniqueLayerFileName(stageName, _layerInfo.layer).c_str();
    _pathEdit->setText(QFileInfo(pathToSaveAs).absoluteFilePath());

    // Set default state of checkbox and proper setting of path (must come after the initial
    // setting above).
    bool shouldCheck = _layerInfo.parent._layerParent
        ? FileSystem::requireUsdPathsRelativeToParentLayer()
        : FileSystem::requireUsdPathsRelativeToDCCSceneFile();
    _relative->setChecked(shouldCheck);
    onRelativeChanged();
}

QString SaveLayerPathRow::layerDisplayName() const { return _label->text(); }

QString SaveLayerPathRow::getAbsolutePath() const
{
    return QString(_absolutePath.generic_string().c_str());
}

bool SaveLayerPathRow::needToSaveAsRelative() const
{
    return _relative->checkState() == Qt::Checked;
}

void SaveLayerPathRow::setSaveAsRelative(bool relative)
{
    _relative->setCheckState(relative ? Qt::Checked : Qt::Unchecked);
}

void SaveLayerPathRow::setPathToSaveAs(const std::string& absolutePath, bool saveAsRelative)
{
    // Calculate relative anchor
    std::string relativeAnchor;
    if (_layerInfo.parent._layerParent) {
        if (saveAsRelative) {
            relativeAnchor = calculateParentLayerDir();
        }
    } else {
        if (saveAsRelative) {
            relativeAnchor = FileSystem::getDCCSceneFileDir();
            if (relativeAnchor.empty()) {
                relativeAnchor
                    = fs::filesystem::path(absolutePath).remove_filename().generic_string();
            }
        }
    }

    _absolutePath = absolutePath;
    _relativeAnchor = relativeAnchor;

    std::string displayPath = absolutePath;
    if (!relativeAnchor.empty()) {
        displayPath = FileSystem::makePathRelativeTo(
                          _absolutePath.generic_string(), _relativeAnchor.generic_string())
                          .first;
    }

    _suppressUserInputCallbacks = true;
    _pathEdit->setText(displayPath.c_str());
    _pathEdit->setEnabled(true);
    setSaveAsRelative(saveAsRelative);
    _suppressUserInputCallbacks = false;

    if (!saveAsRelative) {
        // Quietly uncheck AllAsRelative checkbox
        _parent->quietlyUncheckAllAsRelative();
    }

    postUpdate();
}

std::string SaveLayerPathRow::calculateParentLayerDir() const
{
    auto& parentLayer = _layerInfo.parent._layerParent;
    if (parentLayer) {
        if (parentLayer->IsAnonymous()) {
            auto parentLayerWidget = _parent->findEntry(parentLayer);
            if (auto parentLayerEntry = dynamic_cast<SaveLayerPathRow*>(parentLayerWidget)) {
                return FileSystem::getDir(parentLayerEntry->getAbsolutePath().toStdString());
            }
        } else {
            return FileSystem::getLayerFileDir(parentLayer);
        }
    }

    return std::string();
}

void SaveLayerPathRow::onOpenBrowser()
{
    auto&       parentLayer = _layerInfo.parent._layerParent;
    std::string parentLayerPath = calculateParentLayerDir();

    const bool isParent = (parentLayer != nullptr);
    const bool prev = isParent ? FileSystem::requireUsdPathsRelativeToParentLayer()
                               : FileSystem::requireUsdPathsRelativeToDCCSceneFile();
    if (isParent)
        FileSystem::setRequireUsdPathsRelativeToParentLayer(needToSaveAsRelative());
    else
        FileSystem::setRequireUsdPathsRelativeToDCCSceneFile(needToSaveAsRelative());

    std::string absolutePath;
    if (SaveLayersDialog::saveLayerFilePathUI(absolutePath, parentLayerPath)) {
        const bool saveAsRelative = isParent ? FileSystem::requireUsdPathsRelativeToParentLayer()
                                             : FileSystem::requireUsdPathsRelativeToDCCSceneFile();
        setPathToSaveAs(absolutePath, saveAsRelative);
    }

    if (isParent)
        FileSystem::setRequireUsdPathsRelativeToParentLayer(prev);
    else
        FileSystem::setRequireUsdPathsRelativeToDCCSceneFile(prev);
}

void SaveLayerPathRow::onTextChanged(const QString& text)
{
    if (_suppressUserInputCallbacks) {
        return;
    }

    fs::filesystem::path inputPath(text.toStdString());
    if (inputPath.is_absolute()) {
        _relativeAnchor.clear();
        _absolutePath = inputPath;
    } else if (!_relativeAnchor.empty()) {
        _absolutePath = (_relativeAnchor / inputPath).lexically_normal();
    } else {
        _relativeAnchor = Serialization::getSceneFolder();
        _absolutePath = (_relativeAnchor / inputPath).lexically_normal();
    }

    postUpdate();
}

void SaveLayerPathRow::onRelativeChanged()
{
    if (!_suppressUserInputCallbacks) {
        setPathToSaveAs(_absolutePath.generic_string(), needToSaveAsRelative());
    }
}

void SaveLayerPathRow::postUpdate()
{
    // Update _pathEdit tooltip
    QString tooltip;
    if (!_relativeAnchor.empty()) {
        tooltip = "Directory: ";
        tooltip += _relativeAnchor.generic_string().c_str();
    }
    _pathEdit->setToolTip(tooltip);

    // Update relative anchors of child layers
    _parent->forEachEntry([this](QWidget* w) {
        auto entry = dynamic_cast<SaveLayerPathRow*>(w);
        if (entry && (entry->_layerInfo.parent._layerParent == _layerInfo.layer)
            && entry->needToSaveAsRelative()) {
            fs::filesystem::path relativeAnchor
                = FileSystem::getDir(getAbsolutePath().toStdString());
            fs::filesystem::path relatievPath = entry->_pathEdit->text().toStdString();
            fs::filesystem::path absolutePath = (relativeAnchor / relatievPath).lexically_normal();
            entry->setPathToSaveAs(absolutePath.generic_string(), true);
        }
    });
}

class SaveLayerPathRowArea : public QScrollArea
{
public:
    SaveLayerPathRowArea(QWidget* parent = nullptr)
        : QScrollArea(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    }

    QSize sizeHint() const override
    {
        if (widget() && widget()->layout()) {
            QGridLayout* gridLayout = qobject_cast<QGridLayout*>(widget()->layout());
            if (nullptr != gridLayout) {
                QSize     hint { 0, 0 };
                const int nbCols = gridLayout->columnCount();
                const int nbRows = gridLayout->rowCount();
                for (int r = 0; r < nbRows; ++r) {
                    int rowWidth { 0 };
                    int rowHeight { 0 };
                    for (int c = 0; c < nbCols; ++c) {
                        QWidget* w = gridLayout->itemAtPosition(r, c)->widget();
                        QSize    rowHint = w->sizeHint();
                        rowWidth += rowHint.width();
                        rowHeight = std::max(rowHeight, rowHint.height());
                    }
                    if (hint.width() < rowWidth) {
                        hint.setWidth(rowWidth);
                    }
                    hint.rheight() += rowHeight;
                }

                // Extra padding (enough for 3.5 lines).
                if (hint.height() < DPIScale(120))
                    hint.setHeight(DPIScale(120));
                return hint;
            }

            QVBoxLayout* vLayout = qobject_cast<QVBoxLayout*>(widget()->layout());
            if (nullptr != vLayout) {
                QSize hint { 0, 0 };
                for (int i = 0; i < vLayout->count(); ++i) {
                    QWidget* w = vLayout->itemAt(i)->widget();

                    QSize rowHint = w->sizeHint();
                    if (hint.width() < rowHint.width()) {
                        hint.setWidth(rowHint.width());
                    }
                    if (0 < rowHint.height())
                        hint.rheight() += rowHint.height();
                }

                // Extra padding (enough for 3.5 lines).
                hint.rwidth() += 100;
                if (hint.height() < DPIScale(120))
                    hint.setHeight(DPIScale(120));
                return hint;
            }
        }
        return {};
    }
};

//
// Main Save All Layers Dialog UI
//
namespace UsdLayerEditor {

SaveLayersDialog::SaveLayersDialog(
    QWidget*                            in_parent,
    const std::vector<StageSavingInfo>& infos,
    bool                                isExporting,
    bool                                componentsOnly)
    : QDialog(in_parent)
    , _sessionState(nullptr)
    , _isExporting(isExporting)
{
    std::string msg = String::format(StringResources::kSaveXStages.value, std::to_string(infos.size()));
    setWindowTitle(QString::fromStdString(msg));

    // For each stage collect the layers to save and identify component stages.
    for (const auto& info : infos) {
        std::string dccObjectPath = info.dccObjectPath;
        if (dccObjectPath.empty()) {
            dccObjectPath = UsdUfe::stagePath(info.stage).string();
        }

        // Check if this stage is a component stage
        const bool isComponent
            = UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(info.stage, dccObjectPath);
        if (isComponent) {
            StageSavingInfo componentInfo = info;
            componentInfo.dccObjectPath = dccObjectPath;
            _componentStageInfos.push_back(componentInfo);
            // Component stages are saved via the component system, skip layer collection
            continue;
        }

        // If componentsOnly mode, skip non-component stages entirely
        if (componentsOnly) {
            continue;
        }

        getLayersToSave(info.stage, dccObjectPath, info.stageName);
    }

    QString msg1, msg2, msg3;
    getDialogMessages(
        static_cast<int>(infos.size()),
        static_cast<int>(_anonLayerInfos.size()),
        msg1,
        msg2,
        msg3,
        isExporting);
    buildDialog(msg1, msg2, msg3);
}

SaveLayersDialog::SaveLayersDialog(
    SessionState* in_sessionState,
    QWidget*      in_parent,
    bool          isExporting)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _sessionState(in_sessionState)
    , _isExporting(isExporting)
{
    std::string msg;
    QString     dialogTitle = StringResources::getAsQString(StringResources::kSaveStage);
    if (TF_VERIFY(nullptr != _sessionState)) {
        auto        stageEntry = _sessionState->stageEntry();
        std::string stageName = stageEntry._displayName;
        msg = String::format(StringResources::kSaveName.value, stageName.c_str());
        dialogTitle = QString::fromStdString(msg);

        // Check if this stage is an unsaved component stage.
        if (UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(
                stageEntry._stage, stageEntry._dccObjectPath)) {
            StageSavingInfo info;
            info.stage = stageEntry._stage;
            info.stageName = stageName;
            info.dccObjectPath = stageEntry._dccObjectPath;
            _componentStageInfos.push_back(info);
        } else {
            getLayersToSave(stageEntry._stage, stageEntry._dccObjectPath, stageName);
        }
    }
    setWindowTitle(dialogTitle);

    QString msg1, msg2, msg3;
    getDialogMessages(1, static_cast<int>(_anonLayerInfos.size()), msg1, msg2, msg3, isExporting);
    buildDialog(msg1, msg2, msg3);
}

SaveLayersDialog ::~SaveLayersDialog() { QApplication::restoreOverrideCursor(); }

namespace {
// Test-only override; unset in production. See setExecTestHandler.
SaveLayersDialog::ExecTestHandler& execTestHandler()
{
    static SaveLayersDialog::ExecTestHandler handler;
    return handler;
}
} // namespace

SaveLayersDialog::ExecTestHandler SaveLayersDialog::setExecTestHandler(ExecTestHandler handler)
{
    auto previous = execTestHandler();
    execTestHandler() = std::move(handler);
    return previous;
}

int SaveLayersDialog::exec()
{
    if (auto& handler = execTestHandler())
        return handler();
    return QDialog::exec();
}

void SaveLayersDialog::getLayersToSave(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            objectPath,
    const std::string&            stageName)
{
    // Get the layers to save for this stage. Use the stage directly when available
    // to avoid UFE path-format mismatches (e.g. Maya DAG paths vs |world-prefixed
    // UFE keys) that cause getLayersToSaveFromDCCObject to silently return nothing.
    Serialization::StageLayersToSave StageLayersToSave;
    if (stage) {
        Serialization::getLayersToSaveFromStage(stage, objectPath, StageLayersToSave);
    } else {
        Serialization::getLayersToSaveFromDCCObject(objectPath, StageLayersToSave);
    }

    // Keep track of all the layers for this particular stage.
    for (const auto& layerInfo : StageLayersToSave._anonLayers) {
        _stageLayerMap.emplace(std::make_pair(layerInfo.layer, stageName));
    }

    for (const auto& dirtyLayer : StageLayersToSave._dirtyFileBackedLayers) {
        _stageLayerMap.emplace(std::make_pair(dirtyLayer, stageName));
    }

    // We do not allow saving layers in any of the following conditions:
    // 1- Layer is system locked
    // 2- Layer is anonymous and its parent is locked or system locked

    LayerInfos anonymousLayersUnlocked;
    for (const auto& layerInfo : StageLayersToSave._anonLayers) {
        auto parentLayer = layerInfo.parent._layerParent;
        if (parentLayer != nullptr
            && (isLayerLocked(parentLayer) || isLayerSystemLocked(parentLayer))) {
            continue;
        }
        if (isLayerSystemLocked(layerInfo.layer)) {
            continue;
        }
        anonymousLayersUnlocked.emplace_back(layerInfo);
    }
    moveAppendVector(anonymousLayersUnlocked, _anonLayerInfos);

    // Add these layers to save to our member var for reference later.
    // Note: we use a set for the dirty file back layers because they
    //       can come from multiple stages, but we only want them to
    //       appear once in the dialog.
    std::vector<SdfLayerRefPtr> dirtyFileBackedLayersToDisplay;
    for (const auto& dirtyLayer : StageLayersToSave._dirtyFileBackedLayers) {
        if (isLayerSystemLocked(dirtyLayer)) {
            continue;
        }
        dirtyFileBackedLayersToDisplay.emplace_back(dirtyLayer);
    }
    _dirtyFileBackedLayers.insert(
        std::begin(dirtyFileBackedLayersToDisplay), std::end(dirtyFileBackedLayersToDisplay));
}

void SaveLayersDialog::buildDialog(const QString& msg1, const QString& msg2, const QString& msg3)
{
    const int mainMargin = DPIScale(20);

    // Ok/Cancel button area
    auto buttonsLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(buttonsLayout, 0);
    buttonsLayout->addStretch();
    auto msgResId
        = _isExporting ? StringResources::kSaveStagesAndExport : StringResources::kSaveStages;
    auto okButton = new QPushButton(StringResources::getAsQString(msgResId), this);
    connect(okButton, &QPushButton::clicked, this, &SaveLayersDialog::onSaveAll);
    okButton->setDefault(true);
    auto cancelButton
        = new QPushButton(StringResources::getAsQString(StringResources::kCancel), this);
    connect(cancelButton, &QPushButton::clicked, this, &SaveLayersDialog::onCancel);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    const bool            haveAnonLayers { !_anonLayerInfos.empty() };
    const bool            haveFileBackedLayers { !_dirtyFileBackedLayers.empty() };
    const bool            haveComponentStages { !_componentStageInfos.empty() };
    SaveLayerPathRowArea* anonScrollArea { nullptr };
    SaveLayerPathRowArea* fileScrollArea { nullptr };
    SaveLayerPathRowArea* componentScrollArea { nullptr };
    const int             margin { DPIScale(10) };

    // Anonymous layers.
    if (haveAnonLayers) {
        auto anonLayout = new QGridLayout();
        QtUtils::initLayoutMargins(anonLayout, DPIScale(8));
        anonLayout->setAlignment(Qt::AlignTop);
        // Note: must start from the end so that layers appear in the right order from parent to
        // children.
        int gridRow = 0;
        for (auto iter = _anonLayerInfos.crbegin(); iter != _anonLayerInfos.crend(); ++iter) {
            // Note: the row adds itself as a children of the dialog, so it will be deleted
            //       when the dialog closes.
            auto saveLayerPathRow = new SaveLayerPathRow(this, anonLayout, gridRow++, (*iter));
            // Note: We keep track of the row data so that it can be used when saving layers
            // without having to iterate over SaveLayersDialog's entire layout.
            _saveLayerPathRows.push_back(saveLayerPathRow);
        }

        _anonLayersWidget = new QWidget();
        _anonLayersWidget->setLayout(anonLayout);

        // Setup the scroll area for anonymous layers.
        anonScrollArea = new SaveLayerPathRowArea();
        anonScrollArea->setFrameShape(QFrame::NoFrame);
        anonScrollArea->setBackgroundRole(QPalette::AlternateBase);
        anonScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        anonScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        anonScrollArea->setWidget(_anonLayersWidget);
        anonScrollArea->setWidgetResizable(true);
    }

    // File backed layers
    const bool showFileOverrideSection = confirmExistingFileSave();
    if (showFileOverrideSection && haveFileBackedLayers) {
        auto fileLayout = new QVBoxLayout();
        fileLayout->setContentsMargins(margin, margin, margin, 0);
        fileLayout->setSpacing(DPIScale(8));
        fileLayout->setAlignment(Qt::AlignTop);
        for (const auto& dirtyLayer : _dirtyFileBackedLayers) {
            auto row = new QLabel(dirtyLayer->GetRealPath().c_str(), this);
            row->setToolTip(buildTooltipForLayer(dirtyLayer));
            fileLayout->addWidget(row);
        }

        _fileLayersWidget = new QWidget();
        _fileLayersWidget->setLayout(fileLayout);

        // Setup the scroll area for dirty file backed layers.
        fileScrollArea = new SaveLayerPathRowArea();
        fileScrollArea->setFrameShape(QFrame::NoFrame);
        fileScrollArea->setBackgroundRole(QPalette::AlternateBase);
        fileScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        fileScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        fileScrollArea->setWidget(_fileLayersWidget);
        fileScrollArea->setWidgetResizable(true);
    }

    // Create the main layout for the dialog.
    auto topLayout = new QVBoxLayout();
    QtUtils::initLayoutMargins(topLayout, mainMargin);
    topLayout->setSpacing(DPIScale(8));

    // Component stages section - create ComponentSaveWidget for each component stage
    if (haveComponentStages) {
        auto componentLayout = new QVBoxLayout();
        componentLayout->setContentsMargins(margin, margin, margin, 0);
        componentLayout->setSpacing(DPIScale(8));
        componentLayout->setAlignment(Qt::AlignTop);

        for (size_t i = 0; i < _componentStageInfos.size(); ++i) {
            const auto& componentInfo = _componentStageInfos[i];
            auto        componentWidget
                = new ComponentSaveWidget(this, _sessionState, componentInfo.dccObjectPath);
            componentWidget->setComponentName(QString::fromStdString(componentInfo.stageName));
            // Make compact if not the first component widget
            if (i > 0) {
                componentWidget->setCompactMode(true);
            }
            componentLayout->addWidget(componentWidget);
            _componentSaveWidgets.push_back(componentWidget);
        }

        _componentStagesWidget = new QWidget();
        _componentStagesWidget->setLayout(componentLayout);

        // Setup the scroll area for component stages.
        componentScrollArea = new SaveLayerPathRowArea();
        componentScrollArea->setFrameShape(QFrame::NoFrame);
        componentScrollArea->setBackgroundRole(QPalette::AlternateBase);
        componentScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        componentScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        componentScrollArea->setWidget(_componentStagesWidget);
        componentScrollArea->setWidgetResizable(true);
    }

    if (nullptr != componentScrollArea) {
        // Add message above component save section
        if (!msg3.isEmpty()) {
            auto componentMessage = new QLabel(msg3, this);
            topLayout->addWidget(componentMessage);
        }

        // Add the component scroll area
        topLayout->addWidget(componentScrollArea);

        // Add a separator if we also have anonymous layers or file backed layers
        if (haveAnonLayers || haveFileBackedLayers) {
            auto lineSep = new QFrame();
            lineSep->setFrameShape(QFrame::HLine);
            lineSep->setLineWidth(DPIScale(1));
            QPalette pal(lineSep->palette());
            pal.setColor(QPalette::Base, QColor("#575757"));
            lineSep->setPalette(pal);
            lineSep->setBackgroundRole(QPalette::Base);
            topLayout->addWidget(lineSep);
        }
    }

    if (nullptr != anonScrollArea) {
        // Add the first message.
        if (!msg1.isEmpty()) {
            auto dialogMessage = new QLabel(msg1);
            topLayout->addWidget(dialogMessage);
        }

        // All relative checkbox
        _allAsRelative = new QCheckBox(
            StringResources::getAsQString(StringResources::kBatchSaveAllRelative), this);
        connect(
            _allAsRelative,
            &QCheckBox::stateChanged,
            this,
            &SaveLayersDialog::onAllAsRelativeChanged);

        // Default state for all relative checkbox. If both relative to SceneFile and
        // ParentLayer optionvars are on, then this should be on.
        bool shouldCheck = FileSystem::requireUsdPathsRelativeToParentLayer()
            && FileSystem::requireUsdPathsRelativeToDCCSceneFile();
        _allAsRelative->setChecked(shouldCheck);

        topLayout->addWidget(_allAsRelative);

        // Then add the first scroll area (containing the anonymous layers)
        topLayout->addWidget(anonScrollArea);

        // If we also have dirty file backed layers, add a separator.
        if (showFileOverrideSection && haveFileBackedLayers) {
            auto lineSep = new QFrame();
            lineSep->setFrameShape(QFrame::HLine);
            lineSep->setLineWidth(DPIScale(1));
            QPalette pal(lineSep->palette());
            pal.setColor(QPalette::Base, QColor("#575757"));
            lineSep->setPalette(pal);
            lineSep->setBackgroundRole(QPalette::Base);
            topLayout->addWidget(lineSep);
        }
    }

    if (nullptr != fileScrollArea) {
        // Add the second message.
        if (!msg2.isEmpty()) {
            auto dialogMessage = new QLabel(msg2);
            topLayout->addWidget(dialogMessage);
        }

        // Add the second scroll area (containing the file backed layers).
        topLayout->addWidget(fileScrollArea);
    }

    // Finally add the buttons.
    auto buttonArea = new QWidget(this);
    buttonArea->setLayout(buttonsLayout);
    buttonArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    topLayout->addWidget(buttonArea);

    setLayout(topLayout);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    resize(DPIScale(700), sizeHint().height());
    setSizeGripEnabled(true);
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
}

QString SaveLayersDialog::buildTooltipForLayer(SdfLayerRefPtr layer)
{
    if (nullptr == layer)
        return "";

    // Disable word wrapping on tooltip.
    QString tooltip = "<p style='white-space:pre'>";
    tooltip += StringResources::getAsQString(StringResources::kUsedInStagesTooltip);
    auto range = _stageLayerMap.equal_range(layer);
    bool needComma = false;
    for (auto it = range.first; it != range.second; ++it) {
        if (needComma)
            tooltip.append(", ");
        tooltip.append(it->second.c_str());
        needComma = true;
    }
    return tooltip;
}

QWidget* SaveLayersDialog::findEntry(SdfLayerRefPtr key)
{
    for (int i = 0, count = _saveLayerPathRows.size(); i < count; ++i) {
        auto row = dynamic_cast<SaveLayerPathRow*>(_saveLayerPathRows[i]);
        if (row && row->_layerInfo.layer == key) {
            return row;
        }
    }

    return nullptr;
}

void SaveLayersDialog::forEachEntry(const std::function<void(QWidget*)>& func)
{
    for (int i = 0, count = _saveLayerPathRows.size(); i < count; ++i) {
        func(_saveLayerPathRows[i]);
    }
}

void SaveLayersDialog::onSaveAll()
{
    if (!okToSave()) {
        return;
    }

    _newPaths.clear();
    _problemLayers.clear();
    _emptyLayers.clear();

    // Save component stages first
    for (auto* componentWidget : _componentSaveWidgets) {
        std::string saveLocation = componentWidget->folderLocation().toStdString();
        std::string componentName = componentWidget->componentName().toStdString();
        std::string dccObjectPath = componentWidget->dccObjectPath();

        // SLD-4: move/save the component unconditionally (OLD did not gate this on
        // session state; bulk save has no session state but must still save).
        std::string newRootPath
            = UsdLayerEditor::moveComponent(saveLocation, componentName, dccObjectPath);

        if (!newRootPath.empty()) {
            _newPaths.append(QString::fromStdString(componentName));
            _newPaths.append(QString::fromStdString(newRootPath));

            auto newRootLayer = SdfLayer::FindOrOpen(newRootPath);
            if (newRootLayer) {
                // Capture the component's session layer BEFORE the rename/repath, which
                // recreates the stage with an empty session layer (matches the old editor).
                auto oldSessionLayer = UsdLayerEditor::captureSessionLayer(dccObjectPath);

                // Rename the DCC-side proxy to match the component's new name.
                std::string newDccObjectPath
                    = UsdLayerEditor::renameProxyShape(dccObjectPath, componentName);
                const std::string effectivePath
                    = newDccObjectPath.empty() ? dccObjectPath : newDccObjectPath;

                // SLD-3: rewrite the proxy's root .filePath to the new root layer
                // (renameProxyShape only renames the DAG node, not .filePath).
                UsdLayerEditor::setProxyRootLayerPath(effectivePath, newRootPath, newRootLayer);

                // SLD-2: transfer the captured session-layer opinions onto the new stage.
                UsdLayerEditor::transferSessionLayer(oldSessionLayer, effectivePath);

                // Relocate the stage entry + lock the new root (needs session state).
                if (_sessionState) {
                    auto entries = _sessionState->allStages();
                    for (const auto& entry : entries) {
                        if (entry._dccObjectPath == effectivePath) {
                            _sessionState->setStageEntry(entry);
                            break;
                        }
                    }
                    lockLayer(
                        _sessionState->stageEntry()._dccObjectPath,
                        newRootLayer,
                        LayerLockType::LayerLock_Locked,
                        true);
                }
            }
        } else {
            _problemLayers.append(QString::fromStdString(componentName));
            _problemLayers.append(QString::fromStdString(saveLocation + "/" + componentName));
        }
    }

    // Note: must start from the end so that sub-layers are saved before their parent.
    for (int count = _saveLayerPathRows.size(), i = count - 1; i >= 0; --i) {
        auto row = dynamic_cast<SaveLayerPathRow*>(_saveLayerPathRows[i]);
        if (!row || !row->_layerInfo.layer)
            continue;

        QString absolutePath = row->getAbsolutePath();
        if (!absolutePath.isEmpty()) {
            auto sdfLayer = row->_layerInfo.layer;
            auto parent = row->_layerInfo.parent;
            auto stage = row->_layerInfo.stage;

            Serialization::PathInfo pathInfo;
            pathInfo.absolutePath = absolutePath.toStdString();
            pathInfo.savePathAsRelative = row->needToSaveAsRelative();
            if (pathInfo.savePathAsRelative && parent._layerParent
                && parent._layerParent->IsAnonymous()) {
                pathInfo.customRelativeAnchor = row->calculateParentLayerDir();
            }

            auto newLayer = Serialization::saveAnonymousLayer(stage, sdfLayer, pathInfo, parent);
            if (newLayer) {
                _newPaths.append(QString::fromStdString(sdfLayer->GetDisplayName()));
                _newPaths.append(absolutePath);
            } else {
                _problemLayers.append(QString::fromStdString(sdfLayer->GetDisplayName()));
                _problemLayers.append(absolutePath);
            }
        } else {
            _emptyLayers.append(row->layerDisplayName());
        }
    }

    accept();
}

void SaveLayersDialog::onCancel() { reject(); }

bool SaveLayersDialog::okToSave()
{
    // Block overwriting of components. The target folder must be empty.
    // Otherwise, log an error and abort.
    for (auto* componentWidget : _componentSaveWidgets) {
        fs::filesystem::path location { componentWidget->folderLocation().toStdString() };
        location.append(componentWidget->componentName().toStdString());

        if (fs::filesystem::exists(location) && !fs::filesystem::is_empty(location)) {
            TF_RUNTIME_ERROR(
                "Cannot save %s with the given name since a non-empty folder with the same "
                "name is already in that location. Use a unique name or save to a different "
                "location and try the save again. Folder path: %s",
                componentWidget->componentName().toStdString().c_str(),
                location.generic_string().c_str());
            return false;
        }
    }

    // Files can have the same file names in complicated ways, with one file having two copies,
    // another three, so we keep the exact number of copies per file path.
    QMap<QString, int> alreadySeenPaths;
    QStringList        existingFiles;

    for (int count = _saveLayerPathRows.size(), i = count - 1; i >= 0; --i) {
        auto row = dynamic_cast<SaveLayerPathRow*>(_saveLayerPathRows[i]);
        if (!row || !row->_layerInfo.layer)
            continue;

        QString path = row->getAbsolutePath();

        if (!path.isEmpty()) {
            if (alreadySeenPaths.count(path) > 0) {
                alreadySeenPaths[path] += 1;
            } else {
                alreadySeenPaths[path] = 1;
            }
            QFileInfo fInfo(path);
            if (fInfo.exists()) {
                existingFiles.append(path);
            }
        }
    }

    QStringList identicalFiles;
    int         identicalCount = 0;
    for (const auto& path : alreadySeenPaths.keys()) {
        const int count = alreadySeenPaths[path];
        if (count > 1) {
            identicalFiles.append(path);
            identicalCount += count;
        }
    }

    if (identicalCount > 0) {
        std::string errorMsg = String::format(StringResources::kSaveAnonymousIdenticalFiles.value, std::to_string(identicalCount));

        warningDialog(
            StringResources::getAsQString(StringResources::kSaveAnonymousIdenticalFilesTitle),
            QString::fromStdString(errorMsg),
            &identicalFiles,
            QMessageBox::Icon::Critical,
            this);

        return false;
    }

    if (!existingFiles.isEmpty()) {
        std::string confirmMsg = String::format(StringResources::kSaveAnonymousConfirmOverwrite.value, std::to_string(existingFiles.length()));

        return (confirmDialog(
            StringResources::getAsQString(StringResources::kSaveAnonymousConfirmOverwriteTitle),
            QString::fromStdString(confirmMsg),
            &existingFiles,
            nullptr,
            QMessageBox::Icon::Warning,
            this));
    }

    return true;
}

void SaveLayersDialog::onAllAsRelativeChanged()
{
    if (!_allAsRelative) {
        return;
    }

    bool saveAsRelative = _allAsRelative->checkState() == Qt::Checked;
    forEachEntry([saveAsRelative](QWidget* w) {
        auto entry = dynamic_cast<SaveLayerPathRow*>(w);
        if (entry) {
            entry->setSaveAsRelative(saveAsRelative);
        }
    });
}

void SaveLayersDialog::quietlyUncheckAllAsRelative()
{
    if (!_allAsRelative) {
        return;
    }

    auto allRelative = _allAsRelative;
    _allAsRelative = nullptr; // nullify _allAsRelative to suppress setSaveAllAsRelative callback
    allRelative->setCheckState(Qt::Unchecked);
    _allAsRelative = allRelative; // restore _allAsRelative
}

/*static*/
bool SaveLayersDialog::saveLayerFilePathUI(
    std::string&                  out_filePath,
    const std::string&            parentLayerPath)
{
    QString qfile{ QFileDialog::getSaveFileName(
        nullptr,
        tr("Save Universal Scene Description (USD) File"),
        QString::fromStdString(parentLayerPath),
        tr("USD (*.usd;*.usda;*.usdc)")) };

    std::string file = qfile.toStdString();
    if (file.empty())
        return false;

    out_filePath = file;

    return true;
}

/*static*/
bool SaveLayersDialog::saveLayerFilePathUI(
    std::string&          out_filePath,
    const SdfLayerRefPtr& parentLayer)
{
    std::string parentLayerPath;
    if (parentLayer) {
        parentLayerPath = parentLayer->GetRealPath();
        if (parentLayerPath.empty())
            parentLayerPath = parentLayer->GetIdentifier();
    }
    return saveLayerFilePathUI(out_filePath, parentLayerPath);
}

} // namespace UsdLayerEditor
