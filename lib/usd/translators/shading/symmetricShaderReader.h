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
#ifndef USDTRANSLATORS_SHADING_SYMMETRIC_SHADER_READER_H
#define USDTRANSLATORS_SHADING_SYMMETRIC_SHADER_READER_H

/// \file

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class PxrUsdTranslators_SymmetricShaderReader
/// \brief Provides "literal" translation of USD Shader prims to Maya shading
/// nodes.
///
/// This shader reader performs a "literal" translation of a USD Shader ID to
/// Maya shading nodes of a particular type. Values and connections on inputs
/// and outputs of the Shader prim are translated directly to attributes with
/// the same names on the Maya node.
///
/// A static "RegisterReader()" function is provided to simplify the
/// registration of readers that use this class. Note however that it should be
/// called inside a "TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)" block,
/// for example:
///
/// \code
/// TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
/// {
///     PxrUsdTranslators_SymmetricShaderReader::RegisterReader(
///         TfToken("MyUsdShaderId"),
///         TfToken("myMayaNodeTypeName"),
///         /* shadingConversionName = */ TfToken("myShadingConversionName"));
/// };
/// \endcode
///
class PxrUsdTranslators_SymmetricShaderReader : public UsdMayaShaderReader
{
public:
    /// Register a shader reader to translate USD shaders with ID
    /// \p usdShaderId into Maya nodes of type \p mayaNodeTypeName.
    ///
    /// The shader reader can optionally be restricted to a particular shading
    /// conversion. If no shading conversion name is supplied, the reader's
    /// "CanImport()" function will always return "Fallback". If a shading
    /// conversion name is supplied, "Fallback" is returned if the shading
    /// conversion name matches the one specified in the import args, and
    /// "Unsupported" is returned otherwise.
    ///
    /// Note that this function should generally only be called inside a
    /// TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry) block.
    static void RegisterReader(
            const TfToken& usdShaderId,
            const TfToken& mayaNodeTypeName,
            const TfToken& shadingConversionName = TfToken());

    static ContextSupport CanImport(
            const UsdMayaJobImportArgs& importArgs,
            const TfToken& shadingConversionName = TfToken());

    PxrUsdTranslators_SymmetricShaderReader(
            const UsdMayaPrimReaderArgs& readerArgs,
            const TfToken& mayaNodeTypeName);

    bool Read(UsdMayaPrimReaderContext* context) override;

    TfToken GetMayaNameForUsdAttrName(
            const TfToken& usdAttrName) const override;

private:
    const TfToken _mayaNodeTypeName;
    const UsdMayaShadingNodeType _mayaShadingNodeType;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
