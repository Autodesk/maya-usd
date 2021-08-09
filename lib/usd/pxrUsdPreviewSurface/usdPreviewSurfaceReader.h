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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_READER_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_READER_H

/// \file usdPreviewSurfaceWriter.h

#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPreviewSurface_Reader : public UsdMayaShaderReader
{
    TfToken _mayaTypeName;

public:
    PxrMayaUsdPreviewSurface_Reader(const UsdMayaPrimReaderArgs&, const TfToken& mayaTypeName);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
