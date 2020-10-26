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
#include "TreeModel.h"

#include <functional>

#include <QtWidgets/QTreeView>
#include <QtCore/QSortFilterProxyModel>

#include <pxr/usd/usd/variantSets.h>

#include <mayaUsdUI/ui/TreeItem.h>
#include <mayaUsdUI/ui/ItemDelegate.h>
#include <mayaUsdUI/ui/IMayaMQtUtil.h>

namespace MAYAUSD_NS_DEF {

namespace {

TreeItem* findTreeItem(TreeModel* treeModel, const QModelIndex& parent, std::function<bool(TreeItem*)>fn)
{
	for (int r=0; r<treeModel->rowCount(parent); ++r)
	{
		// Note: only the load column (0) has children, so we use it when looking for children.
		QModelIndex childIndex = treeModel->index(r, TreeModel::kTreeColumn_Load, parent);
		TreeItem* item = static_cast<TreeItem*>(treeModel->itemFromIndex(childIndex));
		if (fn(item))
		{
			return item;
		}
		// If this item is check-enabled, we don't need to check it's children because
		// we know they will be check-disabled.
		else if (treeModel->hasChildren(childIndex))
		{
			TreeItem* tempItem = findTreeItem(treeModel, childIndex, fn);
			if (tempItem != nullptr)
				return tempItem;
		}
	}
	return nullptr;
}

void resetVariantToPrimSelection(TreeItem* variantItem)
{
	assert(variantItem);

	UsdPrim prim = variantItem->prim();
	assert(prim.IsValid() && prim.HasVariantSets());

	UsdVariantSets varSets = prim.GetVariantSets();

	std::vector<std::string> usdVarSetNames;
	varSets.GetNames(&usdVarSetNames);

	QStringList qtVarNames;
	for (auto it=usdVarSetNames.crbegin(); it != usdVarSetNames.crend(); it++)
	{
		UsdVariantSet varSet1 = varSets.GetVariantSet(*it);
		qtVarNames.push_back(QString::fromStdString(varSet1.GetVariantSelection()));
	}

	variantItem->setData(qtVarNames, ItemDelegate::kVariantSelectionRole);
	variantItem->resetVariantSelectionModified();
}

void resetAllVariants(TreeModel* treeModel, const QModelIndex& parent)
{
	for (int r=0; r<treeModel->rowCount(parent); ++r)
	{
		QModelIndex variantIndex = treeModel->index(r, TreeModel::kTreeColumn_Variants, parent);
		TreeItem* variantItem = static_cast<TreeItem*>(treeModel->itemFromIndex(variantIndex));

		if (nullptr != variantItem && variantItem->variantSelectionModified()) 
		{
			resetVariantToPrimSelection(variantItem);
		}

		QModelIndex childIndex = treeModel->index(r, TreeModel::kTreeColumn_Load, parent);
		if (treeModel->hasChildren(childIndex))
		{
			resetAllVariants(treeModel, childIndex);
		}
	}	
}

}

TreeModel::TreeModel(const IMayaMQtUtil& mayaQtUtil, const ImportData* importData /*= nullptr*/, QObject* parent /*= nullptr*/) noexcept
	: ParentClass{ parent }
	, fImportData{ importData }
	, fMayaQtUtil{ mayaQtUtil }
{
}

QVariant TreeModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem* item = static_cast<TreeItem*>(itemFromIndex(index));

	if ((role == Qt::DecorationRole) && (index.column() == kTreeColumn_Load))
		return item->checkImage();

	return ParentClass::data(index, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	// The base class implementation returns a combination of flags that enables
	// the item (ItemIsEnabled) and allows it to be selected (ItemIsSelectable).
	Qt::ItemFlags flags = ParentClass::flags(index);
	if (index.column() == kTreeColumn_Load)
		flags &= ~Qt::ItemIsSelectable;

	return flags;
}

void TreeModel::setParentsCheckState(const QModelIndex &child, TreeItem::CheckState state)
{
	QModelIndex parentIndex = this->parent(child);
	if (parentIndex.isValid())
	{
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(parentIndex));

		// If the parent item state matches the input, no need to recurse.
		const TreeItem::CheckState currState = item->checkState();
		if(currState != state)
		{
			item->setCheckState(state);
			QVector<int> roles;
			roles << Qt::DecorationRole;
			Q_EMIT dataChanged(parentIndex, parentIndex, roles);
			setParentsCheckState(parentIndex, state);
		}
	}
}

