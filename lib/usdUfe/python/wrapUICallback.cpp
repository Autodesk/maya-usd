//
// Copyright 2024 Autodesk
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
#include <usdUfe/utils/uiCallback.h>

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

class PyUICallback : public UsdUfe::UICallback
{
public:
    PyUICallback(PyObject* pyCallable)
        : _pyCb(pyCallable)
    {
    }

    ~PyUICallback() override { }

    void
    operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& callbackData) override
    {
        // Note: necessary to compile the TF_WARN macro as it refers to USD types without using
        //       the namespace prefix.
        PXR_NAMESPACE_USING_DIRECTIVE;

        PXR_NS::TfPyLock pyLock;
        if (!PyCallable_Check(_pyCb)) {
            return;
        }
        boost::python::object dictObject(callbackData);
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
            callbackData = extractedDict;
        }
    }

private:
    PyObject* _pyCb;
};

} // namespace

void wrapUICallback()
{
    // Making the callbacks accessible from Python
    def(
        "registerUICallback", +[](const PXR_NS::TfToken& operation, PyObject* uiCallback) {
            return UsdUfe::registerUICallback(
                operation, std::make_shared<PyUICallback>(uiCallback));
        });
}
