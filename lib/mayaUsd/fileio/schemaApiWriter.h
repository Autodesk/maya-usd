//
// Copyright 2021 Autodesk
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
#ifndef PXRUSDMAYA_SCHEMA_API_WRITER_H
#define PXRUSDMAYA_SCHEMA_API_WRITER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primWriter.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaWriteJobContext;

/// Base class for all built-in and user-defined schema API writers. Appends
/// schema APIs to already written USD prims.
///
class UsdMayaSchemaApiWriter
{
public:
    /// Constructs a schema API writer for writing over a primWriter data.
    ///
    /// The primWriter will have been run before this constructor is called and should provide all
    /// necessary information.
    MAYAUSD_CORE_PUBLIC
    UsdMayaSchemaApiWriter(
        const UsdMayaPrimWriterSharedPtr& primWriter,
        UsdMayaWriteJobContext&           jobCtx);

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaSchemaApiWriter();

    /// Main export function that runs when the traversal hits the node.
    /// The default implementation is currently empty, but in most cases, subclasses will want to
    /// invoke the base class Write() method when overriding to be future proof.
    MAYAUSD_CORE_PUBLIC
    virtual void Write(const UsdTimeCode& usdTime);

    /// Post export function that runs before saving the stage.
    ///
    /// Base implementation handles optional optimization of data.
    MAYAUSD_CORE_PUBLIC
    virtual void PostExport();

    /// Modify all primvars on this prim with single time samples to be static instead.
    MAYAUSD_CORE_PUBLIC
    void MakeSingleSamplesStatic();

    /// Modify a specific primvar attribute with single time samples
    /// to be static.
    MAYAUSD_CORE_PUBLIC
    void MakeSingleSamplesStatic(UsdAttribute attr);

protected:
    /// Gets the current global export args in effect.
    MAYAUSD_CORE_PUBLIC
    const UsdMayaJobExportArgs& _GetExportArgs() const;

    /// Gets the associated prim writer.
    MAYAUSD_CORE_PUBLIC
    const UsdMayaPrimWriterSharedPtr& _GetPrimWriter() const;

    /// Get the attribute value-writer object to be used when writing
    /// attributes. Access to this is provided so that attribute authoring
    /// happening inside non-member functions can make use of it.
    MAYAUSD_CORE_PUBLIC
    UsdUtilsSparseValueWriter* _GetSparseValueWriter();

    UsdMayaWriteJobContext& _writeJobCtx;

    const UsdMayaPrimWriterSharedPtr _primWriter;

private:
    UsdUtilsSparseValueWriter _valueWriter;
};

using UsdMayaSchemaApiWriterSharedPtr = std::shared_ptr<UsdMayaSchemaApiWriter>;
using UsdMayaSchemaApiWriterList = std::vector<UsdMayaSchemaApiWriterSharedPtr>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
