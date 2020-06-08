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
#include "./FrameRange.h"
#include <pxr/usd/usd/schemaBase.h>

#include <pxr/usd/sdf/primSpec.h>

#include <pxr/usd/usd/pyConversions.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/pyUtils.h>
#include <pxr/base/tf/wrapTypeHelpers.h>

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateAnimationStartFrameAttr(AL_usd_FrameRange &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnimationStartFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateStartFrameAttr(AL_usd_FrameRange &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStartFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateEndFrameAttr(AL_usd_FrameRange &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEndFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAnimationEndFrameAttr(AL_usd_FrameRange &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnimationEndFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateCurrentFrameAttr(AL_usd_FrameRange &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCurrentFrameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}

} // anonymous namespace

void wrapAL_usd_FrameRange()
{
    typedef AL_usd_FrameRange This;

    class_<This, bases<UsdTyped> >
        cls("FrameRange");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetAnimationStartFrameAttr",
             &This::GetAnimationStartFrameAttr)
        .def("CreateAnimationStartFrameAttr",
             &_CreateAnimationStartFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStartFrameAttr",
             &This::GetStartFrameAttr)
        .def("CreateStartFrameAttr",
             &_CreateStartFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEndFrameAttr",
             &This::GetEndFrameAttr)
        .def("CreateEndFrameAttr",
             &_CreateEndFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnimationEndFrameAttr",
             &This::GetAnimationEndFrameAttr)
        .def("CreateAnimationEndFrameAttr",
             &_CreateAnimationEndFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCurrentFrameAttr",
             &This::GetCurrentFrameAttr)
        .def("CreateCurrentFrameAttr",
             &_CreateCurrentFrameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

WRAP_CUSTOM {
}

}
