#include "saveLayersDialog.h"

#include "componentSaveWidget.h"
#include "generatedIconButton.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "pathChecker.h"
#include "qtUtils.h"
#include "sessionState.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/layerLocking.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/sdf/layer.h>

#include <maya/MDagPathArray.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGridLayout>
#include <ghc/filesystem.hpp>

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
    bool      isExporting)
{
    MString msg, strNbStages, strNbAnonLayers;
    strNbStages = nbStages;
    strNbAnonLayers = nbAnonLayers;

    auto msgResId = isExporting ? StringResources::kToExportTheStageSaveAnonym
                                : StringResources::kToSaveTheStageSaveAnonym;
    msg.format(StringResources::getAsMString(msgResId), strNbStages, strNbAnonLayers);
    msg1 = MQtUtil::toQString(msg);

    msgResId = isExporting ? StringResources::kToExportTheStageSaveFiles
                           : StringResources::kToSaveTheStageSaveFiles;
    msg.format(StringResources::getAsMString(msgResId), strNbStages);
    msg2 = MQtUtil::toQString(msg);
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
    using LayerInfo = MayaUsd::utils::LayerInfo;
    using LayerInfos = MayaUsd::utils::LayerInfos;

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
    ghc::filesystem::path _absolutePath;
    ghc::filesystem::path _relativeAnchor;

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

    QIcon icon = utils->createIcon(":/fileOpen.png");
    _openBrowser = new GeneratedIconButton(this, icon);
    gridLayout->addWidget(_openBrowser, gridRow, 2);
    connect(_openBrowser, &QAbstractButton::clicked, this, &SaveLayerPathRow::onOpenBrowser);

    QString checkBoxTitle = _layerInfo.parent._layerParent
        ? StringResources::getAsQString(StringResources::kBatchSaveRelativeToParent)
        : StringResources::getAsQString(StringResources::kBatchSaveRelativeToScene);
    MString checkBoxTooltip;
    if (_layerInfo.parent._layerParent) {
        checkBoxTooltip.format(
            StringResources::getAsMString(StringResources::kBatchSaveRelativeToLayerTooltip),
            _layerInfo.parent._layerParent->GetDisplayName().c_str());
    } else {
        checkBoxTooltip
            = StringResources::getAsMString(StringResources::kBatchSaveRelativeToSceneTooltip);
    }

    _relative = new QCheckBox(checkBoxTitle, this);
    _relative->setToolTip(MQtUtil::toQString(checkBoxTooltip));
    connect(_relative, &QCheckBox::stateChanged, this, &SaveLayerPathRow::onRelativeChanged);
    gridLayout->addWidget(_relative, gridRow, 3);

    QString pathToSaveAs
        = MayaUsd::utils::generateUniqueLayerFileName(stageName, _layerInfo.layer).c_str();
    _pathEdit->setText(QFileInfo(pathToSaveAs).absoluteFilePath());

    // Set default state of checkbox and proper setting of path (must come after the initial
    // setting above).
    bool shouldCheck = _layerInfo.parent._layerParent
        ? UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer()
        : UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
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
            relativeAnchor = UsdMayaUtilFileSystem::getMayaSceneFileDir();
            if (relativeAnchor.empty()) {
                relativeAnchor
                    = ghc::filesystem::path(absolutePath).remove_filename().generic_string();
            }
        }
    }

    _absolutePath = absolutePath;
    _relativeAnchor = relativeAnchor;

    std::string displayPath = absolutePath;
    if (!relativeAnchor.empty()) {
        displayPath = UsdMayaUtilFileSystem::makePathRelativeTo(
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
                return UsdMayaUtilFileSystem::getDir(
                    parentLayerEntry->getAbsolutePath().toStdString());
            }
        } else {
            return UsdMayaUtilFileSystem::getLayerFileDir(parentLayer);
        }
    }

    return std::string();
}

void SaveLayerPathRow::onOpenBrowser()
{
    // Calculate parent layer path
    auto&       parentLayer = _layerInfo.parent._layerParent;
    std::string parentLayerPath = calculateParentLayerDir();

    // Update appropriate option var
    const char* optionVarName = parentLayer ? "mayaUsd_MakePathRelativeToParentLayer"
                                            : "mayaUsd_MakePathRelativeToSceneFile";
    const bool optionvarExists = MGlobal::optionVarExists(optionVarName);
    int        optionVarValue = 0;
    if (optionvarExists) {
        optionVarValue = MGlobal::optionVarIntValue(optionVarName);
        MGlobal::setOptionVarValue(optionVarName, needToSaveAsRelative() ? 1 : 0);
    }

    // Run the UI and set the resulting path
    std::string absolutePath;
    if (SaveLayersDialog::saveLayerFilePathUI(absolutePath, !parentLayer, parentLayerPath)) {
        bool saveAsRelative = (optionvarExists && MGlobal::optionVarIntValue(optionVarName) != 0);
        setPathToSaveAs(absolutePath, saveAsRelative);
    }

    // Restore original option var value
    if (optionvarExists) {
        MGlobal::setOptionVarValue(optionVarName, optionVarValue);
    }
}

