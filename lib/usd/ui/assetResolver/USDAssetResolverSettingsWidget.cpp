//
// Copyright 2025 Autodesk
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

#include "USDAssetResolverSettingsWidget.h"

#include "ApplicationHost.h"
#include "HeaderWidget.h"
#include "ui_USDAssetResolverSettingsWidget.h"

#include <qaction.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qlistview.h>
#include <qlistwidget.h>
#include <qpainter.h>
#include <qsplitter.h>
#include <qstringlistmodel.h>
#include <qstyleditemdelegate.h>
#include <qtoolbutton.h>

namespace Adsk {

class ListPanel : public QWidget
{
public:
    ListPanel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        const auto& host = ApplicationHost::instance();
        const auto  update_me = [this]() { this->update(); };
        connect(&host, &ApplicationHost::themeColorsChanged, this, update_me);
        connect(&host, &ApplicationHost::uiScaleChanged, this, update_me);
    }

    void paintEvent(QPaintEvent* event) override
    {
        const auto& host = ApplicationHost::instance();
        QPainter    painter(this);
        painter.setPen(host.themeColor(ApplicationHost::ThemeColors::ListBorder));
        QRect     r = rect();
        const int tiny_padding = host.pm(ApplicationHost::PixelMetric::TinyPadding);
        r.adjust(tiny_padding, 0, -(tiny_padding + 1), -1);
        painter.drawRect(r);
    }
};

class ListView : public QListView
{
public:
    ListView(QWidget* parent = nullptr)
        : QListView(parent)
    {
        setMouseTracking(true);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QListView::mouseMoveEvent(event);
        // We need to update the rect to enable the hovering for the "painted"
        // buttons inside the items.
        QRect r = visualRect(indexAt(event->pos()));
        viewport()->update(r);
    }

    void paintEvent(QPaintEvent* e) override
    {
        QListView::paintEvent(e);
        if (model() && model()->rowCount(rootIndex()) > 0)
            return;
        // The view is empty.
        QPainter p(this->viewport());
        p.drawText(rect(), Qt::AlignCenter, tr("No paths"));
    }
};

class StringListModel : public QStringListModel
{
public:
    StringListModel(QObject* parent = nullptr)
        : QStringListModel(parent)
    {
    }
    StringListModel(const QStringList& strings, QObject* parent = nullptr)
        : QStringListModel(strings, parent)
    {
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QStringListModel::flags(index);
        if (index.isValid()) {
            // cannot drop on items - only between them
            f.setFlag(Qt::ItemFlag::ItemIsDropEnabled, false);
        }
        return f;
    }
};

class ListPanelItemDelegate : public QStyledItemDelegate
{
public:
    ListPanelItemDelegate(QListView* parent = nullptr, bool editable = false)
        : QStyledItemDelegate(parent)
        , editable(editable)
        , listview(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(ApplicationHost::instance().pm(ApplicationHost::PixelMetric::ItemHeight));
        return size;
    }

