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
#include <mayaUsd/utils/diagnosticDelegate.h>

#include <pxr/pxr.h>

#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_arg.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

// This exposes a DiagnosticBatchContext as a Python "context manager"
// object that can be used with the "with"-statement to flush diagostic
// messages
class _PyDiagnosticBatchContext
{
public:
    void __enter__() { UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(0); }
    void __exit__(object, object, object)
    {
        UsdMayaDiagnosticDelegate::Flush();
        UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(100);
    }
};

} // anonymous namespace

void wrapDiagnosticDelegate()
{
    typedef UsdMayaDiagnosticDelegate This;
    class_<This, boost::noncopyable>("DiagnosticDelegate", no_init)
        .def("Flush", &This::Flush)
        .staticmethod("Flush")
        .def("SetMaximumUnbatchedDiagnostics", &This::SetMaximumUnbatchedDiagnostics)
        .staticmethod("SetMaximumUnbatchedDiagnostics");

    typedef _PyDiagnosticBatchContext Context;
    class_<Context, boost::noncopyable>("DiagnosticBatchContext")
        .def("__enter__", &Context::__enter__, return_self<>())
        .def("__exit__", &Context::__exit__);
}
