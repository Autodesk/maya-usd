#include "saveLayersDialog.h"

#include "generatedIconButton.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "pathChecker.h"
#include "qtUtils.h"
#include "sessionState.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/sdf/layer.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>

#include <string>

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Utils.h>
#endif

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

void getDialogMessages(const int nbStages, const int nbAnonLayers, QString& msg1, QString& msg2)
{
    MString msg, strNbStages, strNbAnonLayers;
    strNbStages = nbStages;
    strNbAnonLayers = nbAnonLayers;
    msg.format(
        StringResources::getAsMString(StringResources::kToSaveTheStageSaveAnonym),
        strNbStages,
        strNbAnonLayers);
    msg1 = MQtUtil::toQString(msg);

    msg.format(
        StringResources::getAsMString(StringResources::kToSaveTheStageSaveFiles), strNbStages);
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
    SaveLayerPathRow(
        SaveLayersDialog*                                in_parent,
        const std::pair<SdfLayerRefPtr, SdfLayerRefPtr>& in_layerPair);

    QString layerDisplayName() const;

    QString absolutePath() const;
    QString pathToSaveAs() const;

    void setAbsolutePath(const std::string& path);

protected:
    void onOpenBrowser();
    void onTextChanged(const QString& text);
    void onRelativeButtonChecked(bool checked);

public:
    QString                                   _initialStartFolder;
    QString                                   _absolutePath;
    SaveLayersDialog*                         _parent { nullptr };
    std::pair<SdfLayerRefPtr, SdfLayerRefPtr> _layerPair;
    QLabel*                                   _label { nullptr };
    QLineEdit*                                _pathEdit { nullptr };
    QAbstractButton*                          _openBrowser { nullptr };
};

SaveLayerPathRow::SaveLayerPathRow(
    SaveLayersDialog*                                in_parent,
    const std::pair<SdfLayerRefPtr, SdfLayerRefPtr>& in_layerPair)
    : QWidget(in_parent)
    , _parent(in_parent)
    , _layerPair(in_layerPair)
{
    auto gridLayout = new QGridLayout();
    QtUtils::initLayoutMargins(gridLayout);

    // Since this is an anonymous layer, it should only be associated with a single stage.
    std::string stageName;
    const auto& stageLayers = in_parent->stageLayers();
    if (TF_VERIFY(1 == stageLayers.count(_layerPair.first))) {
        auto search = stageLayers.find(_layerPair.first);
        stageName = search->second;
    }

    QString displayName = _layerPair.first->GetDisplayName().c_str();
    _label = new QLabel(displayName);
    _label->setToolTip(in_parent->buildTooltipForLayer(_layerPair.first));
    gridLayout->addWidget(_label, 0, 0);

    _initialStartFolder = MayaUsd::utils::getSceneFolder().c_str();
    _absolutePath = MayaUsd::utils::generateUniqueFileName(stageName).c_str();

    QFileInfo fileInfo(_absolutePath);
    QString   suggestedFullPath = fileInfo.absoluteFilePath();
    _pathEdit = new AnonLayerPathEdit(this);
    _pathEdit->setText(suggestedFullPath);
    connect(_pathEdit, &QLineEdit::textChanged, this, &SaveLayerPathRow::onTextChanged);
    gridLayout->addWidget(_pathEdit, 0, 1);

    QIcon icon = utils->createIcon(":/fileOpen.png");
    _openBrowser = new GeneratedIconButton(this, icon);
    gridLayout->addWidget(_openBrowser, 0, 2);
    connect(_openBrowser, &QAbstractButton::clicked, this, &SaveLayerPathRow::onOpenBrowser);

    setLayout(gridLayout);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
}

QString SaveLayerPathRow::layerDisplayName() const { return _label->text(); }

QString SaveLayerPathRow::absolutePath() const { return _absolutePath; }

QString SaveLayerPathRow::pathToSaveAs() const { return _absolutePath; }

void SaveLayerPathRow::setAbsolutePath(const std::string& path)
{
    _absolutePath = path.c_str();
    _pathEdit->setText(_absolutePath);
    _pathEdit->setEnabled(true);
}

void SaveLayerPathRow::onOpenBrowser()
{
    std::string fileName;
    if (SaveLayersDialog::saveLayerFilePathUI(fileName)) {
        setAbsolutePath(fileName);
    }
}

void SaveLayerPathRow::onTextChanged(const QString& text) { _absolutePath = text; }

