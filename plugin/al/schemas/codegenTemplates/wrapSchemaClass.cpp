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
#include "{{ libraryPath }}/{{ cls.GetHeaderFile() }}"

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/pyUtils.h>
#include <pxr/base/tf/wrapTypeHelpers.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/pyConversions.h>
#include <pxr/usd/usd/schemaBase.h>

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

{
    % if useExportAPI %
}
{
    {
        namespaceUsing
    }
}

namespace {

{
    % endif %
}
#define WRAP_CUSTOM template <class Cls> static void _CustomWrapCode(Cls& _class)

// fwd decl.
WRAP_CUSTOM;

{% for attrName in cls.attrOrder -%
}
{
    % set attr = cls.attrs[attrName] %
}
static UsdAttribute _Create
{
    {
        Proper(attr.apiName)
    }
}
Attr({ { cls.cppClassName } } & self, object defaultVal, bool writeSparsely)
{
    return self.Create
    {
        {
            Proper(attr.apiName)
        }
    }
    Attr(UsdPythonToSdfType(defaultVal, { { attr.usdType } }), writeSparsely);
}
{ % endfor % } {
    % if useExportAPI %
}

} // anonymous namespace
{
    % endif %
}

void wrap { { cls.cppClassName } }()
{
    typedef
    {
        {
            cls.cppClassName
        }
    }
    This;

    class_<This, bases<{ { cls.parentCppClassName } }>> cls("{{ cls.className }}");

    cls.def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

    {
        % if cls.isConcrete == "true" %
    }
    .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define") { % endif % }

        .def(
            "GetSchemaAttributeNames",
            &This::GetSchemaAttributeNames,
            arg("includeInherited") = true,
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def(
            "_GetStaticTfType",
            (TfType const& (*)())TfType::Find<This>,
            return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

    {% for attrName in cls.attrOrder -%
    }
    { % set attr = cls.attrs[attrName] % }
        .def(
            "Get{{ Proper(attr.apiName) }}Attr",
            &This::Get {
                {
                    Proper(attr.apiName)
                }
            } Attr)
        .def(
            "Create{{ Proper(attr.apiName) }}Attr",
            &_Create {
                {
                    Proper(attr.apiName)
                }
            } Attr,
            (arg("defaultValue") = object(), arg("writeSparsely") = false)) { % endfor % }

    {% for relName in cls.relOrder -%
    }
    { % set rel = cls.rels[relName] % }
        .def(
            "Get{{ Proper(rel.apiName) }}Rel",
            &This::Get {
                {
                    Proper(rel.apiName)
                }
            } Rel)
        .def(
            "Create{{ Proper(rel.apiName) }}Rel", &This::Create {
                {
                    Proper(rel.apiName)
                }
            } Rel) { % endfor % };

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
{
    % if useExportAPI %
}
//
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
{
    % endif %
}
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
