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
#include <mayaUsd/fileio/jobContextRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_EXPORT_JOB_CONTEXT_FCT(NullAPI, "Null API", "Exports an empty API for testing purpose")
{
    VtDictionary extraArgs;
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema]
        = VtValue(std::vector<VtValue> { VtValue(std::string("testApi")) });
    extraArgs[UsdMayaJobExportArgsTokens->chaser]
        = VtValue(std::vector<VtValue> { VtValue(std::string("NullAPIChaser")) });
    VtValue chaserArg(std::vector<VtValue> { VtValue(std::string("NullAPIChaser")),
                                             VtValue(std::string("life")),
                                             VtValue(std::string("42")) });
    extraArgs[UsdMayaJobExportArgsTokens->chaserArgs] = VtValue(std::vector<VtValue> { chaserArg });
    return extraArgs;
}

REGISTER_IMPORT_JOB_CONTEXT_FCT(NullAPI, "Null API", "Imports an empty API for testing purpose")
{
    VtDictionary extraArgs;
    extraArgs[UsdMayaJobImportArgsTokens->apiSchema]
        = VtValue(std::vector<VtValue> { VtValue(std::string("testApiIn")) });
    extraArgs[UsdMayaJobImportArgsTokens->chaser]
        = VtValue(std::vector<VtValue> { VtValue(std::string("NullAPIChaserIn")) });
    VtValue chaserArg(std::vector<VtValue> { VtValue(std::string("NullAPIChaserIn")),
                                             VtValue(std::string("universe")),
                                             VtValue(std::string("42")) });
    extraArgs[UsdMayaJobImportArgsTokens->chaserArgs] = VtValue(std::vector<VtValue> { chaserArg });
    return extraArgs;
}

REGISTER_EXPORT_JOB_CONTEXT_FCT(Thierry, "Thierry", "Exports for Thierry renderer")
{
    return VtDictionary();
}

REGISTER_EXPORT_JOB_CONTEXT_FCT(SceneGrinder, "Scene Grinder", "Exports to Scene Grinder")
{
    return VtDictionary();
}

REGISTER_EXPORT_JOB_CONTEXT_FCT(
    Larry,
    "Larry's special",
    "Test coverage of error handling part uno")
{
    VtDictionary extraArgs;
    // Correct:
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema]
        = VtValue(std::vector<VtValue> { VtValue(std::string("testApi")) });
    extraArgs[UsdMayaJobExportArgsTokens->geomSidedness] = VtValue(std::string("single"));
    // Referencing another context:
    extraArgs[UsdMayaJobExportArgsTokens->jobContext]
        = VtValue(std::vector<VtValue> { VtValue(std::string("Curly")) });
    return extraArgs;
}

REGISTER_EXPORT_JOB_CONTEXT_FCT(
    Curly,
    "Curly's special",
    "Test coverage of error handling part deux")
{
    VtDictionary extraArgs;
    // Incorrect type:
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema] = VtValue(std::string("testApi"));
    return extraArgs;
}

REGISTER_EXPORT_JOB_CONTEXT_FCT(Moe, "Moe's special", "Test coverage of error handling part funf")
{
    VtDictionary extraArgs;
    // Moe is conflicting on value with Larry, but merges nicely with NullAPI:
    extraArgs[UsdMayaJobExportArgsTokens->geomSidedness] = VtValue(std::string("double"));
    VtValue chaserArg(std::vector<VtValue> { VtValue(std::string("NullAPIChaser")),
                                             VtValue(std::string("genre")),
                                             VtValue(std::string("slapstick")) });
    extraArgs[UsdMayaJobExportArgsTokens->chaserArgs] = VtValue(std::vector<VtValue> { chaserArg });
    return extraArgs;
}

// We added this dummy chaser with the export context.

class NullAPIChaser : public UsdMayaExportChaser
{
public:
    virtual ~NullAPIChaser() { }

    virtual bool ExportDefault() override
    {
        TF_RUNTIME_ERROR("Missing implementation for NullAPIChaser::ExportDefault");
        return true;
    }

    virtual bool ExportFrame(const UsdTimeCode& frame) override { return true; }
};

PXRUSDMAYA_DEFINE_EXPORT_CHASER_FACTORY(NullAPIChaser, ctx) { return new NullAPIChaser(); }

PXR_NAMESPACE_CLOSE_SCOPE
