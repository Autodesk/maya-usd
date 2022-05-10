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
#include <pxr/usd/usdShade/shader.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Shader writer for exporting Maya's material shading nodes to MaterialX.
class MtlxUsd_BaseWriter : public UsdMayaShaderWriter
{
public:
    MtlxUsd_BaseWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);
    static ContextSupport CanExport(const UsdMayaJobExportArgs&);

    // Utility to find out in advance if a place2dTextureNode will be exported or not.
    static bool IsAuthoredPlace2dTexture(const MFnDependencyNode& p2dTxFn);

protected:
    // Returns the node graph where all ancillary nodes reside
    UsdPrim GetNodeGraph();

    // Add a swizzle node to extract a channel from a color output:
    UsdAttribute AddSwizzle(const std::string& channel, int numChannels, UsdAttribute nodeOutput);

    // Add a swizzle node to extract a channel from any output:
    UsdAttribute ExtractChannel(size_t channelIndex, UsdAttribute nodeOutput);

    // Add a constructor node for subchannel connection on an input:
    UsdAttribute AddConstructor(UsdAttribute nodeInput, size_t channelIndex, MPlug inputPlug);

    // Add a swizzle node that converts from the type found in \p nodeOutput to \p destType
    UsdAttribute AddConversion(const SdfValueTypeName& destType, UsdAttribute nodeOutput);

    // Add a luminance node to the current node to get an alpha value from an RGB texture:
    UsdAttribute AddLuminance(int numChannels, UsdAttribute nodeOutput);

    // Add normal mapping functionnality to a normal input
    UsdAttribute AddNormalMapping(UsdAttribute normalInput);

    // Make sure that a material-level input uses a nodegraph boundary port for connecting
    // to subgraph nodes:
    UsdAttribute PreserveNodegraphBoundaries(UsdAttribute input);

    /// Adds a schema attribute to the schema \p shaderSchema if the Maya attribute \p
    /// shadingNodeAttrName in dependency node \p depNodeFn has been modified or has an incoming
    /// connection at \p usdTime.
    bool AuthorShaderInputFromShadingNodeAttr(
        const MFnDependencyNode& depNodeFn,
        const TfToken&           shadingNodeAttrName,
        UsdShadeShader&          shaderSchema,
        const UsdTimeCode        usdTime,
        bool                     ignoreIfUnauthored = true);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
