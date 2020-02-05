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

#include <cctype>
#include <type_traits>
#include <memory>

#include "TreeModelFactory.h"
#include "views/TreeItem.h"
#include "views/TreeModel.h"

#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QtGui/QStandardItemModel>

#include <pxr/usd/usd/primRange.h>

MAYAUSD_NS_DEF {

// Ensure the TreeModelFactory is not constructible, as it is intended to be used only through static factory methods.
//
// While additional traits like std::is_copy_constructible or std::is_move_constructible could also be used, the fact
// that the Factory cannot be (traditionally) instantiated prevents other constructors and assignments operators from
// being useful.
static_assert(!std::is_constructible<TreeModelFactory>::value, "TreeModelFactory should not be constructible.");


/*static*/
std::unique_ptr<TreeModel> TreeModelFactory::createEmptyTreeModel(QObject* parent)
{
	std::unique_ptr<TreeModel> treeModel(new TreeModel(parent));
	treeModel->setHorizontalHeaderLabels({	QObject::tr(""),
											QObject::tr("Prim Name"),
											QObject::tr("Prim Type"),
											QObject::tr("Variant Set and Variant") });
	return treeModel;
}

/*static*/
std::unique_ptr<TreeModel> TreeModelFactory::createFromStage(const UsdStageRefPtr& stage, QObject* parent, int* nbItems /*= nullptr*/)
{
	std::unique_ptr<TreeModel> treeModel = createEmptyTreeModel(parent);
	int cnt = buildTreeHierarchy(stage->GetPseudoRoot(), treeModel->invisibleRootItem());
	if (nbItems != nullptr)
		*nbItems = cnt;
	return treeModel;
}

/*static*/
std::unique_ptr<TreeModel> TreeModelFactory::createFromSearch(
		const UsdStageRefPtr& stage, const std::string& searchFilter, QObject* parent, int* nbItems /*= nullptr*/)
{
	// Optimization: If the provided search filter is empty, fallback to directly importing the content of the given USD
	// Stage. This can can happen in cases where the User already typed characters in the search box before pressing
	// backspace up until all characters were removed.
	if (searchFilter.empty())
	{
		return createFromStage(stage, parent, nbItems);
	}

	std::unordered_set<SdfPath, SdfPath::Hash> primsToIncludeInTree;

	for (const auto& matchingPath : findMatchingPrimPaths(stage, searchFilter))
	{
		UsdPrim prim = stage->GetPrimAtPath(matchingPath);

		// When walking up the ancestry chain, the Root Node will end up being considered once and its parent (an
		// invalid Prim) will be selected. Since there is no point iterating up the hierarchy at this point, stop
		// processing the current Prim and move on to the next one matching the search filter.
		while (prim.IsValid())
		{
			bool primAlreadyIncluded = primsToIncludeInTree.find(prim.GetPath()) != primsToIncludeInTree.end();
			if (primAlreadyIncluded)
			{
				// If the USD Prim is already part of the set of search results to be displayed, it is unnecessary to
				// walk up the ancestry chain in an attempt to process further Prims, as it means they have already
				// been added to the list up to the Root Node.
				break;
			}

			primsToIncludeInTree.insert(prim.GetPath());
			prim = prim.GetParent();
		}
	}

	// Optimization: Count the number of USD Prims expected to be inserted in the TreeModel, so that the search
	// process can stop early if all USD Prims have already been found. While additional "narrowing" techniques
	// can be used in the future to further enhance the performance, this may provide sufficient performance in
	// most cases to remain as-is for early User feedback.
	size_t insertionsRemaining = primsToIncludeInTree.size();
	std::unique_ptr<TreeModel> treeModel = TreeModelFactory::createEmptyTreeModel(parent);
	int cnt = buildTreeHierarchy(
			stage->GetPseudoRoot(), treeModel->invisibleRootItem(), primsToIncludeInTree, insertionsRemaining);
	if (nbItems != nullptr)
		*nbItems = cnt;
	return treeModel;
}

/*static*/
std::vector<SdfPath> TreeModelFactory::findMatchingPrimPaths(
		const UsdStageRefPtr& stage, const std::string& searchFilter)
{
	// Using regular expressions when searching through the set of data can be expensive compared to doing a plain text
	// search. In addition, it may be possible for the User to want to search for content containing the "*" character
	// instead of using this token as wildcard, which is not currently supported. In order to properly handle this, the
	// UI could expose search options in the future, where Users would be able to pick the type of search they wish to
	// perform (likely defaulting to a plain text search).
	bool useWildCardSearch = searchFilter.find('*') != std::string::npos;
	std::vector<SdfPath> matchingPrimPaths;

	for (const auto& prim : stage->TraverseAll())
	{
		if (findString(prim.GetName().GetString(), searchFilter, useWildCardSearch))
		{
			matchingPrimPaths.emplace_back(prim.GetPath());
		}
	}

	return matchingPrimPaths;
}

/*static*/
QList<QStandardItem*> TreeModelFactory::createPrimRow(const UsdPrim& prim)
{
	// Cache the values to be displayed, in order to avoid querying the USD Prim too frequently (despite it being
	// cached and optimized for frequent access). Avoiding frequent conversions from USD Strings to Qt Strings helps
	// in keeping memory allocations low.
	QList<QStandardItem*> ret = {
		new TreeItem(prim, TreeItem::Type::kLoad),
		new TreeItem(prim, TreeItem::Type::kName),
		new TreeItem(prim, TreeItem::Type::kType),
		new TreeItem(prim, TreeItem::Type::kVariants)
	};
	return ret;
}

/*static*/
int TreeModelFactory::buildTreeHierarchy(const UsdPrim& prim, QStandardItem* parentItem)
{
	QList<QStandardItem*> primDataCells = createPrimRow(prim);
	parentItem->appendRow(primDataCells);
	int cnt = 1;

	for (const auto& childPrim : prim.GetAllChildren())
	{
		cnt += buildTreeHierarchy(childPrim, primDataCells.front());
	}
	return cnt;
}

/*static*/
int TreeModelFactory::buildTreeHierarchy(const UsdPrim& prim, QStandardItem* parentItem,
		const unordered_sdfpath_set& primsToIncludeInTree, size_t& insertionsRemaining)
{
	int cnt = 0;
	bool primShouldBeIncluded = primsToIncludeInTree.find(prim.GetPath()) != primsToIncludeInTree.end();
	if (primShouldBeIncluded)
	{
		QList<QStandardItem*> primDataCells = createPrimRow(prim);
		parentItem->appendRow(primDataCells);
		++cnt;

		// Only continue processing additional USD Prims if all expected results have not already been found:
		if (--insertionsRemaining > 0)
		{
			for (const auto& childPrim : prim.GetAllChildren())
			{
				cnt += buildTreeHierarchy(childPrim, primDataCells.front(), primsToIncludeInTree, insertionsRemaining);
			}
		}
	}
	return cnt;
}

/*static*/
bool TreeModelFactory::findString(const std::string& haystack, const std::string& needle, bool useWildCardSearch)
{
	// NOTE: Most of the time, the needle is unlikely to contain a wildcard search.
	if (useWildCardSearch)
	{
		// Needle contains at least one wildcard character, proceed with a regular expression search.
		
		// NOTE: Both leading and trailing wildcards are added to the needle in order to make sure search is made
		// against Prims whose name contains the given search filter. Otherwise, searching for "lorem*ipsum" would match
		// "lorem_SOME-TEXT_ipsum" but not "SOME-TEXT_lorem_ipsum", which is inconvenient as too restrictive for casual
		// Users to type. This ensure search results are handled in a similar way to Windows Explorer, for example.
		QRegExp regularExpression(QString::fromStdString("*" + needle + "*"), Qt::CaseSensitivity::CaseInsensitive,
				QRegExp::PatternSyntax::Wildcard);
		return regularExpression.exactMatch(QString::fromStdString(haystack));
	}
	else
	{
		// Needle does not contain any wildcard characters, use a simple case-insensitive search:
		auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
				[](char charA, char charB) { return std::toupper(charA) == std::toupper(charB); });
		return it != haystack.end();
	}
}

} // namespace MayaUsd
