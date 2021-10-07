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

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
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
            [=](const UsdMayaExportChaserRegistry::FactoryContext& contextArgName) {
                auto                  chaser = new ExportChaserWrapper();
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)chaser);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), chaser);
                return chaser;
            },
            true);
    }
};

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
        .def("Register", &ExportChaserWrapper::Register)
        .staticmethod("Register");
}
