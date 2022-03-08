//
// Copyright 2022 Autodesk
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
#ifndef MTLXTRANSLATORS_BASE_READER_H
#define MTLXTRANSLATORS_BASE_READER_H

/// \file

#include <mayaUsd/fileio/shaderReader.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdShade/shader.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader reader for importing MaterialX shading nodes to Maya.
class MtlxUsd_BaseReader : public UsdMayaShaderReader
{
public:
    MtlxUsd_BaseReader(const UsdMayaPrimReaderArgs& readArgs);

protected:
    /// Read back attribute \p attributeName from schema \p shaderSchema and set the value when
    /// valid on \p depNodeFn
    bool ReadShaderInput(
        UsdShadeShader&          shaderSchema,
        const TfToken&           attributeName,
        const MFnDependencyNode& depNodeFn,
        const bool               unlinearizeColors = true);

    /// Extract the color and alpha from an input that could have any number of channels:
    bool GetColorAndAlphaFromInput(
        const UsdShadeShader& shader,
        const TfToken&        inputName,
        GfVec3f&              color,
        float&                alpha);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
