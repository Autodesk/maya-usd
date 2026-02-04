//
// Copyright 2019 Autodesk
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
#include "USDImportDialog.h"

#include "ui_USDImportDialog.h"

#include <mayaUsdUI/ui/IMayaMQtUtil.h>
#include <mayaUsdUI/ui/TreeModelFactory.h>

#include <maya/MGlobal.h>

#include <stdexcept>

namespace MAYAUSD_NS_DEF {

// We need an implementation because the derived class invokes the destructor
// and without it we have an undefined symbol.
IUSDImportView::~IUSDImportView() { }

USDImportDialog::USDImportDialog(
    const std::string&            filename,
    const ImportData*             importData,
    const USDImportDialogOptions& options,
    const IMayaMQtUtil&           mayaQtUtil,
    QWidget*                      parent /*= nullptr*/)
    : QDialog { parent }
    , _options(options)
    , _uiView { new Ui::ImportDialog() }
    , _stage { UsdStage::Open(filename, UsdStage::InitialLoadSet::LoadAll) }
    , _filename { filename }
    , _rootPrimPath("/")
{
    if (!_stage)
        throw std::invalid_argument("Invalid filename passed to USD Import Dialog");

    _uiView->setupUi(this);
    applyOptions();

    // If we were given some import data we will only use it if it matches our
    // input filename. In the case where the user opened dialog, clicked Apply and
    // then reopens the dialog we want to reset the dialog to the previous state.
    const ImportData* matchingImportData = nullptr;
    if ((importData != nullptr) && (_filename == importData->filename())
        && importData->rootPrimPath().size() > 0) {
        _rootPrimPath = importData->rootPrimPath();
        matchingImportData = importData;
    } else if (!_options.showRoot && _stage) {
        UsdPrim defPrim = _stage->GetDefaultPrim();
        if (defPrim.IsValid())
            _rootPrimPath = defPrim.GetPath().GetAsString();
    }

#if QT_DISABLE_DEPRECATED_BEFORE || QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int minW = _uiView->nbPrimsInScopeLabel->fontMetrics().horizontalAdvance("12345");
#else
    int minW = _uiView->nbPrimsInScopeLabel->fontMetrics().width("12345");
#endif

    _uiView->nbPrimsInScopeLabel->setMinimumWidth(minW);
    _uiView->nbVariantsChangedLabel->setMinimumWidth(minW);

    // These calls must come after the UI is initialized via "setupUi()":
    int nbItems = 0;
    _treeModel = TreeModelFactory::createFromStage(
        _stage, mayaQtUtil, matchingImportData, _options, this, &nbItems);
    _proxyModel = std::unique_ptr<QSortFilterProxyModel>(new QSortFilterProxyModel(this));
    QObject::connect(
        _treeModel.get(), SIGNAL(checkedStateChanged(int)), this, SLOT(onCheckedStateChanged(int)));
    QObject::connect(
        _treeModel.get(),
        SIGNAL(modifiedVariantCountChanged(int)),
        this,
        SLOT(onModifiedVariantsChanged(int)));

    // Set the root prim path in the tree model. This will set the default check states.
    _treeModel->setRootPrimPath(_rootPrimPath);

    // Configure the TreeView of the dialog:
    _proxyModel->setSourceModel(_treeModel.get());
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    _proxyModel->setRecursiveFilteringEnabled(true);
#endif
    _proxyModel->setDynamicSortFilter(false);
    _proxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    _uiView->treeView->setModel(_proxyModel.get());
    _uiView->treeView->setTreePosition(TreeItem::kColumnName);
    _uiView->treeView->setAlternatingRowColors(true);
    _uiView->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    QObject::connect(
        _uiView->treeView,
        SIGNAL(clicked(const QModelIndex&)),
        this,
        SLOT(onItemClicked(const QModelIndex&)));
    QObject::connect(
        _uiView->actionReset_File, SIGNAL(triggered(bool)), this, SLOT(onResetFileTriggered()));
    QObject::connect(
        _uiView->actionHelp_on_Hierarchy_View,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onHierarchyViewHelpTriggered()));

    QHeaderView* header = _uiView->treeView->header();

    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    _itemDelegate = std::unique_ptr<ItemDelegate>(new ItemDelegate(_uiView->treeView));
    QObject::connect(
        _itemDelegate.get(),
        SIGNAL(variantModified()),
        _treeModel.get(),
        SLOT(updateModifiedVariantCount()));

    // Set our item delegate on the treeview so we can customize the drawing of
    // the variant sets.
    _uiView->treeView->setItemDelegate(_itemDelegate.get());

    // Must be done AFTER we set our item delegate
    _treeModel->openPersistentEditors(_uiView->treeView, QModelIndex());

    // This request to expand the tree to a default depth of 3 should come after the creation
    // of the editors since it can trigger calls to things like sizeHint before we've put any of
    // the variant set UI in place.
    _uiView->treeView->expandToDepth(3);

