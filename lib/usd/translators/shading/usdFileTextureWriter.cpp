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
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
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

    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override;

    void    WriteTransform2dNode(const UsdTimeCode& usdTime, const UsdShadeShader& texShaderSchema);
    SdfPath getPlace2DTexturePath(const MFnDependencyNode& depNodeFn);
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, PxrUsdTranslators_FileTextureWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // UsdPrimvarReader_float2 Prim Name
    ((PrimvarReaderShaderName, "shared_TexCoordReader"))

    // Usd2dTransform Prim Name
    ((UsdTransform2dShaderName, "UsdTransform2d"))

);
// clang-format on

UsdMayaShaderWriter::ContextSupport
PxrUsdTranslators_FileTextureWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    if (exportArgs.convertMaterialsTo == UsdImagingTokens->UsdPreviewSurface) {
        return ContextSupport::Supported;
    }
    // Only report as fallback if UsdPreviewSurface was not explicitly requested:
    if (exportArgs.allMaterialConversions.count(UsdImagingTokens->UsdPreviewSurface) == 0) {
        return ContextSupport::Fallback;
    }
    return ContextSupport::Unsupported;
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
    const SdfPath primvarReaderShaderPath = getPlace2DTexturePath(depNodeFn);

    if (!GetUsdStage()->GetPrimAtPath(primvarReaderShaderPath)) {
        UsdShadeShader primvarReaderShaderSchema
            = UsdShadeShader::Define(GetUsdStage(), primvarReaderShaderPath);

        primvarReaderShaderSchema.CreateIdAttr(VtValue(TrUsdTokens->UsdPrimvarReader_float2));

        UsdShadeInput varnameInput
            = primvarReaderShaderSchema.CreateInput(TrUsdTokens->varname, SdfValueTypeNames->Token);

        TfToken inputName(
            TfStringPrintf("%s:%s", depNodeFn.name().asChar(), TrUsdTokens->varname.GetText()));

        // We expose the primvar reader varname attribute to the material to allow
        // easy specialization based on UV mappings to geometries:
        UsdPrim          materialPrim = primvarReaderShaderSchema.GetPrim().GetParent();
        UsdShadeMaterial materialSchema(materialPrim);
        while (!materialSchema && materialPrim) {
            UsdShadeNodeGraph intermediateNodeGraph(materialPrim);
            if (intermediateNodeGraph) {
                UsdShadeInput intermediateInput
                    = intermediateNodeGraph.CreateInput(inputName, SdfValueTypeNames->Token);
                varnameInput.ConnectToSource(intermediateInput);
                varnameInput = intermediateInput;
            }

            materialPrim = materialPrim.GetParent();
            materialSchema = UsdShadeMaterial(materialPrim);
        }

        if (materialSchema) {
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

        UsdShadeOutput primvarReaderOutput = primvarReaderShaderSchema.CreateOutput(
            TrUsdTokens->result, SdfValueTypeNames->Float2);

        // Connect the output of the primvar reader to the texture coordinate
        // input of the UV texture.
        texShaderSchema.CreateInput(TrUsdTokens->st, SdfValueTypeNames->Float2)
            .ConnectToSource(primvarReaderOutput);
    } else {
        // Re-using an existing primvar reader:
        UsdShadeShader primvarReaderShaderSchema(
            GetUsdStage()->GetPrimAtPath(primvarReaderShaderPath));
        UsdShadeOutput primvarReaderOutput
            = primvarReaderShaderSchema.GetOutput(TrUsdTokens->result);

        // Connect the output of the primvar reader to the texture coordinate
        // input of the UV texture.
        texShaderSchema.CreateInput(TrUsdTokens->st, SdfValueTypeNames->Float2)
            .ConnectToSource(primvarReaderOutput);
    }
}

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

    const MPlug tilingAttr
        = depNodeFn.findPlug(TrMayaTokens->uvTilingMode.GetText(), true, &status);
    const bool isUDIM = (status == MS::kSuccess && tilingAttr.asInt() == 3);

    // We use the ExportArgs fileName here instead of the USD root layer path
    // to make sure that we are basing logic of the final export location
    UsdMayaShadingUtil::ResolveUsdTextureFileName(
        fileTextureName, _GetExportArgs().GetResolvedFileName(), isUDIM);

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

        // Set the sourceColorSpace as well. The color space metadata will not be transmitted via
        // Hydra, so we need to set this attribute as well if we want hdStorm and the VP2 render
        // delegate to look correct
        if (colorSpace == TrMayaTokens->Raw.GetText()) {
            shaderSchema.CreateInput(TrUsdTokens->sourceColorSpace, SdfValueTypeNames->Token)
                .Set(TrUsdTokens->raw);
        } else if (colorSpace == TrMayaTokens->sRGB.GetText()) {
            shaderSchema.CreateInput(TrUsdTokens->sourceColorSpace, SdfValueTypeNames->Token)
                .Set(TrUsdTokens->sRGB);
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
    const SdfPath        primvarReaderShaderPath = getPlace2DTexturePath(depNodeFn);
    const UsdShadeShader primvarReaderShader
        = texShaderSchema.Get(GetUsdStage(), primvarReaderShaderPath);

    const UsdShadeOutput primvarReaderShaderOutput
        = primvarReaderShader.GetOutput(TrUsdTokens->result);

    // We have two cases. If the node is connected to a place2DTransform, then the transform data
    // was on the placement node. If not, then the transform data was on the file node.
    std::string usdUvTransformName;
    if (primvarReaderShaderPath.GetName() == _tokens->PrimvarReaderShaderName.GetString()) {
        usdUvTransformName = TfStringPrintf(
            "%s_%s", depNodeFn.name().asChar(), _tokens->UsdTransform2dShaderName.GetText());

    } else {
        usdUvTransformName = TfStringPrintf(
            "%s_%s",
            primvarReaderShaderPath.GetName().c_str(),
            _tokens->UsdTransform2dShaderName.GetText());
    }

    const SdfPath transform2dShaderPath = texShaderSchema.GetPath().GetParentPath().AppendChild(
        TfToken(usdUvTransformName.c_str()));

    if (!GetUsdStage()->GetPrimAtPath(transform2dShaderPath)) {
        // Create the Transform2d node as a child of the UsdUVTexture node
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
                "At least one of the components of RepeatUV for %s are set to zero.  To avoid "
                "divide "
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

        // Get and connect the file texture input "st" to the Transform2d  output "result"
        UsdShadeInput texShaderSchemaInput = texShaderSchema.GetInput(TrUsdTokens->st);
        texShaderSchemaInput.ConnectToSource(transform2dOutput);
    } else {
        // Re-using an existing transform node:
        UsdShadeShader transform2dShaderSchema(GetUsdStage()->GetPrimAtPath(transform2dShaderPath));
        UsdShadeOutput transform2dOutput = transform2dShaderSchema.GetOutput(TrUsdTokens->result);
        // Get and connect the file texture input "st" to the Transform2d  output "result"
        UsdShadeInput texShaderSchemaInput = texShaderSchema.GetInput(TrUsdTokens->st);
        texShaderSchemaInput.ConnectToSource(transform2dOutput);
    }
}

