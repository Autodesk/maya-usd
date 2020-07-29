//
// Copyright 2016 Pixar
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
#include <mayaUsd/fileio/utils/roundTripUtil.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/pyConversions.h>
#include <pxr/usd/usdGeom/primvar.h>

#include <boost/python.hpp>

using namespace std;
using namespace boost::python;
using namespace boost;

PXR_NAMESPACE_USING_DIRECTIVE;

void wrapRoundTripUtil()
{
    typedef UsdMayaRoundTripUtil This;
    scope                        RoundTripUtil
        = class_<This>("RoundTripUtil", no_init)
              .def("IsAttributeUserAuthored", This::IsAttributeUserAuthored)
              .staticmethod("IsAttributeUserAuthored")

              .def("IsAttributeMayaGenerated", This::IsAttributeMayaGenerated)
              .staticmethod("IsAttributeMayaGenerated")
              .def("MarkAttributeAsMayaGenerated", This::MarkAttributeAsMayaGenerated)
              .staticmethod("MarkAttributeAsMayaGenerated")

              .def("IsPrimvarClamped", This::IsPrimvarClamped)
              .staticmethod("IsPrimvarClamped")

              .def("MarkPrimvarAsClamped", This::MarkPrimvarAsClamped)
              .staticmethod("MarkPrimvarAsClamped");
}
