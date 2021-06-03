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
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUtils/pipeline.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <ghc/filesystem.hpp>

#include <regex>
#include <system_error>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_FileTextureWriter : public UsdMayaShaderWriter
{
public:
    PxrUsdTranslators_FileTextureWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    static ContextSupport CanExport(const UsdMayaJobExportArgs&);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

    void WriteTransform2dNode(const UsdTimeCode& usdTime, const UsdShadeShader& texShaderSchema);
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, PxrUsdTranslators_FileTextureWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // UsdPrimvarReader_float2 Prim Name
    ((PrimvarReaderShaderName, "TexCoordReader"))

    // Usd2dTransform Prim Name
    ((UsdTransform2dShaderName, "UsdTransform2d"))

);
// clang-format on

UsdMayaShaderWriter::ContextSupport
PxrUsdTranslators_FileTextureWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == UsdImagingTokens->UsdPreviewSurface
        ? ContextSupport::Supported
        : ContextSupport::Fallback;
}

PxrUsdTranslators_FileTextureWriter::PxrUsdTranslators_FileTextureWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    // Create a UsdUVTexture shader as the "primary" shader for this writer.
    UsdShadeShader texShaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            texShaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    texShaderSchema.CreateIdAttr(VtValue(TrUsdTokens->UsdUVTexture));

    _usdPrim = texShaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            texShaderSchema.GetPath().GetText())) {
        return;
    }

    // Now create a UsdPrimvarReader shader that the UsdUvTexture shader will
    // use.
    const SdfPath primvarReaderShaderPath
        = texShaderSchema.GetPath().AppendChild(_tokens->PrimvarReaderShaderName);
    UsdShadeShader primvarReaderShaderSchema
        = UsdShadeShader::Define(GetUsdStage(), primvarReaderShaderPath);

    primvarReaderShaderSchema.CreateIdAttr(VtValue(TrUsdTokens->UsdPrimvarReader_float2));

    UsdShadeInput varnameInput
        = primvarReaderShaderSchema.CreateInput(TrUsdTokens->varname, SdfValueTypeNames->Token);

    // We expose the primvar reader varname attribute to the material to allow
    // easy specialization based on UV mappings to geometries:
    SdfPath          materialPath = GetUsdPath().GetParentPath();
    UsdShadeMaterial materialSchema(GetUsdStage()->GetPrimAtPath(materialPath));
    while (!materialSchema && !materialPath.IsEmpty()) {
        materialPath = materialPath.GetParentPath();
        materialSchema = UsdShadeMaterial(GetUsdStage()->GetPrimAtPath(materialPath));
    }

    if (materialSchema) {
        TfToken inputName(
            TfStringPrintf("%s:%s", depNodeFn.name().asChar(), TrUsdTokens->varname.GetText()));
        UsdShadeInput materialInput
            = materialSchema.CreateInput(inputName, SdfValueTypeNames->Token);
        materialInput.Set(UsdUtilsGetPrimaryUVSetName());
        varnameInput.ConnectToSource(materialInput);
        // Note: This needs to be done for all nodes that require UV input. In
        // the UsdPreviewSurface case, the file node is the only one, but for
        // other Maya nodes like cloth, checker, mandelbrot, we will also need
        // to resolve the UV channels. This means traversing UV inputs until we
        // find the unconnected one that implicitly connects to uvSet[0] of the
        // geometry, or an explicit uvChooser node connecting to alternate uvSets.
    } else {
        varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
    }

    UsdShadeOutput primvarReaderOutput
        = primvarReaderShaderSchema.CreateOutput(TrUsdTokens->result, SdfValueTypeNames->Float2);

    // Connect the output of the primvar reader to the texture coordinate
    // input of the UV texture.
    texShaderSchema.CreateInput(TrUsdTokens->st, SdfValueTypeNames->Float2)
        .ConnectToSource(primvarReaderOutput);
}

