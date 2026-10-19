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
#include <pxr/usd/usd/stage.h>

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MProfiler.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/sceneItem.h>

#include <exception>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

const auto profilerCategory = MProfiler::addCategory("USD Details", "USD Details");

struct ShapeTargets
{
    PXR_NS::UsdStageRefPtr stage;
    PXR_NS::SdfPath        shapeRoot;
    PXR_NS::SdfPathVector  roots;
    PXR_NS::UsdTimeCode    time;
    PXR_NS::SdfPathVector  excluded;
    bool                   drawRender = false;
    bool                   drawProxy = true;
    bool                   drawGuide = false;
};

using TargetMap = std::unordered_map<MayaUsdProxyShapeBase*, ShapeTargets>;

MayaUsdProxyShapeBase* proxyShapeFor(const Ufe::Path& path)
{
    return path.empty() ? nullptr : ufe::getProxyShape(path);
}

std::vector<MayaUsdProxyShapeBase*> proxyShapesUnder(const Ufe::Path& path)
{
    std::vector<MayaUsdProxyShapeBase*> shapes;

    if (MayaUsdProxyShapeBase* shape = proxyShapeFor(path)) {
        shapes.push_back(shape);
        return shapes;
    }

    const MDagPath dagPath = ufe::ufeToDagPath(path);
    if (!dagPath.isValid()) {
        return shapes;
    }

    MItDag dagIt;
    for (dagIt.reset(dagPath, MItDag::kDepthFirst, MFn::kPluginShape); !dagIt.isDone();
         dagIt.next()) {
        const MFnDagNode fnDagNode(dagIt.currentItem());
        if (auto* shape = dynamic_cast<MayaUsdProxyShapeBase*>(fnDagNode.userNode())) {
            shapes.push_back(shape);
        }
    }

    return shapes;
}

ShapeTargets* shapeTargetsFor(MayaUsdProxyShapeBase* shape, TargetMap* targets)
{
    if (!shape) {
        return nullptr;
    }

    const auto found = targets->find(shape);
    if (found != targets->end()) {
        return &found->second;
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
        return nullptr;
    }

    if (!rootPrim || !rootPrim.IsValid()) {
        return nullptr;
    }

    ShapeTargets entry;
    entry.stage = rootPrim.GetStage();
    entry.shapeRoot = rootPrim.GetPath();
    entry.time = time;
    entry.excluded = excluded;
    entry.drawRender = drawRender;
    entry.drawProxy = drawProxy;
    entry.drawGuide = drawGuide;
    return &targets->emplace(shape, std::move(entry)).first->second;
}

bool addTargetFromShape(MayaUsdProxyShapeBase* shape, TargetMap* targets)
{
    ShapeTargets* entry = shapeTargetsFor(shape, targets);
    if (!entry) {
        return false;
    }

    entry->roots.push_back(entry->shapeRoot);
    return true;
}

bool addTargetFromUfePath(const Ufe::Path& path, TargetMap* targets)
{
    if (path.nbSegments() <= 1) {
        bool added = false;
        for (MayaUsdProxyShapeBase* shape : proxyShapesUnder(path)) {
            added = addTargetFromShape(shape, targets) || added;
        }
        return added;
    }

    ShapeTargets* entry = shapeTargetsFor(proxyShapeFor(path), targets);
    if (!entry) {
        return false;
    }

    const PXR_NS::UsdPrim prim = ufe::ufePathToPrim(path);
    if (!prim || !prim.IsValid()) {
        return false;
    }

    entry->roots.push_back(prim.GetPath());
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

bool addTargetFromString(const std::string& arg, TargetMap* targets)
{
    Ufe::Path path;
    try {
        path = Ufe::PathString::path(normalizeMayaSegment(arg));
    } catch (const std::exception&) {
        return false;
    }

    return addTargetFromUfePath(path, targets);
}

bool addTargetsFromSelection(TargetMap* targets)
{
    const auto globalSelection = Ufe::GlobalSelection::get();
    if (!globalSelection) {
        return false;
    }

    bool added = false;
    for (const Ufe::SceneItem::Ptr& item : *globalSelection) {
        if (item && addTargetFromUfePath(item->path(), targets)) {
            added = true;
        }
    }

    return added;
}

void addTargetsFromAllStages(TargetMap* targets)
{
    for (const Ufe::Path& path : ufe::getAllStagesPaths()) {
        addTargetFromShape(ufe::getProxyShape(path), targets);
    }
}

} // namespace

StageStats
computeMayaStageStats(const std::vector<std::string>& objects, const StageStatsOptions& requested)
{
    MProfilingScope profilingScope(profilerCategory, MProfiler::kColorD_L1, "Compute USD Details");

    TargetMap targets;

    for (const std::string& object : objects) {
        if (!addTargetFromString(object, &targets)) {
            MGlobal::displayWarning(
                MString("USD Details: ignoring unresolved object \"") + object.c_str() + "\"");
        }
    }

    if (objects.empty() && !addTargetsFromSelection(&targets)) {
        addTargetsFromAllStages(&targets);
    }

    StageStats result;

    for (auto& entry : targets) {
        ShapeTargets& shapeTargets = entry.second;

        // Overlapping selections must only be counted once: drop duplicate roots and roots
        // already covered by a selected ancestor.
        PXR_NS::SdfPath::RemoveDescendentPaths(&shapeTargets.roots);

        StageStatsOptions options = requested;
        options.excludedPaths = shapeTargets.excluded;
        options.time = shapeTargets.time;
        options.drawRender = shapeTargets.drawRender;
        options.drawProxy = shapeTargets.drawProxy;
        options.drawGuide = shapeTargets.drawGuide;

        for (const PXR_NS::SdfPath& root : shapeTargets.roots) {
            const PXR_NS::UsdPrim prim = shapeTargets.stage->GetPrimAtPath(root);
            if (prim) {
                result += computeStageStats(prim, options);
            }
        }
    }

    return result;
}

} // namespace MAYAUSD_NS_DEF