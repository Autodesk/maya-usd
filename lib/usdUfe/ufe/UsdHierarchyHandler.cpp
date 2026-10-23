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
#include "UsdHierarchyHandler.h"

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdHierarchy.h>
#include <usdUfe/ufe/UsdRootChildHierarchy.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

namespace {

// Number of prims traversed between two polls of the interrupt callback.
constexpr std::size_t kInterruptPollInterval = 1024;

// Traversal shared by all the stages searched by findPrims().
class PrimFinder
{
public:
    PrimFinder(
        const UsdHierarchyHandler::NameMatchFcn& matches,
        const Ufe::Hierarchy::ChildFilter&       childFilter,
        const std::function<bool()>&             interrupt)
        : _matches(matches)
        , _interrupt(interrupt)
    {
        PXR_NAMESPACE_USING_DIRECTIVE

        // Mirror UsdHierarchy::children() for an empty filter, and
        // UsdHierarchy::filteredChildren() otherwise.
        if (childFilter.empty()) {
            _predicate = UsdTraverseInstanceProxies(UsdPrimIsDefined && !UsdPrimIsAbstract);
            _filterInactive = true;
        } else {
            _predicate = UsdTraverseInstanceProxies(getUsdPredicate(childFilter));
            _removeRenderPrims = !showsRenderPrims(childFilter);
        }
    }

    const PXR_NS::Usd_PrimFlagsPredicate& predicate() const { return _predicate; }

    bool interrupted() const { return _interrupted; }

    // Append to found the prims of range whose name matches. Only matches
    // are turned into UFE scene items.
    void search(
        const PXR_NS::UsdPrimRange&              range,
        const Ufe::Path&                         stagePath,
        const UsdHierarchyHandler::FindPrimHook& hook,
        Ufe::Selection&                          found)
    {
        for (auto it = range.begin(); it != range.end(); ++it) {
            if (_interrupt && (_nbTraversed++ % kInterruptPollInterval) == 0 && _interrupt()) {
                _interrupted = true;
                return;
            }

            const PXR_NS::UsdPrim& prim = *it;
            if ((hook && hook(prim, found)) || (_filterInactive && !prim.IsActive())
                || (_removeRenderPrims && isRenderPrimOrRenderOnlyScope(prim, _predicate))) {
                it.PruneChildren();
                continue;
            }

            const std::string& primName = prim.GetName().GetString();
            if (primName.empty() || !_matches(primName))
                continue;
            const Ufe::Path path = stagePath + usdPathToUfePathSegment(prim.GetPath());
            found.append(UsdSceneItem::create(path, prim));
        }
    }

private:
    const UsdHierarchyHandler::NameMatchFcn& _matches;
    bool                                     _filterInactive { false };
    bool                                     _removeRenderPrims { false };
    PXR_NS::Usd_PrimFlagsPredicate           _predicate;
    const std::function<bool()>&             _interrupt;
    std::size_t                              _nbTraversed { 0 };
    bool                                     _interrupted { false };
};

} // namespace

USDUFE_VERIFY_CLASS_SETUP(Ufe::HierarchyHandler, UsdHierarchyHandler);

/*static*/
UsdHierarchyHandler::Ptr UsdHierarchyHandler::create()
{
    return std::make_shared<UsdHierarchyHandler>();
}

//------------------------------------------------------------------------------
// UsdHierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr UsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    auto usdItem = downcast(item);
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }
    return isRootChild(usdItem->path()) ? UsdRootChildHierarchy::create(usdItem)
                                        : UsdHierarchy::create(usdItem);
}

Ufe::SceneItem::Ptr UsdHierarchyHandler::createItem(const Ufe::Path& path) const
{
    PXR_NS::UsdPrim prim;
    const int       instanceIndex = ufePathToInstanceIndex(path, &prim);
    return prim.IsValid() ? UsdSceneItem::create(path, prim, instanceIndex) : nullptr;
}

Ufe::Hierarchy::ChildFilter UsdHierarchyHandler::childFilter() const
{
    Ufe::Hierarchy::ChildFilter childFilters;
    childFilters.emplace_back("InactivePrims", "Inactive Prims", true);
    childFilters.emplace_back("ClassPrims", "Class Prims", false);
    childFilters.emplace_back("RenderPrims", "Render Prims", false);
    return childFilters;
}

/*static*/
Ufe::Selection UsdHierarchyHandler::findPrims(
    const std::vector<Ufe::Path>&      stagesPaths,
    const Ufe::Path&                   searchRoot,
    const NameMatchFcn&                matches,
    const Ufe::Hierarchy::ChildFilter& childFilter,
    const std::function<bool()>&       interrupt,
    const FindPrimHookFactory&         hookFactory)
{
    PXR_NAMESPACE_USING_DIRECTIVE

    Ufe::Selection found;
    if (!matches)
        return found;

    PrimFinder finder(matches, childFilter, interrupt);

    if (!searchRoot.empty() && searchRoot.runTimeId() == getUsdRunTimeId()) {
        const UsdPrim rootPrim = ufePathToPrim(searchRoot);
        if (rootPrim) {
            const Ufe::Path stagePath = searchRoot.popSegment();
            finder.search(
                UsdPrimRange(rootPrim, finder.predicate()),
                stagePath,
                hookFactory ? hookFactory(stagePath) : FindPrimHook(),
                found);
        }
        return found;
    }

    for (const Ufe::Path& stagePath : stagesPaths) {
        if (!searchRoot.empty() && !stagePath.startsWith(searchRoot))
            continue;
        UsdStageWeakPtr stage = getStage(stagePath);
        if (!stage)
            continue;
        finder.search(
            stage->Traverse(finder.predicate()),
            stagePath,
            hookFactory ? hookFactory(stagePath) : FindPrimHook(),
            found);
        if (finder.interrupted())
            break;
    }

    return found;
}

} // namespace USDUFE_NS_DEF
