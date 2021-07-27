//
// Copyright 2016 Pixar
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
#include "schemaApiWriter.h"

#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaSchemaApiWriter::UsdMayaSchemaApiWriter(
    const UsdMayaPrimWriterSharedPtr& primWriter,
    UsdMayaWriteJobContext&           jobCtx)
    : _writeJobCtx(jobCtx)
    , _primWriter(primWriter)
{
}

/* virtual */
UsdMayaSchemaApiWriter::~UsdMayaSchemaApiWriter() { }

/* virtual */
void UsdMayaSchemaApiWriter::Write(const UsdTimeCode&) { }

/* virtual */
void UsdMayaSchemaApiWriter::PostExport() { }

const UsdMayaJobExportArgs& UsdMayaSchemaApiWriter::_GetExportArgs() const
{
    return _writeJobCtx.GetArgs();
}

UsdUtilsSparseValueWriter* UsdMayaSchemaApiWriter::_GetSparseValueWriter() { return &_valueWriter; }

void UsdMayaSchemaApiWriter::MakeSingleSamplesStatic()
{
    auto exportArgs = _GetExportArgs();

    if (!exportArgs.staticSingleSample) {
        return;
    }

    UsdPrim prim = _primWriter->GetUsdPrim();
    if (!prim.IsValid()) {
        return;
    }

    for (auto& attr : prim.GetAttributes()) {
        MakeSingleSamplesStatic(attr);
    }
};

void UsdMayaSchemaApiWriter::MakeSingleSamplesStatic(UsdAttribute attr)
{
    std::vector<double> samples;
    if (attr.GetNumTimeSamples() != 1) {
        return;
    }

    attr.GetTimeSamples(&samples);

    VtValue sample;
    attr.Get(&sample, samples[0]);

    // Clear all time samples and then sets a default
    attr.Clear();
    attr.Set(sample);
}

PXR_NAMESPACE_CLOSE_SCOPE
