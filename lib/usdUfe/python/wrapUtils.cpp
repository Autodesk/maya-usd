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
//#include <mayaUsd/ufe/Global.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

#include <string>

using namespace boost::python;

std::string _uniqueChildName(const PXR_NS::UsdPrim& parent, const std::string& name)
{
    return UsdUfe::uniqueChildName(parent, name);
}

std::string _stripInstanceIndexFromUfePath(const std::string& ufePathString)
{
    const Ufe::Path path = Ufe::PathString::path(ufePathString);
    return Ufe::PathString::string(UsdUfe::stripInstanceIndexFromUfePath(path));
}

PXR_NS::UsdPrim _ufePathToPrim(const std::string& ufePathString)
{
    return UsdUfe::ufePathToPrim(Ufe::PathString::path(ufePathString));
}

int _ufePathToInstanceIndex(const std::string& ufePathString)
{
    return UsdUfe::ufePathToInstanceIndex(Ufe::PathString::path(ufePathString));
}


bool _isEditTargetLayerModifiable(const PXR_NS::UsdStageWeakPtr stage)
{
    return UsdUfe::isEditTargetLayerModifiable(stage);
}

void wrapUtils()
{
    // Because mayaUsd and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("uniqueChildName", _uniqueChildName);
    def("stripInstanceIndexFromUfePath", _stripInstanceIndexFromUfePath, (arg("ufePathString")));
    def("ufePathToPrim", _ufePathToPrim);
    def("ufePathToInstanceIndex", _ufePathToInstanceIndex);
    def("isEditTargetLayerModifiable", _isEditTargetLayerModifiable);
}
