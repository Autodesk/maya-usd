//
// Copyright 2017 Animal Logic
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
#include "AL/usd/utils/MayaTransformAPI.h"

#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapMayaTransformAPI()
{
  {
    typedef AL::usd::utils::RotationOrder This;
    enum_<This>("RotationOrder")
        .value("kXYZ", This::kXYZ)
        .value("kYZX", This::kYZX)
        .value("kZXY", This::kZXY)
        .value("kXZY", This::kXZY)
        .value("kYXZ", This::kYXZ)
        .value("kZYX", This::kZYX);
  }

  {
    typedef AL::usd::utils::MayaTransformAPI This;
    class_<This>("MayaTransformAPI", no_init)
      .def(init<const UsdPrim&, bool>((arg("prim"), arg("convertMatrixToComponents"))))
      .def("scale", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::scale)
      .def("scale", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::scale)
      .def("translate", (void (This::*)(const GfVec3d&, const UsdTimeCode&)) &This::translate)
      .def("translate", (GfVec3d (This::*)(const UsdTimeCode&) const) &This::translate)
      .def("rotate", (void (This::*)(const GfVec3f&, AL::usd::utils::RotationOrder, const UsdTimeCode&)) &This::rotate)
      .def("rotate", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::rotate)
      .def("rotateAxis", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::rotateAxis)
      .def("rotateAxis", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::rotateAxis)
      .def("scalePivot", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::scalePivot)
      .def("scalePivot", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::scalePivot)
      .def("rotatePivot", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::rotatePivot)
      .def("rotatePivot", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::rotatePivot)
      .def("scalePivotTranslate", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::scalePivotTranslate)
      .def("scalePivotTranslate", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::scalePivotTranslate)
      .def("rotatePivotTranslate", (void (This::*)(const GfVec3f&, const UsdTimeCode&)) &This::rotatePivotTranslate)
      .def("rotatePivotTranslate", (GfVec3f (This::*)(const UsdTimeCode&) const) &This::rotatePivotTranslate)
      .def("inheritsTransform", (void (This::*)(const bool)) &This::inheritsTransform)
      .def("inheritsTransform", (bool (This::*)() const) &This::inheritsTransform)
      .def("asMatrix", &This::asMatrix)
      .def("setFromMatrix", &This::setFromMatrix)
      .def("isValid", &This::isValid)
      .def("rotateOrder", &This::rotateOrder)
    ;
  }
}
