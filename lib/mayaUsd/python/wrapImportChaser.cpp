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

#include <mayaUsd/fileio/chaser/importChaser.h>
#include <mayaUsd/fileio/chaser/importChaserRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/pyPolymorphic.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaImportChaser
//----------------------------------------------------------------------------------------------------------------------
class ImportChaserWrapper
    : public UsdMayaImportChaser
    , public TfPyPolymorphic<UsdMayaImportChaser>
{
public:
    typedef ImportChaserWrapper This;
    typedef UsdMayaImportChaser base_t;

    static ImportChaserWrapper*
    New(const UsdMayaImportChaserRegistry::FactoryContext& factoryContext, uintptr_t createdWrapper)
    {
        return (ImportChaserWrapper*)createdWrapper;
    }

    virtual ~ImportChaserWrapper() = default;

    bool default_PostImport(
        Usd_PrimFlagsPredicate&     returnPredicate,
        const UsdStagePtr&          stage,
        const MDagPathArray&        dagPaths,
        const SdfPathVector&        sdfPaths,
        const UsdMayaJobImportArgs& jobArgs)
    {
        return base_t::PostImport(returnPredicate, stage, dagPaths, sdfPaths, jobArgs);
    }
    bool PostImport(
        Usd_PrimFlagsPredicate&     returnPredicate,
        const UsdStagePtr&          stage,
        const MDagPathArray&        dagPaths,
        const SdfPathVector&        sdfPaths,
        const UsdMayaJobImportArgs& jobArgs) override
    {
        return this->CallVirtual<bool>("PostImport", &This::default_PostImport)(
            returnPredicate, stage, dagPaths, sdfPaths, jobArgs);
    }

    bool default_Redo() { return base_t::Redo(); }
    bool Redo() override { return this->CallVirtual<>("Redo", &This::default_Redo)(); }

    bool default_Undo() { return base_t::Undo(); }
    bool Undo() override { return this->CallVirtual<>("Undo", &This::default_Undo)(); }

    //---------------------------------------------------------------------------------------------
    /// \brief  wraps a factory function that allows registering an updated Python class
    //---------------------------------------------------------------------------------------------
    class FactoryFnWrapper : public UsdMayaPythonObjectRegistry
    {
    public:
        // Instances of this class act as "function objects" that are fully compatible with the
        // std::function requested by UsdMayaSchemaApiAdaptorRegistry::Register. These will create
        // python wrappers based on the latest class registered.
        UsdMayaImportChaser*
        operator()(const UsdMayaImportChaserRegistry::FactoryContext& factoryContext)
        {
            boost::python::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return nullptr;
            }
            auto                  chaser = new ImportChaserWrapper();
            TfPyLock              pyLock;
            boost::python::object instance = pyClass(factoryContext, (uintptr_t)chaser);
            boost::python::incref(instance.ptr());
            initialize_wrapper(instance.ptr(), chaser);
            return chaser;
        }

        // Create a new wrapper for a Python class that is seen for the first time for a given
        // purpose. It we already have a registration for this purpose: update the class to
        // allow the previously issued factory function to use it.
        static UsdMayaImportChaserRegistry::FactoryFn
        Register(boost::python::object cl, const std::string& mayaTypeName)
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
        static void Unregister(boost::python::object cl, const std::string& mayaTypeName)
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
        static std::string GetKey(boost::python::object cl, const std::string& mayaTypeName)
        {
            // Is it a Python class:
            if (!IsPythonClass(cl)) {
                TfPyThrowRuntimeError("First argument must be a Python class");
            }

            auto nameAttr = cl.attr("__name__");
            if (!nameAttr) {
                TfPyThrowRuntimeError("Unexpected Python error: No __name__ attribute");
            }

            std::string key = boost::python::extract<std::string>(nameAttr);
            key = key + "," + mayaTypeName + "," + ",ImportChaser";
            return key;
        }
    };

    static void Register(boost::python::object cl, const std::string& mayaTypeName)
    {
        UsdMayaImportChaserRegistry::FactoryFn fn = FactoryFnWrapper::Register(cl, mayaTypeName);
        if (fn) {
            UsdMayaImportChaserRegistry::GetInstance().RegisterFactory(
                mayaTypeName.c_str(), fn, true);
        }
    }

    static void Unregister(boost::python::object cl, const std::string& mayaTypeName)
    {
        FactoryFnWrapper::Unregister(cl, mayaTypeName);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapImportChaserRegistryFactoryContext()
{
    boost::python::class_<UsdMayaImportChaserRegistry::FactoryContext>(
        "UsdMayaImportChaserRegistryFactoryContext", boost::python::no_init)
        .def("GetStage", &UsdMayaImportChaserRegistry::FactoryContext::GetStage)
        .def(
            "GetImportedDagPaths",
            &UsdMayaImportChaserRegistry::FactoryContext::GetImportedDagPaths,
            boost::python::return_internal_reference<>())
        .def(
            "GetImportedPrims",
            &UsdMayaImportChaserRegistry::FactoryContext::GetImportedPrims,
            boost::python::return_internal_reference<>())
        .def(
            "GetJobArgs",
            &UsdMayaImportChaserRegistry::FactoryContext::GetJobArgs,
            boost::python::return_internal_reference<>());
}

//----------------------------------------------------------------------------------------------------------------------
void wrapImportChaser()
{
    typedef UsdMayaImportChaser This;

    boost::python::class_<ImportChaserWrapper, boost::noncopyable>(
        "ImportChaser", boost::python::no_init)
        .def("__init__", make_constructor(&ImportChaserWrapper::New))
        .def("PostImport", &This::PostImport, &ImportChaserWrapper::default_PostImport)
        .def("Redo", &This::Redo, &ImportChaserWrapper::default_Redo)
        .def("Undo", &This::Undo, &ImportChaserWrapper::default_Undo)
        .def("Register", &ImportChaserWrapper::Register)
        .staticmethod("Register")
        .def("Unregister", &ImportChaserWrapper::Unregister)
        .staticmethod("Unregister");
}
