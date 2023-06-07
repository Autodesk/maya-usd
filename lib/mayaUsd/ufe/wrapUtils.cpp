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
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateStageWithNewLayerCommand.h>

#include <ufe/undoableCommandMgr.h>
#endif

#include <boost/python.hpp>
#include <boost/python/def.hpp>

#include <string>

using namespace MayaUsd;
using namespace boost::python;

#ifdef UFE_V2_FEATURES_AVAILABLE
PXR_NS::UsdPrim getPrimFromRawItem(uint64_t rawItem)
{
    Ufe::SceneItem*       item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    UsdUfe::UsdSceneItem* usdItem = dynamic_cast<UsdUfe::UsdSceneItem*>(item);
    if (nullptr != usdItem) {
        return usdItem->prim();
    }
    return PXR_NS::UsdPrim();
}

std::string getNodeNameFromRawItem(uint64_t rawItem)
{
    std::string     name;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
        name = item->nodeName();
    return name;
}

std::string getNodeTypeFromRawItem(uint64_t rawItem)
{
    std::string     type;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item) {
        // Prepend the name of the runtime manager of this item to the type.
        type = Ufe::RunTimeMgr::instance().getName(item->runTimeId()) + item->nodeType();
    }
    return type;
}
#endif

PXR_NS::UsdStageWeakPtr _getStage(const std::string& ufePathString)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    return ufe::getStage(Ufe::PathString::path(ufePathString));
#else
    // This function works on a single-segment path, i.e. the Maya Dag path
    // segment to the proxy shape.  We know the Maya run-time ID is 1,
    // separator is '|'.
    // The helper function proxyShapeHandle() assumes Maya path starts
    // with "|world" and will pop it off. So make sure our string has it.
    // Note: std::string starts_with only added for C++20. So use diff trick.
    std::string proxyPath;
    if (ufePathString.rfind("|world", 0) == std::string::npos) {
        proxyPath = "|world" + ufePathString;
    } else {
        proxyPath = ufePathString;
    }
    return ufe::getStage(Ufe::Path(Ufe::PathSegment(proxyPath, 1, '|')));
#endif
}

std::vector<PXR_NS::UsdStageRefPtr> _getAllStages()
{
    auto                                allStages = ufe::getAllStages();
    std::vector<PXR_NS::UsdStageRefPtr> output;
    for (auto stage : allStages) {
        PXR_NS::UsdStageRefPtr stageRefPtr { stage };
        output.push_back(stageRefPtr);
    }
    return output;
}

std::string _stagePath(PXR_NS::UsdStageWeakPtr stage)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    // Even though the Proxy shape node's UFE path is a single segment, we always
    // need to return as a Ufe::PathString (to remove |world).
    return Ufe::PathString::string(ufe::stagePath(stage));
#else
    // Remove the leading '|world' component.
    return ufe::stagePath(stage).popHead().string();
#endif
}

#ifndef UFE_V2_FEATURES_AVAILABLE
// Helper function for UFE versions before version 2 for converting a path
// string to a UFE path.
static Ufe::Path _UfeV1StringToUsdPath(const std::string& ufePathString)
{
    Ufe::Path path;

    // The path string is a list of segment strings separated by ',' comma
    // separator.
    auto segmentStrings = PXR_NS::TfStringTokenize(ufePathString, ",");

    // If there are fewer than two segments, there cannot be a USD segment, so
    // return an invalid path.
    if (segmentStrings.size() < 2u) {
        return path;
    }

    // We have the path string split into segments. Build up the Ufe::Path one
    // segment at a time. The path segment separator is the first character
    // of each segment. We know that USD's separator is '/' and Maya's
    // separator is '|', so use a map to get the corresponding UFE run-time ID.
    static std::map<char, Ufe::Rtid> sepToRtid
        = { { '/', ufe::getUsdRunTimeId() }, { '|', ufe::getMayaRunTimeId() } };
    for (size_t i = 0u; i < segmentStrings.size(); ++i) {
        const auto& segmentString = segmentStrings[i];
        char        sep = segmentString[0u];
        path = path + Ufe::PathSegment(segmentString, sepToRtid.at(sep), sep);
    }

    return path;
}
#endif

PXR_NS::TfTokenVector _getProxyShapePurposes(const std::string& ufePathString)
{
    auto path =
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::PathString::path(
#else
        _UfeV1StringToUsdPath(
#endif
            ufePathString);
    return ufe::getProxyShapePurposes(path);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string createStageWithNewLayer(const std::string& parentPathString)
{
    auto parentPath = Ufe::PathString::path(parentPathString);
    auto parent = Ufe::Hierarchy::createItem(parentPath);
    auto command = MayaUsd::ufe::UsdUndoCreateStageWithNewLayerCommand::create(parent);
    if (!command) {
        return "";
    }

    Ufe::UndoableCommandMgr::instance().executeCmd(command);
    if (!command->sceneItem()) {
        return "";
    }

    return Ufe::PathString::string(command->sceneItem()->path());
}
#endif

void wrapUtils()
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    def("getPrimFromRawItem", getPrimFromRawItem);
    def("getNodeNameFromRawItem", getNodeNameFromRawItem);
    def("getNodeTypeFromRawItem", getNodeTypeFromRawItem);
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
    def("createStageWithNewLayer", createStageWithNewLayer);
#endif

    // Because mayaUsd and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("getStage", _getStage);
    def("getAllStages", _getAllStages, return_value_policy<PXR_NS::TfPySequenceToList>());
    def("stagePath", _stagePath);
    def("getProxyShapePurposes", _getProxyShapePurposes);
}
