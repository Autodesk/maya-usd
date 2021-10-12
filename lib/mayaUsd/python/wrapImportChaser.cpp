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

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <maya/MFnDependencyNode.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
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
            [=](const UsdMayaImportChaserRegistry::FactoryContext& context) {
                auto                  chaser = new ImportChaserWrapper();
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)(ImportChaserWrapper*)chaser);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), chaser);
                return chaser;
            },
            true);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapImportChaser()
{
    typedef ImportChaserWrapper* ImportChaserWrapperPtr;

    boost::python::class_<ImportChaserWrapper, boost::noncopyable>(
        "ImportChaser", boost::python::no_init)
        .def("__init__", make_constructor(&ImportChaserWrapper::New))
        .def(
            "PostImport",
            &ImportChaserWrapper::PostImport,
            &ImportChaserWrapper::default_PostImport)
        .def("Redo", &ImportChaserWrapper::Redo, &ImportChaserWrapper::default_Redo)
        .def("Undo", &ImportChaserWrapper::Undo, &ImportChaserWrapper::default_Undo)
        .def(
            "Register",
            &ImportChaserWrapper::Register,
            (boost::python::arg("class"), boost::python::arg("type")))
        .staticmethod("Register");
}
