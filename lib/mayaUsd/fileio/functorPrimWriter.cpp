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
#include "functorPrimWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/transformWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_FunctorPrimWriter::UsdMaya_FunctorPrimWriter(
    const MFnDependencyNode&            depNodeFn,
    const SdfPath&                      usdPath,
    UsdMayaWriteJobContext&             jobCtx,
    UsdMayaPrimWriterRegistry::WriterFn plugFn)
    : UsdMayaTransformWriter(depNodeFn, usdPath, jobCtx)
    , _plugFn(plugFn)
    , _exportsGprims(false)
    , _pruneChildren(false)
{
}

/* virtual */
UsdMaya_FunctorPrimWriter::~UsdMaya_FunctorPrimWriter() { }

/* virtual */
void UsdMaya_FunctorPrimWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaTransformWriter::Write(usdTime);

    const UsdMayaPrimWriterArgs args(GetDagPath(), _GetExportArgs().exportRefsAsInstanceable);

    UsdMayaPrimWriterContext ctx(usdTime, GetUsdPath(), GetUsdStage());

    _plugFn(args, &ctx);

    _exportsGprims = ctx.GetExportsGprims();
    _pruneChildren = ctx.GetPruneChildren();
    _modelPaths = ctx.GetModelPaths();
}

/* virtual */
bool UsdMaya_FunctorPrimWriter::ExportsGprims() const { return _exportsGprims; }

/* virtual */
bool UsdMaya_FunctorPrimWriter::ShouldPruneChildren() const { return _pruneChildren; }

/* virtual */
const SdfPathVector& UsdMaya_FunctorPrimWriter::GetModelPaths() const { return _modelPaths; }

/* static */
UsdMayaPrimWriterSharedPtr UsdMaya_FunctorPrimWriter::Create(
    const MFnDependencyNode&            depNodeFn,
    const SdfPath&                      usdPath,
    UsdMayaWriteJobContext&             jobCtx,
    UsdMayaPrimWriterRegistry::WriterFn plugFn)
{
    return UsdMayaPrimWriterSharedPtr(
        new UsdMaya_FunctorPrimWriter(depNodeFn, usdPath, jobCtx, plugFn));
}

/* static */
UsdMayaPrimWriterRegistry::WriterFactoryFn
UsdMaya_FunctorPrimWriter::CreateFactory(UsdMayaPrimWriterRegistry::WriterFn fn)
{
    return [=](const MFnDependencyNode& depNodeFn,
               const SdfPath&           usdPath,
               UsdMayaWriteJobContext&  jobCtx) { return Create(depNodeFn, usdPath, jobCtx, fn); };
}

PXR_NAMESPACE_CLOSE_SCOPE
