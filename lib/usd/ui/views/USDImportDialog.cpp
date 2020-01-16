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
#include "factories/TreeModelFactory.h"
//#include "TreeHeader.h"
#include "ItemDelegate.h"
#include "ui_USDImportDialog.h"

#include <stdexcept>

MAYAUSD_NS_DEF {

IUSDImportView::~IUSDImportView() { }

USDImportDialog::USDImportDialog(const std::string& filename, QWidget* parent)
	: QDialog{ parent }
	, fUI{ new Ui::ImportDialog() }
	, fStage{ UsdStage::Open(filename, UsdStage::InitialLoadSet::LoadNone) }
	, fFilename{ filename }
	, fRootPrimPath("/")
{
	if (!fStage)
		throw std::invalid_argument("Invalid filename passed to USD Import Dialog");

	fUI->setupUi(this);

	// WIP - the search buttons are not implemented yet, so disable them.
	fUI->findPrev->setHidden(true);
	fUI->findNext->setHidden(true);
	fUI->findAll->setHidden(true);

	// These calls must come after the UI is initialized via "setupUi()":
	int nbItems = 0;
	fTreeModel = TreeModelFactory::createFromStage(fStage, this, &nbItems);
	fProxyModel = std::unique_ptr<QSortFilterProxyModel>(new QSortFilterProxyModel(this));

	// Configure the TreeView of the dialog:
	fProxyModel->setSourceModel(fTreeModel.get());
	fProxyModel->setRecursiveFilteringEnabled(true);
	fProxyModel->setDynamicSortFilter(false);
	fProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	fUI->treeView->setModel(fProxyModel.get());
	fUI->treeView->expandToDepth(3);
	fUI->treeView->setTreePosition(TreeModel::kTreeColumn_Name);
	fUI->treeView->setAlternatingRowColors(true);

// 	// Create the Spinner overlay on top of the TreeView, once it is configured:
// 	fOverlay = std::unique_ptr<SpinnerOverlayWidget>(new SpinnerOverlayWidget(fUI->treeView));

	// WIP - The header checkbox is not yet hooked up to anything so keep using 
	//		 the normal Qt header view for now.
//	TreeHeader* header = new TreeHeader(Qt::Horizontal, this);
//	fUI->treeView->setHeader(header);	// Tree view takes ownership of header and deletes it.
	QHeaderView* header = fUI->treeView->header();

	header->setStretchLastSection(true);
	header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	// Set our item delegate on the treeview so we can customize the drawing of
	// the variant sets.
	fUI->treeView->setItemDelegate(new ItemDelegate(fUI->treeView));

	// Must be done AFTER we set our item delegate
	fTreeModel->openPersistentEditors(fUI->treeView, QModelIndex());

	// Use the same width for the NAME column of the TreeView as the 1.8 * width of the "filter" text box above it:
	constexpr int kLoadWidth = 25;
	constexpr int kTypeWidth = 120;
	constexpr int kNameWidth = 500;
	header->setMinimumSectionSize(kLoadWidth);
	header->resizeSection(TreeModel::kTreeColumn_Load, kLoadWidth);
	header->resizeSection(TreeModel::kTreeColumn_Name, kNameWidth);
	header->resizeSection(TreeModel::kTreeColumn_Type, kTypeWidth);

	// Display the full path of the file to import:
	fUI->usdFilePath->setText(QString::fromStdString(fFilename));

	// Display the number of prims:
	QString nbPrimText = fUI->primCount->text().arg(nbItems);
	fUI->primCount->setText(nbPrimText);

	connect(fUI->filterLineEdit, &QLineEdit::textChanged, this, &USDImportDialog::onSearchFilterChanged);
	connect(fUI->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
			&USDImportDialog::onTreeViewSelectionChanged);

	// Select the first row by default:
	fUI->treeView->setCurrentIndex(fProxyModel->index(0, 0));
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
// 	treePal.setBrush(QPalette::Active, QPalette::AlternateBase, treePal.color(QPalette::Active, QPalette::Base).lighter(130));
// 	fUI->treeView->setPalette(treePal);
	return exec() == QDialog::Accepted;
}

std::string USDImportDialog::filename() const
{
	return fFilename;
}

std::string USDImportDialog::rootPrimPath() const
{
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
	return fUI->loadPayloads->isChecked() ? UsdStage::InitialLoadSet::LoadAll
										  : UsdStage::InitialLoadSet::LoadNone;
}

void USDImportDialog::onSearchFilterChanged(const QString& searchFilter)
{
	// Stop any search that was already ongoing but that has not yet completed:
	if (fSearchThread != nullptr && !fSearchThread->isFinished())
	{
		fSearchThread->quit();
		fSearchThread->wait();
	}

	// Create a timer that will display a Spinner if the search has been ongoing for a (small) amount of time, to let
	// the User know that a background task is ongoing and that the application is not frozen:
	fSearchTimer = std::unique_ptr<QTimer>(new QTimer(this));
	fSearchTimer->setSingleShot(true);
	connect(fSearchTimer.get(), &QTimer::timeout, fSearchTimer.get(), [this]() {
		fUI->treeView->setEnabled(false);
//		fOverlay->StartSpinning();
	});
	fSearchTimer->start(std::chrono::milliseconds(125));


	// Create a thread to perform a search for the given criteria in the background in order to maintain a responsive
	// UI that continues accepting input from the User:
	fSearchThread = std::unique_ptr<USDSearchThread>(new USDSearchThread(fStage, searchFilter.toStdString()));
	connect(fSearchThread.get(), &USDSearchThread::finished, fSearchThread.get(), [&, this]() {
		// Since results have been received, discard the timer that was waiting for results so that the Spinner Widget
		// is not displayed:
		fSearchTimer->stop();

		// Set the search results as the new effective data:
		fTreeModel = std::move(fSearchThread->consumeResults());
		fProxyModel->setSourceModel(fTreeModel.get());

		// Set the View to a sensible state to reflect the new data:
		bool searchYieledResults = fProxyModel->hasChildren();
		fUI->treeView->expandAll();
		fUI->treeView->selectionModel()->clearSelection();
		fUI->treeView->setEnabled(searchYieledResults);
		fUI->applyButton->setEnabled(false);
// 		if (searchYieledResults)
// 		{
// 			fOverlay->Hide();
// 		}
// 		else
// 		{
// 			fOverlay->ShowInformationMessage(tr("Your search did not match any Prim."));
// 		}
	});
	fSearchThread->start(QThread::Priority::TimeCriticalPriority);
}

void USDImportDialog::onTreeViewSelectionChanged(
		const QItemSelection& selectedItems, const QItemSelection& deselectedItems)
{
	// Ensure that items cannot be deselected from the TreeView, to avoid being in a state where no item of the
	// hierarchy from which to import is selected.
	//
	// Note that Qt does not trigger "selectionChanged" signals when changing selection from within the propagation
	// chain, so this will not cause an infinite callback loop.
	QItemSelectionModel* selectionModel = fUI->treeView->selectionModel();
	if (selectionModel != nullptr)
	{
		bool selectionIsEmpty = selectionModel->selection().isEmpty();
		if (selectionIsEmpty)
		{
			selectionModel->select(deselectedItems, QItemSelectionModel::Select);
		}

		// Make sure the "Import" button is disabled if no item of the Tree is selected.
		fUI->applyButton->setEnabled(!selectionIsEmpty);
	}
}

} // namespace MayaUsd
