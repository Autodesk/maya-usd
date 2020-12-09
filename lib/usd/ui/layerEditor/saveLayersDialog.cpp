#include "saveLayersDialog.h"

#include "generatedIconButton.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "pathChecker.h"
#include "qtUtils.h"
#include "sessionState.h"
#include "warningDialogs.h"

#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/usdFileFormat.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtGui/QFontMetrics>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>

#include <cassert>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using namespace UsdLayerEditor;

bool isAbsolutePath(const QString& in_path)
{
    QFileInfo fileInfo(in_path);
    return fileInfo.isAbsolute();
}

// Starting at the given tree item, walk up the layer hierarchy looking for
// the first file backed layer.
// If we find a file backed layer then this will be the folder to save in.
// If we don't find a folder then we will ask the session layer for the
// current Maya scene and use that location.
//
QString suggestedStartFolder(const LayerTreeItem* treeItem)
{
    QString startPath;

    auto item = treeItem;
    while (item != nullptr) {
        if (!item->isAnonymous()) {
            startPath = QFileInfo(item->layer()->GetRealPath().c_str()).dir().absolutePath();
            break;
        }
        item = item->parentLayerItem();
    }

    if (startPath.isEmpty()) {
        startPath = treeItem->parentModel()->sessionState()->defaultLoadPath().c_str();
    }

    if (!startPath.isEmpty() && startPath.back() != QDir::separator()) {
        startPath += QDir::separator();
    }

    return QDir::fromNativeSeparators(startPath);
}

QString suggestedFileName(const LayerTreeItem* treeItem, const QString& startFolder)
{
    QString templateName = startFolder + QString::fromStdString(treeItem->displayName())
        + QString("_XXXXXX.") + QString(UsdUsdFileFormatTokens->Id.GetText());
    QTemporaryFile tFile(templateName);
    if (tFile.open()) {
        return tFile.fileName();
    } else {
        return startFolder + QString::fromStdString(treeItem->displayName()) + QString(".")
            + QString(UsdUsdFileFormatTokens->Id.GetText());
    }
}

// TODO: helper class copied from Load Layers Dialog.
class MyLineEdit : public QLineEdit
{
public:
    MyLineEdit(QWidget* in_parent)
        : QLineEdit(in_parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }

