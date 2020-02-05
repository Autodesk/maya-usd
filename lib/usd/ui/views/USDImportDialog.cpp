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
#include "ItemDelegate.h"
#include "ui_USDImportDialog.h"

#include <stdexcept>

MAYAUSD_NS_DEF {

// We need an implementation because the derived class invokes the destructor
// and without it we have an undefined symbol.
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
// 	treePal.setBrush(QPalette::Active, QPalette::AlternateBase, treePal.color(QPalette::Active, QPalette::Base).lighter(130));
// 	fUI->treeView->setPalette(treePal);
	return exec() == QDialog::Accepted;
}

const std::string& USDImportDialog::filename() const
{
	return fFilename;
}

const std::string& USDImportDialog::rootPrimPath() const
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
	return UsdStage::InitialLoadSet::LoadAll;
}

} // namespace MayaUsd