void SaveLayerPathRow::onTextChanged(const QString& text)
{
    if (_suppressUserInputCallbacks) {
        return;
    }

    ghc::filesystem::path inputPath(text.toStdString());
    if (inputPath.is_absolute()) {
        _relativeAnchor.clear();
        _absolutePath = inputPath;
    } else if (!_relativeAnchor.empty()) {
        _absolutePath = (_relativeAnchor / inputPath).lexically_normal();
    } else {
        _relativeAnchor = MayaUsd::utils::getSceneFolder();
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
            ghc::filesystem::path relativeAnchor
                = UsdMayaUtilFileSystem::getDir(getAbsolutePath().toStdString());
            ghc::filesystem::path relatievPath = entry->_pathEdit->text().toStdString();
            ghc::filesystem::path absolutePath = (relativeAnchor / relatievPath).lexically_normal();
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
    QWidget*                                     in_parent,
    const std::vector<MayaUsd::StageSavingInfo>& infos,
    bool                                         isExporting)
    : QDialog(in_parent)
    , _sessionState(nullptr)
    , _isExporting(isExporting)
{
    MString msg, nbStages;

    nbStages = infos.size();
    msg.format(StringResources::getAsMString(StringResources::kSaveXStages), nbStages);
    setWindowTitle(MQtUtil::toQString(msg));

    // For each stage collect the layers to save and identify component stages.
    for (const auto& info : infos) {
        std::string proxyPath = info.dagPath.fullPathName().asChar();
        
        // Check if this stage is a component stage
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(proxyPath)) {
            _componentStageInfos.push_back(info);
        }

        getLayersToSave(
            info.stage,
            proxyPath,
            info.dagPath.partialPathName().asChar());
    }

    QString msg1, msg2;
    getDialogMessages(
        static_cast<int>(infos.size()),
        static_cast<int>(_anonLayerInfos.size()),
        msg1,
        msg2,
        isExporting);
    buildDialog(msg1, msg2);
}

SaveLayersDialog::SaveLayersDialog(
    SessionState* in_sessionState,
    QWidget*      in_parent,
    bool          isExporting)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _sessionState(in_sessionState)
    , _isExporting(isExporting)
{
    MString msg;
    QString dialogTitle = StringResources::getAsQString(StringResources::kSaveStage);
    if (TF_VERIFY(nullptr != _sessionState)) {
        auto        stageEntry = _sessionState->stageEntry();
        std::string stageName = stageEntry._displayName;
        msg.format(StringResources::getAsMString(StringResources::kSaveName), stageName.c_str());
        dialogTitle = MQtUtil::toQString(msg);
        getLayersToSave(stageEntry._stage, stageEntry._proxyShapePath, stageName);
    }
    setWindowTitle(dialogTitle);

    QString msg1, msg2;
    getDialogMessages(1, static_cast<int>(_anonLayerInfos.size()), msg1, msg2, isExporting);
    buildDialog(msg1, msg2);
}

SaveLayersDialog ::~SaveLayersDialog() { QApplication::restoreOverrideCursor(); }

void SaveLayersDialog::getLayersToSave(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            proxyPath,
    const std::string&            stageName)
{
    // Get the layers to save for this stage.
    MayaUsd::utils::StageLayersToSave StageLayersToSave;
    MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);

    // Keep track of all the layers for this particular stage.
    for (const auto& layerInfo : StageLayersToSave._anonLayers) {
        _stageLayerMap.emplace(std::make_pair(layerInfo.layer, stageName));
    }

    for (const auto& dirtyLayer : StageLayersToSave._dirtyFileBackedLayers) {
        _stageLayerMap.emplace(std::make_pair(dirtyLayer, stageName));
    }

    // We do not allow saving layers in any of the following conditions:
    // 1- Layer is system locked
    // 2- Layer is anonymous and its parent is system locked

    LayerInfos anonymousLayersUnlocked;
    for (const auto& layerInfo : StageLayersToSave._anonLayers) {
        auto parentLayer = layerInfo.parent._layerParent;
        if (parentLayer != nullptr && MayaUsd::isLayerSystemLocked(parentLayer)) {
            continue;
        }
        if (MayaUsd::isLayerSystemLocked(layerInfo.layer)) {
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
        if (MayaUsd::isLayerSystemLocked(dirtyLayer)) {
            continue;
        }
        dirtyFileBackedLayersToDisplay.emplace_back(dirtyLayer);
    }
    _dirtyFileBackedLayers.insert(
        std::begin(dirtyFileBackedLayersToDisplay), std::end(dirtyFileBackedLayersToDisplay));
}

