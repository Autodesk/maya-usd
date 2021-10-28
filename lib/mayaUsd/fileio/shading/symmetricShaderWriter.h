//
// Copyright 2020 Pixar
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
#ifndef MAYAUSD_FILEIO_SHADING_SYMMETRIC_SHADER_WRITER_H
#define MAYAUSD_FILEIO_SHADING_SYMMETRIC_SHADER_WRITER_H

/// \file

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaSymmetricShaderWriter
/// \brief Provides "literal" translation of Maya shading nodes to USD Shader
/// prims.
///
/// This shader writer performs a "literal" translation of a Maya shading node
/// type to USD. Input and output attributes on the Maya node translate
/// directly to inputs and outputs with the same names on the exported
/// UsdShadeShader.
///
/// A static "RegisterWriter()" function is provided to simplify the
/// registration of writers that use this class. Note however that it should be
/// called inside a "TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)" block,
/// for example:
///
/// \code
/// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
/// {
///     UsdMayaSymmetricShaderWriter::RegisterWriter(
///         TfToken("myMayaNodeTypeName"),
///         TfToken("MyUsdShaderId"),
///         /* materialConversionName = */ TfToken("myMaterialConversionName"));
/// };
/// \endcode
///
class UsdMayaSymmetricShaderWriter : public UsdMayaShaderWriter
{
public:
    /// Register a shader writer to translate \p mayaNodeTypeName Maya nodes to
    /// USD shaders with ID \p usdShaderId.
    ///
    /// The shader writer can optionally be restricted to a particular material
    /// conversion. If no conversion name is supplied, the writer's
    /// "CanExport()" function will always return "Supported". If a conversion
    /// name is supplied, "Supported" is returned if the conversion name
    /// matches the one specified in the export args, and "Unsupported" is
    /// returned otherwise.
    ///
    /// Note that this function should generally only be called inside a
    /// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry) block.
    MAYAUSD_CORE_PUBLIC
    static void RegisterWriter(
        const TfToken& mayaNodeTypeName,
        const TfToken& usdShaderId,
        const TfToken& materialConversionName = TfToken(),
        bool           fromPython = false);

    MAYAUSD_CORE_PUBLIC
    static ContextSupport CanExport(
        const UsdMayaJobExportArgs& exportArgs,
        const TfToken&              materialConversionName = TfToken());

    MAYAUSD_CORE_PUBLIC
    UsdMayaSymmetricShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx,
        const TfToken&           usdShaderId);

    MAYAUSD_CORE_PUBLIC
    void Write(const UsdTimeCode& usdTime) override;

    MAYAUSD_CORE_PUBLIC
    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
