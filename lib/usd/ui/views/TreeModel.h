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
#pragma once

#include <mayaUsd/ui/api.h>

#include <QtGui/QStandardItemModel>

#include <mayaUsd/fileio/importData.h>

#include <pxr/usd/usd/stagePopulationMask.h>

class QTreeView;

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

/**
 * \brief Qt Model to explore the hierarchy of a USD file.
 * \remarks Populating the Model with the content of a USD file is done through
 * the APIs exposed by the TreeModelFactory.
 */
class MAYAUSD_UI_PUBLIC TreeModel : public QStandardItemModel
{
	Q_OBJECT

public:
	using ParentClass = QStandardItemModel;

	/**
	 * \brief Constructor.
	 * \param parent A reference to the parent of the TreeModel.
	 */
	explicit TreeModel(QObject* parent = nullptr) noexcept;

	// QStandardItemModel overrides
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	/**
	 * \brief Order of the columns as they appear in the Tree.
	 * \remarks The order of the enumeration is important.
	 */
	enum TREE_COLUMNS
	{
		kTreeColumn_Load,				// Should we load this prim?
		kTreeColumn_Name,				// Name of the item as it appears in the TreeView.
		kTreeColumn_Type,				// Type of the primitive.
		kTreeColumn_Variants,			// Variant Set(s) and Variant Selection of the primitive.
		kTreeColumn_Last				// Last element of the enum.
	};

	void fillStagePopulationMask(UsdStagePopulationMask& popMask, const QModelIndex& parent);
	void fillPrimVariantSelections(ImportData::PrimVariantSelections& primVariantSelections, const QModelIndex& parent);
	void openPersistentEditors(QTreeView* tv, const QModelIndex& parent);

private:
	void setParentsCheckState(const QModelIndex &child, Qt::CheckState state);
	void setChildCheckState(const QModelIndex &parent, Qt::CheckState state);
};

} // namespace MayaUsd
