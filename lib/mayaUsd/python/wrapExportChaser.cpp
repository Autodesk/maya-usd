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

#include <mayaUsd/fileio/chaser/exportChaser.h>
#include <mayaUsd/fileio/chaser/exportChaserRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/pyPolymorphic.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaExportChaser
//----------------------------------------------------------------------------------------------------------------------
class ExportChaserWrapper
    : public UsdMayaExportChaser
    , public TfPyPolymorphic<UsdMayaExportChaser>
{
public:
    typedef ExportChaserWrapper This;
    typedef UsdMayaExportChaser base_t;

    static ExportChaserWrapper* New(uintptr_t createdWrapper)
    {
        return (ExportChaserWrapper*)createdWrapper;
    }

    ExportChaserWrapper(const UsdMayaExportChaserRegistry::FactoryContext& factoryContext)
        : _factoryContext(factoryContext)
    {
    }

    virtual ~ExportChaserWrapper() { }

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

    static void Register(boost::python::object cl, const std::string& mayaTypeName)
    {
        UsdMayaExportChaserRegistry::GetInstance().RegisterFactory(
            mayaTypeName,
            [=](const UsdMayaExportChaserRegistry::FactoryContext& factoryContext) {
                auto                  chaser = new ExportChaserWrapper(factoryContext);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)chaser);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), chaser);
                return chaser;
            },
            true);
    }

    const UsdMayaExportChaserRegistry::FactoryContext& GetFactoryContext()
    {
        return _factoryContext;
    }

    private:
        const UsdMayaExportChaserRegistry::FactoryContext& _factoryContext;
};

//----------------------------------------------------------------------------------------------------------------------
void wrapExportChaserRegistryFactoryContext()
{
    boost::python::class_<UsdMayaExportChaserRegistry::FactoryContext>(
        "UsdMayaExportChaserRegistryFactoryContext", boost::python::no_init)
        .def(
            "GetStage",
            &UsdMayaExportChaserRegistry::FactoryContext::GetStage)
        .def(
            "GetDagToUsdMap",
            &UsdMayaExportChaserRegistry::FactoryContext::GetDagToUsdMap,
            boost::python::return_internal_reference<>())
        .def(
            "GetJobArgs",
            &UsdMayaExportChaserRegistry::FactoryContext::GetJobArgs,
            boost::python::return_internal_reference<>());
}

//----------------------------------------------------------------------------------------------------------------------
void wrapExportChaser()
{
    typedef UsdMayaExportChaser This;

    boost::python::class_<ExportChaserWrapper, boost::noncopyable>(
        "ExportChaser", boost::python::no_init)
        .def("__init__", make_constructor(&ExportChaserWrapper::New))
        .def("ExportDefault", &This::ExportDefault, &ExportChaserWrapper::default_ExportDefault)
        .def("ExportFrame", &This::ExportFrame, &ExportChaserWrapper::default_ExportFrame)
        .def("PostExport", &This::PostExport, &ExportChaserWrapper::default_PostExport)
        .def(
            "GetFactoryContext",
            &ExportChaserWrapper::GetFactoryContext,
            boost::python::return_internal_reference<>())
        .def("Register", &ExportChaserWrapper::Register)
        .staticmethod("Register");
}
