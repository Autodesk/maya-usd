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
#include <mayaUsd/utils/editRouter.h>

#include <pxr/base/tf/pyError.h>
#include <pxr/base/tf/pyLock.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

#include <iostream>

using namespace boost::python;

namespace {

std::string handlePythonException()
{
    PyObject* exc = nullptr;
    PyObject* val = nullptr;
    PyObject* tb = nullptr;
    PyErr_Fetch(&exc, &val, &tb);
    handle<> hexc(exc);
    handle<> hval(allow_null(val));
    handle<> htb(allow_null(tb));
    object   traceback(import("traceback"));
    object   format_exception_only(traceback.attr("format_exception_only"));
    object   formatted_list = format_exception_only(hexc, hval);
    object   formatted = str("\n").join(formatted_list);
    return extract<std::string>(formatted);
}

class PyEditRouter : public MayaUsd::EditRouter
{
public:
    PyEditRouter(PyObject* pyCallable)
        : _pyCb(pyCallable)
    {
    }

    ~PyEditRouter() override { }

    void operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData) override
    {
        // Note: necessary to compile the TF_WARN macro as it refers to USD types without using
        //       the namespace prefix.
        PXR_NAMESPACE_USING_DIRECTIVE;

        PXR_NS::TfPyLock pyLock;
        if (!PyCallable_Check(_pyCb)) {
            return;
        }
        boost::python::object dictObject(routingData);
        try {
            call<void>(_pyCb, context, dictObject);
        } catch (const boost::python::error_already_set&) {
            const std::string errorMessage = handlePythonException();
            boost::python::handle_exception();
            PyErr_Clear();
            TF_WARN("%s", errorMessage.c_str());
            throw std::runtime_error(errorMessage);
        } catch (const std::exception& ex) {
            TF_WARN("%s", ex.what());
            throw;
        }
        boost::python::extract<PXR_NS::VtDictionary> extractedDict(dictObject);
        if (extractedDict.check()) {
            routingData = extractedDict;
        }
    }

private:
    PyObject* _pyCb;
};

} // namespace

void wrapEditRouter()
{
    // As per
    // https://stackoverflow.com/questions/18889028/a-positive-lambda-what-sorcery-is-this
    // the plus (+) sign before the lambda triggers a conversion to a plain old
    // function pointer for the lambda, which is required for successful Boost
    // Python compilation of the lambda and its argument.

    def(
        "registerEditRouter", +[](const PXR_NS::TfToken& operation, PyObject* editRouter) {
            return MayaUsd::registerEditRouter(
                operation, std::make_shared<PyEditRouter>(editRouter));
        });

    def("restoreDefaultEditRouter", &MayaUsd::restoreDefaultEditRouter);
}
