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
#ifndef MTLXTRANSLATORS_SYMMETRIC_SHADER_WRITER_H
#define MTLXTRANSLATORS_SYMMETRIC_SHADER_WRITER_H

/// \file

#include "mtlxBaseWriter.h"

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

/// \class MtlxUsd_SymmetricShaderWriter
/// \brief Provides "literal" translation of Maya shading nodes to USD Shader
/// prims that are MaterialX-compatible.
///
/// This shader writer performs a "literal" translation of a Maya shading node
/// type to USD. Input and output attributes on the Maya node translate
/// directly to inputs and outputs with the same names on the exported
/// UsdShadeShader. With one major exception: color and alpha are kept together
/// to solve some temporary issues about multi-output management in MaterialX/USD.
///
/// A static "RegisterWriter()" function is provided to simplify the
/// registration of writers that use this class. Note however that it should be
/// called inside a "TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)" block,
/// for example:
///
/// \code
/// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
/// {
///     MtlxUsd_SymmetricShaderWriter::RegisterWriter(
///         TfToken("checker"),
///         TfToken("MayaND_checker_color3"));
/// };
/// \endcode
///
class MtlxUsd_SymmetricShaderWriter : public MtlxUsd_BaseWriter
{
public:
    /// Register a shader writer to translate \p mayaNodeTypeName Maya nodes to
    /// USD shaders with ID \p usdShaderId.
    ///
    /// Note that this function should generally only be called inside a
    /// TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry) block.
    static void RegisterWriter(
        const TfToken& mayaNodeTypeName,
        const TfToken& usdShaderId,
        bool           fromPython = false);

    static ContextSupport CanExport(const UsdMayaJobExportArgs& exportArgs);

    MtlxUsd_SymmetricShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx,
        const TfToken&           usdShaderId);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
