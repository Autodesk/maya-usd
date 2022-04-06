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
#include "mtlxBaseReader.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/utils.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class MtlxUsd_SymmetricShaderReader
/// \brief Provides "literal" translation of USD MaterialX Shader prims to Maya
/// shading nodes.
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
///     MtlxUsd_SymmetricShaderReader::RegisterReader(
///         TfToken("MayaND_checker_color3"),
///         TfToken("checker"),
/// };
/// \endcode
///
class MtlxUsd_SymmetricShaderReader : public MtlxUsd_BaseReader
{
public:
    /// Register a shader reader to translate USD MaterialX shaders with ID
    /// \p usdShaderId into Maya nodes of type \p mayaNodeTypeName.
    ///
    /// Note that this function should generally only be called inside a
    /// TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry) block.
    MAYAUSD_CORE_PUBLIC
    static void RegisterReader(
        const TfToken& usdShaderId,
        const TfToken& mayaNodeTypeName,
        bool           fromPython = false);

    MAYAUSD_CORE_PUBLIC
    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    MAYAUSD_CORE_PUBLIC
    MtlxUsd_SymmetricShaderReader(
        const UsdMayaPrimReaderArgs& readerArgs,
        const TfToken&               mayaNodeTypeName);

    MAYAUSD_CORE_PUBLIC
    bool Read(UsdMayaPrimReaderContext& context) override;

    MAYAUSD_CORE_PUBLIC
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

private:
    const TfToken                _mayaNodeTypeName;
    const UsdMayaShadingNodeType _mayaShadingNodeType;
};

/* static */
void MtlxUsd_SymmetricShaderReader::RegisterReader(
    const TfToken& usdShaderId,
    const TfToken& mayaNodeTypeName,
    bool           fromPython)
{
    UsdMayaShaderReaderRegistry::Register(
        usdShaderId,
        [](const UsdMayaJobImportArgs& importArgs) {
            return MtlxUsd_SymmetricShaderReader::CanImport(importArgs);
        },
        [mayaNodeTypeName](const UsdMayaPrimReaderArgs& readerArgs) {
            return std::make_shared<MtlxUsd_SymmetricShaderReader>(readerArgs, mayaNodeTypeName);
        },
        fromPython);
}

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    // These will have to be moved to a MaterialX aware version of the symmetric shader reader.
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_lambert_surfaceshader, TrMayaTokens->lambert);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_phong_surfaceshader, TrMayaTokens->phong);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_blinn_surfaceshader, TrMayaTokens->blinn);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_place2dTexture_vector2, TrMayaTokens->place2dTexture);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->LdkND_FloatCorrect_float, TrMayaTokens->floatCorrect);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->LdkND_ColorCorrect_color4, TrMayaTokens->colorCorrect);
    MtlxUsd_SymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_clamp_vector3, TrMayaTokens->clamp);
};

/* static */
UsdMayaShaderReader::ContextSupport
MtlxUsd_SymmetricShaderReader::CanImport(const UsdMayaJobImportArgs& importArgs)
{
    if (importArgs.GetMaterialConversion() == TrMtlxTokens->conversionName) {
        return ContextSupport::Supported;
    }

    return ContextSupport::Unsupported;
}

MtlxUsd_SymmetricShaderReader::MtlxUsd_SymmetricShaderReader(
    const UsdMayaPrimReaderArgs& readerArgs,
    const TfToken&               mayaNodeTypeName)
    : MtlxUsd_BaseReader(readerArgs)
    , _mayaNodeTypeName(mayaNodeTypeName)
    , _mayaShadingNodeType(
          UsdMayaTranslatorUtil::ComputeShadingNodeTypeForMayaTypeName(mayaNodeTypeName))
{
}

/* override */
bool MtlxUsd_SymmetricShaderReader::Read(UsdMayaPrimReaderContext& context)
{
    const UsdPrim&       prim = _GetArgs().GetUsdPrim();
    const UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    TfToken shaderId;
    if (!shaderSchema.GetShaderId(&shaderId)) {
        return false;
    }

    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depNodeFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              _mayaNodeTypeName.GetText(),
              _mayaShadingNodeType,
              &status,
              &mayaObject)
          && depNodeFn.setObject(mayaObject))) {
        TF_RUNTIME_ERROR(
            "Could not create node of type %s for shader '%s'. "
            "Probably missing a loadPlugin.\n",
            _mayaNodeTypeName.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);
    RegisterConstructorNodes(context, mayaObject);

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        const UsdAttribute& usdAttr = input.GetAttr();
        const std::string   mayaAttrName = usdAttr.GetBaseName().GetString();

        MPlug attrPlug = depNodeFn.findPlug(mayaAttrName.c_str(), &status);
        if (status != MS::kSuccess) {
            continue;
        }

        UsdMayaUtil::setPlugValue(usdAttr, attrPlug);
    }

    return true;
}

/* override */
TfToken MtlxUsd_SymmetricShaderReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdBaseName;
    UsdShadeAttributeType usdAttrType;
    std::tie(usdBaseName, usdAttrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    // The one edge case we're handling here is the connection to a "top-level"
    // shader from one of its Material prim's terminal outputs. We make an
    // assumption about the name of the Maya shading node's primary output
    // attribute.
    if (usdAttrType == UsdShadeAttributeType::Output
        && (usdBaseName == UsdShadeTokens->surface || usdBaseName == UsdShadeTokens->displacement
            || usdBaseName == UsdShadeTokens->volume)) {
        return TrMayaTokens->outColor;
    }

    // Otherwise, assume there's a Maya attribute with the same name as the USD
    // attribute.
    return usdBaseName;
}

PXR_NAMESPACE_CLOSE_SCOPE