void SaveLayerPathRow::onRelativeButtonChecked(bool checked)
{
    if (checked) {
        QDir dir(_initialStartFolder);

        QString relativePath = dir.relativeFilePath(_absolutePath);
        _pathEdit->setText(relativePath);
        _pathEdit->setEnabled(false);
    } else {
        _pathEdit->setEnabled(true);
        _pathEdit->setText(_absolutePath);
    }
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
        QSize hint;

        if (widget() && widget()->layout()) {
            auto l = widget()->layout();
            for (int i = 0; i < l->count(); ++i) {
                QWidget* w = l->itemAt(i)->widget();

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
        }
        return hint;
    }
};

//
// Main Save All Layers Dialog UI
//
namespace UsdLayerEditor {

#if defined(WANT_UFE_BUILD)
SaveLayersDialog::SaveLayersDialog(QWidget* in_parent, const std::vector<UsdStageRefPtr>& stages)
    : QDialog(in_parent)
    , _sessionState(nullptr)
{
    MString msg, nbStages;

    nbStages = stages.size();
    msg.format(StringResources::getAsMString(StringResources::kSaveXStages), nbStages);
    setWindowTitle(MQtUtil::toQString(msg));

    // For each stage collect the layers to save.
    for (const auto& stage : stages) {
        // Get the name of this stage.
        auto        stagePath = MayaUsd::ufe::stagePath(stage);
        std::string stageName = !stagePath.empty() ? stagePath.back().string() : "Unknown";

        getLayersToSave(stage, stageName);
    }

    QString msg1, msg2;
    getDialogMessages(
        static_cast<int>(stages.size()), static_cast<int>(_anonLayerPairs.size()), msg1, msg2);
    buildDialog(msg1, msg2);
}
#endif

SaveLayersDialog::SaveLayersDialog(SessionState* in_sessionState, QWidget* in_parent)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _sessionState(in_sessionState)
{
    MString msg;
    QString dialogTitle = StringResources::getAsQString(StringResources::kSaveStage);
    if (TF_VERIFY(nullptr != _sessionState)) {
        auto        stageEntry = _sessionState->stageEntry();
        std::string stageName = stageEntry._displayName;
        msg.format(StringResources::getAsMString(StringResources::kSaveName), stageName.c_str());
        dialogTitle = MQtUtil::toQString(msg);
        getLayersToSave(stageEntry._stage, stageName);
    }
    setWindowTitle(dialogTitle);

    QString msg1, msg2;
    getDialogMessages(1, static_cast<int>(_anonLayerPairs.size()), msg1, msg2);
    buildDialog(msg1, msg2);
}

SaveLayersDialog ::~SaveLayersDialog() { QApplication::restoreOverrideCursor(); }

void SaveLayersDialog::getLayersToSave(UsdStageRefPtr stage, const std::string& stageName)
{
    // Get the layers to save for this stage.
    MayaUsd::utils::stageLayersToSave stageLayersToSave;
    MayaUsd::utils::getLayersToSaveFromProxy(stage, stageLayersToSave);

    // Keep track of all the layers for this particular stage.
    for (const auto& layerPairs : stageLayersToSave.anonLayers) {
        _stageLayerMap.emplace(std::make_pair(layerPairs.first, stageName));
    }
    for (const auto& dirtyLayer : stageLayersToSave.dirtyFileBackedLayers) {
        _stageLayerMap.emplace(std::make_pair(dirtyLayer, stageName));
    }

    // Add these layers to save to our member var for reference later.
    // Note: we use a set for the dirty file back layers because they
    //       can come from multiple stages, but we only want them to
    //       appear once in the dialog.
    moveAppendVector(stageLayersToSave.anonLayers, _anonLayerPairs);
    _dirtyFileBackedLayers.insert(
        std::begin(stageLayersToSave.dirtyFileBackedLayers),
        std::end(stageLayersToSave.dirtyFileBackedLayers));
}

void SaveLayersDialog::buildDialog(const QString& msg1, const QString& msg2)
{
    const int mainMargin = DPIScale(20);

    // Ok/Cancel button area
    auto buttonsLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(buttonsLayout, 0);
    buttonsLayout->addStretch();
    auto okButton
        = new QPushButton(StringResources::getAsQString(StringResources::kSaveStages), this);
    connect(okButton, &QPushButton::clicked, this, &SaveLayersDialog::onSaveAll);
    okButton->setDefault(true);
    auto cancelButton
        = new QPushButton(StringResources::getAsQString(StringResources::kCancel), this);
    connect(cancelButton, &QPushButton::clicked, this, &SaveLayersDialog::onCancel);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    const bool            haveAnonLayers { !_anonLayerPairs.empty() };
    const bool            haveFileBackedLayers { !_dirtyFileBackedLayers.empty() };
    SaveLayerPathRowArea* anonScrollArea { nullptr };
    SaveLayerPathRowArea* fileScrollArea { nullptr };
    const int             margin { DPIScale(10) };

    // Anonymous layers.
    if (haveAnonLayers) {
        auto anonLayout = new QVBoxLayout();
        anonLayout->setContentsMargins(margin, margin, margin, 0);
        anonLayout->setSpacing(DPIScale(8));
        anonLayout->setAlignment(Qt::AlignTop);
        for (auto iter = _anonLayerPairs.cbegin(); iter != _anonLayerPairs.cend(); ++iter) {
            auto row = new SaveLayerPathRow(this, (*iter));
            anonLayout->addWidget(row);
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
    if (haveFileBackedLayers) {
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

    if (nullptr != anonScrollArea) {
        // Add the first message.
        if (!msg1.isEmpty()) {
            auto dialogMessage = new QLabel(msg1);
            topLayout->addWidget(dialogMessage);
        }

        // Then add the first scroll area (containing the anonymous layers)
        topLayout->addWidget(anonScrollArea);

        // If we also have dirty file backed layers, add a separator.
        if (haveFileBackedLayers) {
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

void SaveLayersDialog::onSaveAll()
{
    if (!okToSave()) {
        return;
    }

    int           i, count;
    std::string   newRoot;
    SessionState* rootSessionState = nullptr;

    _newPaths.clear();
    _problemLayers.clear();
    _emptyLayers.clear();

    // The anonymous layer section in the dialog can be empty.
    if (nullptr != _anonLayersWidget) {
        QLayout* anonLayout = _anonLayersWidget->layout();
        for (i = 0, count = anonLayout->count(); i < count; ++i) {
            auto row = dynamic_cast<SaveLayerPathRow*>(anonLayout->itemAt(i)->widget());
            if (!row || !row->_layerPair.first)
                continue;

            QString path = row->pathToSaveAs();
            if (!path.isEmpty()) {
                auto sdfLayer = row->_layerPair.first;
                auto parentLayer = row->_layerPair.second;
                auto qFileName = row->absolutePath();
                auto sFileName = qFileName.toStdString();

                auto newLayer
                    = MayaUsd::utils::saveAnonymousLayer(sdfLayer, sFileName, parentLayer);

                if (!parentLayer) {
                    newRoot = sFileName;
                    rootSessionState = _sessionState;
                } else {
                    if (newLayer) {
                        // if (item->isTargetLayer()) {
                        //     sessionState->stage()->SetEditTarget(newLayer);
                        // }

                        _newPaths.append(QString::fromStdString(sdfLayer->GetDisplayName()));
                        _newPaths.append(qFileName);
                    } else {
                        _problemLayers.append(QString::fromStdString(sdfLayer->GetDisplayName()));
                        _problemLayers.append(qFileName);
                    }
                }
            } else {
                _emptyLayers.append(row->layerDisplayName());
            }
        }

        if (rootSessionState) {
            rootSessionState->rootLayerPathChanged(newRoot);
        }
    }

    accept();
} // namespace UsdLayerEditor

void SaveLayersDialog::onCancel() { reject(); }

bool SaveLayersDialog::okToSave()
{
    int         i, count;
    QStringList existingFiles;

    // The anonymous layer section in the dialog can be empty.
    if (nullptr != _anonLayersWidget) {
        QLayout* anonLayout = _anonLayersWidget->layout();
        for (i = 0, count = anonLayout->count(); i < count; ++i) {
            auto row = dynamic_cast<SaveLayerPathRow*>(anonLayout->itemAt(i)->widget());
            if (nullptr == row)
                continue;

            QString path = row->pathToSaveAs();
            if (!path.isEmpty()) {
                QFileInfo fInfo(path);
                if (fInfo.exists()) {
                    existingFiles.append(path);
                }
            }
        }
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
            &existingFiles));
    }

    return true;
}

/*static*/
bool SaveLayersDialog::saveLayerFilePathUI(std::string& out_filePath)
{
    MString fileSelected;
    MGlobal::executeCommand(
        MString("UsdLayerEditor_SaveLayerFileDialog"),
        fileSelected,
        /*display*/ true,
        /*undo*/ false);
    if (fileSelected.length() == 0)
        return false;

    out_filePath = fileSelected.asChar();

    return true;
}

} // namespace UsdLayerEditor
