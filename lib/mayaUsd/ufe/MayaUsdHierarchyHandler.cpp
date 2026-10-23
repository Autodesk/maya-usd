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
#include "MayaUsdHierarchyHandler.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/fileio/pullInformation.h>
#include <mayaUsd/ufe/MayaUsdHierarchy.h>
#include <mayaUsd/ufe/MayaUsdRootChildHierarchy.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <ufe/hierarchy.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>

#include <string>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdHierarchyHandler, MayaUsdHierarchyHandler);

/*static*/
MayaUsdHierarchyHandler::Ptr MayaUsdHierarchyHandler::create()
{
    return std::make_shared<MayaUsdHierarchyHandler>();
}

//------------------------------------------------------------------------------
// UsdHierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr MayaUsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    auto usdItem = downcast(item);
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }
    if (UsdUfe::isRootChild(usdItem->path())) {
        return MayaUsdRootChildHierarchy::create(usdItem);
    }
    return MayaUsdHierarchy::create(usdItem);
}

#ifdef MAYAUSD_NEED_OUTLINER_FILTER_FIX

static std::map<std::string, bool> fsCachedFilterValues = {
    { "InactivePrims", true },
    { "ClassPrims", false },
    { "RenderPrims", false },
};

// This flag prevents infinite recursion.
static std::atomic<bool> hasPendingFilterDefaultsUpdate(false);

static void updateFilterDefaults()
{
    // This script finds a valid outiner panel that has valid UFE filters
    // settings. Calling this script call to the outlinerEditor command
    // triggers a call to UFE childFilter function, which would cause an
    // infinite recursion if we did not protect against it.
    static const char outlinerScriptTemplate[] = R"MEL(
        proc string _getUSDFilterValue() {
            string $outliners[] = `getPanel -type "outlinerPanel"`;
            for ($index = 0; $index < size($outliners); $index++) {
                string $outliner = $outliners[$index];
                string $value = `outlinerEditor -query -ufeFilter "USD" "^1s" -ufeFilterValue $outliner`;
                if (size($value) > 0) {
                    return $value;
                }
            }
            return "";
        }
        _getUSDFilterValue()
        )MEL";

    try {
        for (auto& nameAndValue : fsCachedFilterValues) {
            MString script;
            script.format(outlinerScriptTemplate, nameAndValue.first.c_str());
            const MString value = MGlobal::executeCommandStringResult(script);
            if (value.length() > 0 && value.isInt()) {
                nameAndValue.second = (value.asInt() != 0);
            }
        }
    } catch (const std::exception&) {
        // Ignore exceptions in callbacks.
    }

    // Allow another filter update in the future.
    hasPendingFilterDefaultsUpdate = false;
}

static void triggerUpdateFilterDefaults()
{
    // Don't trigger an update if there is already one pending.
    if (hasPendingFilterDefaultsUpdate)
        return;

    // Mark that an update will be pending.
    hasPendingFilterDefaultsUpdate = true;

    updateFilterDefaults();
}

Ufe::Hierarchy::ChildFilter MayaUsdHierarchyHandler::childFilter() const
{
    Ufe::Hierarchy::ChildFilter filters = UsdUfe::UsdHierarchyHandler::childFilter();
    for (Ufe::ChildFilterFlag& filter : filters) {
        const auto iter = fsCachedFilterValues.find(filter.name);
        if (iter != fsCachedFilterValues.end()) {
            filter.value = fsCachedFilterValues[filter.name];
        }
    }

    triggerUpdateFilterDefaults();

    return filters;
}

#else

Ufe::Hierarchy::ChildFilter MayaUsdHierarchyHandler::childFilter() const
{
    return UsdUfe::UsdHierarchyHandler::childFilter();
}

#endif

#ifdef UFE_HIERARCHY_HAS_FIND
Ufe::Selection MayaUsdHierarchyHandler::find(
    Ufe::Rtid /*runTimeId*/,
    const Ufe::Path&                        searchRoot,
    const Ufe::Hierarchy::NameMatchFcn&     matches,
    const Ufe::Hierarchy::ChildFilter&      childFilter,
    const Ufe::Hierarchy::FindInterruptFcn& interrupt) const
{
    return findStagePrims(searchRoot, matches, childFilter, interrupt);
}
#endif

/*static*/
Ufe::Selection MayaUsdHierarchyHandler::findStagePrims(
    const Ufe::Path&                   searchRoot,
    const NameMatchFcn&                matches,
    const Ufe::Hierarchy::ChildFilter& childFilter,
    const std::function<bool()>&       interrupt)
{
    PXR_NAMESPACE_USING_DIRECTIVE

    UsdStageMap::PathValidationScope pathValidationScope;

    const bool hasPulledPrims = PrimUpdaterManager::getInstance().hasPulledPrims();

    using FindPrimHook = UsdUfe::UsdHierarchyHandler::FindPrimHook;

    // Mirrors the filtering and remapping of mayaUsdHierarchyChildrenHook()
    // and GatewayHierarchy::createUFEChildList().
    auto hookFactory = [&](const Ufe::Path& stagePath) -> FindPrimHook {
        const SdfPath primPath = getProxyShapePrimPath(stagePath);
        if (primPath.IsEmpty()) {
            return [](const UsdPrim&, Ufe::Selection&) { return true; };
        }
        const bool isScoped = primPath != SdfPath::AbsoluteRootPath();
        if (!isScoped && !hasPulledPrims) {
            return {};
        }

        return [&, primPath, isScoped](const UsdPrim& prim, Ufe::Selection& found) {
            if (isScoped) {
                const SdfPath& path = prim.GetPath();
                if (!path.HasPrefix(primPath) && !primPath.HasPrefix(path)) {
                    return true;
                }
            }

            std::string dagPathStr;
            if (!hasPulledPrims || !MayaUsd::readPullInformation(prim, dagPathStr)) {
                return false;
            }
            const Ufe::SceneItem::Ptr mayaItem
                = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
            if (!mayaItem) {
                return false;
            }
#ifdef UFE_HIERARCHY_HAS_FIND
            const Ufe::Rtid mayaRtid = mayaItem->runTimeId();
            if (auto mayaHandler = Ufe::RunTimeMgr::instance().hierarchyHandler(mayaRtid)) {
                const Ufe::Selection mayaFound = mayaHandler->find(
                    mayaRtid, mayaItem->path(), matches, childFilter, interrupt);
                for (const auto& item : mayaFound) {
                    found.append(item);
                }
            }
#endif
            return true;
        };
    };

    return UsdUfe::UsdHierarchyHandler::findPrims(
        UsdStageMap::getInstance().allStagesPaths(),
        searchRoot,
        matches,
        childFilter,
        interrupt,
        hookFactory);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
