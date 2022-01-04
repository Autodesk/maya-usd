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
#include "mtlxBaseWriter.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

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
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <ghc/filesystem.hpp>

#include <regex>

PXR_NAMESPACE_OPEN_SCOPE

class MtlxUsd_FileWriter : public MtlxUsd_BaseWriter
{
public:
    MtlxUsd_FileWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override;

private:
    SdfPath _GetPlace2DTexturePath(const MFnDependencyNode& depNodeFn);

    int _numChannels = 4;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, MtlxUsd_FileWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Shared primvar writer (when no place2dTexture found):
    ((PrimvarReaderShaderName, "shared_MayaGeomPropValue"))
);
// clang-format on

MtlxUsd_FileWriter::MtlxUsd_FileWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    // Everything must be added in the material node graph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not get UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    SdfPath nodegraphPath = nodegraphSchema.GetPath();
    SdfPath texPath
        = nodegraphPath.AppendChild(TfToken(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar())));

    // Create a image shader as the "primary" shader for this writer.
    UsdShadeShader texSchema = UsdShadeShader::Define(GetUsdStage(), texPath);
    if (!TF_VERIFY(
            texSchema, "Could not define UsdShadeShader at path '%s'\n", texPath.GetText())) {
        return;
    }

    _usdPrim = texSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            texPath.GetText())) {
        return;
    }

    // We need to know how many channels the texture has:
    MPlug       texNamePlug = depNodeFn.findPlug("fileTextureName");
    std::string filename(texNamePlug.asString().asChar());

    // Not resolving UDIM tags. We want to actually open one of these files:
    UsdMayaShadingUtil::ResolveUsdTextureFileName(
        filename, _GetExportArgs().GetResolvedFileName(), false);

    _numChannels = UsdMayaShadingUtil::GetNumberOfChannels(filename);
    switch (_numChannels) {
    case 1:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_float));
        texSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float);
        break;
    case 2:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_vector2));
        texSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);
        break;
    case 3:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_color3));
        texSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_color4));
        texSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color4f);
        break;
    default: TF_CODING_ERROR("Unsupported format"); return;
    }

    // Now create a geompropvalue reader that the image shader will use.
    SdfPath        primvarReaderPath = _GetPlace2DTexturePath(depNodeFn);
    UsdShadeOutput primvarReaderOutput;

    if (!GetUsdStage()->GetPrimAtPath(primvarReaderPath)) {
        UsdShadeShader primvarReaderSchema
            = UsdShadeShader::Define(GetUsdStage(), primvarReaderPath);
        primvarReaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_geompropvalue_vector2));
        UsdShadeInput varnameInput
            = primvarReaderSchema.CreateInput(TrMtlxTokens->geomprop, SdfValueTypeNames->String);

        TfToken inputName(
            TfStringPrintf("%s:%s", depNodeFn.name().asChar(), TrMtlxTokens->varnameStr.GetText()));

        // We expose the primvar reader varnameStr attribute to the material to allow
        // easy specialization based on UV mappings to geometries:
        UsdPrim          materialPrim = primvarReaderSchema.GetPrim().GetParent();
        UsdShadeMaterial materialSchema(materialPrim);
        while (!materialSchema && materialPrim) {
            UsdShadeNodeGraph intermediateNodeGraph(materialPrim);
            if (intermediateNodeGraph) {
                UsdShadeInput intermediateInput
                    = intermediateNodeGraph.CreateInput(inputName, SdfValueTypeNames->String);
                varnameInput.ConnectToSource(intermediateInput);
                varnameInput = intermediateInput;
            }

            materialPrim = materialPrim.GetParent();
            materialSchema = UsdShadeMaterial(materialPrim);
        }

        if (materialSchema) {
            UsdShadeInput materialInput
                = materialSchema.CreateInput(inputName, SdfValueTypeNames->String);
            materialInput.Set(UsdUtilsGetPrimaryUVSetName().GetString());
            varnameInput.ConnectToSource(materialInput);
        } else {
            varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
        }

        primvarReaderOutput
            = primvarReaderSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);
    } else {
        // Re-using an existing primvar reader:
        UsdShadeShader primvarReaderShaderSchema(GetUsdStage()->GetPrimAtPath(primvarReaderPath));
        primvarReaderOutput = primvarReaderShaderSchema.GetOutput(TrMtlxTokens->out);
    }
    // TODO: Handle UV SRT with a ND_place2d_vector2 node. Make sure the name derives from the
    //       place2dTexture node if there is one (see usdFileTextureWriter for details)

    // Connect the output of the primvar reader to the texture coordinate
    // input of the UV texture.
    texSchema.CreateInput(TrMtlxTokens->texcoord, SdfValueTypeNames->Float2)
        .ConnectToSource(primvarReaderOutput);
}

