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

#ifndef MAYAUSDUI_ITEM_DELEGATE_H
#define MAYAUSDUI_ITEM_DELEGATE_H


#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtWidgets/QStyledItemDelegate>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>

#include <mayaUsd/mayaUsd.h>

#include <mayaUsdUI/ui/api.h>

class QLabel;
class QComboBox;

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class TreeItem;
class VariantsEditorWidget;

/**
 * \brief Item delegate for displaying and editing the variant sets.
 */
class MAYAUSD_UI_PUBLIC ItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	using ParentClass = QStyledItemDelegate;

	using QStyledItemDelegate::QStyledItemDelegate;

	enum DELEGATE_TYPE
	{
		kNone,
		kVariants,
	};

	enum Roles {
		// The type role is used to distinguish what type of delegate item we are dealing
		// with. It should be set with one of the DELEGATE_TYPE from above.
		kTypeRole = Qt::UserRole + 1,

		// The variant name role is used to hold the current variant names (from the
		// qt labels in the editor widget).
		// Data = QStringList of variant names.
		kVariantNameRole = Qt::UserRole + 2,

		// The variant selection role is used to hold the current variant selections
		// (from the qt comboboxes in the editor widget).
		// Data = QStringList of variant selections.
		kVariantSelectionRole = Qt::UserRole + 3
	};

	// QStyledItemDelegate overrides
	QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;
	void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;
	void setEditorData(QWidget*, const QModelIndex&) const override;
	void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override;
	QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override;
	void updateEditorGeometry(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;

Q_SIGNALS:
	void variantModified() const;

public Q_SLOTS:
	void commitVariantSelection(VariantsEditorWidget* editor);

private:
	TreeItem* getTreeItemForIndex(const QModelIndex& index) const;

}; // ItemDelegate

/**
 * \brief Widget that will contain all the variants for a prim.
 */
class VariantsEditorWidget: public QWidget
{
	Q_OBJECT

public:
	VariantsEditorWidget(QWidget* parent, const ItemDelegate* itemDelegate, const UsdPrim& prim, const SdfVariantSelectionMap& varSelMap);

	QStringList variantNames() const;

	QStringList variantSelections() const;
	void setVariantSelections(const QStringList& varSel);

protected:
	bool eventFilter(QObject *obj, QEvent *ev) override;

private:
	QLayout* createVariantSet(const ItemDelegate* itemDelegate, const QString& varName, QStringList& varNames);

private:
	QList<QLabel*>			fVariantLabels;
	QList<QComboBox*>		fVariantCombos;

}; // VariantsEditorWidget

} // namespace MayaUsd


#endif
