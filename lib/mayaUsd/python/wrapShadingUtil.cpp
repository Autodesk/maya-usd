//
// Copyright 2021 Autodesk
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

#include <mayaUsd/fileio/utils/shadingUtil.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

class ShadingUtil
{
};

//----------------------------------------------------------------------------------------------------------------------
void wrapShadingUtil()
{
    boost::python::class_<ShadingUtil, boost::noncopyable>("ShadingUtil", boost::python::no_init)
        .def("GetStandardAttrName", &UsdMayaShadingUtil::GetStandardAttrName)
        .staticmethod("GetStandardAttrName");
}