    QSize sizeHint() const override
    {
        auto hint = QLineEdit::sizeHint();
        if (!text().isEmpty()) {
            QFontMetrics appFont = QApplication::fontMetrics();
            int          pathWidth = appFont.horizontalAdvance(text()) + 100;
            hint.setWidth(DPIScale(pathWidth));
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
    SaveLayerPathRow(SaveLayersDialog* in_parent, LayerTreeItem* in_treeItem);

    QString layerDisplayName() const;

    QString absolutePath() const;
    QString pathToSaveAs() const;
    QString usdFormatTag() const;

    void setAbsolutePath(const std::string& path, const std::string& tag);

protected:
    void onOpenBrowser();
    void onTextChanged(const QString& text);
    void onRelativeButtonChecked(bool checked);

public:
    QString           _initialStartFolder;
    QString           _absolutePath;
    QString           _formatTag;
    SaveLayersDialog* _parent;
    LayerTreeItem*    _treeItem;
    QLabel*           _label;
    QLineEdit*        _pathEdit;
    QAbstractButton*  _openBrowser;
};

SaveLayerPathRow::SaveLayerPathRow(SaveLayersDialog* in_parent, LayerTreeItem* in_treeItem)
    : QWidget(in_parent)
    , _formatTag("usdc")
    , _parent(in_parent)
    , _treeItem(in_treeItem)
{
    auto gridLayout = new QGridLayout();
    QtUtils::initLayoutMargins(gridLayout);

    QString displayName = _treeItem->displayName().c_str();
    _label = new QLabel(displayName);
    gridLayout->addWidget(_label, 0, 0);

    _initialStartFolder = suggestedStartFolder(_treeItem);
    _absolutePath = suggestedFileName(_treeItem, _initialStartFolder);

    QFileInfo fileInfo(_absolutePath);
    QString   suggestedFullPath = fileInfo.absoluteFilePath();
    _pathEdit = new MyLineEdit(this);
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

QString SaveLayerPathRow::usdFormatTag() const { return _formatTag; }

void SaveLayerPathRow::setAbsolutePath(const std::string& path, const std::string& tag)
{
    _absolutePath = path.c_str();
    _pathEdit->setText(_absolutePath);
    _pathEdit->setEnabled(true);
    _formatTag = tag.c_str();
}

void SaveLayerPathRow::onOpenBrowser()
{
    auto        sessionState = _treeItem->parentModel()->sessionState();
    std::string fileName, formatTag;
    if (sessionState->saveLayerUI(nullptr, &fileName, &formatTag)) {
        setAbsolutePath(fileName, formatTag);
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
            hint.rheight() += 100;
            hint.rwidth() += 100;
        }
        return hint;
    }
};

//
// Main Save All Layers Dialog UI
//
namespace UsdLayerEditor {

SaveLayersDialog::SaveLayersDialog(
    const QString&                     title,
    const QString&                     message,
    const std::vector<LayerTreeItem*>& layerItems,
    QWidget*                           in_parent)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _layerItems(layerItems)
{
    setWindowTitle(title);

    _rowsLayout = new QVBoxLayout();
    int margin = DPIScale(5) + DPIScale(20);
    _rowsLayout->setContentsMargins(margin, margin, margin, 0);
    _rowsLayout->setSpacing(DPIScale(8));
    _rowsLayout->setAlignment(Qt::AlignTop);

    if (!message.isEmpty()) {
        auto dialogMessage = new QLabel(message);
        _rowsLayout->addWidget(dialogMessage);
    }

    for (auto iter = _layerItems.crbegin(); iter != _layerItems.crend(); ++iter) {
        auto row = new SaveLayerPathRow(this, (*iter));
        _rowsLayout->addWidget(row);
    }

    // Ok/Cancel button area
    auto buttonsLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(buttonsLayout, DPIScale(20));
    buttonsLayout->addStretch();
    auto okButton = new QPushButton("Save Stage", this);
    connect(okButton, &QPushButton::clicked, this, &SaveLayersDialog::onSaveAll);
    okButton->setDefault(true);
    auto cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &SaveLayersDialog::onCancel);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    // setup a scroll area
    auto dialogContentParent = new QWidget();
    dialogContentParent->setLayout(_rowsLayout);

    auto scrollArea = new SaveLayerPathRowArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setWidget(dialogContentParent);
    scrollArea->setWidgetResizable(true);

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

    setSizeGripEnabled(true);
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

    for (i = 0, count = _rowsLayout->count(); i < count; ++i) {
        auto row = dynamic_cast<SaveLayerPathRow*>(_rowsLayout->itemAt(i)->widget());
        auto item = row ? row->_treeItem : nullptr;
        if (nullptr == item)
            continue;

        QString path = row->pathToSaveAs();
        if (!path.isEmpty()) {
            pxr::SdfFileFormat::FileFormatArguments formatArgs;
            formatArgs["format"] = row->usdFormatTag().toStdString();

            auto sessionState = item->parentModel()->sessionState();
            auto sdfLayer = item->layer();
            auto qFileName = row->absolutePath();
            auto sFileName = qFileName.toStdString();

            sdfLayer->Export(sFileName, "", formatArgs);

            if (item->isRootLayer()) {
                newRoot = sFileName;
                rootSessionState = sessionState;
            } else {
                // now replace the layer in the parent
                auto parentItem = item->parentLayerItem();
                auto newLayer = SdfLayer::FindOrOpen(sFileName);
                if (newLayer) {
                    bool setTarget = item->isTargetLayer();
                    parentItem->layer()->GetSubLayerPaths().Replace(
                        sdfLayer->GetIdentifier(), newLayer->GetIdentifier());
                    if (setTarget) {
                        sessionState->stage()->SetEditTarget(newLayer);
                    }

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

    accept();
}

void SaveLayersDialog::onCancel() { reject(); }

bool SaveLayersDialog::okToSave()
{
    int         i, count;
    QStringList existingFiles;

    for (i = 0, count = _rowsLayout->count(); i < count; ++i) {
        auto row = dynamic_cast<SaveLayerPathRow*>(_rowsLayout->itemAt(i)->widget());
        auto item = row ? row->_treeItem : nullptr;
        if (nullptr == item)
            continue;

        QString path = row->pathToSaveAs();
        if (!path.isEmpty()) {
            QFileInfo fInfo(path);
            if (fInfo.exists()) {
                existingFiles.append(path);
            }
        }
    }

    if (!existingFiles.isEmpty()) {
        QString message
            = QString("%1 file(s) already exist and will be overwritten.  Do you want to continue?")
                  .arg(existingFiles.length());
        return (confirmDialog("Overwrite Files", message, &existingFiles));
    }

    return true;
}

} // namespace UsdLayerEditor