    void updateEditorGeometry(
        QWidget*                    editor,
        const QStyleOptionViewItem& option,
        const QModelIndex&          index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        editor->setGeometry(opt.rect);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)
        const override
    {
        if (editable && option.state.testFlag(QStyle::State_MouseOver)
            && !option.state.testFlag(QStyle::State_Editing)) {

            const auto& host = ApplicationHost::instance();
            const int   itemHeight = host.pm(ApplicationHost::PixelMetric::ItemHeight);
            const int   tinyPadding = host.pm(ApplicationHost::PixelMetric::TinyPadding);

            const int s = itemHeight - tinyPadding * 2;

            // we may need to elide the text to make room for the buttons
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            QFontMetrics fm(option.font);
            opt.text
                = fm.elidedText(opt.text, opt.textElideMode, opt.rect.width() - (itemHeight * 2));
            QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

            QPoint cursorPos = listview->mapFromGlobal(QCursor::pos());

            QStyleOptionToolButton buttonOption;
            buttonOption.rect
                = QRect(option.rect.right() - itemHeight, option.rect.top() + tinyPadding, s, s);
            buttonOption.state = QStyle::State_Enabled;
            buttonOption.state.setFlag(
                QStyle::State_MouseOver, buttonOption.rect.contains(cursorPos));
            buttonOption.icon = host.icon(ApplicationHost::IconName::Delete);
            buttonOption.iconSize = QSize(s - tinyPadding * 2, s - tinyPadding * 2);
            buttonOption.arrowType = Qt::NoArrow;
            buttonOption.activeSubControls = QStyle::SC_None;
            buttonOption.subControls = QStyle::SC_ToolButton;
            buttonOption.toolButtonStyle = Qt::ToolButtonIconOnly;
            buttonOption.font = option.font;
            buttonOption.palette = option.palette;
            buttonOption.direction = option.direction;

            style->drawComplexControl(QStyle::CC_ToolButton, &buttonOption, painter, listview);

            buttonOption.rect.moveLeft(buttonOption.rect.left() - (itemHeight - tinyPadding));
            buttonOption.icon = host.icon(ApplicationHost::IconName::OpenFile);
            buttonOption.state.setFlag(
                QStyle::State_MouseOver, buttonOption.rect.contains(cursorPos));

            style->drawComplexControl(QStyle::CC_ToolButton, &buttonOption, painter, listview);
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    bool editorEvent(
        QEvent*                     event,
        QAbstractItemModel*         model,
        const QStyleOptionViewItem& option,
        const QModelIndex&          index) override
    {
        if (editable && !option.state.testFlag(QStyle::State_Editing)) {
            if (event->type() == QEvent::MouseButtonRelease) {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                const auto&  host = ApplicationHost::instance();
                const int    itemHeight = host.pm(ApplicationHost::PixelMetric::ItemHeight);
                const int    tinyPadding = host.pm(ApplicationHost::PixelMetric::TinyPadding);
                const int    s = itemHeight - tinyPadding * 2;
                QRect        delButtonRect(
                    option.rect.right() - itemHeight, option.rect.top() + tinyPadding, s, s);
                if (delButtonRect.contains(me->pos())) {
                    // Remove this item
                    model->removeRow(index.row(), index.parent());
                    return true;
                }
                QRect browseButtonRect(
                    option.rect.right() - itemHeight * 2 + tinyPadding,
                    option.rect.top() + tinyPadding,
                    s,
                    s);
                if (browseButtonRect.contains(me->pos())) {
                    // Browse for a new path
                    QString dir = QFileDialog::getExistingDirectory(
                        listview,
                        tr("Select Directory"),
                        model->data(index, Qt::DisplayRole).toString(),
                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
                    if (!dir.isEmpty()) {
                        model->setData(index, dir, Qt::EditRole);
                    }
                    return true;
                }
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

private:
    bool       editable = false;
    QListView* listview = nullptr;
};

class USDAssetResolverSettingsWidgetPrivate
{
public:
    USDAssetResolverSettingsWidgetPrivate(Ui::USDAssetResolverSettingsWidget* ui)
        : ui(ui)
    {
    }

    std::unique_ptr<Ui::USDAssetResolverSettingsWidget> ui = nullptr;

    QString    mappingFilePath;
    QSplitter* searchPathsSplitter = nullptr;

    QToolButton* userPathsFirstButton = nullptr;
    bool         userPathsFirst = true;

    QCheckBox* userPathsOnlyCheckBox = nullptr;
    bool       userPathsOnly = false;

    HeaderWidget*    userPathsHeader = nullptr;
    QLabel*          userPathsHeaderLabel = nullptr;
    QStringList      userPaths;
    StringListModel* userPathsModel = nullptr;

    HeaderWidget*     extAndEnvPathsHeader = nullptr;
    QWidget*          extAndEnvPathsWidget = nullptr;
    QStringList       extAndEnvPaths;
    QStringListModel* extAndEnvPathsModel = nullptr;

    QModelIndex currentlyAddingNewUserPath;
    bool        aboutToAddUserPath = false;
};

USDAssetResolverSettingsWidget::USDAssetResolverSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , d_ptr(new USDAssetResolverSettingsWidgetPrivate(new Ui::USDAssetResolverSettingsWidget()))
{
    Q_D(USDAssetResolverSettingsWidget);

    d->ui->setupUi(this);
    const auto& host = ApplicationHost::instance();
    d->ui->mainLayout->setColumnMinimumWidth(0, qRound(host.uiScale() * 100.0f));

    // Add a browse action to the mapping file path line edit
    QAction* browseAction
        = new QAction(host.icon(ApplicationHost::IconName::OpenFile), tr("Browse..."), this);
    connect(&host, &ApplicationHost::iconsChanged, this, [browseAction]() {
        browseAction->setIcon(
            ApplicationHost::instance().icon(ApplicationHost::IconName::OpenFile));
    });

    browseAction->setToolTip(tr("Browse for a mapping file"));
    connect(browseAction, &QAction::triggered, this, [this, d]() {
        QString filePath = QFileDialog::getOpenFileName(
            this, tr("Select Mapping File"), QString(), tr("USD Files (*.usda);;All Files (*.*)"));
        if (!filePath.isEmpty()) {
            if (filePath != d->mappingFilePath) {
                d->ui->mappingFilePath->setText(filePath);
                d->mappingFilePath = filePath;
                Q_EMIT mappingFilePathChanged(filePath);
            }
        }
    });

    connect(d->ui->mappingFilePath, &QLineEdit::editingFinished, this, [this, d]() {
        QString text = d->ui->mappingFilePath->text();
        if (text != d->mappingFilePath) {
            d->mappingFilePath = text;
            Q_EMIT mappingFilePathChanged(text);
        }
    });

    d->ui->mappingFilePath->addAction(browseAction, QLineEdit::ActionPosition::TrailingPosition);

    connect(
        d->ui->includeProjectTokens,
        &QCheckBox::toggled,
        this,
        &USDAssetResolverSettingsWidget::includeProjectTokensChanged);

    const int tiny_padding = host.pm(ApplicationHost::PixelMetric::TinyPadding);

    // User and System search paths (collapseable section)
    auto user_paths = new ListPanel(this);
    {
        auto layout = new QVBoxLayout(user_paths);
        layout->setContentsMargins(tiny_padding + 1, 0, tiny_padding + 1, 0);
        layout->setSpacing(0);
        // we're not using the title of the header widget here - as the buttons
        // may overlap it - instead we use a simple label for the title.
        d->userPathsHeader = new HeaderWidget("", user_paths);
        layout->addWidget(d->userPathsHeader, 0);
        auto headerLayout = new QHBoxLayout(d->userPathsHeader);
        d->userPathsHeaderLabel
            = new QLabel(tr("%1 User Paths").arg(userPathsFirst() ? tr("1.") : tr("2.")));
        headerLayout->addWidget(d->userPathsHeaderLabel, 1);
        headerLayout->addSpacing(tiny_padding);
        headerLayout->setContentsMargins(tiny_padding, 0, tiny_padding, 0);

        auto listview = new ListView(user_paths);
        listview->setUniformItemSizes(true);
        listview->setItemDelegate(new ListPanelItemDelegate(listview, true));
        listview->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        listview->setVerticalScrollMode(QListView::ScrollMode::ScrollPerPixel);
        listview->setTextElideMode(Qt::TextElideMode::ElideMiddle);
        listview->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        listview->setContentsMargins(1, 0, 1, 1);
        d->userPathsModel = new StringListModel(d->userPaths, this);
        listview->setModel(d->userPathsModel);

        listview->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
        listview->setDropIndicatorShown(true);
        listview->setDragDropOverwriteMode(false);

        auto addButton = new QToolButton(d->userPathsHeader);
        addButton->setIcon(ApplicationHost::instance().icon(ApplicationHost::IconName::Add));
        addButton->setToolTip(tr("Add User Path"));
        connect(addButton, &QToolButton::clicked, this, [this, listview]() {
            Q_D(USDAssetResolverSettingsWidget);
            d->aboutToAddUserPath = true;
            if (d->userPathsModel->insertRow(d->userPathsModel->rowCount())) {
                d->currentlyAddingNewUserPath
                    = d->userPathsModel->index(d->userPathsModel->rowCount() - 1);
                d->userPathsModel->setData(d->currentlyAddingNewUserPath, tr(""));
                listview->scrollTo(d->currentlyAddingNewUserPath);
                listview->setFocus();
                listview->edit(d->currentlyAddingNewUserPath);
                listview->update();
            } else {
                // TODO: show error?
                d->aboutToAddUserPath = false;
            }
        });

        connect(
            listview->itemDelegate(),
            &QAbstractItemDelegate::closeEditor,
            this,
            [this](QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
                Q_D(USDAssetResolverSettingsWidget);
                if (d->currentlyAddingNewUserPath.isValid()) {
                    QModelIndex index = d->currentlyAddingNewUserPath;
                    if (hint == QAbstractItemDelegate::RevertModelCache) {
                        d->userPathsModel->removeRow(index.row());
                        d->currentlyAddingNewUserPath = QModelIndex();
                    }
                    d->currentlyAddingNewUserPath = QModelIndex();
                    d->aboutToAddUserPath = false;
                    Q_EMIT d->userPathsModel->dataChanged(index, index);
                }
            });

        headerLayout->addWidget(addButton);

        auto addBrowseButton = new QToolButton(d->userPathsHeader);
        addBrowseButton->setIcon(host.icon(ApplicationHost::IconName::OpenFile));
        addBrowseButton->setToolTip(tr("Add User Path with the browser"));
        connect(addBrowseButton, &QToolButton::clicked, this, [this]() {
            Q_D(USDAssetResolverSettingsWidget);
            QString filePath
                = QFileDialog::getExistingDirectory(this, tr("Select User Path to Add"));
            if (!filePath.isEmpty()) {
                if (d->userPathsModel->insertRow(d->userPathsModel->rowCount())) {
                    QModelIndex index = d->userPathsModel->index(d->userPathsModel->rowCount() - 1);
                    d->userPathsModel->setData(index, filePath);
                    Q_EMIT d->userPathsModel->dataChanged(index, index);
                }
            }
        });

        headerLayout->addWidget(addBrowseButton);

        d->userPathsFirstButton = new QToolButton(d->userPathsHeader);
        d->userPathsFirstButton->setIcon(ApplicationHost::instance().icon(
            userPathsFirst() ? ApplicationHost::IconName::MoveDown
                             : ApplicationHost::IconName::MoveUp));
        connect(d->userPathsFirstButton, &QToolButton::clicked, this, [this]() {
            setUserPathsFirst(!userPathsFirst());
        });
        headerLayout->addWidget(d->userPathsFirstButton);

        auto line = new QFrame(d->userPathsHeader);
        line->setFrameShape(QFrame::VLine);
        headerLayout->addWidget(line);

        d->userPathsOnlyCheckBox = new QCheckBox(tr("User Paths Only"), d->userPathsHeader);
        connect(d->userPathsOnlyCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
            setUserPathsOnly(checked);
        });
        headerLayout->addWidget(d->userPathsOnlyCheckBox);

        layout->addWidget(listview, 1);

        connect(d->userPathsModel, &QStringListModel::dataChanged, this, [this]() {
            Q_D(USDAssetResolverSettingsWidget);
            if (d->aboutToAddUserPath) {
                return; // ignore the first change, which is from the initial empty string
            }
            if (d->userPaths != d->userPathsModel->stringList()) {
                d->userPaths = d->userPathsModel->stringList();
                Q_EMIT userPathsChanged(d->userPaths);
            }
        });
        connect(d->userPathsModel, &QStringListModel::rowsMoved, this, [this]() {
            Q_D(USDAssetResolverSettingsWidget);
            if (d->aboutToAddUserPath) {
                return; // ignore the first change, which is from the initial empty string
            }
            if (d->userPaths != d->userPathsModel->stringList()) {
                d->userPaths = d->userPathsModel->stringList();
                Q_EMIT userPathsChanged(d->userPaths);
            }
        });
        connect(d->userPathsModel, &QStringListModel::rowsRemoved, this, [this]() {
            Q_D(USDAssetResolverSettingsWidget);
            if (d->aboutToAddUserPath) {
                return; // ignore the first change, which is from the initial empty string
            }
            if (d->userPaths != d->userPathsModel->stringList()) {
                d->userPaths = d->userPathsModel->stringList();
                Q_EMIT userPathsChanged(d->userPaths);
            }
        });
    }

    d->extAndEnvPathsWidget = new ListPanel(this);
    {
        auto layout = new QVBoxLayout(d->extAndEnvPathsWidget);
        layout->setContentsMargins(tiny_padding + 1, 0, tiny_padding + 1, 0);
        layout->setSpacing(0);
        d->extAndEnvPathsHeader = new HeaderWidget(
            tr("%1 Extension & Environment Paths").arg(userPathsFirst() ? tr("2.") : tr("1.")),
            d->extAndEnvPathsWidget);
        layout->addWidget(d->extAndEnvPathsHeader, 0);

        auto listview = new ListView(d->extAndEnvPathsWidget);
        layout->addWidget(listview, 1);
        listview->setUniformItemSizes(true);
        listview->setItemDelegate(new ListPanelItemDelegate(listview));
        listview->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        listview->setVerticalScrollMode(QListView::ScrollMode::ScrollPerPixel);
        listview->setTextElideMode(Qt::TextElideMode::ElideMiddle);
        listview->setContentsMargins(1, 0, 1, 1);
        // no editing
        listview->setEditTriggers(QAbstractItemView::NoEditTriggers);
        listview->setSelectionMode(QAbstractItemView::NoSelection);
        d->extAndEnvPathsModel = new QStringListModel(d->extAndEnvPaths, this);
        listview->setModel(d->extAndEnvPathsModel);
    }

    d->searchPathsSplitter = new QSplitter(Qt::Orientation::Vertical);
    d->searchPathsSplitter->setChildrenCollapsible(false);
    d->searchPathsSplitter->setHandleWidth(tiny_padding * 8);

    d->searchPathsSplitter->addWidget(user_paths);
    d->searchPathsSplitter->addWidget(d->extAndEnvPathsWidget);

    auto collapseable
        = ApplicationHost::instance().wrapWithCollapseable("Search Paths", d->searchPathsSplitter);

    d->ui->mainLayout->addWidget(collapseable, 2, 0, 1, 2);
    connect(d->ui->saveButton, &QPushButton::clicked, this, [this]() { Q_EMIT saveRequested(); });
    connect(d->ui->closeButton, &QPushButton::clicked, this, [this]() { Q_EMIT closeRequested(); });
}

USDAssetResolverSettingsWidget::~USDAssetResolverSettingsWidget() = default;

QString USDAssetResolverSettingsWidget::mappingFilePath() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->mappingFilePath;
}

void USDAssetResolverSettingsWidget::setMappingFilePath(const QString& path)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (path != d->mappingFilePath) {
        d->mappingFilePath = path;
        d->ui->mappingFilePath->setText(path);
        Q_EMIT mappingFilePathChanged(path);
    }
}

bool USDAssetResolverSettingsWidget::includeProjectTokens() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->ui->includeProjectTokens->isChecked();
}

void USDAssetResolverSettingsWidget::setIncludeProjectTokens(bool include)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (include != d->ui->includeProjectTokens->isChecked()) {
        d->ui->includeProjectTokens->setChecked(include);
    }
}

bool USDAssetResolverSettingsWidget::userPathsFirst() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->userPathsFirst;
}

