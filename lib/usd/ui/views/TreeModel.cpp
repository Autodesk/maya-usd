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
#include "TreeItem.h"
#include "ItemDelegate.h"

#include <QtWidgets/QTreeView>
#include <QtCore/QSortFilterProxyModel>

MAYAUSD_NS_DEF {

TreeModel::TreeModel(QObject* parent) noexcept
	: ParentClass{ parent }
{
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if (orientation == Qt::Horizontal)
	{
		switch (role)
		{
		case Qt::CheckStateRole:
			if (section == kTreeColumn_Load)
			{
				return QVariant::fromValue<int>(Qt::Checked);
			}
			break;
		}
	}
	return ParentClass::headerData(section, orientation, role);
}

QVariant TreeModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem* item = static_cast<TreeItem*>(itemFromIndex(index));

	if ((role == Qt::CheckStateRole) && (index.column() == kTreeColumn_Load))
		return item->checkState();

	return ParentClass::data(index, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (index.column() == kTreeColumn_Load)
		flags |= Qt::ItemIsUserCheckable;
	else
		flags = ParentClass::flags(index);

	return flags;
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role /*= Qt::EditRole*/)
{
	if (index.column() == kTreeColumn_Load)
	{
		if (role == Qt::EditRole)
			return false;

		if (role == Qt::CheckStateRole)
		{
			TreeItem* item = static_cast<TreeItem*>(itemFromIndex(index));
			item->setCheckState(static_cast<Qt::CheckState>(value.toInt()));
			emit dataChanged(index, index);

			// If we 'unchecked' this item, we need to uncheck all its children.
			// If we 'checked' this item, we need to check all of the parent items
			// and all of the child items.
			if (item->checkState() == Qt::Unchecked) {
				setChildCheckState(index, Qt::Unchecked);
			} else {
				setParentsCheckState(index, Qt::Checked);
				setChildCheckState(index, Qt::Checked);
			}
			return true;
		}
	}

	return ParentClass::setData(index, value, role);
}

void TreeModel::setParentsCheckState(const QModelIndex &child, Qt::CheckState state)
{
	QModelIndex parentIndex = this->parent(child);
	if (parentIndex.isValid())
	{
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(parentIndex));

		// If this parent item is already checked then it means its parents
		// would also have to be, so no need to recurse.
		const Qt::CheckState currState = item->checkState();
		if(currState != state)
		{
			item->setCheckState(state);
			emit dataChanged(parentIndex, parentIndex);
			setParentsCheckState(parentIndex, state);
		}
	}
}

void TreeModel::setChildCheckState(const QModelIndex &parent, Qt::CheckState state)
{
	int rMin = -1, rMax = -1;
	for (int r=0; r<rowCount(parent); ++r)
	{
		QModelIndex childIndex = this->index(r, 0, parent);
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(childIndex));

		// If this item is already checked/unchecked that means all its children
		// would have to be, so no need to recurse.
		// TODO - verify this assumption. I think it holds true when using the
		//		  checkbox. But with context menus (to come) it might not.
		const Qt::CheckState currState = item->checkState();
		if (currState != state)
		{
			if (rMin == -1) rMin = r;
			if (r > rMax) rMax = r;
			item->setCheckState(state);
			if (hasChildren(childIndex))
				setChildCheckState(childIndex, state);
		}
	}
	QModelIndex rMinIndex = this->index(rMin, 0, parent);
	QModelIndex rMaxIndex = this->index(rMax, 0, parent);
	emit dataChanged(rMinIndex, rMaxIndex);
}

void TreeModel::fillStagePopulationMask(UsdStagePopulationMask& popMask, const QModelIndex& parent)
{
	bool allChildrenChecked = true;		// Start with this assumption
	for (int r=0; r<rowCount(parent); ++r)
	{
		QModelIndex childIndex = this->index(r, 0, parent);
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(childIndex));
		if (item->checkState() == Qt::Checked)
		{
			if (hasChildren(childIndex))
			{
				fillStagePopulationMask(popMask, childIndex);
			}
			auto primPath = item->prim().GetPath();
			if (!popMask.Includes(primPath))
				popMask.Add(primPath);
		}
		else
			allChildrenChecked = false;
	}
	if (allChildrenChecked)
	{
		// We would have added each individual child item's paths to the pop mask.
		// Instead let's add the parent path.
		TreeItem* item = static_cast<TreeItem*>(itemFromIndex(parent));
		if ((item != nullptr) && !item->prim().IsPseudoRoot())
		{
			auto primPath = item->prim().GetPath();
			popMask.Add(primPath);
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

		QModelIndex childIndex = this->index(r, 0, parent);
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
		QModelIndex childIndex = this->index(r, 0, parent);
		if (hasChildren(childIndex))
		{
			openPersistentEditors(tv, childIndex);
		}
	}
}

} // namespace MayaUsd
