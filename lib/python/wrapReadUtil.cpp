//
// Copyright 2018 Pixar
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

#include "pxr/pxr.h"
#include <mayaUsd/utils/util.h>
#include <mayaUsd/fileio/utils/readUtil.h>

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/pyConversions.h"

#include <maya/MObject.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

static
std::string _FindOrCreateMayaAttr(
    const SdfValueTypeName& typeName,
    const SdfVariability variability,
    const std::string& nodeName,
    const std::string& attrName,
    const std::string& attrNiceName = std::string())
{
    std::string attrPath;

    MObject obj;
    MStatus status = UsdMayaUtil::GetMObjectByName(nodeName, obj);
    CHECK_MSTATUS_AND_RETURN(status, attrPath);

    MFnDependencyNode depNode(obj, &status);
    CHECK_MSTATUS_AND_RETURN(status, attrPath);

    MObject attrObj = UsdMayaReadUtil::FindOrCreateMayaAttr(
            typeName, variability, depNode, attrName, attrNiceName);
    if (attrObj.isNull()) {
        return attrPath;
    }

    attrPath = depNode.findPlug(attrObj, true).name().asChar();
    return attrPath;
}

static
bool _SetMayaAttr(
    const std::string& attrPath,
    const VtValue& newValue)
{
    MPlug plug;
    MStatus status = UsdMayaUtil::GetPlugByName(attrPath, plug);
    if (!status) {
        TF_RUNTIME_ERROR("Couldn't find plug '%s'", attrPath.c_str());
        return false;
    }

    return UsdMayaReadUtil::SetMayaAttr(plug, newValue);
}

static
void _SetMayaAttrKeyableState(
    const std::string& attrPath,
    const SdfVariability variability)
{
    MPlug plug;
    MStatus status = UsdMayaUtil::GetPlugByName(attrPath, plug);
    if (!status) {
        TF_RUNTIME_ERROR("Couldn't find plug '%s'", attrPath.c_str());
        return;
    }

    UsdMayaReadUtil::SetMayaAttrKeyableState(plug, variability);
}

void wrapReadUtil()
{
    typedef UsdMayaReadUtil This;
    class_<This>("ReadUtil", no_init)
        .def("ReadFloat2AsUV", This::ReadFloat2AsUV)
        .staticmethod("ReadFloat2AsUV")
        .def("FindOrCreateMayaAttr", _FindOrCreateMayaAttr,
            (arg("typeName"), arg("variability"), arg("nodeName"),
             arg("attrName"), arg("attrNiceName")=std::string()))
        .staticmethod("FindOrCreateMayaAttr")
        .def("SetMayaAttr", _SetMayaAttr)
        .staticmethod("SetMayaAttr")
        .def("SetMayaAttrKeyableState", _SetMayaAttrKeyableState)
        .staticmethod("SetMayaAttrKeyableState")
    ;
}
