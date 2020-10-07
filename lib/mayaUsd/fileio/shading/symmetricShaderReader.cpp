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
#include "symmetricShaderReader.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usd/usdShade/utils.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <string>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((MayaShaderOutputName, "outColor"))
);


/* static */
void
UsdMayaSymmetricShaderReader::RegisterReader(
        const TfToken& usdShaderId,
        const TfToken& mayaNodeTypeName,
        const TfToken& materialConversion)
{
    UsdMayaShaderReaderRegistry::Register(
        usdShaderId,
        [materialConversion](const UsdMayaJobImportArgs& importArgs) {
            return UsdMayaSymmetricShaderReader::CanImport(
                importArgs,
                materialConversion);
        },
        [mayaNodeTypeName](const UsdMayaPrimReaderArgs& readerArgs) {
            return std::make_shared<UsdMayaSymmetricShaderReader>(
                readerArgs,
                mayaNodeTypeName);
        });
}

/* static */
UsdMayaShaderReader::ContextSupport
UsdMayaSymmetricShaderReader::CanImport(
        const UsdMayaJobImportArgs& importArgs,
        const TfToken& materialConversion)
{
    if (materialConversion.IsEmpty()
        || importArgs.shadingModes.front().second == materialConversion) {
        // This shader reader advertises "Fallback" support so that any more
        // specialized readers for a particular shader ID can take precedence.
        return ContextSupport::Fallback;
    }

    return ContextSupport::Unsupported;
}

static
UsdMayaShadingNodeType
_ComputeShadingNodeTypeForMayaTypeName(const TfToken& mayaNodeTypeName)
{
    // Use NonShading as a fallback.
    UsdMayaShadingNodeType shadingNodeType =
        UsdMayaShadingNodeType::NonShading;

    MStatus status;
    MString cmd;
    status = cmd.format("getClassification ^1s", mayaNodeTypeName.GetText());
    CHECK_MSTATUS_AND_RETURN(status, shadingNodeType);

    MStringArray compoundClassifications;
    status = MGlobal::executeCommand(cmd, compoundClassifications, false, false);
    CHECK_MSTATUS_AND_RETURN(status, shadingNodeType);

    static const std::vector<std::pair<std::string, UsdMayaShadingNodeType>>
        _classificationsToTypes = {
            { "texture/", UsdMayaShadingNodeType::Texture },
            { "utility/", UsdMayaShadingNodeType::Utility },
            { "shader/", UsdMayaShadingNodeType::Shader }
        };

    // The docs for getClassification() are pretty confusing. You'd think that
    // the string array returned would give you each "classification", but
    // instead, it's a list of "single compound classification string by
    // joining the individual classifications with ':'".

    // Loop over the compoundClassifications, though I believe
    // compoundClassifications will always have size 0 or 1.
#if MAYA_API_VERSION >= 20190000
    for (const MString& compoundClassification : compoundClassifications) {
#else
    for (unsigned int i = 0u; i < compoundClassifications.length(); ++i) {
        const MString& compoundClassification = compoundClassifications[i];
#endif
        const std::string compoundClassificationStr(
            compoundClassification.asChar());
        for (const std::string& classification :
                TfStringSplit(compoundClassificationStr, ":")) {
            for (const auto& classPrefixAndType : _classificationsToTypes) {
                if (TfStringStartsWith(
                        classification,
                        classPrefixAndType.first)) {
                    return classPrefixAndType.second;
                }
            }
        }
    }

    return shadingNodeType;
}

UsdMayaSymmetricShaderReader::UsdMayaSymmetricShaderReader(
        const UsdMayaPrimReaderArgs& readerArgs,
        const TfToken& mayaNodeTypeName) :
    UsdMayaShaderReader(readerArgs),
    _mayaNodeTypeName(mayaNodeTypeName),
    _mayaShadingNodeType(
        _ComputeShadingNodeTypeForMayaTypeName(mayaNodeTypeName))
{
}

/* override */
bool
UsdMayaSymmetricShaderReader::Read(UsdMayaPrimReaderContext* context)
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    const UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    TfToken shaderId;
    if (!shaderSchema.GetShaderId(&shaderId)) {
        return false;
    }

    MStatus status;
    MObject mayaObject;
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

    context->RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        const UsdAttribute& usdAttr = input.GetAttr();
        const std::string mayaAttrName = usdAttr.GetBaseName().GetString();

        MPlug attrPlug = depNodeFn.findPlug(mayaAttrName.c_str(), &status);
        if (status != MS::kSuccess) {
            continue;
        }

        unsigned int index = 0u;
        if (UsdMayaRoundTripUtil::GetAttributeArray(usdAttr, &index)) {
            attrPlug = attrPlug.elementByLogicalIndex(index, &status);
            if (status != MS::kSuccess) {
                continue;
            }
        }

        UsdMayaUtil::setPlugValue(usdAttr, attrPlug);
    }

    return true;
}

/* override */
TfToken
UsdMayaSymmetricShaderReader::GetMayaNameForUsdAttrName(
        const TfToken& usdAttrName) const
{
    TfToken usdBaseName;
    UsdShadeAttributeType usdAttrType;
    std::tie(usdBaseName, usdAttrType) =
        UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    // The one edge case we're handling here is the connection to a "top-level"
    // shader from one of its Material prim's terminal outputs. We make an
    // assumption about the name of the Maya shading node's primary output
    // attribute.
    if (usdAttrType == UsdShadeAttributeType::Output &&
            (usdBaseName == UsdShadeTokens->surface ||
             usdBaseName == UsdShadeTokens->displacement ||
             usdBaseName == UsdShadeTokens->volume)) {
        return _tokens->MayaShaderOutputName;
    }

    // Otherwise, assume there's a Maya attribute with the same name as the USD
    // attribute.
    return usdBaseName;
}


PXR_NAMESPACE_CLOSE_SCOPE
