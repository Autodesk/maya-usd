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
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/pyConversions.h>

#include <maya/MObject.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

static VtValue _GetVtValue(const std::string& attrPath, const SdfValueTypeName& typeName)
{
    VtValue val;

    MPlug   plug;
    MStatus status = UsdMayaUtil::GetPlugByName(attrPath, plug);
    CHECK_MSTATUS_AND_RETURN(status, val);

    val = UsdMayaWriteUtil::GetVtValue(plug, typeName);
    return val;
}

void wrapWriteUtil()
{
    typedef UsdMayaWriteUtil This;
    class_<This>("WriteUtil", no_init)
        .def("WriteUVAsFloat2", This::WriteUVAsFloat2)
        .staticmethod("WriteUVAsFloat2")
        .def("WriteMap1AsST", This::WriteMap1AsST)
        .staticmethod("WriteMap1AsST")
        .def("GetVtValue", _GetVtValue)
        .staticmethod("GetVtValue");
}
