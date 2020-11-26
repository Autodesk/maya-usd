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
    const std::string&  filename,
    const ImportData*   importData,
    const IMayaMQtUtil& mayaQtUtil,
    QWidget*            parent /*= nullptr*/)
    : QDialog { parent }
    , fUI { new Ui::ImportDialog() }
    , fStage { UsdStage::Open(filename, UsdStage::InitialLoadSet::LoadNone) }
    , fFilename { filename }
    , fRootPrimPath("/")
{
    if (!fStage)
        throw std::invalid_argument("Invalid filename passed to USD Import Dialog");

    fUI->setupUi(this);

    // If we were given some import data we will only use it if it matches our
    // input filename. In the case where the user opened dialog, clicked Apply and
    // then reopens the dialog we want to reset the dialog to the previous state.
    const ImportData* matchingImportData = nullptr;
    if ((importData != nullptr) && (fFilename == importData->filename())) {
        fRootPrimPath = importData->rootPrimPath();
        matchingImportData = importData;
    }

    int minW = fUI->nbPrimsInScopeLabel->fontMetrics().width("12345");
    fUI->nbPrimsInScopeLabel->setMinimumWidth(minW);
    fUI->nbVariantsChangedLabel->setMinimumWidth(minW);

    // These calls must come after the UI is initialized via "setupUi()":
    int nbItems = 0;
    fTreeModel
        = TreeModelFactory::createFromStage(fStage, mayaQtUtil, matchingImportData, this, &nbItems);
    fProxyModel = std::unique_ptr<QSortFilterProxyModel>(new QSortFilterProxyModel(this));
    QObject::connect(
        fTreeModel.get(), SIGNAL(checkedStateChanged(int)), this, SLOT(onCheckedStateChanged(int)));
    QObject::connect(
        fTreeModel.get(),
        SIGNAL(modifiedVariantCountChanged(int)),
        this,
        SLOT(onModifiedVariantsChanged(int)));

    // Set the root prim path in the tree model. This will set the default check states.
    fTreeModel->setRootPrimPath(fRootPrimPath);

    // Configure the TreeView of the dialog:
    fProxyModel->setSourceModel(fTreeModel.get());
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    fProxyModel->setRecursiveFilteringEnabled(true);
#endif
    fProxyModel->setDynamicSortFilter(false);
    fProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    fUI->treeView->setModel(fProxyModel.get());
    fUI->treeView->setTreePosition(TreeModel::kTreeColumn_Name);
    fUI->treeView->setAlternatingRowColors(true);
    fUI->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    QObject::connect(
        fUI->treeView,
        SIGNAL(clicked(const QModelIndex&)),
        this,
        SLOT(onItemClicked(const QModelIndex&)));
    QObject::connect(
        fUI->actionReset_File, SIGNAL(triggered(bool)), this, SLOT(onResetFileTriggered()));
    QObject::connect(
        fUI->actionHelp_on_Hierarchy_View,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onHierarchyViewHelpTriggered()));

    QHeaderView* header = fUI->treeView->header();

    header->setStretchLastSection(true);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    fItemDelegate = std::unique_ptr<ItemDelegate>(new ItemDelegate(fUI->treeView));
    QObject::connect(
        fItemDelegate.get(),
        SIGNAL(variantModified()),
        fTreeModel.get(),
        SLOT(updateModifiedVariantCount()));

    // Set our item delegate on the treeview so we can customize the drawing of
    // the variant sets.
    fUI->treeView->setItemDelegate(fItemDelegate.get());

    // Must be done AFTER we set our item delegate
    fTreeModel->openPersistentEditors(fUI->treeView, QModelIndex());

    // This request to expand the tree to a default depth of 3 should come after the creation
    // of the editors since it can trigger calls to things like sizeHint before we've put any of
    // the variant set UI in place.
    fUI->treeView->expandToDepth(3);

    // Set some initial widths for the tree view columns.
    const int     kLoadWidth = mayaQtUtil.dpiScale(25);
    constexpr int kTypeWidth = 120;
    constexpr int kNameWidth = 500;
    header->setMinimumSectionSize(kLoadWidth);
    header->resizeSection(TreeModel::kTreeColumn_Load, kLoadWidth);
    header->resizeSection(TreeModel::kTreeColumn_Name, kNameWidth);
    header->resizeSection(TreeModel::kTreeColumn_Type, kTypeWidth);
    header->setSectionResizeMode(0, QHeaderView::Fixed);

    // Display the full path of the file to import:
    fUI->usdFilePath->setText(QString::fromStdString(fFilename));

    // Make sure the "Import" button is enabled.
    fUI->applyButton->setEnabled(true);
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

const std::string& USDImportDialog::filename() const { return fFilename; }

const std::string& USDImportDialog::rootPrimPath() const
{
    std::string rootPrimPath;
    fTreeModel->getRootPrimPath(rootPrimPath, QModelIndex());
    if (!rootPrimPath.empty())
        fRootPrimPath = rootPrimPath;
    return fRootPrimPath;
}

UsdStagePopulationMask USDImportDialog::stagePopulationMask() const
{
    UsdStagePopulationMask stagePopulationMask;
    fTreeModel->fillStagePopulationMask(stagePopulationMask, QModelIndex());
    return stagePopulationMask;
}

ImportData::PrimVariantSelections USDImportDialog::primVariantSelections() const
{
    ImportData::PrimVariantSelections varSels;
    fTreeModel->fillPrimVariantSelections(varSels, QModelIndex());
    return varSels;
}

UsdStage::InitialLoadSet USDImportDialog::stageInitialLoadSet() const
{
    return UsdStage::InitialLoadSet::LoadAll;
}

void USDImportDialog::onItemClicked(const QModelIndex& index)
{
    TreeItem* item
        = static_cast<TreeItem*>(fTreeModel->itemFromIndex(fProxyModel->mapToSource(index)));
    if (item != nullptr) {
        // When user checks a prim that is in a collapsed state, then that prim
        // gets checked-enabled and it expands to show its immediate children.
        fTreeModel->onItemClicked(item);
        if (!fUI->treeView->isExpanded(index)) {
            if (item->checkState() == TreeItem::CheckState::kChecked)
                fUI->treeView->expand(index);
        }
    }
}

void USDImportDialog::onResetFileTriggered()
{
    if (nullptr != fTreeModel) {
        fTreeModel->resetVariants();
        fTreeModel->setRootPrimPath("/");
    }
}

void USDImportDialog::onHierarchyViewHelpTriggered()
{
    MGlobal::executeCommand("showHelp \"UsdHierarchyView\"");
}

void USDImportDialog::onCheckedStateChanged(int nbChecked)
{
    QString nbLabel;
    nbLabel.setNum(nbChecked);
    fUI->nbPrimsInScopeLabel->setText(nbLabel);
}

void USDImportDialog::onModifiedVariantsChanged(int nbModified)
{
    QString nbLabel;
    nbLabel.setNum(nbModified);
    fUI->nbVariantsChangedLabel->setText(nbLabel);
}

int USDImportDialog::primsInScopeCount() const { return fUI->nbPrimsInScopeLabel->text().toInt(); }

int USDImportDialog::switchedVariantCount() const
{
    return fUI->nbVariantsChangedLabel->text().toInt();
}

} // namespace MAYAUSD_NS_DEF
