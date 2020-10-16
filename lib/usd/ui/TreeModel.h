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

#ifndef MAYAUSDUI_TREE_MODEL_H
#define MAYAUSDUI_TREE_MODEL_H


#include <QtGui/QStandardItemModel>

#include <pxr/usd/usd/stagePopulationMask.h>

#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/mayaUsd.h>

#include <mayaUsdUI/ui/api.h>
#include <mayaUsdUI/ui/TreeItem.h>

class QTreeView;

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class IMayaMQtUtil;

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
	explicit TreeModel(const IMayaMQtUtil& mayaQtUtil, const ImportData* importData = nullptr, QObject* parent = nullptr) noexcept;

	// QStandardItemModel overrides
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

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

	void setRootPrimPath(const std::string& path);
	void getRootPrimPath(std::string&, const QModelIndex& parent);
	void fillStagePopulationMask(UsdStagePopulationMask& popMask, const QModelIndex& parent);
	void fillPrimVariantSelections(ImportData::PrimVariantSelections& primVariantSelections, const QModelIndex& parent);
	void openPersistentEditors(QTreeView* tv, const QModelIndex& parent);

	const ImportData* importData() const { return fImportData; }
	const IMayaMQtUtil& mayaQtUtil() const { return fMayaQtUtil; }

	void onItemClicked(TreeItem* item);

	void resetVariants();
	
private:
	void uncheckEnableTree();
	void checkEnableItem(TreeItem* item);

	void updateCheckedItemCount() const;
	void countCheckedItems(const QModelIndex &parent, int& nbChecked, int& nbVariantsModified) const;

	void setParentsCheckState(const QModelIndex &child, TreeItem::CheckState state);
	void setChildCheckState(const QModelIndex &parent, TreeItem::CheckState state);

Q_SIGNALS:
	void checkedStateChanged(int nbChecked) const;
	void modifiedVariantCountChanged(int nbModified) const;

public Q_SLOTS:
	void updateModifiedVariantCount() const;

private:
	// Extra import data, if any to set the initial state of dialog from.
	const ImportData*			fImportData;

	// Special interface we can use to perform Maya Qt utilities (such as Pixmap loading).
	const IMayaMQtUtil&			fMayaQtUtil;
};

} // namespace MayaUsd


#endif