    // Set some initial widths for the tree view columns.
    const int     kLoadWidth = mayaQtUtil.dpiScale(25);
    constexpr int kTypeWidth = 120;
    constexpr int kNameWidth = 500;
    header->setMinimumSectionSize(kLoadWidth);
    header->resizeSection(TreeItem::kColumnLoad, kLoadWidth);
    header->resizeSection(TreeItem::kColumnName, kNameWidth);
    if (_options.showVariants) {
        header->resizeSection(TreeItem::kColumnType, kTypeWidth);
    }
    header->setSectionResizeMode(0, QHeaderView::Fixed);

    // Display the full path of the file to import:
    _uiView->usdFilePath->setText(QString::fromStdString(_filename));

    // Make sure the "Import" button is enabled.
    _uiView->applyButton->setEnabled(true);
}

void USDImportDialog::applyOptions()
{
    if (_options.title.size() > 0) {
        setWindowTitle(_options.title.c_str());
    }

    if (_options.helpLabel.size() > 0) {
        _uiView->actionHelp_on_Hierarchy_View->setText(
            QCoreApplication::translate("ImportDialog", _options.helpLabel.c_str(), nullptr));
    }

    _uiView->nbVariantsChanged->setVisible(_options.showVariants);
    _uiView->nbVariantsChangedLabel->setVisible(_options.showVariants);

    _uiView->selectPrims->setVisible(_options.showHeaderMessage);
}

USDImportDialog::~USDImportDialog()
{
    // Note: the destructor is needed here so we can forward declare the Ui::ImportDialog
    //		 and only include "ui_USDImportDialog.h" here in the .cpp (instead of .h).
}

bool USDImportDialog::execute()
{
    // Fixup the palette by changing the alternate base color which is used for
    // painting the tree alternate rows.
    // TODO - there is a small problem here in that this works, but not until you
    //		  force the tree to redraw by clicking in it.
    // 	QPalette treePal = fUI->treeView->palette();
    // 	treePal.setBrush(QPalette::Active, QPalette::AlternateBase, treePal.color(QPalette::Active,
    // QPalette::Base).lighter(130)); 	fUI->treeView->setPalette(treePal);
    return exec() == QDialog::Accepted;
}

const std::string& USDImportDialog::filename() const { return _filename; }

const std::string& USDImportDialog::rootPrimPath() const
{
    std::string rootPrimPath;
    _treeModel->getRootPrimPath(rootPrimPath, QModelIndex());
    if (!rootPrimPath.empty())
        _rootPrimPath = rootPrimPath;
    return _rootPrimPath;
}

UsdStagePopulationMask USDImportDialog::stagePopulationMask() const
{
    UsdStagePopulationMask stagePopulationMask;
    _treeModel->fillStagePopulationMask(stagePopulationMask, QModelIndex());
    return stagePopulationMask;
}

ImportData::PrimVariantSelections USDImportDialog::primVariantSelections() const
{
    ImportData::PrimVariantSelections varSels;
    _treeModel->fillPrimVariantSelections(varSels, QModelIndex());
    return varSels;
}

UsdStage::InitialLoadSet USDImportDialog::stageInitialLoadSet() const
{
    return UsdStage::InitialLoadSet::LoadAll;
}

void USDImportDialog::onItemClicked(const QModelIndex& index)
{
    TreeItem* item
        = static_cast<TreeItem*>(_treeModel->itemFromIndex(_proxyModel->mapToSource(index)));
    if (item != nullptr) {
        // When user checks a prim that is in a collapsed state, then that prim
        // gets checked-enabled and it expands to show its immediate children.
        _treeModel->onItemClicked(item);
        if (!_uiView->treeView->isExpanded(index)) {
            if (item->checkState() == TreeItem::CheckState::kChecked)
                _uiView->treeView->expand(index);
        }
    }
}

void USDImportDialog::onResetFileTriggered()
{
    if (nullptr != _treeModel) {
        _treeModel->resetVariants();
        _treeModel->setRootPrimPath(_rootPrimPath);
    }
}

void USDImportDialog::onHierarchyViewHelpTriggered()
{
    MString url = _options.helpURL.size() > 0 ? _options.helpURL.c_str() : "UsdHierarchyView";
    MString cmd;
    cmd.format("from mayaUsdUtils import showHelpMayaUSD; showHelpMayaUSD('^1s');", url);
    MGlobal::executePythonCommand(cmd);
}

void USDImportDialog::onCheckedStateChanged(int nbChecked)
{
    QString nbLabel;
    nbLabel.setNum(nbChecked);
    _uiView->nbPrimsInScopeLabel->setText(nbLabel);
}

void USDImportDialog::onModifiedVariantsChanged(int nbModified)
{
    QString nbLabel;
    nbLabel.setNum(nbModified);
    _uiView->nbVariantsChangedLabel->setText(nbLabel);
}

int USDImportDialog::primsInScopeCount() const
{
    return _uiView->nbPrimsInScopeLabel->text().toInt();
}

int USDImportDialog::switchedVariantCount() const
{
    return _uiView->nbVariantsChangedLabel->text().toInt();
}

} // namespace MAYAUSD_NS_DEF