namespace {
// Match UDIM pattern, from 1001 to 1999
const std::regex
    _udimRegex(".*[^\\d](1(?:[0-9][0-9][1-9]|[1-9][1-9]0|0[1-9]0|[1-9]00))(?:[^\\d].*|$)");
} // namespace

/* virtual */
void PxrUsdTranslators_FileTextureWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    // File
    const MPlug fileTextureNamePlug = depNodeFn.findPlug(
        TrMayaTokens->fileTextureName.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    std::string fileTextureName(fileTextureNamePlug.asString(&status).asChar());
    if (status != MS::kSuccess) {
        return;
    }

    // WARNING: This extremely minimal attempt at making the file path relative
    //          to the USD stage is a stopgap measure intended to provide
    //          minimal interop. It will be replaced by proper use of Maya and
    //          USD asset resolvers. For package files, the exporter needs full
    //          paths.

    // We use the ExportArgs fileName here instead of the USD root layer path
    // to make sure that we are basing logic of the final export location
    const std::string fileName = _GetExportArgs().GetResolvedFileName();
    TfToken           fileExt(TfGetExtension(fileName));
    if (fileExt != UsdMayaTranslatorTokens->UsdFileExtensionPackage) {
        ghc::filesystem::path usdDir(fileName);
        usdDir = usdDir.parent_path();
        std::error_code       ec;
        ghc::filesystem::path relativePath = ghc::filesystem::relative(fileTextureName, usdDir, ec);
        if (!ec && !relativePath.empty()) {
            fileTextureName = relativePath.generic_string();
        }
    }

    // Update filename in case of UDIM
    const MPlug tilingAttr
        = depNodeFn.findPlug(TrMayaTokens->uvTilingMode.GetText(), true, &status);
    if (status == MS::kSuccess && tilingAttr.asInt() == 3) {
        std::smatch match;
        if (std::regex_search(fileTextureName, match, _udimRegex) && match.size() == 2) {
            fileTextureName = std::string(match[0].first, match[1].first)
                + TrMayaTokens->UDIMTag.GetString() + std::string(match[1].second, match[0].second);
        }
    }

    UsdShadeInput fileInput = shaderSchema.CreateInput(TrUsdTokens->file, SdfValueTypeNames->Asset);
    fileInput.Set(SdfAssetPath(fileTextureName.c_str()), usdTime);

    MPlug colorSpacePlug = depNodeFn.findPlug(TrMayaTokens->colorSpace.GetText(), true, &status);
    if (status == MS::kSuccess) {
        MString colorRuleCmd;
        colorRuleCmd.format(
            "colorManagementFileRules -evaluate \"^1s\";", fileTextureNamePlug.asString());
        const MString colorSpaceByRule(MGlobal::executeCommandStringResult(colorRuleCmd));
        const MString colorSpace(colorSpacePlug.asString(&status));
        if (status == MS::kSuccess && colorSpace != colorSpaceByRule) {
            fileInput.GetAttr().SetColorSpace(TfToken(colorSpace.asChar()));
        }
    }

    // The Maya file node's 'colorGain' and 'alphaGain' attributes map to the
    // UsdUVTexture's scale input.
    bool    isScaleAuthored = false;
    GfVec4f scale(1.0f, 1.0f, 1.0f, 1.0f);

    // Color Gain
    const MPlug colorGainPlug = depNodeFn.findPlug(
        TrMayaTokens->colorGain.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(colorGainPlug)) {
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            scale[i] = colorGainPlug.child(i).asFloat(&status);
            if (status != MS::kSuccess) {
                return;
            }
        }

        isScaleAuthored = true;
    }

    // Alpha Gain
    const MPlug alphaGainPlug = depNodeFn.findPlug(
        TrMayaTokens->alphaGain.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(alphaGainPlug)) {
        scale[3u] = alphaGainPlug.asFloat(&status);
        if (status != MS::kSuccess) {
            return;
        }

        isScaleAuthored = true;
    }

    if (isScaleAuthored) {
        shaderSchema.CreateInput(TrUsdTokens->scale, SdfValueTypeNames->Float4).Set(scale, usdTime);
    }

    // The Maya file node's 'colorOffset' and 'alphaOffset' attributes map to
    // the UsdUVTexture's bias input.
    bool    isBiasAuthored = false;
    GfVec4f bias(0.0f, 0.0f, 0.0f, 0.0f);

    // Color Offset
    const MPlug colorOffsetPlug = depNodeFn.findPlug(
        TrMayaTokens->colorOffset.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(colorOffsetPlug)) {
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            bias[i] = colorOffsetPlug.child(i).asFloat(&status);
            if (status != MS::kSuccess) {
                return;
            }
        }

        isBiasAuthored = true;
    }

    // Alpha Offset
    const MPlug alphaOffsetPlug = depNodeFn.findPlug(
        TrMayaTokens->alphaOffset.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(alphaOffsetPlug)) {
        bias[3u] = alphaOffsetPlug.asFloat(&status);
        if (status != MS::kSuccess) {
            return;
        }

        isBiasAuthored = true;
    }

    if (isBiasAuthored) {
        shaderSchema.CreateInput(TrUsdTokens->bias, SdfValueTypeNames->Float4).Set(bias, usdTime);
    }

    // Default Color
    const MPlug defaultColorPlug = depNodeFn.findPlug(
        TrMayaTokens->defaultColor.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    // The defaultColor plug does not include an alpha, so only look for
    // three components, even though we're putting the values in a GfVec4f.
    // We also don't check whether it is authored in Maya, since Maya's
    // unauthored value (0.5, 0.5, 0.5) differs from UsdUVTexture's fallback
    // value.
    GfVec4f fallback(0.0f, 0.0f, 0.0f, 1.0f);
    for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
        fallback[i] = defaultColorPlug.child(i).asFloat(&status);
        if (status != MS::kSuccess) {
            return;
        }
    }

    shaderSchema.CreateInput(TrUsdTokens->fallback, SdfValueTypeNames->Float4)
        .Set(fallback, usdTime);

    // Wrap U/V
    const TfToken wrapMirrorTriples[2][3] {
        { TrMayaTokens->wrapU, TrMayaTokens->mirrorU, TrUsdTokens->wrapS },
        { TrMayaTokens->wrapV, TrMayaTokens->mirrorV, TrUsdTokens->wrapT }
    };
    for (auto wrapMirrorTriple : wrapMirrorTriples) {
        auto wrapUVToken = wrapMirrorTriple[0];
        auto mirrorUVToken = wrapMirrorTriple[1];
        auto wrapSTToken = wrapMirrorTriple[2];

        const MPlug wrapUVPlug = depNodeFn.findPlug(
            wrapUVToken.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
        if (status != MS::kSuccess) {
            return;
        }

        // Don't check if authored, because maya's default is effectively wrapS/wrapT,
        // while USD's fallback is "useMetadata", which might be different
        const bool wrapVal = wrapUVPlug.asBool(&status);
        if (status != MS::kSuccess) {
            return;
        }

        TfToken outputValue;
        if (!wrapVal) {
            outputValue = TrUsdTokens->black;
        } else {
            const MPlug mirrorUVPlug = depNodeFn.findPlug(
                mirrorUVToken.GetText(),
                /* wantNetworkedPlug = */ true,
                &status);
            if (status != MS::kSuccess) {
                return;
            }

            const bool mirrorVal = mirrorUVPlug.asBool(&status);
            if (status != MS::kSuccess) {
                return;
            }
            outputValue = mirrorVal ? TrUsdTokens->mirror : TrUsdTokens->repeat;
        }
        shaderSchema.CreateInput(wrapSTToken, SdfValueTypeNames->Token).Set(outputValue, usdTime);
    }

    WriteTransform2dNode(usdTime, shaderSchema);
}

