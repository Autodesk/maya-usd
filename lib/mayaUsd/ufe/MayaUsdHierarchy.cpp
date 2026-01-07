//
// Copyright 2023 Autodesk
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
#include "MayaUsdHierarchy.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/trace/trace.h>
#include <pxr/base/work/dispatcher.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <tbb/concurrent_unordered_map.h>

#include <mutex>
#include <set>
#include <string>

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(
    MAYAUSD_ENABLE_HIERARCHY_CHILDREN_CACHE,
    true,
    "Enable UsdHierarchy children cache.");

TF_DEFINE_ENV_SETTING(
    MAYAUSD_DEBUG_HIERARCHY_CHILDREN_CACHE,
    false,
    "Debug UsdHierarchy children cache.");

Ufe::Path createUfePath(const std::string& path)
{
    // Ufe::Path object is reference counted, so we need an exclusive mutex to prevent
    // concurrent creation issues.
    static std::mutex ufePathMutex;
    std::lock_guard   lock(ufePathMutex);
    return Ufe::PathString::path(path);
}

// Create a UFE scene item from pull information (Maya DAG path)
bool createUfeSceneItem(const std::string& pullInfo, Ufe::SceneItemList& sceneItems)
{
    if (auto sceneItem = Ufe::Hierarchy::createItem(createUfePath(pullInfo))) {
        sceneItems.emplace_back(sceneItem);
        return true;
    }
    return false;
}

// Create a UFE scene item from USD prim
bool createUfeSceneItem(
    const std::string&  ufePath,
    const UsdPrim&      prim,
    Ufe::SceneItemList& sceneItems)
{
    if (auto sceneItem = USDUFE_NS_DEF::UsdSceneItem::create(createUfePath(ufePath), prim)) {
        sceneItems.emplace_back(sceneItem);
        return true;
    }
    return false;
}

namespace cache {

// Cache to manage UsdHierarchy children
class UsdHierarchyCache
{
public:
    using PathMap = tbb::concurrent_unordered_map<SdfPath, Ufe::SceneItemList, TfHash>;

    UsdHierarchyCache() = default;

    const Ufe::SceneItemList&
    getChildren(const Ufe::Path& ufePath, const UsdPrim& prim, const Usd_PrimFlagsPredicate& pred)
    {
        if (const auto it = _cache.find(pred); it != _cache.end()) {
            auto& pathMap = it->second;
            if (const auto pathIt = pathMap.find(prim.GetPath()); pathIt != pathMap.end()) {
                return pathIt->second;
            }
        }

        // Init the proxy shape path
        // NOTE: This is only done once per cache instance that is per stage,
        //       proxy shape and stage *should* have a 1 to 1 mapping so the overhead is minimal.
        if (_proxyShapePrimPath.IsEmpty()) {
            _proxyShapePrimPath = MAYAUSD_NS_DEF::ufe::getProxyShapePrimPath(ufePath);
        }

        if (_proxyShapeDagPath.empty()) {
            // NOTE: Parse the ufe path to get the proxy shape dag path (prefix of the usd path)
            //       This is slightly faster compared to accessing via Utils::getProxyShape(),
            //       since we knew the proxy shape must be valid if we are here.
            if (const auto slash = ufePath.string().find_first_of('/');
                slash != std::string::npos) {
                _proxyShapeDagPath = ufePath.string().substr(0, slash);
            }
        }

        if (_proxyShapePrimPath.IsEmpty() || _proxyShapeDagPath.empty()) {
            static Ufe::SceneItemList emptyList;
            return emptyList;
        }

        auto& pathMap = _cache[pred];
        // Then populate from requested prim
        // It's possible that the requested prim is not in the root subtree
        if (_populate(pred, prim, pathMap)) {
            _dispatcher.Wait();
        }
        return pathMap[prim.GetPath()];
    }

private:
    bool _populate(const Usd_PrimFlagsPredicate& pred, const UsdPrim& prim, PathMap& pathMap)
    {
        const auto& path = prim.GetPath();
        if (const auto it = pathMap.find(path); it != pathMap.end()) {
            // Already populated
            return false;
        }

        // NOTE: Skip point instancer prims
        if (!prim || prim.IsA<UsdGeomPointInstancer>()) {
            pathMap.emplace(path, Ufe::SceneItemList());
            return false;
        }

        // NOTE: Skip traversing into instance proxies
        const auto children = prim.GetFilteredChildren(pred);

        pathMap.emplace(path, Ufe::SceneItemList());
        if (!children.empty()) {
            std::list<UsdPrim> pendingChildren;
            auto&              sceneItems = pathMap[path];
            for (const auto& child : children) {
                bool itemCreated = false;
                // Create the ufe item from pull info if the hook has handled it
                if (MAYAUSD_NS_DEF::ufe::mayaUsdHierarchyChildrenHook(
                        _proxyShapePrimPath, child, sceneItems, true, &itemCreated)) {
                    if (!itemCreated) {
                        // Skip if scene item not created: item should be filtered out
                        continue;
                    }
                } else if (child.IsActive()) {
                    // Create the ufe item if no pull info or not handled by the hook
                    createUfeSceneItem(
                        _proxyShapeDagPath + child.GetPath().GetString(), child, sceneItems);
                }
                // Queue for further population
                pendingChildren.emplace_back(child);
            }

            if (!pendingChildren.empty()) {
                _dispatcher.Run([this, pred, pendingChildren, &pathMap]() {
                    for (const auto& child : pendingChildren) {
                        _populate(pred, child, pathMap);
                    }
                });
                return true;
            }
        }
        return false;
    }

