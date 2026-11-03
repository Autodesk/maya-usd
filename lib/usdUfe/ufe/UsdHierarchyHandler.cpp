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
#include <pxr/base/tf/envSetting.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE
TF_DEFINE_ENV_SETTING(
    USDUFE_FIND_INTERRUPT_POLL_INTERVAL,
    1024,
    "Number of prims traversed between two polls of the interrupt callback during a "
    "hierarchy search. Values below 1 are treated as 1.");
PXR_NAMESPACE_CLOSE_SCOPE

namespace USDUFE_NS_DEF {

namespace {

std::size_t interruptPollInterval()
{
    static const std::size_t interval = static_cast<std::size_t>(
        std::max(1, PXR_NS::TfGetEnvSetting(PXR_NS::USDUFE_FIND_INTERRUPT_POLL_INTERVAL)));
    return interval;
}

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
        , _pollInterval(interruptPollInterval())
    {
        // Mirror UsdHierarchy::children() for an empty filter, and
        // UsdHierarchy::filteredChildren() otherwise.
        if (childFilter.empty()) {
            _predicate = PXR_NS::UsdTraverseInstanceProxies(
                PXR_NS::UsdPrimIsDefined && !PXR_NS::UsdPrimIsAbstract);
            _filterInactive = true;
        } else {
            _predicate = PXR_NS::UsdTraverseInstanceProxies(getUsdPredicate(childFilter));
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
            if (_interrupt && (_nbTraversed++ % _pollInterval) == 0 && _interrupt()) {
                _interrupted = true;
                return;
            }

            const PXR_NS::UsdPrim& prim = *it;
            if (hook && hook(prim, found)) {
                // The hook may have run a nested search that was interrupted
                // by the same callback, without this finder knowing about it.
                if (_interrupt && _interrupt()) {
                    _interrupted = true;
                    return;
                }
                it.PruneChildren();
                continue;
            }
            if ((_filterInactive && !prim.IsActive())
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
    const std::size_t                        _pollInterval;
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
    Ufe::Selection found;
    if (!matches)
        return found;

    PrimFinder finder(matches, childFilter, interrupt);

    if (!searchRoot.empty() && searchRoot.runTimeId() == getUsdRunTimeId()) {
        PXR_NS::UsdPrim    rootPrim;
        const int          instanceIndex = ufePathToInstanceIndex(searchRoot, &rootPrim);
        const Ufe::Path    stagePath = searchRoot.popSegment();
        const FindPrimHook hook
            = (rootPrim && hookFactory) ? hookFactory(stagePath) : FindPrimHook();
        if (rootPrim && instanceIndex >= 0) {
            // A point instance has no children, see UsdHierarchy. The hook
            // still decides whether its instancer is shown.
            if (interrupt && interrupt())
                return found;
            Ufe::Selection hookFound;
            if (hook && hook(rootPrim, hookFound))
                return found;
            if (matches(searchRoot.back().string())) {
                found.append(UsdSceneItem::create(searchRoot, rootPrim, instanceIndex));
            }
        } else if (rootPrim) {
            finder.search(
                PXR_NS::UsdPrimRange(rootPrim, finder.predicate()), stagePath, hook, found);
        }
        return found;
    }

    for (const Ufe::Path& stagePath : stagesPaths) {
        if (!searchRoot.empty() && !stagePath.startsWith(searchRoot))
            continue;
        PXR_NS::UsdStageWeakPtr stage = getStage(stagePath);
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
