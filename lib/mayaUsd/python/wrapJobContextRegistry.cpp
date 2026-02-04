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
#include <pxr_python.h>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief Python binding for the UsdMayaJobContextRegistry
//----------------------------------------------------------------------------------------------------------------------
class JobContextRegistry
{
public:
    static PXR_BOOST_PYTHON_NAMESPACE::object GetJobContextInfo(const TfToken& jobContext)
    {
        UsdMayaJobContextRegistry::ContextInfo ctx
            = UsdMayaJobContextRegistry::GetInstance().GetJobContextInfo(jobContext);
        PXR_BOOST_PYTHON_NAMESPACE::dict dict;
        dict["jobContext"] = ctx.jobContext;
        dict["niceName"] = ctx.niceName;
        dict["exportDescription"] = ctx.exportDescription;
        //        dict["exportEnablerCallback"] = ctx.exportEnablerCallback;
        dict["importDescription"] = ctx.importDescription;
        //        dict["importEnablerCallback"] = ctx.importEnablerCallback;
        return std::move(dict);
    }

    static void RegisterImportJobContext(
        const std::string&                 jobContext,
        const std::string&                 niceName,
        const std::string&                 description,
        PXR_BOOST_PYTHON_NAMESPACE::object enablerFct)
    {
        if (!PyCallable_Check(enablerFct.ptr()))
            TF_CODING_ERROR(
                "Parameter enablerFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().RegisterImportJobContext(
            jobContext, niceName, description, [=]() { return callEnablerFn(enablerFct); }, true);
    }
    static void RegisterExportJobContext(
        const std::string&                 jobContext,
        const std::string&                 niceName,
        const std::string&                 description,
        PXR_BOOST_PYTHON_NAMESPACE::object enablerFct)
    {
        if (!PyCallable_Check(enablerFct.ptr()))
            TF_CODING_ERROR(
                "Parameter enablerFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().RegisterExportJobContext(
            jobContext, niceName, description, [=]() { return callEnablerFn(enablerFct); }, true);
    }

    static void
    SetExportOptionsUI(const std::string& jobContext, PXR_BOOST_PYTHON_NAMESPACE::object uiFct)
    {
        if (!PyCallable_Check(uiFct.ptr()))
            TF_CODING_ERROR(
                "Parameter uiFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().SetExportOptionsUI(
            jobContext,
            [=](const TfToken&      jobContext,
                const std::string&  parentUI,
                const VtDictionary& settings) {
                return callUIFn(uiFct, jobContext, parentUI, settings);
            },
            true);
    }

    static void
    SetImportOptionsUI(const std::string& jobContext, PXR_BOOST_PYTHON_NAMESPACE::object uiFct)
    {
        if (!PyCallable_Check(uiFct.ptr()))
            TF_CODING_ERROR(
                "Parameter uiFct should be a callable function returning a dictionary.");

        UsdMayaJobContextRegistry::GetInstance().SetImportOptionsUI(
            jobContext,
            [=](const TfToken&      jobContext,
                const std::string&  parentUI,
                const VtDictionary& settings) {
                return callUIFn(uiFct, jobContext, parentUI, settings);
            },
            true);
    }

private:
    static VtDictionary callEnablerFn(PXR_BOOST_PYTHON_NAMESPACE::object fnc)
    {
        auto res = TfPyCall<VtDictionary>(fnc)();
        return res;
    }

    static VtDictionary callUIFn(
        PXR_BOOST_PYTHON_NAMESPACE::object fnc,
        const TfToken&                     jobContext,
        const std::string&                 parentUI,
        const VtDictionary&                settings)
    {
        auto res = TfPyCall<VtDictionary>(fnc)(jobContext, parentUI, settings);
        return res;
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapJobContextRegistry()
{
    PXR_BOOST_PYTHON_NAMESPACE::class_<JobContextRegistry, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "JobContextRegistry", PXR_BOOST_PYTHON_NAMESPACE::no_init)
        .def("GetJobContextInfo", &JobContextRegistry::GetJobContextInfo)
        .staticmethod("GetJobContextInfo")
        .def("RegisterImportJobContext", &JobContextRegistry::RegisterImportJobContext)
        .staticmethod("RegisterImportJobContext")
        .def("RegisterExportJobContext", &JobContextRegistry::RegisterExportJobContext)
        .staticmethod("RegisterExportJobContext")
        .def("SetExportOptionsUI", &JobContextRegistry::SetExportOptionsUI)
        .staticmethod("SetExportOptionsUI")
        .def("SetImportOptionsUI", &JobContextRegistry::SetImportOptionsUI)
        .staticmethod("SetImportOptionsUI");
}
