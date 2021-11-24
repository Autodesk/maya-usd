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

    static ImportChaserWrapper* New(uintptr_t createdWrapper)
    {
        return (ImportChaserWrapper*)createdWrapper;
    }

    ImportChaserWrapper(const UsdMayaImportChaserRegistry::FactoryContext& factoryContext)
        : _factoryContext(factoryContext)
    {
    }

    virtual ~ImportChaserWrapper() { }

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

    static void Register(boost::python::object cl, const char* name)
    {
        UsdMayaImportChaserRegistry::GetInstance().RegisterFactory(
            name,
            [=](const UsdMayaImportChaserRegistry::FactoryContext& factoryContext) {
                auto                  chaser = new ImportChaserWrapper(factoryContext);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)(ImportChaserWrapper*)chaser);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), chaser);
                return chaser;
            },
            true);
    }

    const UsdMayaImportChaserRegistry::FactoryContext& GetFactoryContext()
    {
        return _factoryContext;
    }

private:
    const UsdMayaImportChaserRegistry::FactoryContext& _factoryContext;
};

//----------------------------------------------------------------------------------------------------------------------
void wrapImportChaserRegistryFactoryContext()
{
    boost::python::class_<UsdMayaImportChaserRegistry::FactoryContext>(
        "UsdMayaExportChaserRegistryFactoryContext", boost::python::no_init)
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
            "GetImportJobArgs",
            &UsdMayaImportChaserRegistry::FactoryContext::GetImportJobArgs,
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
        .def(
            "GetFactoryContext",
            &ImportChaserWrapper::GetFactoryContext,
            boost::python::return_internal_reference<>())
        .def("Register", &ImportChaserWrapper::Register)
        .staticmethod("Register");
}
