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
#include <pxr_python.h>

#include <iostream>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

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
        if (_pyCb) {
            // As long as we may use this object, we need to keep a reference to
            // it to keep the garbage collector away.
            Py_IncRef(_pyCb);
        }
    }

    ~PyUICallback() override
    {
        if (_pyCb) {
            PXR_NS::TfPyLock pyLock;
            // Release the reference to the python object.
            Py_DecRef(_pyCb);
        }
    }

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
        PXR_BOOST_PYTHON_NAMESPACE::object dictObject(callbackData);
        try {
            call<void>(_pyCb, context, dictObject);
        } catch (const PXR_BOOST_PYTHON_NAMESPACE::error_already_set&) {
            const std::string errorMessage = handlePythonException();
            PXR_BOOST_PYTHON_NAMESPACE::handle_exception();
            PyErr_Clear();
            TF_WARN("%s", errorMessage.c_str());
            throw std::runtime_error(errorMessage);
        } catch (const std::exception& ex) {
            TF_WARN("%s", ex.what());
            throw;
        }
        PXR_BOOST_PYTHON_NAMESPACE::extract<PXR_NS::VtDictionary> extractedDict(dictObject);
        if (extractedDict.check()) {
            callbackData = extractedDict;
        }
    }

private:
    PyObject* _pyCb;
};

// Since the Python wrapping accepts Python objects but the UFE C++ API requires
// a specific callback class instance, keep a mapping of Python to C++ so we can
// remove the callback later with the same C++ object.
std::map<std::pair<PXR_NS::TfToken, PyObject*>, UsdUfe::UICallback::Ptr>&
getRegisteredPythonCallbacks()
{
    static std::map<std::pair<PXR_NS::TfToken, PyObject*>, UsdUfe::UICallback::Ptr> callbacks;
    return callbacks;
}

PXR_NS::VtDictionary _triggerUICallback(
    const PXR_NS::TfToken&      operation,
    const PXR_NS::VtDictionary& cbContext,
    const PXR_NS::VtDictionary& cbData)
{
    if (!UsdUfe::isUICallbackRegistered(operation))
        return cbData;

    //
    // Need to copy the input data into non-const reference to call C++ version.
    // This allows the python callback function to modify the data which we then
    // return to the triggering function.
    //
    // Trying to use "PXR_NS::VtDictionary& cbData" results in python error:
    //
    // Python argument types in usdUfe._usdUfe.triggerUICallback(str, dict, dict)
    // did not match C++ signature:
    //     triggerUICallback(
    //       class pxrInternal_v0_24__pxrReserved__::TfToken,
    //       class pxrInternal_v0_24__pxrReserved__::VtDictionary,
    //       class pxrInternal_v0_24__pxrReserved__::VtDictionary{lvalue})
    //
    PXR_NS::VtDictionary cbDataReturned(cbData);
    UsdUfe::triggerUICallback(operation, cbContext, cbDataReturned);
    return cbDataReturned;
}

void _addPythonCallback(const PXR_NS::TfToken& operation, PyObject* uiCallback)
{
    if (!uiCallback)
        return;

    auto key = std::make_pair(operation, uiCallback);

    // We don't allow registering the same callback for the same operation twice.
    auto& callbacks = getRegisteredPythonCallbacks();
    if (callbacks.find(key) != callbacks.end())
        return;

    // Remember the Python-to-C++ mapping.
    auto cb = std::make_shared<PyUICallback>(uiCallback);
    callbacks[key] = cb;

    // Register the callback with UFE.
    UsdUfe::registerUICallback(operation, cb);
}

void _removePythonCallback(const PXR_NS::TfToken& operation, PyObject* uiCallback)
{
    if (!uiCallback)
        return;

    auto key = std::make_pair(operation, uiCallback);

    // Make sure the callback really was registered.
    auto& callbacks = getRegisteredPythonCallbacks();
    if (callbacks.find(key) == callbacks.end())
        return;

    // Erase the Python-to-C++ mapping.
    auto cb = callbacks[key];
    callbacks.erase(key);

    // Unregister the callback from UFE.
    UsdUfe::unregisterUICallback(operation, cb);
}

} // namespace

void wrapUICallback()
{
    // Making the callbacks accessible from Python
    def("registerUICallback", _addPythonCallback);
    def("unregisterUICallback", _removePythonCallback);

    // Helper function to trigger a callback for a given operation.
    // Caller should supply the callback context and data.
    def("triggerUICallback", _triggerUICallback);
}
