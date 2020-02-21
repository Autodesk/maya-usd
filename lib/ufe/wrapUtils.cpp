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

#include <boost/python.hpp>

#include "UsdSceneItem.h"
#include "Utils.h"
#include "Global.h"

#include <ufe/runTimeMgr.h>
#include <ufe/rtid.h>

#include <pxr/base/tf/stringUtils.h>

using namespace MayaUsd;
using namespace boost::python;

UsdPrim getPrimFromRawItem(uint64_t rawItem)
{
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    ufe::UsdSceneItem* usdItem = dynamic_cast<ufe::UsdSceneItem*>(item);
    if (nullptr != usdItem)
    {
        return usdItem->prim();
    }
    return UsdPrim();
}

#ifdef UFE_V2_FEATURES_AVAILABLE
std::string getNodeNameFromRawItem(uint64_t rawItem)
{
    std::string name;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
        name = item->nodeName();
    return name;
}
#endif

std::string getNodeTypeFromRawItem(uint64_t rawItem)
{
    std::string type;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
    {
        // Prepend the name of the runtime manager of this item to the type.
        type = Ufe::RunTimeMgr::instance().getName(item->runTimeId()) + item->nodeType();
    }
    return type;
}

UsdStageWeakPtr getStage(const std::string& ufePathString)
{
    // This function works on a single-segment path, i.e. the Maya Dag path
    // segment to the proxy shape.  We know the Maya run-time ID is 1,
    // separator is '|'.
    return ufe::getStage(Ufe::Path(Ufe::PathSegment(ufePathString, 1, '|')));
}

std::string stagePath(UsdStageWeakPtr stage)
{
    // Proxy shape node's UFE path is a single segment, so no need to separate
    // segments with commas.
    return ufe::stagePath(stage).string();
}

UsdPrim ufePathToPrim(const std::string& ufePathString)
{
    // The path string is a list of segment strings separated by ',' comma
    // separator.
    auto segmentStrings = TfStringTokenize(ufePathString, ",");

    // If there's just one segment, it's the Maya Dag path segment, so it can't
    // have a prim.
    if (segmentStrings.size() == 1) {
        return UsdPrim();
    }

    // We have the path string split into segments.  Build up the Ufe::Path one
    // segment at a time.  The path segment separator is the first character
    // of each segment.  We know that USD's separator is '/' and Maya's
    // separator is '|', so use a map to get the corresponding UFE run-time ID.
    Ufe::Path path;
    static std::map<char, Ufe::Rtid> sepToRtid = {
        {'/', ufe::getUsdRunTimeId()}, {'|', 1}};
    for (std::size_t i = 0; i < segmentStrings.size(); ++i) {
        const auto& segmentString = segmentStrings[i];
        char sep = segmentString[0];
        path = path + Ufe::PathSegment(segmentString, sepToRtid.at(sep), sep);
    }
    return ufe::ufePathToPrim(path);
}

BOOST_PYTHON_MODULE(ufe)
{
    def("getPrimFromRawItem", getPrimFromRawItem);
    
    #ifdef UFE_V2_FEATURES_AVAILABLE
        def("getNodeNameFromRawItem", getNodeNameFromRawItem);
    #endif

    def("getNodeTypeFromRawItem", getNodeTypeFromRawItem);

    // Because mayaUsd and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("getStage", getStage);
    def("stagePath", stagePath);
    def("ufePathToPrim", ufePathToPrim);
}