    tbb::concurrent_unordered_map<Usd_PrimFlagsPredicate, PathMap, TfHash> _cache;
    WorkDispatcher                                                         _dispatcher;
    SdfPath                                                                _proxyShapePrimPath;
    std::string                                                            _proxyShapeDagPath;
};

// Hash and compare methods to fulfil tbb::concurrent_unordered_map requirements
struct UsdStageWeakPtrHashCompare
{
    size_t operator()(const UsdStageWeakPtr& x) const { return TfHash()(x.GetUniqueIdentifier()); }

    bool operator()(const UsdStageWeakPtr& x, const UsdStageWeakPtr& y) const { return x == y; }
};

// Guard to indicate we are in a stage changing operation,
// the caller is in main thread, a simple bool is sufficient.
bool s_inStageChangingGuard = false;

// Map of hierarchy caches per stage
using UsdHierarchyCacheMap = tbb::concurrent_unordered_map<
    UsdStageWeakPtr,
    std::unique_ptr<UsdHierarchyCache>,
    UsdStageWeakPtrHashCompare>;
UsdHierarchyCacheMap s_hierarchyCacheMap;

bool getChildren(
    const Ufe::Path&              ufePath,
    const UsdPrim&                prim,
    const Usd_PrimFlagsPredicate& pred,
    Ufe::SceneItemList&           children)
{
    if (!s_inStageChangingGuard) {
        return false;
    }

    if (!TfGetEnvSetting(MAYAUSD_ENABLE_HIERARCHY_CHILDREN_CACHE)) {
        return false;
    }

    // We have to organize the cache per stage, because at the time when `children()` is called,
    // we cannot be sure that the stage is always the same, if we could have more detail info from
    // the caller, we might be able to have a better cache key.
    const auto stage = prim.GetStage();
    if (const auto it = s_hierarchyCacheMap.find(stage); it != s_hierarchyCacheMap.end()) {
        children = it->second->getChildren(ufePath, prim, pred);
        return true;
    }

    s_hierarchyCacheMap.emplace(stage, std::make_unique<UsdHierarchyCache>());
    children = s_hierarchyCacheMap[stage]->getChildren(ufePath, prim, pred);
    return true;
}

} // namespace cache
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

void mayaUsdHierarchyStageChangedBegin() { cache::s_inStageChangingGuard = true; }

void mayaUsdHierarchyStageChangedEnd()
{
    cache::s_inStageChangingGuard = false;
    cache::s_hierarchyCacheMap.clear();
}

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdHierarchy, MayaUsdHierarchy);