/* virtual */
void MtlxUsd_FileWriter::Write(const UsdTimeCode& usdTime)
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

    UsdMayaShadingUtil::ResolveUsdTextureFileName(
        fileTextureName, _GetExportArgs().GetResolvedFileName(), isUDIM);

    UsdShadeInput fileInput
        = shaderSchema.CreateInput(TrMtlxTokens->file, SdfValueTypeNames->Asset);
    fileInput.Set(SdfAssetPath(fileTextureName.c_str()), usdTime);

    // Source color space:
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

    // Default Color (which needs to have a matching number of channels)
    const MPlug defaultColorPlug = depNodeFn.findPlug(
        TrMayaTokens->defaultColor.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    switch (_numChannels) {
    case 1: {
        float fallback = 0.0f;
        defaultColorPlug.child(0).getValue(fallback);
        shaderSchema.CreateInput(TrMtlxTokens->paramDefault, SdfValueTypeNames->Float)
            .Set(fallback, usdTime);

    } break;
    case 2: {
        GfVec2f fallback(0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(TrMtlxTokens->paramDefault, SdfValueTypeNames->Float2)
            .Set(fallback, usdTime);
    } break;
    case 3: {
        GfVec3f fallback(0.0f, 0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(TrMtlxTokens->paramDefault, SdfValueTypeNames->Color3f)
            .Set(fallback, usdTime);
    } break;
    case 4: {
        GfVec4f fallback(0.0f, 0.0f, 0.0f, 1.0f);
        for (size_t i = 0u; i < 3; ++i) { // defaultColor is a 3Float
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(TrMtlxTokens->paramDefault, SdfValueTypeNames->Color4f)
            .Set(fallback, usdTime);
    } break;
    default: TF_CODING_ERROR("Unsupported format for default"); return;
    }

    // uaddressmode type="string" value="periodic" enum="constant,clamp,periodic,mirror"
    // vaddressmode type="string" value="periodic" enum="constant,clamp,periodic,mirror"
    const TfToken wrapMirror[2][3] {
        { TrMayaTokens->wrapU, TrMayaTokens->mirrorU, TrMtlxTokens->uaddressmode },
        { TrMayaTokens->wrapV, TrMayaTokens->mirrorV, TrMtlxTokens->vaddressmode }
    };
    for (auto wrapMirrorTriple : wrapMirror) {
        auto wrapUVToken = wrapMirrorTriple[0];
        auto mirrorUVToken = wrapMirrorTriple[1];
        auto addressModeToken = wrapMirrorTriple[2];

        const MPlug wrapUVPlug = depNodeFn.findPlug(
            wrapUVToken.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
        if (status != MS::kSuccess) {
            return;
        }

        // Don't check if authored
        const bool wrapVal = wrapUVPlug.asBool(&status);
        if (status != MS::kSuccess) {
            return;
        }

        std::string outputValue;
        if (!wrapVal) {
            outputValue = TrMtlxTokens->clamp.GetString();
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
            outputValue
                = mirrorVal ? TrMtlxTokens->mirror.GetString() : TrMtlxTokens->periodic.GetString();
        }
        shaderSchema.CreateInput(addressModeToken, SdfValueTypeNames->String)
            .Set(outputValue, usdTime);
    }

    // We could try to do filtertype, but the values do not map 1:1 between MaterialX and Maya
}

/* virtual */
UsdAttribute MtlxUsd_FileWriter::GetShadingAttributeForMayaAttrName(
    const TfToken&          mayaAttrName,
    const SdfValueTypeName& typeName)
{
    UsdShadeShader nodeSchema(_usdPrim);
    if (!nodeSchema) {
        return UsdAttribute();
    }

    if (mayaAttrName == TrMayaTokens->outColor) {
        switch (_numChannels) {
        case 1: return AddSwizzle("rrr", _numChannels);
        case 2:
            // Monochrome + alpha: use xxx swizzle of ND_image_vector2
            return AddSwizzle("xxx", _numChannels);
        case 3:
            // Non-swizzled:
            if (typeName == SdfValueTypeNames->Color3f) {
                return nodeSchema.GetOutput(TrMtlxTokens->out);
            } else if (typeName == SdfValueTypeNames->Float3) {
                return AddConversion(
                    SdfValueTypeNames->Color3f, typeName, nodeSchema.GetOutput(TrMtlxTokens->out));
            }
        case 4:
            if (typeName == SdfValueTypeNames->Color3f) {
                return AddSwizzle("rgb", _numChannels);
            } else if (typeName == SdfValueTypeNames->Float3) {
                return AddConversion(
                    SdfValueTypeNames->Color3f, typeName, AddSwizzle("rgb", _numChannels));
            }

        default: TF_CODING_ERROR("Unsupported format for outColor"); return UsdAttribute();
        }
    }

    // Starting here, we handle subcomponent requests:

    if (_numChannels == 2) {
        // This will be ND_image_vector2, so requires xyz swizzles:
        if (mayaAttrName == TrMayaTokens->outColorR || mayaAttrName == TrMayaTokens->outColorG
            || mayaAttrName == TrMayaTokens->outColorB) {
            return AddSwizzle("x", _numChannels);
        }
        if (mayaAttrName == TrMayaTokens->outAlpha) {
            return AddSwizzle("y", _numChannels);
        }
    }

    if (mayaAttrName == TrMayaTokens->outColorR) {
        return AddSwizzle("r", _numChannels);
    }

    if (mayaAttrName == TrMayaTokens->outColorG) {
        return AddSwizzle("g", _numChannels);
    }

    if (mayaAttrName == TrMayaTokens->outColorB) {
        return AddSwizzle("b", _numChannels);
    }

    if (mayaAttrName == TrMayaTokens->outAlpha) {
        bool                    alphaIsLuminance = false;
        MStatus                 status;
        const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
        if (status == MS::kSuccess) {
            MPlug plug = depNodeFn.findPlug(TrMayaTokens->alphaIsLuminance.GetText());
            plug.getValue(alphaIsLuminance);
        }

        if (alphaIsLuminance || _numChannels == 3) {
            return AddLuminance(_numChannels);
        } else {
            return AddSwizzle("a", _numChannels);
        }
    }

    return UsdAttribute();
}

SdfPath MtlxUsd_FileWriter::_GetPlace2DTexturePath(const MFnDependencyNode& depNodeFn)
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
            usdUvTextureName = UsdMayaUtil::SanitizeName(sourceNode.name().asChar());
        }
    }

    if (usdUvTextureName.empty()) {
        // We want a single UV reader for all file nodes not connected to a place2DTexture node
        usdUvTextureName = _tokens->PrimvarReaderShaderName.GetString();
    }

    return GetNodeGraph().GetPath().AppendChild(TfToken(usdUvTextureName.c_str()));
}

PXR_NAMESPACE_CLOSE_SCOPE
