//
// Copyright 2022 Autodesk
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
#include "MayaUIInfoHandler.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

// Needed for Tf diagnostic macros.
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Find a pulled Maya node's USD pulled ancestor by iterating up the Maya
// path.  Iteration stops when a Maya node with pull information has been
// found, which can be for the initial path itself, when the Maya node is
// the root of the pulled sub-hierarchy.
//
// Note that if it exists, the USD pulled ancestor prim is inactive, which is
// done to avoid having that prim and its sub-hierarchy exist in the scene as
// stale USD duplicates of Maya pulled nodes.  If the USD pulled ancestor does
// not exist, the argument Maya node is orphaned.
MayaUsd::ufe::UsdSceneItem::Ptr pulledUsdAncestorItem(const Ufe::SceneItem::Ptr& mayaItem)
{
    // This function requires a Maya item to compute its USD ancestor.
    TF_AXIOM(mayaItem->runTimeId() == MayaUsd::ufe::getMayaRunTimeId());

    // Find the pulled ancestor by iterating up the Maya path.
    auto      mayaPath = mayaItem->path();
    bool      found { false };
    Ufe::Path usdItemPath;
    while (!found) {
        // A pulled node either has the pull information itself, or has a
        // pulled ancestor that does.
        TF_AXIOM(!mayaPath.empty());
        const auto mayaPathStr = Ufe::PathString::string(mayaPath);
        const auto dagPath = UsdMayaUtil::nameToDagPath(mayaPathStr);
        if (PrimUpdaterManager::readPullInformation(dagPath, usdItemPath)) {
            found = true;
        } else {
            mayaPath = mayaPath.pop();
        }
    }

    // Try to create a USD scene item (and its underlying prim) from the pulled
    // ancestor USD path.  If no such USD prim exists, our argument Maya node
    // is orphaned.
    return std::static_pointer_cast<MayaUsd::ufe::UsdSceneItem>(
        Ufe::Hierarchy::createItem(usdItemPath));
}

// A Maya node is orphaned if its pulled ancestor is not in the scene.
bool isOrphaned(const Ufe::SceneItem::Ptr& mayaItem)
{
    return pulledUsdAncestorItem(mayaItem) == nullptr;
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

MayaUIInfoHandler::MayaUIInfoHandler()
    : Ufe::UIInfoHandler()
{
}

MayaUIInfoHandler::~MayaUIInfoHandler() { }

/*static*/
MayaUIInfoHandler::Ptr MayaUIInfoHandler::create() { return std::make_shared<MayaUIInfoHandler>(); }

//------------------------------------------------------------------------------
// Ufe::UIInfoHandler overrides
//------------------------------------------------------------------------------

bool MayaUIInfoHandler::treeViewCellInfo(const Ufe::SceneItem::Ptr& mayaItem, Ufe::CellInfo& info)
    const
{
    if (isOrphaned(mayaItem)) {
        // If the Maya node is orphaned, give it the orphaned color.
        info.textFgColor.set(0.403922f, 0.403922f, 0.403922f);
        return true;
    }

    // If the argument Maya scene item corresponds to the root of the pulled
    // hierarchy, set its font to italics.
    auto      dagPath = UsdMayaUtil::nameToDagPath(Ufe::PathString::string(mayaItem->path()));
    Ufe::Path usdItemPath;
    if (PrimUpdaterManager::readPullInformation(dagPath, usdItemPath)) {
        info.fontItalics = true;
        return true;
    }
    return false;
}

Ufe::UIInfoHandler::Icon MayaUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& mayaItem) const
{
    return isOrphaned(mayaItem)
        ? Ufe::UIInfoHandler::Icon("", "orphaned_node_badge", Ufe::UIInfoHandler::LowerRight)
        : Ufe::UIInfoHandler::Icon();
}

std::string MayaUIInfoHandler::treeViewTooltip(const Ufe::SceneItem::Ptr& mayaItem) const
{
    // If the pulled USD ancestor does not exist, the Maya node is orphaned.
    auto usdItem = pulledUsdAncestorItem(mayaItem);
    if (!usdItem) {
        return {};
    }

    // Show the stage of the pulled item, and that pulled nodes are locked.
    // Stage name is the last node of the first segment.
    auto        stageName = Ufe::Path(usdItem->path().getSegments()[0]).back().string();
    std::string toolTip { "<b>Stage:</b> " };
    toolTip += stageName;
    toolTip += "<br>Locked Node";

    return toolTip;
}

std::string MayaUIInfoHandler::getLongRunTimeLabel() const { return "Maya"; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
