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
#include "TreeModelFactory.h"

#include <mayaUsdUI/ui/IMayaMQtUtil.h>
#include <mayaUsdUI/ui/TreeItem.h>
#include <mayaUsdUI/ui/TreeModel.h>

#include <QtCore/QObject>
#include <QtGui/QStandardItemModel>

#include <cctype>
#include <memory>
#include <type_traits>

namespace MAYAUSD_NS_DEF {

// Ensure the TreeModelFactory is not constructible, as it is intended to be used only through
// static factory methods.
//
// While additional traits like std::is_copy_constructible or std::is_move_constructible could also
// be used, the fact that the Factory cannot be (traditionally) instantiated prevents other
// constructors and assignments operators from being useful.
static_assert(
    !std::is_constructible<TreeModelFactory>::value,
    "TreeModelFactory should not be constructible.");

/*static*/
std::unique_ptr<TreeModel> TreeModelFactory::createEmptyTreeModel(
    const IMayaMQtUtil& mayaQtUtil,
    const ImportData*   importData /*= nullptr*/,
    QObject*            parent /*= nullptr*/)
{
    std::unique_ptr<TreeModel> treeModel(new TreeModel(mayaQtUtil, importData, parent));
    treeModel->setHorizontalHeaderLabels({ QObject::tr(""),
                                           QObject::tr("Prim Name"),
                                           QObject::tr("Prim Type"),
                                           QObject::tr("Variant Set and Variant") });
    return treeModel;
}

/*static*/
std::unique_ptr<TreeModel> TreeModelFactory::createFromStage(
    const UsdStageRefPtr& stage,
    const IMayaMQtUtil&   mayaQtUtil,
    const ImportData*     importData /*= nullptr*/,
    QObject*              parent,
    int*                  nbItems /*= nullptr*/
)
{
    std::unique_ptr<TreeModel> treeModel = createEmptyTreeModel(mayaQtUtil, importData, parent);
    int cnt = buildTreeHierarchy(stage->GetPseudoRoot(), treeModel->invisibleRootItem());
    if (nbItems != nullptr)
        *nbItems = cnt;
    return treeModel;
}

/*static*/
QList<QStandardItem*> TreeModelFactory::createPrimRow(const UsdPrim& prim)
{
    // Cache the values to be displayed, in order to avoid querying the USD Prim too frequently
    // (despite it being cached and optimized for frequent access). Avoiding frequent conversions
    // from USD Strings to Qt Strings helps in keeping memory allocations low.
    QList<QStandardItem*> ret = { new TreeItem(prim, TreeItem::Type::kLoad),
                                  new TreeItem(prim, TreeItem::Type::kName),
                                  new TreeItem(prim, TreeItem::Type::kType),
                                  new TreeItem(prim, TreeItem::Type::kVariants) };
    return ret;
}

/*static*/
int TreeModelFactory::buildTreeHierarchy(const UsdPrim& prim, QStandardItem* parentItem)
{
    QList<QStandardItem*> primDataCells = createPrimRow(prim);
    parentItem->appendRow(primDataCells);
    int cnt = 1;

    for (const auto& childPrim : prim.GetAllChildren()) {
        cnt += buildTreeHierarchy(childPrim, primDataCells.front());
    }
    return cnt;
}

/*static*/
int TreeModelFactory::buildTreeHierarchy(
    const UsdPrim&               prim,
    QStandardItem*               parentItem,
    const unordered_sdfpath_set& primsToIncludeInTree,
    size_t&                      insertionsRemaining)
{
    int  cnt = 0;
    bool primShouldBeIncluded
        = primsToIncludeInTree.find(prim.GetPath()) != primsToIncludeInTree.end();
    if (primShouldBeIncluded) {
        QList<QStandardItem*> primDataCells = createPrimRow(prim);
        parentItem->appendRow(primDataCells);
        ++cnt;

        // Only continue processing additional USD Prims if all expected results have not already
        // been found:
        if (--insertionsRemaining > 0) {
            for (const auto& childPrim : prim.GetAllChildren()) {
                cnt += buildTreeHierarchy(
                    childPrim, primDataCells.front(), primsToIncludeInTree, insertionsRemaining);
            }
        }
    }
    return cnt;
}

} // namespace MAYAUSD_NS_DEF
