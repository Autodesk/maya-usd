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
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

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

#include <boost/python.hpp>

#include <string>

using namespace MayaUsd;
using namespace boost::python;

#ifdef UFE_V2_FEATURES_AVAILABLE
PXR_NS::UsdPrim getPrimFromRawItem(uint64_t rawItem)
{
    Ufe::SceneItem*    item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    ufe::UsdSceneItem* usdItem = dynamic_cast<ufe::UsdSceneItem*>(item);
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

PXR_NS::UsdStageWeakPtr getStage(const std::string& ufePathString)
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

std::string stagePath(PXR_NS::UsdStageWeakPtr stage)
{
    // Proxy shape node's UFE path is a single segment, so no need to separate
    // segments with commas.
    return ufe::stagePath(stage).string();
}

std::string usdPathToUfePathSegment(
    const PXR_NS::SdfPath& usdPath,
    int                    instanceIndex = PXR_NS::UsdImagingDelegate::ALL_INSTANCES)
{
    return ufe::usdPathToUfePathSegment(usdPath, instanceIndex).string();
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

PXR_NS::UsdTimeCode getTime(const std::string& pathStr)
{
    const Ufe::Path path =
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::PathString::path(
#else
        _UfeV1StringToUsdPath(
#endif
            pathStr);
    return ufe::getTime(path);
}

std::string stripInstanceIndexFromUfePath(const std::string& ufePathString)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    const Ufe::Path path = Ufe::PathString::path(ufePathString);
    return Ufe::PathString::string(ufe::stripInstanceIndexFromUfePath(path));
#else
    const Ufe::Path path = _UfeV1StringToUsdPath(ufePathString);
    return ufe::stripInstanceIndexFromUfePath(path).string();
#endif
}

PXR_NS::UsdPrim ufePathToPrim(const std::string& ufePathString)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    return ufe::ufePathToPrim(Ufe::PathString::path(ufePathString));
#else
    Ufe::Path path = _UfeV1StringToUsdPath(ufePathString);

    // If there are fewer than two segments, there cannot be a USD segment, so
    // return an invalid UsdPrim.
    if (path.getSegments().size() < 2u) {
        return PXR_NS::UsdPrim();
    }

    return ufe::ufePathToPrim(path);
#endif
}

int ufePathToInstanceIndex(const std::string& ufePathString)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    return ufe::ufePathToInstanceIndex(Ufe::PathString::path(ufePathString));
#else
    Ufe::Path path = _UfeV1StringToUsdPath(ufePathString);

    // If there are fewer than two segments, there cannot be a USD segment, so
    // return ALL_INSTANCES.
    if (path.getSegments().size() < 2u) {
        return UsdImagingDelegate::ALL_INSTANCES;
    }

    return ufe::ufePathToInstanceIndex(path);
#endif
}

PXR_NS::TfTokenVector getProxyShapePurposes(const std::string& ufePathString)
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

void wrapUtils()
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    def("getPrimFromRawItem", getPrimFromRawItem);
    def("getNodeNameFromRawItem", getNodeNameFromRawItem);
    def("getNodeTypeFromRawItem", getNodeTypeFromRawItem);
#endif

    // Because mayaUsd and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("getStage", getStage);
    def("stagePath", stagePath);
    def("usdPathToUfePathSegment",
        usdPathToUfePathSegment,
        (arg("usdPath"), arg("instanceIndex") = PXR_NS::UsdImagingDelegate::ALL_INSTANCES));
    def("getTime", getTime);
    def("stripInstanceIndexFromUfePath", stripInstanceIndexFromUfePath, (arg("ufePathString")));
    def("ufePathToPrim", ufePathToPrim);
    def("ufePathToInstanceIndex", ufePathToInstanceIndex);
    def("getProxyShapePurposes", getProxyShapePurposes);
}