MayaUsdHierarchy::MayaUsdHierarchy(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdUfe::UsdHierarchy(item)
{
}

/*static*/
MayaUsdHierarchy::Ptr MayaUsdHierarchy::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdHierarchy>(item);
}

//------------------------------------------------------------------------------
// UsdHierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItemList MayaUsdHierarchy::children() const
{
    const auto         item = usdSceneItem();
    Ufe::SceneItemList children;
    if (cache::getChildren(
            item->path(), item->prim(), UsdHierarchy::usdUfePrimDefaultPredicate(), children)) {

        if (!TfGetEnvSetting(MAYAUSD_DEBUG_HIERARCHY_CHILDREN_CACHE)) {
            return children;
        }

        // Debugging code to compare cached children with UsdHierarchy::children()
        std::set<std::string> cachedPaths;
        for (const auto& child : children) {
            cachedPaths.emplace(child->path().string());
        }

        Ufe::SceneItemList    nonCachedChildren = UsdHierarchy::children();
        std::set<std::string> nonCachedPaths;
        for (const auto& child : nonCachedChildren) {
            auto path = child->path().string();
            if (cachedPaths.find(path) == cachedPaths.end()) {
                if (auto usdUfeItem = downcast(child)) {
                    const auto prim = usdUfeItem->prim();
                    if (prim.IsInstance() || prim.IsInstanceProxy()) {
                        // Skip instance and instance proxy,
                        // these are known cases where the diff is expected
                        continue;
                    }
                }
            }
            nonCachedPaths.emplace(path);
        }

        if (cachedPaths != nonCachedPaths) {
            TF_WARN(
                "Cached children (%ld) different from non-cached children (%ld) for path: "
                "%s",
                cachedPaths.size(),
                nonCachedPaths.size(),
                item->path().string().c_str());

            std::set<std::string> inCache;
            std::set_difference(
                cachedPaths.begin(),
                cachedPaths.end(),
                nonCachedPaths.begin(),
                nonCachedPaths.end(),
                std::inserter(inCache, inCache.begin()));

            std::set<std::string> inNonCache;
            std::set_difference(
                nonCachedPaths.begin(),
                nonCachedPaths.end(),
                cachedPaths.begin(),
                cachedPaths.end(),
                std::inserter(inNonCache, inNonCache.begin()));

            if (!inCache.empty()) {
                TF_WARN("    In cache only: %ld", inCache.size());
                for (const auto& name : inCache) {
                    TF_WARN("        %s", name.c_str());
                }
            }
            if (!inNonCache.empty()) {
                TF_WARN("    In non-cache only: %ld", inNonCache.size());
                for (const auto& name : inNonCache) {
                    TF_WARN("        %s", name.c_str());
                }
            }
        }
        return nonCachedChildren;
    }

    // Caching disabled, get children the usual way.
    return UsdHierarchy::children();
}

bool MayaUsdHierarchy::childrenHook(
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive) const
{
    return mayaUsdHierarchyChildrenHook(
        getProxyShapePrimPath(sceneItem()->path()), child, children, filterInactive);
}

bool mayaUsdHierarchyChildrenHook(
    const PXR_NS::SdfPath& proxyShapePrimPath,
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive,
    bool*                  itemCreated)
{
    if (proxyShapePrimPath.IsEmpty()) {
        // An empty primPath means we're in a bad state.  We'll return true here
        // without populating children.
        return true;
    }

    const PXR_NS::SdfPath& childPath = child.GetPath();
    const bool             isAncestorOrDescendant
        = childPath.HasPrefix(proxyShapePrimPath) || proxyShapePrimPath.HasPrefix(childPath);
    if (!isAncestorOrDescendant) {
        // If it is not an ancestor or a descendent, we exclude it from the
        // children list.
        return true;
    }

    std::string dagPathStr;
    if (MayaUsd::readPullInformation(child, dagPathStr)) {
        // if we mapped to a valid object, insert it. it's possible that we got stale object
        // so in this case simply fallback to the usual processing of items
        if (createUfeSceneItem(dagPathStr, children)) {
            if (itemCreated) {
                *itemCreated = true;
            }
            return true;
        }
    }
    return false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