void SaveLayersDialog::buildDialog(const QString& msg1, const QString& msg2)
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
            // Note: We keep track of the row data so that it can be used when saving layers without
            // having to iterate over SaveLayersDialog's entire layout.
            _saveLayerPathRows.push_back(saveLayerPathRow);
        }

        _anonLayersWidget = new QWidget();
        _anonLayersWidget->setLayout(anonLayout);

        // Setup the scroll area for anonymous layers.
        anonScrollArea = new SaveLayerPathRowArea();
        anonScrollArea->setFrameShape(QFrame::NoFrame);
        anonScrollArea->setBackgroundRole(QPalette::Midlight);
        anonScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        anonScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        anonScrollArea->setWidget(_anonLayersWidget);
        anonScrollArea->setWidgetResizable(true);
    }

    // File backed layers
    static const MString kConfirmExistingFileSave
        = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
    const bool showFileOverrideSection = MGlobal::optionVarExists(kConfirmExistingFileSave)
        && MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0;
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
        fileScrollArea->setBackgroundRole(QPalette::Midlight);
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
        for (const auto& componentInfo : _componentStageInfos) {
            std::string proxyPath = componentInfo.dagPath.fullPathName().asChar();
            auto componentWidget = new ComponentSaveWidget(this, proxyPath);
            componentWidget->setComponentName(QString(componentInfo.dagPath.partialPathName().asChar()));
            componentWidget->setFolderLocation(QString(MayaUsd::utils::getSceneFolder().c_str()));
            topLayout->addWidget(componentWidget);
        }
        
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
        bool shouldCheck = UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer()
            && UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
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

            MayaUsd::utils::PathInfo pathInfo;
            pathInfo.absolutePath = absolutePath.toStdString();
            pathInfo.savePathAsRelative = row->needToSaveAsRelative();
            if (pathInfo.savePathAsRelative && parent._layerParent
                && parent._layerParent->IsAnonymous()) {
                pathInfo.customRelativeAnchor = row->calculateParentLayerDir();
            }

            auto newLayer = MayaUsd::utils::saveAnonymousLayer(stage, sdfLayer, pathInfo, parent);
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
        MString errorMsg;
        MString count;
        count = identicalCount;
        errorMsg.format(
            StringResources::getAsMString(StringResources::kSaveAnonymousIdenticalFiles), count);

        warningDialog(
            StringResources::getAsQString(StringResources::kSaveAnonymousIdenticalFilesTitle),
            MQtUtil::toQString(errorMsg),
            &identicalFiles,
            QMessageBox::Icon::Critical);

        return false;
    }

    if (!existingFiles.isEmpty()) {
        MString confirmMsg;
        MString count;
        count = existingFiles.length();
        confirmMsg.format(
            StringResources::getAsMString(StringResources::kSaveAnonymousConfirmOverwrite), count);

        return (confirmDialog(
            StringResources::getAsQString(StringResources::kSaveAnonymousConfirmOverwriteTitle),
            MQtUtil::toQString(confirmMsg),
            &existingFiles,
            nullptr,
            QMessageBox::Icon::Warning));
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
    const PXR_NS::SdfLayerRefPtr& parentLayer)
{
    const bool useSceneFileForRoot = true;
    UsdMayaUtilFileSystem::prepareLayerSaveUILayer(parentLayer, useSceneFileForRoot);

    std::string parentLayerPath = "\"\"";
    if (parentLayer) {
        ghc::filesystem::path parentPath(parentLayer->GetRealPath());
        parentLayerPath = "\"" + parentPath.parent_path().generic_string() + "\"";
    }

    MString cmd;
    cmd.format(
        "UsdLayerEditor_SaveLayerFileDialog(^1s,^2s,0)",
        parentLayer ? "0" : "1",
        parentLayerPath.c_str());

    MString fileSelected;
    MGlobal::executeCommand(
        cmd,
        fileSelected,
        /*display*/ true,
        /*undo*/ false);
    if (fileSelected.length() == 0)
        return false;

    out_filePath = fileSelected.asChar();

    return true;
}

/*static*/
bool SaveLayersDialog::saveLayerFilePathUI(
    std::string&       out_filePath,
    bool               isRootLayer,
    const std::string& parentLayerPath)
{
    UsdMayaUtilFileSystem::prepareLayerSaveUILayer(parentLayerPath);

    MString cmd;
    cmd.format(
        "UsdLayerEditor_SaveLayerFileDialog(^1s,\"^2s\",1)",
        isRootLayer ? "1" : "0",
        ghc::filesystem::path(parentLayerPath).generic_string().c_str());

    MString fileSelected;
    MGlobal::executeCommand(
        cmd,
        fileSelected,
        /*display*/ true,
        /*undo*/ false);
    if (fileSelected.length() == 0)
        return false;

    out_filePath = fileSelected.asChar();

    return true;
}

} // namespace UsdLayerEditor