void PxrUsdTranslators_FileTextureWriter::WriteTransform2dNode(
    const UsdTimeCode&    usdTime,
    const UsdShadeShader& texShaderSchema)
{
    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    // Gather UV transform data.  If it differs from default values,
    // create a Transform2d Node, connect it to the output "result"
    // of the TexCoordReader node and the input "st" of the FileTexture node.
    const GfVec2f translationFallbackValue(0.0f, 0.0f);
    const float   rotationFallbackValue = 0.0f;
    const GfVec2f scaleFallbackValue(1.0f, 1.0f);

    GfVec2f translationValue = translationFallbackValue;
    float   rotationValue = rotationFallbackValue;
    GfVec2f scaleValue = scaleFallbackValue;

    bool transformationsAreAuthored = false;

    // Translation
    const MPlug offsetPlug = depNodeFn.findPlug(
        TrMayaTokens->offset.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(offsetPlug)) {
        for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
            translationValue[i] = offsetPlug.child(i).asFloat(&status);
            if (status != MS::kSuccess) {
                return;
            }
        }

        if (translationValue != translationFallbackValue) {
            transformationsAreAuthored = true;
        }
    }

    // Rotation
    const MPlug rotateUVPlug = depNodeFn.findPlug(
        TrMayaTokens->rotateUV.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(rotateUVPlug)) {
        rotationValue = rotateUVPlug.asFloat(&status);
        if (status != MS::kSuccess) {
            return;
        }

        if (rotationValue != rotationFallbackValue) {
            transformationsAreAuthored |= true;
        }

        // Convert rotation from radians to degrees
        rotationValue = GfRadiansToDegrees(rotationValue);
    }

    // Scale
    const MPlug repeatUVPlug = depNodeFn.findPlug(
        TrMayaTokens->repeatUV.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(offsetPlug)) {
        for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
            scaleValue[i] = repeatUVPlug.child(i).asFloat(&status);
            if (status != MS::kSuccess) {
                return;
            }
        }

        if (scaleValue != scaleFallbackValue) {
            transformationsAreAuthored |= true;
        }
    }

    if (!transformationsAreAuthored) {
        return;
    }

    // Get the TexCoordReader node and its output "result"
    const SdfPath primvarReaderShaderPath
        = texShaderSchema.GetPath().AppendChild(_tokens->PrimvarReaderShaderName);

    const UsdShadeShader primvarReaderShader
        = texShaderSchema.Get(GetUsdStage(), primvarReaderShaderPath);

    const UsdShadeOutput primvarReaderShaderOutput
        = primvarReaderShader.GetOutput(TrUsdTokens->result);

    // Create the Transform2d node as a child of the UsdUVTexture node
    const SdfPath transform2dShaderPath
        = texShaderSchema.GetPath().AppendChild(_tokens->UsdTransform2dShaderName);
    UsdShadeShader transform2dShaderSchema
        = UsdShadeShader::Define(GetUsdStage(), transform2dShaderPath);

    transform2dShaderSchema.CreateIdAttr(VtValue(TrUsdTokens->UsdTransform2d));

    // Create the Transform2d input "in" attribute and connect it
    // to the TexCoordReader output "result"
    transform2dShaderSchema.CreateInput(TrUsdTokens->in, SdfValueTypeNames->Float2)
        .ConnectToSource(primvarReaderShaderOutput);

    // Compute the Transform2d values, converting from Maya's coordinates to USD coordinates

    // Maya's place2dtexture transform order seems to be `in * T * S * R`, where the rotation
    // pivot is (0.5, 0.5) and scale pivot is (0,0). USD's Transform2d transform order is `in *
    // S * R * T`, where the rotation and scale pivots are (0,0). This conversion translates
    // from place2dtexture's UV space to Transform2d's UV space: `in * S * T * Rpivot_inverse *
    // R * Rpivot`
    GfMatrix4f pivotXform = GfMatrix4f().SetTranslate(GfVec3f(0.5, 0.5, 0));
    GfMatrix4f translateXform
        = GfMatrix4f().SetTranslate(GfVec3f(translationValue[0], translationValue[1], 0));
    GfRotation rotation = GfRotation(GfVec3f::ZAxis(), rotationValue);
    GfMatrix4f rotationXform = GfMatrix4f().SetRotate(rotation);
    GfVec3f    scale;
    if (fabs(scaleValue[0]) <= std::numeric_limits<float>::epsilon()
        || fabs(scaleValue[1]) <= std::numeric_limits<float>::epsilon()) {
        TF_WARN(
            "At least one of the components of RepeatUV for %s are set to zero.  To avoid divide "
            "by zero exceptions, these values are changed to the smallest finite float greater "
            "than zero.",
            UsdMayaUtil::GetMayaNodeName(GetMayaObject()).c_str());

        scale = GfVec3f(
            1.0 / std::max(scaleValue[0], std::numeric_limits<float>::min()),
            1.0 / std::max(scaleValue[1], std::numeric_limits<float>::min()),
            1.0);
    } else {
        scale = GfVec3f(1.0 / scaleValue[0], 1.0 / scaleValue[1], 1.0);
    }

    GfMatrix4f scaleXform = GfMatrix4f().SetScale(scale);

    GfMatrix4f transform
        = scaleXform * translateXform * pivotXform.GetInverse() * rotationXform * pivotXform;
    GfVec3f translationResult = transform.ExtractTranslation();
    translationValue.Set(translationResult[0], translationResult[1]);

    // Create and set the Transform2d input attributes
    transform2dShaderSchema.CreateInput(TrUsdTokens->translation, SdfValueTypeNames->Float2)
        .Set(translationValue);

    transform2dShaderSchema.CreateInput(TrUsdTokens->rotation, SdfValueTypeNames->Float)
        .Set(rotationValue);

    transform2dShaderSchema.CreateInput(TrUsdTokens->scale, SdfValueTypeNames->Float2)
        .Set(scaleValue);

    // Create the Transform2d output "result" attribute
    UsdShadeOutput transform2dOutput
        = transform2dShaderSchema.CreateOutput(TrUsdTokens->result, SdfValueTypeNames->Float2);

    // Get and connect the TexCoordReader input "st" to the Transform2d
    // output "result"
    UsdShadeInput texShaderSchemaInput = texShaderSchema.GetInput(TrUsdTokens->st);
    texShaderSchemaInput.ConnectToSource(transform2dOutput);
}

