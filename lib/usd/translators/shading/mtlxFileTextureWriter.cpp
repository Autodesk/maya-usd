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
#include <pxr/imaging/hio/image.h>
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
    void _GetResolvedTextureName(std::string&);

    int _numChannels = 4;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, MtlxUsd_FileWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Prefix for helper nodes:
    ((PrimvarReaderPrefix, "MayaGeomPropValue"))
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
    SdfPath texPath = nodegraphPath.AppendChild(TfToken(depNodeFn.name().asChar()));

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

    _GetResolvedTextureName(filename);

    // Using Hio because the Maya texture node does not provide the information:
    HioImageSharedPtr image = HioImage::OpenForReading(filename.c_str());

    HioFormat imageFormat = image ? image->GetFormat() : HioFormat::HioFormatUNorm8Vec4;

    // In case of unknown, use 4 channel image:
    if (imageFormat == HioFormat::HioFormatInvalid) {
        imageFormat = HioFormat::HioFormatUNorm8Vec4;
    }
    _numChannels = HioGetComponentCount(imageFormat);
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
    TfToken primvarReaderName(
        TfStringPrintf("%s_%s", _tokens->PrimvarReaderPrefix.GetText(), depNodeFn.name().asChar()));
    const SdfPath  primvarReaderPath = nodegraphPath.AppendChild(primvarReaderName);
    UsdShadeShader primvarReaderSchema = UsdShadeShader::Define(GetUsdStage(), primvarReaderPath);

    primvarReaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_geompropvalue_vector2));

    UsdShadeInput varnameInput
        = primvarReaderSchema.CreateInput(TrMtlxTokens->geomprop, SdfValueTypeNames->String);

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
            = materialSchema.CreateInput(inputName, SdfValueTypeNames->String);
        materialInput.Set(UsdUtilsGetPrimaryUVSetName().GetString());
        varnameInput.ConnectToSource(materialInput);
    } else {
        varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
    }

    UsdShadeOutput primvarReaderOutput
        = primvarReaderSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);

    // TODO: Handle UV SRT with a ND_place2d_vector2 node.

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

    _GetResolvedTextureName(fileTextureName);

    // WARNING: This extremely minimal attempt at making the file path relative
    //          to the USD stage is a stopgap measure intended to provide
    //          minimal interop. It will be replaced by proper use of Maya and
    //          USD asset resolvers. For package files, the exporter needs full
    //          paths.
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

void MtlxUsd_FileWriter::_GetResolvedTextureName(std::string& fileTextureName)
{
    // WARNING: This extremely minimal attempt at making the file path relative
    //          to the USD stage is a stopgap measure intended to provide
    //          minimal interop. It will be replaced by proper use of Maya and
    //          USD asset resolvers. For package files, the exporter needs full
    //          paths.
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
}

PXR_NAMESPACE_CLOSE_SCOPE
