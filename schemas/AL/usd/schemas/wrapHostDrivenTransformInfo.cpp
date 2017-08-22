//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "./HostDrivenTransformInfo.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

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
_CreateTransformSourceNodesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTransformSourceNodesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateScaleAttributesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleAttributesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateScaleAttributeIndicesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleAttributeIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateRotateAttributesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateAttributesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateRotateAttributeIndicesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateAttributeIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTranslateAttributesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTranslateAttributesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateTranslateAttributeIndicesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTranslateAttributeIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateRotateOrderAttributesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateOrderAttributesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateRotateOrderAttributeIndicesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateOrderAttributeIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateVisibilitySourceNodesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVisibilitySourceNodesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateVisibilityAttributesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVisibilityAttributesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateVisibilityAttributeIndicesAttr(AL_usd_HostDrivenTransformInfo &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVisibilityAttributeIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}

} // anonymous namespace

void wrapAL_usd_HostDrivenTransformInfo()
{
    typedef AL_usd_HostDrivenTransformInfo This;

    class_<This, bases<UsdTyped> >
        cls("HostDrivenTransformInfo");

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

        
        .def("GetTransformSourceNodesAttr",
             &This::GetTransformSourceNodesAttr)
        .def("CreateTransformSourceNodesAttr",
             &_CreateTransformSourceNodesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleAttributesAttr",
             &This::GetScaleAttributesAttr)
        .def("CreateScaleAttributesAttr",
             &_CreateScaleAttributesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleAttributeIndicesAttr",
             &This::GetScaleAttributeIndicesAttr)
        .def("CreateScaleAttributeIndicesAttr",
             &_CreateScaleAttributeIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateAttributesAttr",
             &This::GetRotateAttributesAttr)
        .def("CreateRotateAttributesAttr",
             &_CreateRotateAttributesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateAttributeIndicesAttr",
             &This::GetRotateAttributeIndicesAttr)
        .def("CreateRotateAttributeIndicesAttr",
             &_CreateRotateAttributeIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTranslateAttributesAttr",
             &This::GetTranslateAttributesAttr)
        .def("CreateTranslateAttributesAttr",
             &_CreateTranslateAttributesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTranslateAttributeIndicesAttr",
             &This::GetTranslateAttributeIndicesAttr)
        .def("CreateTranslateAttributeIndicesAttr",
             &_CreateTranslateAttributeIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateOrderAttributesAttr",
             &This::GetRotateOrderAttributesAttr)
        .def("CreateRotateOrderAttributesAttr",
             &_CreateRotateOrderAttributesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateOrderAttributeIndicesAttr",
             &This::GetRotateOrderAttributeIndicesAttr)
        .def("CreateRotateOrderAttributeIndicesAttr",
             &_CreateRotateOrderAttributeIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVisibilitySourceNodesAttr",
             &This::GetVisibilitySourceNodesAttr)
        .def("CreateVisibilitySourceNodesAttr",
             &_CreateVisibilitySourceNodesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVisibilityAttributesAttr",
             &This::GetVisibilityAttributesAttr)
        .def("CreateVisibilityAttributesAttr",
             &_CreateVisibilityAttributesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVisibilityAttributeIndicesAttr",
             &This::GetVisibilityAttributeIndicesAttr)
        .def("CreateVisibilityAttributeIndicesAttr",
             &_CreateVisibilityAttributeIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetTargetTransformsRel",
             &This::GetTargetTransformsRel)
        .def("CreateTargetTransformsRel",
             &This::CreateTargetTransformsRel)
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