void TreeModel::setChildCheckState(const QModelIndex &parent, TreeItem::CheckState state)
{
	int rMin = -1, rMax = -1;
	for (int r=0; r<rowCount(parent); ++r)
	{
		// Note: only the load column (0) has children, so we use it when looking for children.
		QModelIndex childIndex = this->index(r, kTreeColumn_Load, parent);
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(childIndex));

		// If child item state matches the input, no need to recurse.
		const TreeItem::CheckState currState = item->checkState();
		if (currState != state)
		{
			if (rMin == -1) rMin = r;
			if (r > rMax) rMax = r;
			item->setCheckState(state);
			if (hasChildren(childIndex))
				setChildCheckState(childIndex, state);
		}
	}
	QModelIndex rMinIndex = this->index(rMin, kTreeColumn_Load, parent);
	QModelIndex rMaxIndex = this->index(rMax, kTreeColumn_Load, parent);
	QVector<int> roles;
	roles << Qt::DecorationRole;
	Q_EMIT dataChanged(rMinIndex, rMaxIndex, roles);
}

void TreeModel::getRootPrimPath(std::string& rootPrimPath, const QModelIndex& parent)
{
	// We simply need to find the single item that is "check-enabled" as there can
	// only be one.
	auto fn = [](TreeItem* item) -> bool
	{
		if (item->checkState() == TreeItem::CheckState::kChecked)
			return true;
		return false;
	};
	TreeItem* item = findTreeItem(this, QModelIndex(), fn);
	if (item != nullptr)
		rootPrimPath = item->prim().GetPath().GetString();
}

void TreeModel::fillStagePopulationMask(UsdStagePopulationMask& popMask, const QModelIndex& parent)
{
	for (int r=0; r<rowCount(parent); ++r)
	{
		// Note: only the load column (0) has children, so we use it when looking for children.
		QModelIndex childIndex = this->index(r, kTreeColumn_Load, parent);
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(childIndex));
		if (item->checkState() == TreeItem::CheckState::kChecked)
		{
			auto primPath = item->prim().GetPath();
			if (!popMask.Includes(primPath))
				popMask.Add(primPath);
			return;
		}
		// If this item is check-enabled, we don't need to check it's children because
		// we know they will be check-disabled.
		else if (hasChildren(childIndex))
		{
			fillStagePopulationMask(popMask, childIndex);
		}
	}
}

void TreeModel::fillPrimVariantSelections(ImportData::PrimVariantSelections& primVariantSelections, const QModelIndex& parent)
{
	for (int r=0; r<rowCount(parent); ++r)
	{
		QModelIndex variantIndex = this->index(r, kTreeColumn_Variants, parent);
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(variantIndex));
		if (item->variantSelectionModified())
		{
			// Note: both the variant name and variant selection roles contain a
			//		 QStringList for data.
			QVariant varSel = variantIndex.data(ItemDelegate::kVariantSelectionRole);
			if (varSel.isValid() && varSel.canConvert<QStringList>())
			{
				// The name role must be valid if the selection role was.
				QStringList varSelections = varSel.toStringList();
				QStringList varNames = variantIndex.data(ItemDelegate::kVariantNameRole).toStringList();
				assert(varSelections.count() == varNames.count());
				if (varSelections.count() == varNames.count())
				{
					SdfVariantSelectionMap varSels;
					for (int i=0; i<varNames.count(); ++i)
					{
						varSels[varNames.at(i).toStdString()] = varSelections.at(i).toStdString();
					}
					primVariantSelections[item->prim().GetPath()] = varSels;
				}
			}
		}

		// Note: only the load column (0) has children, so we use it when looking for children.
		QModelIndex childIndex = this->index(r, kTreeColumn_Load, parent);
		if (hasChildren(childIndex))
		{
			fillPrimVariantSelections(primVariantSelections, childIndex);
		}
	}
}

void TreeModel::openPersistentEditors(QTreeView* tv, const QModelIndex& parent)
{
	for (int r=0; r<rowCount(parent); ++r)
	{
		QModelIndex varSelIndex = this->index(r, kTreeColumn_Variants, parent);
		int type = varSelIndex.data(ItemDelegate::kTypeRole).toInt();
		if (type == ItemDelegate::kVariants)
		{
			QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(tv->model());
			tv->openPersistentEditor(proxyModel->mapFromSource(varSelIndex));
		}

		// Note: only the load column (0) has children, so we use it when looking for children.
		QModelIndex childIndex = this->index(r, kTreeColumn_Load, parent);
		if (hasChildren(childIndex))
		{
			openPersistentEditors(tv, childIndex);
		}
	}
}

