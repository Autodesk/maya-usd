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
#include <mayaUsd/fileio/exportContextRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/schemaApiWriter.h>
#include <mayaUsd/fileio/schemaApiWriterRegistry.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_EXPORT_CONTEXT_FCT(NullAPI, "Null API Export", "Exports an empty API for testing purpose")
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

REGISTER_EXPORT_CONTEXT_FCT(Thierry, "Thierry", "Exports for Thierry renderer")
{
    return VtDictionary();
}

REGISTER_EXPORT_CONTEXT_FCT(SceneGrinder, "Scene Grinder", "Exports to Scene Grinder")
{
    return VtDictionary();
}

REGISTER_EXPORT_CONTEXT_FCT(Larry, "Larry's special", "Test coverage of error handling part uno")
{
    VtDictionary extraArgs;
    // Correct:
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema]
        = VtValue(std::vector<VtValue> { VtValue(std::string("testApi")) });
    extraArgs[UsdMayaJobExportArgsTokens->convertMaterialsTo] = VtValue(std::string("MaterialX"));
    // Referencing another context:
    extraArgs[UsdMayaJobExportArgsTokens->extraContext]
        = VtValue(std::vector<VtValue> { VtValue(std::string("Curly")) });
    return extraArgs;
}

REGISTER_EXPORT_CONTEXT_FCT(Curly, "Curly's special", "Test coverage of error handling part deux")
{
    VtDictionary extraArgs;
    // Incorrect type:
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema] = VtValue(std::string("testApi"));
    return extraArgs;
}

REGISTER_EXPORT_CONTEXT_FCT(Moe, "Moe's special", "Test coverage of error handling part funf")
{
    VtDictionary extraArgs;
    // Moe is conflicting on value with Larry, but merges nicely with NullAPI:
    extraArgs[UsdMayaJobExportArgsTokens->convertMaterialsTo]
        = VtValue(std::string("rendermanForMaya"));
    VtValue chaserArg(std::vector<VtValue> { VtValue(std::string("NullAPIChaser")),
                                             VtValue(std::string("genre")),
                                             VtValue(std::string("slapstick")) });
    extraArgs[UsdMayaJobExportArgsTokens->chaserArgs] = VtValue(std::vector<VtValue> { chaserArg });
    return extraArgs;
}

class TestSchemaExporter : public UsdMayaSchemaApiWriter
{
    bool _isValidChaser = false;
    bool _isValidChaserArgs = false;
    bool _isValidMaterialConversion = false;

public:
    TestSchemaExporter(const UsdMayaPrimWriterSharedPtr& primWriter, UsdMayaWriteJobContext& jobCtx)
        : UsdMayaSchemaApiWriter(primWriter, jobCtx)
    {
        const UsdMayaJobExportArgs& jobArgs = jobCtx.GetArgs();
        _isValidChaser
            = jobArgs.chaserNames.size() == 1 && jobArgs.chaserNames.front() == "NullAPIChaser";

        // Default value of "UsdPreviewsurface" overwritten by stronger context:
        _isValidMaterialConversion = jobArgs.convertMaterialsTo == "rendermanForMaya";

        // Validate chaser args were merged:
        std::map<std::string, std::string> myArgs;
        TfMapLookup(jobArgs.allChaserArgs, "NullAPIChaser", &myArgs);
        _isValidChaserArgs = myArgs.count("life") > 0 && myArgs["life"] == "42"
            && myArgs.count("genre") > 0 && myArgs["genre"] == "slapstick";
    }

    void Write(const UsdTimeCode&) override
    {
        if (!_isValidChaser) {
            TF_RUNTIME_ERROR("Missing chaser name NullAPIChaser in job arguments");
        }
        if (!_isValidMaterialConversion) {
            TF_RUNTIME_ERROR("Incorrect material conversion in job arguments");
        }
        if (!_isValidChaserArgs) {
            TF_RUNTIME_ERROR("Incorrect chaser args in job arguments");
        }
        TF_RUNTIME_ERROR("Missing implementation for TestSchemaExporter::Write");
    }

    void PostExport() override
    {
        TF_RUNTIME_ERROR("Missing implementation for TestSchemaExporter::PostExport");
    }
};

PXRUSDMAYA_REGISTER_SCHEMA_API_WRITER(mesh, testApi, TestSchemaExporter);

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

// The following exporter gets registered for a schema that is not in the list.
// It should not execute.

class UnusedSchemaExporter : public UsdMayaSchemaApiWriter
{
public:
    UnusedSchemaExporter(
        const UsdMayaPrimWriterSharedPtr& primWriter,
        UsdMayaWriteJobContext&           jobCtx)
        : UsdMayaSchemaApiWriter(primWriter, jobCtx)
    {
    }

    void Write(const UsdTimeCode&) override
    {
        TF_RUNTIME_ERROR("SHOULD NOT BE CALLED: UnusedSchemaExporter::Write");
    }

    void PostExport() override
    {
        TF_RUNTIME_ERROR("SHOULD NOT BE CALLED: UnusedSchemaExporter::PostExport");
    }
};

PXRUSDMAYA_REGISTER_SCHEMA_API_WRITER(mesh, unusedApi, UnusedSchemaExporter);

PXR_NAMESPACE_CLOSE_SCOPE
