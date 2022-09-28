//
// Copyright 2016 Pixar
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
#include "primUpdaterContext.h"

#include <mayaUsd/ufe/Utils.h>

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#endif

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimUpdaterContext::UsdMayaPrimUpdaterContext(
    const UsdTimeCode&            timeCode,
    const UsdStageRefPtr&         stage,
    const VtDictionary&           userArgs,
    const UsdPathToDagPathMapPtr& pathMap)
    : _timeCode(timeCode)
    , _stage(stage)
    , _pathMap(pathMap)
    , _userArgs(userArgs)
    , _args(UsdMayaPrimUpdaterArgs::createFromDictionary(userArgs))
{
}

MDagPath UsdMayaPrimUpdaterContext::MapSdfPathToDagPath(const SdfPath& sdfPath) const
{
    if (!_pathMap || sdfPath.IsEmpty()) {
        return MDagPath();
    }

    auto found = _pathMap->find(sdfPath);
    return found == _pathMap->end() ? MDagPath() : found->second;
}

void UsdMayaPrimUpdaterContext::replicateExtrasFromUSD_Start(Ufe::SceneItem::Ptr ufeItem)
{
    auto node = Ufe::Hierarchy::hierarchy(ufeItem);
    if (!node) {
        return;
    }

    // Go through the entire hierarchy
    for (auto child : node->children()) {
        replicateExtrasFromUSD_Start(child);
    }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Prepare _displayLayerMap
    MFnDisplayLayerManager displayLayerManager(
        MFnDisplayLayerManager::currentDisplayLayerManager());

    MObject displayLayerObj
        = displayLayerManager.getLayer(Ufe::PathString::string(ufeItem->path()).c_str());
    if (displayLayerObj.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(displayLayerObj);
        if (displayLayer.name() != "defaultLayer") {
            _displayLayerMap[ufeItem->path()] = displayLayerObj;
        }
    }
#endif
}

void UsdMayaPrimUpdaterContext::replicateExtrasFromUSD_Item(
    const Ufe::Path& path,
    const MObject&   mayaObject) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API    
    // Replicate display layer membership
    auto it = _displayLayerMap.find(path);
    if (it != _displayLayerMap.end() && it->second.hasFn(MFn::kDisplayLayer)) {
        MDagPath dagPath;
        if (MDagPath::getAPathTo(mayaObject, dagPath) == MStatus::kSuccess) {
            MFnDisplayLayer displayLayer(it->second);
            displayLayer.add(dagPath);

            // In case display layer membership was removed from the USD prim that we are
            // replicating, we want to restore it here to make sure that the prim will stay in its
            // display layer on DiscardEdits
            displayLayer.add(Ufe::PathString::string(path).c_str());
        }
    }
#endif    
}

void UsdMayaPrimUpdaterContext::replicateExtrasToUSD_Item(
    const MDagPath& dagPath,
    const SdfPath&  usdPath) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Populate display layer membership map

    // Since multiple dag paths may lead to a single USD path (like transform and node),
    // we have to make sure we don't overwrite a non-default layer with a default one
    bool displayLayerAssigned = false;
    auto entry = _primToLayerMap.find(usdPath);
    if (entry != _primToLayerMap.end() && entry->second.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(entry->second);
        displayLayerAssigned = (displayLayer.name() != "defaultLayer");
    }

    if (!displayLayerAssigned) {
        MFnDisplayLayerManager displayLayerManager(
            MFnDisplayLayerManager::currentDisplayLayerManager());
        _primToLayerMap[usdPath] = displayLayerManager.getLayer(dagPath);
    }
#endif
}

void UsdMayaPrimUpdaterContext::replicateExtrasToUSD_End() const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    for (const auto& entry : _primToLayerMap) {
        if (entry.second.hasFn(MFn::kDisplayLayer)) {
            auto                psPath = MayaUsd::ufe::stagePath(_stage);
            Ufe::Path::Segments segments { psPath.getSegments()[0],
                                           MayaUsd::ufe::usdPathToUfePathSegment(entry.first) };
            Ufe::Path           ufePath(std::move(segments));

            MFnDisplayLayer displayLayer(entry.second);
            displayLayer.add(Ufe::PathString::string(ufePath).c_str());
        }
    }
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