void TreeModel::setRootPrimPath(const std::string& path)
{
	// Find the prim matching the root prim path from the import data and
	// check-enable it.
	SdfPath rootPrimPath(path);
	auto fnFindRoot = [rootPrimPath](TreeItem* item) -> bool
	{
		if (item->prim().GetPath() == rootPrimPath)
			return true;
		return false;
	};
	TreeItem* item = findTreeItem(this, QModelIndex(), fnFindRoot);
	if (item != nullptr)
	{
		checkEnableItem(item);
	}
}

void TreeModel::uncheckEnableTree()
{
	// When unchecking any item we uncheck-enable the entire tree.
	setChildCheckState(QModelIndex(), TreeItem::CheckState::kUnchecked);

	updateCheckedItemCount();
}

void TreeModel::checkEnableItem(TreeItem* item)
{
	if (item != nullptr)
	{
		// All ancestors [and their descendants] become unchecked-disabled, and all descendants
		// become checked-disabled.

		// First run-thru entire tree and uncheck-disable everything.
		setChildCheckState(QModelIndex(), TreeItem::CheckState::kUnchecked_Disabled);

		// Then check the item that was clicked.
		item->setCheckState(TreeItem::CheckState::kChecked);
		QModelIndex modelIndex = indexFromItem(item);
		QVector<int> roles;
		roles << Qt::DecorationRole;
		Q_EMIT dataChanged(modelIndex, modelIndex, roles);

		// Then check-disable all the children of the clicked item.
		setChildCheckState(modelIndex, TreeItem::CheckState::kChecked_Disabled);

		updateCheckedItemCount();
	}
}

void TreeModel::updateCheckedItemCount() const
{
	int nbChecked = 0, nbVariantsModified = 0;
	countCheckedItems(QModelIndex(), nbChecked, nbVariantsModified);

	// When the checked items change we will count, and emit signals for, the number of
	// checked items as well as the number of in-scope modified variants.
	Q_EMIT checkedStateChanged(nbChecked);
	Q_EMIT modifiedVariantCountChanged(nbVariantsModified);
}

void TreeModel::countCheckedItems(const QModelIndex &parent, int& nbChecked, int& nbVariantsModified) const
{
	for (int r=0; r<rowCount(parent); ++r)
	{
		TreeItem* item;

		QModelIndex checkedChildIndex = this->index(r, kTreeColumn_Load, parent);
		item = static_cast<TreeItem*>(itemFromIndex(checkedChildIndex));

		const TreeItem::CheckState state = item->checkState();
		if (TreeItem::CheckState::kChecked == state || 
			TreeItem::CheckState::kChecked_Disabled == state)
		{
			nbChecked++;

			// We are only counting modified variants of in-scope prims
			QModelIndex variantChildIndex = this->index(r, kTreeColumn_Variants, parent);
			item = static_cast<TreeItem*>(itemFromIndex(variantChildIndex));

			if (item->variantSelectionModified())
			{
				nbVariantsModified++;
			}			
		}

		if (hasChildren(checkedChildIndex))
			countCheckedItems(checkedChildIndex, nbChecked, nbVariantsModified);
	}
}

void TreeModel::updateModifiedVariantCount() const
{
	int nbChecked = 0, nbVariantsModified = 0;
	countCheckedItems(QModelIndex(), nbChecked, nbVariantsModified);

	Q_EMIT modifiedVariantCountChanged(nbVariantsModified);
}

void TreeModel::onItemClicked(TreeItem* item)
{
	if (item->index().column() == kTreeColumn_Load)
	{
		// We only allow toggling an enabled checked or unchecked item.
		TreeItem::CheckState st = item->checkState();
		if (st == TreeItem::CheckState::kChecked)
		{
			uncheckEnableTree();
		}
		else if (st == TreeItem::CheckState::kUnchecked)
		{
			checkEnableItem(item);
		}
	}
}

void TreeModel::resetVariants()
{
	resetAllVariants(this, QModelIndex());
}

} // namespace MayaUsd
