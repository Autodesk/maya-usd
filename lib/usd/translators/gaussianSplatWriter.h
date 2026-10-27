//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_TRANSLATORS_GAUSSIAN_SPLAT_WRITER_H
#define MAYAUSD_TRANSLATORS_GAUSSIAN_SPLAT_WRITER_H

/// \file

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

namespace MAYAUSD_NS_DEF {

/// Exports an mtoa aiGaussianSplat node as a UsdVolParticleField3DGaussianSplat prim by
/// reading the splat cloud directly from the node's .ply file.
class GaussianSplatWriter : public PXR_NS::UsdMayaPrimWriter
{
public:
    GaussianSplatWriter(
        const MFnDependencyNode&        depNodeFn,
        const PXR_NS::SdfPath&          usdPath,
        PXR_NS::UsdMayaWriteJobContext& jobCtx);

    void Write(const PXR_NS::UsdTimeCode& usdTime) override;

    bool ExportsGprims() const override { return true; }

    // MtoA registers a generic ArnoldDagWriter fallback for every Arnold DAG node type it
    // doesn't explicitly ignore, including aiGaussianSplat. That writer and this one both
    // target the same Maya type, so this one must claim Supported to win the
    // UsdMayaPrimWriterRegistry tie-break deterministically instead of depending on
    // registration order.
    static ContextSupport
    CanExport(const PXR_NS::UsdMayaJobExportArgs& exportArgs, const MObject& exportObj);
};

} // namespace MAYAUSD_NS_DEF

#endif
