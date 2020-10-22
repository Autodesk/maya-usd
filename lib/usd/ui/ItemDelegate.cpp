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
#include "ItemDelegate.h"

#include <cassert>

#include <QtCore/QEvent>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QPainter>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>

#include <mayaUsd/fileio/importData.h>

#include <mayaUsdUI/ui/TreeItem.h>
#include <mayaUsdUI/ui/TreeModel.h>

namespace MAYAUSD_NS_DEF {

QWidget* ItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	// Check out special type role (instead of column) since it is only set then this
	// prim has a variant set and variant selection.
	int type = index.data(kTypeRole).toInt();
	if (type == kVariants)
	{
		TreeItem* treeItem = getTreeItemForIndex(index);
		assert(treeItem != nullptr);
		if (nullptr != treeItem)
		{
			// Look in the import data (from the tree model) to see if it has a prim variant selection
			// matching the prim from this item. If found we send that variant selection to the editor
			// so that it will set that value rather (in the combobox) rather than the one from prim.
			SdfVariantSelectionMap varSelMap;
			TreeModel* treeModel = qobject_cast<TreeModel*>(treeItem->model());
			if ((treeModel != nullptr) && (treeModel->importData() != nullptr))
			{
				const ImportData::PrimVariantSelections& primVarSel = treeModel->importData()->primVariantSelections();
				ImportData::PrimVariantSelections::const_iterator iter = primVarSel.find(treeItem->prim().GetPath());
				if (iter != std::end(primVarSel))
				{
					varSelMap = iter->second;
				}
			}

			VariantsEditorWidget* editor = new VariantsEditorWidget(parent, this, treeItem->prim(), varSelMap);
			if (!varSelMap.empty())
			{
				setModelData(editor, treeItem->model(), treeItem->model()->indexFromItem(treeItem));
			}
			return editor;
		}
	}

	return ParentClass::createEditor(parent, option, index);
}

void ItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	// Draw gridlines - doesn't work correctly as it sometimes draws lines in the middle of cells!
// 	painter->save();
// 	painter->setPen(QColor(60, 60, 60)/*option.palette.color(QPalette::Base)*/);
// 	painter->drawRect(QRect(0, option.rect.y(), option.rect.right(), option.rect.bottom()));
// 	painter->restore();

	ParentClass::paint(painter, option, index);
}

void ItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	int type = index.data(kTypeRole).toInt();
	if (type == kVariants)
	{
		VariantsEditorWidget* widget = static_cast<VariantsEditorWidget*>(editor);
		widget->blockSignals(true);
		// We don't bother getting the kVariantNameRole and setting it in the widget
		// as it will never change once initially set.
		QVariant varSel = index.data(kVariantSelectionRole);
		if (varSel.isValid() && varSel.canConvert<QStringList>())
		{
			widget->setVariantSelections(varSel.toStringList());
		}
		widget->blockSignals(false);
	}
	ParentClass::setEditorData(editor, index);
}

void ItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	int type = index.data(kTypeRole).toInt();
	if (type == kVariants)
	{
		VariantsEditorWidget* widget = static_cast<VariantsEditorWidget*>(editor);
		model->setData(index, widget->sizeHint(), Qt::SizeHintRole);
		model->setData(index, widget->variantNames(), kVariantNameRole);
 		model->setData(index, widget->variantSelections(), kVariantSelectionRole);

		TreeItem* treeItem = getTreeItemForIndex(index);
		assert(treeItem != nullptr);
		if (nullptr != treeItem)
		{
			treeItem->setVariantSelectionModified();
			Q_EMIT variantModified();
		}
	}

	ParentClass::setModelData(editor, model, index);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	int type = index.data(kTypeRole).toInt();
	if (type == kVariants)
	{
		QSize s =  index.data(Qt::SizeHintRole).toSize();
		return s.isValid() ? s : ParentClass::sizeHint(option, index);
	}

	return ParentClass::sizeHint(option, index);
}

void ItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	int type = index.data(kTypeRole).toInt();
	if (type == kVariants)
	{
		editor->setGeometry(option.rect);
	}
	else
	{
		ParentClass::updateEditorGeometry(editor, option, index);
	}
}

TreeItem* ItemDelegate::getTreeItemForIndex(const QModelIndex& index) const
{
	// Do we have an item directly from our tree model?
	const TreeModel* treeModel = qobject_cast<const TreeModel*>(index.model());
	if (nullptr != treeModel)
	{
		TreeItem* item = static_cast<TreeItem*>(treeModel->itemFromIndex(index));
		return item;
	}

	// If the item is not directly from the tree model, then it should be from
	// the proxy model.
	const QSortFilterProxyModel* proxyModel = qobject_cast<const QSortFilterProxyModel*>(index.model());
	if (proxyModel != nullptr)
	{
		treeModel = qobject_cast<TreeModel*>(proxyModel->sourceModel());
		if (treeModel != nullptr)
		{
			TreeItem* item = static_cast<TreeItem*>(treeModel->itemFromIndex(proxyModel->mapToSource(index)));
			return item;
		}
	}

	return nullptr;
}

