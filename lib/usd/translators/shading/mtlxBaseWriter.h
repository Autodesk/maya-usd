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
#ifndef MTLXTRANSLATORS_BASE_WRITER_H
#define MTLXTRANSLATORS_BASE_WRITER_H

/// \file

#include <mayaUsd/fileio/shaderWriter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for exporting Maya's material shading nodes to MaterialX.
class MaterialXTranslators_BaseWriter : public UsdMayaShaderWriter
{
public:
    MaterialXTranslators_BaseWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);
    static ContextSupport CanExport(const UsdMayaJobExportArgs&);

protected:
    // Returns the node graph where all ancillary nodes reside
    UsdPrim GetNodeGraph();

    // Add a swizzle node to the current node to extract a channel from a color output:
    UsdAttribute AddSwizzle(const std::string& channel, int numChannels);

    // Add a swizzle node to extract a channel from a color output:
    UsdAttribute AddSwizzle(const std::string& channel, int numChannels, UsdAttribute nodeOutput);

    // Add a luminance node to get an alpha value from a RGB texture:
    UsdAttribute AddLuminance(int numChannels);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