/* virtual */
TfToken PxrUsdTranslators_FileTextureWriter::GetShadingAttributeNameForMayaAttrName(
    const TfToken& mayaAttrName)
{
    TfToken          usdAttrName;
    SdfValueTypeName usdTypeName = SdfValueTypeNames->Float;

    if (mayaAttrName == TrMayaTokens->outColor) {
        usdAttrName = TrUsdTokens->RGBOutputName;
        usdTypeName = SdfValueTypeNames->Float3;
    } else if (mayaAttrName == TrMayaTokens->outColorR) {
        usdAttrName = TrUsdTokens->RedOutputName;
    } else if (mayaAttrName == TrMayaTokens->outColorG) {
        usdAttrName = TrUsdTokens->GreenOutputName;
    } else if (mayaAttrName == TrMayaTokens->outColorB) {
        usdAttrName = TrUsdTokens->BlueOutputName;
    } else if (
        mayaAttrName == TrMayaTokens->outAlpha || mayaAttrName == TrMayaTokens->outTransparency
        || mayaAttrName == TrMayaTokens->outTransparencyR
        || mayaAttrName == TrMayaTokens->outTransparencyG
        || mayaAttrName == TrMayaTokens->outTransparencyB) {
        usdAttrName = TrUsdTokens->AlphaOutputName;
    }

    if (!usdAttrName.IsEmpty()) {
        UsdShadeShader shaderSchema(_usdPrim);
        if (!shaderSchema) {
            return TfToken();
        }

        shaderSchema.CreateOutput(usdAttrName, usdTypeName);

        usdAttrName = UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Output);
    }

    return usdAttrName;
}

PXR_NAMESPACE_CLOSE_SCOPE