void ItemDelegate::commitVariantSelection(VariantsEditorWidget* editor)
{
	Q_EMIT commitData(editor);
}

//------------------------------------------------------------------------------
// VariantsEditorWidget
//------------------------------------------------------------------------------

VariantsEditorWidget::VariantsEditorWidget(QWidget* parent, const ItemDelegate* itemDelegate, const UsdPrim& prim, const SdfVariantSelectionMap& varSelMap)
	: QWidget(parent)
{
	assert(prim.HasVariantSets());
	UsdVariantSets varSets = prim.GetVariantSets();

	// We can have multiple variant selections (meaning multiple combo boxes).
	// So we need a vertical layout for them.
	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0,1,0,1);
	layout->setSpacing(2);

	// Note: the variant set names are returned in reverse order.
	std::vector<std::string> usdVarSetNames;
	varSets.GetNames(&usdVarSetNames);
	for (auto it=usdVarSetNames.crbegin(); it != usdVarSetNames.crend(); it++)
	{
		UsdVariantSet varSet1 = varSets.GetVariantSet(*it);

		// The names list will contain the variant selection, followed by all
		// the variant names.
		QStringList qtVarNames;

		// Check to see if the input variant selection map contains this variant.
		// If yes, then we use the selection from the map, rather than the prim.
		SdfVariantSelectionMap::const_iterator iter = varSelMap.find(varSet1.GetName());
		if (iter != std::end(varSelMap))
		{
			qtVarNames.push_back(QString::fromStdString(iter->second));
		}
		else
		{
			qtVarNames.push_back(QString::fromStdString(varSet1.GetVariantSelection()));
		}

		// Set the tree item with the data needed to fill the combo box.
		const auto usdVarNames = varSet1.GetVariantNames();
		for (const auto& vn : usdVarNames)
		{
			qtVarNames.push_back(vn.c_str());
		}

		QString qtVarName = QString::fromStdString(varSet1.GetName());
		QLayout* varSetLayout = createVariantSet(itemDelegate, qtVarName, qtVarNames);
		layout->addLayout(varSetLayout);
	}

	setLayout(layout);
}

QLayout* VariantsEditorWidget::createVariantSet(const ItemDelegate* itemDelegate, const QString& varName, QStringList& varNames)
{
	// We'll display the variant set and variant selection on a single row.
	QHBoxLayout* layout = new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);

	QLabel* lbl = new QLabel(varName);
	layout->addWidget(lbl);
	fVariantLabels.append(lbl);

	QString varSel = varNames.takeFirst();
	QComboBox* cb = new QComboBox;

	ItemDelegate* id = const_cast<ItemDelegate*>(itemDelegate);
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
	connect(cb, QOverload<const QString &>::of(&QComboBox::activated),
		id, [this, id]{ id->commitVariantSelection(this); } );
#else
	connect(cb, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
		id, [this, id]{ id->commitVariantSelection(this); } );
#endif

	cb->addItems(varNames);
	cb->setCurrentText(varSel);
	cb->setFocusPolicy( Qt::StrongFocus );
	cb->installEventFilter(this);

	layout->addWidget(cb);
	fVariantCombos.append(cb);

	return layout;
}

QStringList VariantsEditorWidget::variantNames() const
{
	QStringList names;
	for (auto lbl : fVariantLabels)
	{
		names.append(lbl->text());
	}
	return names;
}

QStringList VariantsEditorWidget::variantSelections() const
{
	QStringList sels;
	for (auto cb : fVariantCombos)
	{
		sels.append(cb->currentText());
	}
	return sels;
}

void VariantsEditorWidget::setVariantSelections(const QStringList& varSel)
{
	assert(varSel.count() == fVariantCombos.count());
	if (varSel.count() != fVariantCombos.count()) return;

	for (int i=0; i<varSel.count(); ++i)
	{
		fVariantCombos.at(i)->setCurrentText(varSel.at(i));
	}
}

bool VariantsEditorWidget::eventFilter( QObject * o, QEvent * e ) 
{
	// Block wheel event scrolling when just hovering over the variants
	if ( e->type() == QEvent::Wheel && qobject_cast<QComboBox*>( o ) )
	{
		e->ignore();
		return true;
	}

	return QWidget::eventFilter( o, e );
}

} // namespace MayaUsd
