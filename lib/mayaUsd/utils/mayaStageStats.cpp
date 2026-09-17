//
// Copyright 2026 Sony Interactive Entertainment
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
#include "mayaStageStats.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MProfiler.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/sceneItem.h>

#include <exception>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

const auto profilerCategory = MProfiler::addCategory("USD Details", "USD Details");

// Subtree target to traverse
struct Target
{
    PXR_NS::UsdPrim       prim;     // root of the subtree to traverse
    PXR_NS::UsdTimeCode   time;     // time the shape is showing
    PXR_NS::SdfPathVector excluded; // excludePrimPaths
    bool                  drawRender = false;
    bool                  drawProxy = true;
    bool                  drawGuide = false;
};

MayaUsdProxyShapeBase* proxyShapeFor(const Ufe::Path& path)
{
    if (path.empty()) {
        return nullptr;
    }

    if (MayaUsdProxyShapeBase* shape = ufe::getProxyShape(path)) {
        return shape;
    }

    const Ufe::SceneItem::Ptr item = Ufe::Hierarchy::createItem(path);
    return item ? ufe::getProxyShapeFromItemOrChildren(item) : nullptr;
}

bool targetFromShape(MayaUsdProxyShapeBase* shape, Target* target)
{
    if (!shape) {
        return false;
    }

    PXR_NS::UsdPrim       rootPrim;
    PXR_NS::SdfPathVector excluded;
    int                   complexity = 0;
    PXR_NS::UsdTimeCode   time;
    bool                  drawRender = false;
    bool                  drawProxy = false;
    bool                  drawGuide = false;

    if (!shape->GetAllRenderAttributes(
            &rootPrim, &excluded, &complexity, &time, &drawRender, &drawProxy, &drawGuide)) {
        return false;
    }

    if (!rootPrim || !rootPrim.IsValid()) {
        return false;
    }

    target->prim = rootPrim;
    target->time = time;
    target->excluded = excluded;
    target->drawRender = drawRender;
    target->drawProxy = drawProxy;
    target->drawGuide = drawGuide;
    return true;
}

bool targetFromUfePath(const Ufe::Path& path, Target* target)
{
    if (!targetFromShape(proxyShapeFor(path), target)) {
        return false;
    }

    if (path.nbSegments() > 1) {
        const PXR_NS::UsdPrim prim = ufe::ufePathToPrim(path);
        if (!prim || !prim.IsValid()) {
            return false;
        }
        target->prim = prim;
    }

    return true;
}

std::string normalizeMayaSegment(const std::string& arg)
{
    const std::string::size_type sep = arg.find(Ufe::PathString::pathSegmentSeparator());
    const std::string            dagPart = arg.substr(0, sep);

    if (dagPart.empty() || dagPart.front() == '|') {
        return arg;
    }

    const MDagPath dagPath = UsdMayaUtil::nameToDagPath(dagPart);
    if (!dagPath.isValid()) {
        return arg;
    }

    const std::string fullPath = dagPath.fullPathName().asChar();
    return sep == std::string::npos ? fullPath : fullPath + arg.substr(sep);
}

bool targetFromString(const std::string& arg, Target* target)
{
    Ufe::Path path;
    try {
        path = Ufe::PathString::path(normalizeMayaSegment(arg));
    } catch (const std::exception&) {
        return false;
    }

    return targetFromUfePath(path, target);
}

std::vector<Target> targetsFromSelection()
{
    std::vector<Target> targets;

    const auto globalSelection = Ufe::GlobalSelection::get();
    if (!globalSelection) {
        return targets;
    }

    for (const Ufe::SceneItem::Ptr& item : *globalSelection) {
        if (!item) {
            continue;
        }
        Target target;
        if (targetFromUfePath(item->path(), &target)) {
            targets.push_back(target);
        }
    }

    return targets;
}

std::vector<Target> targetsFromAllStages()
{
    std::vector<Target> targets;
    for (const Ufe::Path& path : ufe::getAllStagesPaths()) {
        Target target;
        if (targetFromShape(ufe::getProxyShape(path), &target)) {
            targets.push_back(target);
        }
    }

    return targets;
}

} // namespace

StageStats
ComputeMayaStageStats(const std::vector<std::string>& objects, const StageStatsOptions& requested)
{
    MProfilingScope profilingScope(profilerCategory, MProfiler::kColorD_L1, "Compute USD Details");

    std::vector<Target> targets;

    for (const std::string& object : objects) {
        Target target;
        if (targetFromString(object, &target)) {
            targets.push_back(target);
        } else {
            MGlobal::displayWarning(
                MString("USD Details: ignoring unresolved object \"") + object.c_str() + "\"");
        }
    }

    if (objects.empty()) {
        targets = targetsFromSelection();
        if (targets.empty()) {
            targets = targetsFromAllStages();
        }
    }

    StageStats result;

    for (const Target& target : targets) {
        StageStatsOptions options = requested;
        options.excludedPaths = target.excluded;
        options.time = target.time;
        options.drawRender = target.drawRender;
        options.drawProxy = target.drawProxy;
        options.drawGuide = target.drawGuide;

        result += ComputeStageStats(target.prim, options);
    }

    return result;
}

} // namespace MAYAUSD_NS_DEF