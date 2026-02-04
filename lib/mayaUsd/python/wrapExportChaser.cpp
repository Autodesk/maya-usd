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

#include "pythonObjectRegistry.h"

#include <mayaUsd/fileio/chaser/exportChaser.h>
#include <mayaUsd/fileio/chaser/exportChaserRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr_python.h>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief Python binding for the UsdMayaExportChaser
//----------------------------------------------------------------------------------------------------------------------
class ExportChaserWrapper
    : public UsdMayaExportChaser
    , public TfPyPolymorphic<UsdMayaExportChaser>
{
public:
    typedef ExportChaserWrapper This;
    typedef UsdMayaExportChaser base_t;

    static ExportChaserWrapper*
    New(const UsdMayaExportChaserRegistry::FactoryContext& factoryContext, uintptr_t createdWrapper)
    {
        return (ExportChaserWrapper*)createdWrapper;
    }

    virtual ~ExportChaserWrapper() = default;

    bool default_ExportDefault() { return base_t::ExportDefault(); }
    bool ExportDefault() override
    {
        return this->CallVirtual<>("ExportDefault", &This::default_ExportDefault)();
    }

    bool default_ExportFrame(const UsdTimeCode& time) { return base_t::ExportFrame(time); }
    bool ExportFrame(const UsdTimeCode& time) override
    {
        return this->CallVirtual<>("ExportFrame", &This::default_ExportFrame)(time);
    }

    bool default_PostExport() { return base_t::PostExport(); }
    bool PostExport() override
    {
        return this->CallVirtual<>("PostExport", &This::default_PostExport)();
    }

    //---------------------------------------------------------------------------------------------
    /// \brief  wraps a factory function that allows registering an updated Python class
    //---------------------------------------------------------------------------------------------
    class FactoryFnWrapper : public UsdMayaPythonObjectRegistry
    {
    public:
        // Instances of this class act as "function objects" that are fully compatible with the
        // std::function requested by UsdMayaSchemaApiAdaptorRegistry::Register. These will create
        // python wrappers based on the latest Class registered.
        UsdMayaExportChaser*
        operator()(const UsdMayaExportChaserRegistry::FactoryContext& factoryContext)
        {
            PXR_BOOST_PYTHON_NAMESPACE::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return nullptr;
            }
            auto                               chaser = new ExportChaserWrapper();
            TfPyLock                           pyLock;
            PXR_BOOST_PYTHON_NAMESPACE::object instance
                = pyClass(factoryContext, (uintptr_t)chaser);
            PXR_BOOST_PYTHON_NAMESPACE::incref(instance.ptr());
            initialize_wrapper(instance.ptr(), chaser);
            return chaser;
        }

        // Create a new wrapper for a Python class that is seen for the first time for a given
        // purpose. If we already have a registration for this purpose: update the class to
        // allow the previously issued factory function to use it.
        static UsdMayaExportChaserRegistry::FactoryFn
        Register(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& mayaTypeName)
        {
            size_t classIndex = RegisterPythonObject(cl, GetKey(cl, mayaTypeName));
            if (classIndex != UsdMayaPythonObjectRegistry::UPDATED) {
                // Return a new factory function:
                return FactoryFnWrapper { classIndex };
            } else {
                // We already registered a factory function for this purpose:
                return nullptr;
            }
        }

        // Unregister a class for a given purpose. This will cause the associated factory
        // function to stop producing this Python class.
        static void
        Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& mayaTypeName)
        {
            UnregisterPythonObject(cl, GetKey(cl, mayaTypeName));
        }

    private:
        // Function object constructor. Requires only the index of the Python class to use.
        FactoryFnWrapper(size_t classIndex)
            : _classIndex(classIndex) {};

        size_t _classIndex;

        // Generates a unique key based on the name of the class, along with the class
        // purpose:
        static std::string
        GetKey(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& mayaTypeName)
        {
            return ClassName(cl) + "," + mayaTypeName + "," + ",ExportChaser";
        }
    };

    static void Register(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& mayaTypeName)
    {
        UsdMayaExportChaserRegistry::FactoryFn fn = FactoryFnWrapper::Register(cl, mayaTypeName);
        if (fn) {
            UsdMayaExportChaserRegistry::GetInstance().RegisterFactory(mayaTypeName, fn, true);
        }
    }

    static void Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& mayaTypeName)
    {
        FactoryFnWrapper::Unregister(cl, mayaTypeName);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapExportChaserRegistryFactoryContext()
{
    PXR_BOOST_PYTHON_NAMESPACE::class_<UsdMayaExportChaserRegistry::FactoryContext::DagToUsdMap>(
        "DagToUsdMap")
        .def(PXR_BOOST_PYTHON_NAMESPACE::map_indexing_suite<
             UsdMayaExportChaserRegistry::FactoryContext::DagToUsdMap>());

    PXR_BOOST_PYTHON_NAMESPACE::class_<UsdMayaExportChaserRegistry::FactoryContext>(
        "UsdMayaExportChaserRegistryFactoryContext", PXR_BOOST_PYTHON_NAMESPACE::no_init)
        .def("GetStage", &UsdMayaExportChaserRegistry::FactoryContext::GetStage)
        .def(
            "GetDagToUsdMap",
            &UsdMayaExportChaserRegistry::FactoryContext::GetDagToUsdMap,
            PXR_BOOST_PYTHON_NAMESPACE::return_internal_reference<>())
        .def(
            "GetJobArgs",
            &UsdMayaExportChaserRegistry::FactoryContext::GetJobArgs,
            PXR_BOOST_PYTHON_NAMESPACE::return_internal_reference<>());
}

//----------------------------------------------------------------------------------------------------------------------
void wrapExportChaser()
{
    typedef UsdMayaExportChaser This;

    PXR_BOOST_PYTHON_NAMESPACE::
        class_<ExportChaserWrapper, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
            "ExportChaser", PXR_BOOST_PYTHON_NAMESPACE::no_init)
            .def("__init__", make_constructor(&ExportChaserWrapper::New))
            .def("ExportDefault", &This::ExportDefault, &ExportChaserWrapper::default_ExportDefault)
            .def("ExportFrame", &This::ExportFrame, &ExportChaserWrapper::default_ExportFrame)
            .def("PostExport", &This::PostExport, &ExportChaserWrapper::default_PostExport)
            .def(
                "RegisterExtraPrimsPaths",
                &This::RegisterExtraPrimsPaths,
                PXR_BOOST_PYTHON_NAMESPACE::arg("extraPrimPaths"),
                "Method to cache the path for any extra prim path created by the chaser.")
            .def(
                "GetExtraPrimsPaths",
                &This::GetExtraPrimsPaths,
                PXR_BOOST_PYTHON_NAMESPACE::return_internal_reference<>(),
                "Get the array of the currently cached extra paths.")
            .def("Register", &ExportChaserWrapper::Register)
            .staticmethod("Register")
            .def("Unregister", &ExportChaserWrapper::Unregister)
            .staticmethod("Unregister");
}
