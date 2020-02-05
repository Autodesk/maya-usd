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

#include <memory>
#include <unordered_set>

#include <QtCore/QList>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

class QObject;
class QStandardItem;

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

class TreeModel;

/**
 * \brief Factory to create a tree-like structure of USD content suitable to be displayed in a TreeView.
 */
class MAYAUSD_UI_PUBLIC TreeModelFactory
{
private:
	/**
	 * \brief Default Constructor (deleted).
	 */
	TreeModelFactory() = delete;

public:
	/**
	 * \brief Create an empty TreeModel.
	 * \param parent A reference to the parent of the TreeModel.
	 * \return An empty TreeModel.
	 */
	static std::unique_ptr<TreeModel> createEmptyTreeModel(QObject* parent = nullptr);

	/**
	 * \brief Create a TreeModel from the given USD Stage.
	 * \param stage A reference to the USD Stage from which to create a TreeModel.
	 * \param parent A reference to the parent of the TreeModel.
	 * \param nbItems Number of items added to the TreeModel.
	 * \return A TreeModel created from the given USD Stage.
	 */
	static std::unique_ptr<TreeModel> createFromStage(const UsdStageRefPtr& stage, QObject* parent = nullptr, int* nbItems = nullptr);

protected:
	// Type definition for an STL unordered set of SDF Paths:
	using unordered_sdfpath_set = std::unordered_set<SdfPath, SdfPath::Hash>;

	/**
	 * \brief Create the list of data cells used to represent the given USD Prim's data in the tree.
	 * \param prim The USD Prim for which to create the list of data cells.
	 * \return The List of data cells used to represent the given USD Prim's data in the tree.
	 */
	static QList<QStandardItem*> createPrimRow(const UsdPrim& prim);

	/**
	 * \brief Build the tree hierarchy starting at the given USD Prim.
	 * \param prim The USD Prim from which to start building the tree hierarchy.
	 * \param parentItem The parent into which to attach the tree hierarchy.
	 * \return The number of items added.
	 */
	static int buildTreeHierarchy(const UsdPrim& prim, QStandardItem* parentItem);
	/**
	 * \brief Build the tree hierarchy starting at the given USD Prim.
	 * \param prim The USD Prim from which to start building the tree hierarchy.
	 * \param parentItem The parent into which to attach the tree hierarchy.
	 * \param primsToIncludeInTree The list of USD Prim paths to include when building the tree.
	 * \param insertionsRemaining The number of items yet to be inserted.
	 * \return The number of items added.
	 */
	static int buildTreeHierarchy(const UsdPrim& prim, QStandardItem* parentItem,
			const unordered_sdfpath_set& primsToIncludeInTree, size_t& insertionsRemaining);
};

} // namespace MayaUsd
