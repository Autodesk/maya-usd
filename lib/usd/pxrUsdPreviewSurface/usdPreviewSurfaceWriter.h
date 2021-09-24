//
// Copyright 2018 Pixar
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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_WRITER_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_WRITER_H

/// \file usdPreviewSurfaceWriter.h

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPreviewSurface_Writer : public UsdMayaShaderWriter
{
public:
    PxrMayaUsdPreviewSurface_Writer(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    static ContextSupport CanExport(const UsdMayaJobExportArgs&, const TfToken&);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