void USDAssetResolverSettingsWidget::setUserPathsFirst(bool userPathsFirst)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (userPathsFirst != d->userPathsFirst) {
        d->userPathsFirst = userPathsFirst;
        d->userPathsHeaderLabel->setText(
            tr("%1 User Paths").arg(userPathsFirst ? tr("1.") : tr("2.")));
        d->extAndEnvPathsHeader->setTitle(
            tr("%1 Extension & Environment Paths").arg(userPathsFirst ? tr("2.") : tr("1.")));
        d->userPathsFirstButton->setIcon(ApplicationHost::instance().icon(
            userPathsFirst ? ApplicationHost::IconName::MoveDown
                           : ApplicationHost::IconName::MoveUp));
        // swap items in the layout
        d->searchPathsSplitter->insertWidget(
            userPathsFirst ? 0 : 1, d->userPathsHeader->parentWidget());
        Q_EMIT userPathsFirstChanged(userPathsFirst);
    }
}

bool USDAssetResolverSettingsWidget::userPathsOnly() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->userPathsOnly;
}

void USDAssetResolverSettingsWidget::setUserPathsOnly(bool userPathsOnly)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (userPathsOnly != d->userPathsOnly) {
        d->userPathsOnly = userPathsOnly;
        d->userPathsOnlyCheckBox->setChecked(userPathsOnly);
        d->extAndEnvPathsWidget->setDisabled(userPathsOnly);
        Q_EMIT userPathsOnlyChanged(userPathsOnly);
    }
}

QStringList USDAssetResolverSettingsWidget::extAndEnvPaths() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->extAndEnvPaths;
}

void USDAssetResolverSettingsWidget::setExtAndEnvPaths(const QStringList& paths)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (paths != d->extAndEnvPaths) {
        d->extAndEnvPaths = paths;
        d->extAndEnvPathsModel->setStringList(paths);
        Q_EMIT extAndEnvPathsChanged(paths);
    }
}

QStringList USDAssetResolverSettingsWidget::userPaths() const
{
    Q_D(const USDAssetResolverSettingsWidget);
    return d->userPaths;
}

void USDAssetResolverSettingsWidget::setUserPaths(const QStringList& paths)
{
    Q_D(USDAssetResolverSettingsWidget);
    if (paths != d->userPaths) {
        d->userPaths = paths;
        d->userPathsModel->setStringList(paths);
        Q_EMIT userPathsChanged(paths);
    }
}

} // namespace Adsk