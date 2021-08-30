//
// Copyright 2020 Autodesk
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

REGISTER_EXPORT_CONTEXT_FCT(
    NullAPI,
    "Null API Export",
    "Exports an empty API for testing purpose",
    userArgs)
{
    auto addToVector = [](VtDictionary& userArgs, const TfToken& key, const std::string& val) {
        std::vector<VtValue> vals;
        if (VtDictionaryIsHolding<std::vector<VtValue>>(userArgs, key)) {
            vals = VtDictionaryGet<std::vector<VtValue>>(userArgs, key);
        }
        vals.push_back(VtValue(val));
        userArgs[key] = VtValue(vals);
    };

    addToVector(userArgs, UsdMayaJobExportArgsTokens->apiSchema, "testApi");
    addToVector(userArgs, UsdMayaJobExportArgsTokens->chaser, "NullAPIChaser");

    return true;
}

class TestSchemaExporter : public UsdMayaSchemaApiWriter
{
    bool _isValid = false;

public:
    TestSchemaExporter(const UsdMayaPrimWriterSharedPtr& primWriter, UsdMayaWriteJobContext& jobCtx)
        : UsdMayaSchemaApiWriter(primWriter, jobCtx)
    {
        const UsdMayaJobExportArgs& jobArgs = jobCtx.GetArgs();
        for (const std::string& chaserName : jobArgs.chaserNames) {
            if (chaserName == "NullAPIChaser") {
                _isValid = true;
                break;
            }
        }
    }

    void Write(const UsdTimeCode&) override
    {
        if (!_isValid) {
            TF_RUNTIME_ERROR("Missing chaser name NullAPIChaser in job arguments");
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
