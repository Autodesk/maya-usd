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

#include <mayaUsd/fileio/jobContextRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/pyPolymorphic.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaJobContextRegistry
//----------------------------------------------------------------------------------------------------------------------
class JobContextRegistry
{
public:
    static boost::python::object GetJobContextInfo(const TfToken& jobContext)
    {
        UsdMayaJobContextRegistry::ContextInfo ctx
            = UsdMayaJobContextRegistry::GetInstance().GetJobContextInfo(jobContext);
        boost::python::dict dict;
        dict["jobContext"] = ctx.jobContext;
        dict["niceName"] = ctx.niceName;
        dict["exportDescription"] = ctx.exportDescription;
        //        dict["exportEnablerCallback"] = ctx.exportEnablerCallback;
        dict["importDescription"] = ctx.importDescription;
        //        dict["importEnablerCallback"] = ctx.importEnablerCallback;
        return dict;
    }

    static void RegisterImportJobContext(
        const std::string&    jobContext,
        const std::string&    niceName,
        const std::string&    description,
        boost::python::object enablerFct)
    {
        if (!PyCallable_Check(enablerFct.ptr()))
            TF_CODING_ERROR(
                "Parameter enablerFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().RegisterImportJobContext(
            jobContext, niceName, description, [=]() { return callEnablerFn(enablerFct); }, true);
    }
    static void RegisterExportJobContext(
        const std::string&    jobContext,
        const std::string&    niceName,
        const std::string&    description,
        boost::python::object enablerFct)
    {
        if (!PyCallable_Check(enablerFct.ptr()))
            TF_CODING_ERROR(
                "Parameter enablerFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().RegisterExportJobContext(
            jobContext, niceName, description, [=]() { return callEnablerFn(enablerFct); }, true);
    }

private:
    static VtDictionary callEnablerFn(boost::python::object fnc)
    {
        auto res = std::function<boost::python::object()>(TfPyCall<boost::python::object>(fnc))();
        return boost::python::extract<VtDictionary>(res);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapJobContextRegistry()
{
    boost::python::class_<JobContextRegistry, boost::noncopyable>(
        "JobContextRegistry", boost::python::no_init)
        .def("GetJobContextInfo", &JobContextRegistry::GetJobContextInfo)
        .staticmethod("GetJobContextInfo")
        .def("RegisterImportJobContext", &JobContextRegistry::RegisterImportJobContext)
        .staticmethod("RegisterImportJobContext")
        .def("RegisterExportJobContext", &JobContextRegistry::RegisterExportJobContext)
        .staticmethod("RegisterExportJobContext");
}
