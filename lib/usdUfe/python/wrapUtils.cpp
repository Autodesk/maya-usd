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

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/Utils.h>
#include <usdUfe/utils/schemas.h>

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
#include <vector>

using namespace boost::python;

PXR_NS::UsdStageWeakPtr _getStage(const std::string& ufePathString)
{
    return UsdUfe::getStage(Ufe::PathString::path(ufePathString));
}

std::string _stagePath(PXR_NS::UsdStageWeakPtr stage)
{
    // Even though the Proxy shape node's UFE path is a single segment, we always
    // need to return as a Ufe::PathString (to remove |world).
    return Ufe::PathString::string(UsdUfe::stagePath(stage));
}

std::string _usdPathToUfePathSegment(
    const PXR_NS::SdfPath& usdPath,
    int                    instanceIndex = PXR_NS::UsdImagingDelegate::ALL_INSTANCES)
{
    return UsdUfe::usdPathToUfePathSegment(usdPath, instanceIndex).string();
}

std::string _uniqueName(const std::vector<std::string>& existingNames, std::string srcName)
{
    PXR_NS::TfToken::HashSet existingNamesSet;
    for (const auto& n : existingNames) {
        existingNamesSet.insert(PXR_NS::TfToken(n));
    }
    return UsdUfe::uniqueName(existingNamesSet, srcName);
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

bool _isAnyLayerModifiable(const PXR_NS::UsdStageWeakPtr stage)
{
    return UsdUfe::isAnyLayerModifiable(stage);
}

bool _isEditTargetLayerModifiable(const PXR_NS::UsdStageWeakPtr stage)
{
    return UsdUfe::isEditTargetLayerModifiable(stage);
}

PXR_NS::UsdTimeCode _getTime(const std::string& pathStr)
{
    const Ufe::Path path = Ufe::PathString::path(pathStr);
    return UsdUfe::getTime(path);
}

bool _isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr)
{
    return UsdUfe::isAttributeEditAllowed(attr);
}

static boost::python::dict _convertSchemaInfo(const UsdUfe::SchemaInfo& info)
{
    boost::python::dict infoDict;

    infoDict["pluginName"] = info.pluginName;
    infoDict["schemaType"] = info.schemaType;
    infoDict["schemaTypeName"] = info.schemaTypeName;
    infoDict["isMultiApply"] = info.isMultiApply;

    return infoDict;
}

static boost::python::list _getKnownApplicableSchemas()
{
    boost::python::list schemasList;

    UsdUfe::KnownSchemas knownSchemas = UsdUfe::getKnownApplicableSchemas();

    for (const auto& info : knownSchemas)
        schemasList.append(_convertSchemaInfo(info.second));

    return schemasList;
}

static boost::python::dict _findSchemasByTypeName(const PXR_NS::TfToken& schemaTypeName)
{
    auto maybeInfo = UsdUfe::findSchemasByTypeName(schemaTypeName);
    if (!maybeInfo)
        return {};

    return _convertSchemaInfo(*maybeInfo);
}

bool _applySchemaToPrim(PXR_NS::UsdPrim& prim, const PXR_NS::TfType& schemaType)
{
    return UsdUfe::applySchemaToPrim(prim, schemaType);
}

bool _applyMultiSchemaToPrim(
    PXR_NS::UsdPrim&       prim,
    const PXR_NS::TfType&  schemaType,
    const PXR_NS::TfToken& instanceName)
{
    return UsdUfe::applyMultiSchemaToPrim(prim, schemaType, instanceName);
}

void wrapUtils()
{
    // Because UsdUfe and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("getStage", _getStage);
    def("stagePath", _stagePath);
    def("usdPathToUfePathSegment",
        _usdPathToUfePathSegment,
        (arg("usdPath"), arg("instanceIndex") = PXR_NS::UsdImagingDelegate::ALL_INSTANCES));
    def("uniqueName", _uniqueName);
    def("uniqueChildName", UsdUfe::uniqueChildName);
    def("stripInstanceIndexFromUfePath", _stripInstanceIndexFromUfePath, (arg("ufePathString")));
    def("ufePathToPrim", _ufePathToPrim);
    def("ufePathToInstanceIndex", _ufePathToInstanceIndex);
    def("isAnyLayerModifiable", _isAnyLayerModifiable);
    def("isEditTargetLayerModifiable", _isEditTargetLayerModifiable);
    def("getTime", _getTime);
    def("prettifyName", &UsdUfe::prettifyName);
    def("isAttributeEditAllowed", _isAttributeEditAllowed);
    def("getKnownApplicableSchemas", _getKnownApplicableSchemas);
    def("applySchemaToPrim", _applySchemaToPrim);
    def("applyMultiSchemaToPrim", _applyMultiSchemaToPrim);
    def("findSchemasByTypeName", _findSchemasByTypeName);
}
