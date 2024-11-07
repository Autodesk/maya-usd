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
#include <pxr_python.h>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

// This exposes a DiagnosticBatchContext as a Python "context manager"
// object that can be used with the "with"-statement to flush diagostic
// messages
class _PyDiagnosticBatchContext
{
public:
    _PyDiagnosticBatchContext() { }
    _PyDiagnosticBatchContext(int c)
        : _count(c)
    {
    }
    void __enter__() { UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(_count); }
    void __exit__(object, object, object)
    {
        UsdMayaDiagnosticDelegate::Flush();
        UsdMayaDiagnosticDelegate::SetMaximumUnbatchedDiagnostics(_previousCount);
    }

private:
    int _count = 0;
    int _previousCount = UsdMayaDiagnosticDelegate::GetMaximumUnbatchedDiagnostics();
};

} // anonymous namespace

void wrapDiagnosticDelegate()
{
    typedef UsdMayaDiagnosticDelegate This;
    class_<This, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>("DiagnosticDelegate", no_init)
        .def("Flush", &This::Flush)
        .staticmethod("Flush")
        .def("SetMaximumUnbatchedDiagnostics", &This::SetMaximumUnbatchedDiagnostics)
        .staticmethod("SetMaximumUnbatchedDiagnostics")
        .def("GetMaximumUnbatchedDiagnostics", &This::GetMaximumUnbatchedDiagnostics)
        .staticmethod("GetMaximumUnbatchedDiagnostics");

    typedef _PyDiagnosticBatchContext Context;
    class_<Context, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>("DiagnosticBatchContext")
        .def(init<int>())
        .def("__enter__", &Context::__enter__, return_self<>())
        .def("__exit__", &Context::__exit__);
}