SdfPath
PxrUsdTranslators_FileTextureWriter::getPlace2DTexturePath(const MFnDependencyNode& depNodeFn)
{
    MStatus     status;
    std::string usdUvTextureName;
    const MPlug plug = depNodeFn.findPlug(
        TrMayaTokens->uvCoord.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess && plug.isDestination(&status)) {
        MPlug source = plug.source(&status);
        if (status == MS::kSuccess && !source.isNull()) {
            MFnDependencyNode sourceNode(source.node());
            usdUvTextureName = sourceNode.name().asChar();
        }
    }

    if (usdUvTextureName.empty()) {
        // We want a single UV reader for all file nodes not connected to a place2DTexture node
        usdUvTextureName = _tokens->PrimvarReaderShaderName.GetString();
    }

    return GetUsdPath().GetParentPath().AppendChild(TfToken(usdUvTextureName.c_str()));
}

/* virtual */
UsdAttribute PxrUsdTranslators_FileTextureWriter::GetShadingAttributeForMayaAttrName(
    const TfToken&          mayaAttrName,
    const SdfValueTypeName& typeName)
{
    UsdAttribute     usdAttr;
    TfToken          usdAttrName;
    SdfValueTypeName usdTypeName = SdfValueTypeNames->Float;

    if (mayaAttrName == TrMayaTokens->outColor) {
        if (typeName == SdfValueTypeNames->Color3f || typeName == SdfValueTypeNames->Normal3f) {
            usdAttrName = TrUsdTokens->RGBOutputName;
            usdTypeName = SdfValueTypeNames->Float3;
        } else {
            // Float input detected. Happens when connecting outColor to opacity and requires an
            // alpha channel or a monochrome texture
            const MFnDependencyNode depNodeFn(GetMayaObject());
            const MPlug             fileTextureNamePlug = depNodeFn.findPlug(
                TrMayaTokens->fileTextureName.GetText(),
                /* wantNetworkedPlug = */ true);
            std::string fileTextureName(fileTextureNamePlug.asString().asChar());

            UsdMayaShadingUtil::ResolveUsdTextureFileName(
                fileTextureName, _GetExportArgs().GetResolvedFileName(), false);
            int numChannels = UsdMayaShadingUtil::GetNumberOfChannels(fileTextureName);
            if (numChannels == 1) {
                usdAttrName = TrUsdTokens->RedOutputName;
            } else if (numChannels == 2) {
                // Mono texture with alpha channel. Corner case. Should it connect to the second
                // channel G or should it always connect on the A channel?
                usdAttrName = TrUsdTokens->AlphaOutputName;
            } else if (numChannels == 4) {
                usdAttrName = TrUsdTokens->AlphaOutputName;
            } else {
                // Impossible to read the user's mind here. Use the red channel by default.
                usdAttrName = TrUsdTokens->RedOutputName;
                TF_WARN(
                    "Arbitrarily connecting the red channel of %s on %s might result in unexpected "
                    "opacity results. Try a monochrome texture, a texture with an alpha channel, "
                    "or explicit connections.",
                    fileTextureName.c_str(),
                    depNodeFn.name().asChar());
            }
        }
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
            return usdAttr;
        }

        usdAttr = shaderSchema.CreateOutput(usdAttrName, usdTypeName);
    }

    return usdAttr;
}

PXR_NAMESPACE_CLOSE_SCOPE
