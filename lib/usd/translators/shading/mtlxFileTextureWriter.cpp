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

class MaterialXTranslators_FileTextureWriter : public MaterialXTranslators_BaseWriter
{
public:
    MaterialXTranslators_FileTextureWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute GetShadingAttributeForMayaAttrName(const TfToken& mayaAttrName) override;

private:
    int _numChannels = 4;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, MaterialXTranslators_FileTextureWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya "file" node attribute names
    (fileTextureName)
    (colorSpace)
    (defaultColor)
    (wrapU)
    (wrapV)
    (mirrorU)
    (mirrorV)
    (filterType)
    (outColor)
    (outColorR)
    (outColorG)
    (outColorB)
    (outAlpha)

    // "varname" at the material level for UV handling
    (varname)

    // MaterialX nodedefs
    (ND_image_float)
    (ND_image_vector2)
    (ND_image_color3)
    (ND_image_color4)
    (ND_geompropvalue_vector2)

    // Prefix for helper nodes:
    ((PrimvarReaderPrefix, "MayaGeomPropValue"))

    // MaterialX common output name
    (out)

    // geompropvalue parameter names
    (geomprop)

    // image parameter names
    (file)
    ((paramDefault, "default"))
    (texcoord)
    (uaddressmode)
    (vaddressmode)
    (filtertype)

    // image parameter values
    (clamp)
    (periodic)
    (mirror)
    (closest)
    (linear)
    (cubic)
);
// clang-format on

MaterialXTranslators_FileTextureWriter::MaterialXTranslators_FileTextureWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MaterialXTranslators_BaseWriter(depNodeFn, usdPath, jobCtx)
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
    MPlug   texNamePlug = depNodeFn.findPlug("fileTextureName");
    MString filename;
    texNamePlug.getValue(filename);

    // Using Hio because the Maya texture node does not provide the information:
    HioImageSharedPtr image = HioImage::OpenForReading(filename.asChar());

    HioFormat imageFormat = image->GetFormat();

    // In case of unknown, use 4 channel image:
    if (imageFormat == HioFormat::HioFormatInvalid) {
        imageFormat = HioFormat::HioFormatUNorm8Vec4;
    }
    _numChannels = HioGetComponentCount(imageFormat);
    switch (_numChannels) {
    case 1:
        texSchema.CreateIdAttr(VtValue(_tokens->ND_image_float));
        texSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Float);
        break;
    case 2:
        texSchema.CreateIdAttr(VtValue(_tokens->ND_image_vector2));
        texSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Float2);
        break;
    case 3:
        texSchema.CreateIdAttr(VtValue(_tokens->ND_image_color3));
        texSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        texSchema.CreateIdAttr(VtValue(_tokens->ND_image_color4));
        texSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color4f);
        break;
    default: TF_CODING_ERROR("Unsupported format"); return;
    }

    // Now create a geompropvalue reader that the image shader will use.
    TfToken primvarReaderName(
        TfStringPrintf("%s_%s", _tokens->PrimvarReaderPrefix.GetText(), depNodeFn.name().asChar()));
    const SdfPath  primvarReaderPath = nodegraphPath.AppendChild(primvarReaderName);
    UsdShadeShader primvarReaderSchema = UsdShadeShader::Define(GetUsdStage(), primvarReaderPath);

    primvarReaderSchema.CreateIdAttr(VtValue(_tokens->ND_geompropvalue_vector2));

    UsdShadeInput varnameInput
        = primvarReaderSchema.CreateInput(_tokens->geomprop, SdfValueTypeNames->String);

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
            TfStringPrintf("%s:%s", depNodeFn.name().asChar(), _tokens->varname.GetText()));
        UsdShadeInput materialInput
            = materialSchema.CreateInput(inputName, SdfValueTypeNames->String);
        materialInput.Set(UsdUtilsGetPrimaryUVSetName().GetString());
        varnameInput.ConnectToSource(materialInput);
    } else {
        varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
    }

    UsdShadeOutput primvarReaderOutput
        = primvarReaderSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Float2);

    // No place2dTexture support at this time. Adding math nodes to support
    // "offset", "repeat", and "rotate" does not work that well on import. It
    // would be preferable to have a ND_Maya_place2dTexture node definition that
    // could be implemented as a nodegraph and would allow setting parameters
    // in a way that is import friendly.

    // Connect the output of the primvar reader to the texture coordinate
    // input of the UV texture.
    texSchema.CreateInput(_tokens->texcoord, SdfValueTypeNames->Float2)
        .ConnectToSource(primvarReaderOutput);
}

/* virtual */
void MaterialXTranslators_FileTextureWriter::Write(const UsdTimeCode& usdTime)
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
        _tokens->fileTextureName.GetText(),
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

    UsdShadeInput fileInput = shaderSchema.CreateInput(_tokens->file, SdfValueTypeNames->Asset);
    fileInput.Set(SdfAssetPath(fileTextureName.c_str()), usdTime);

    // Source color space:
    MPlug colorSpacePlug = depNodeFn.findPlug(_tokens->colorSpace.GetText(), true, &status);
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
        _tokens->defaultColor.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    switch (_numChannels) {
    case 1: {
        float fallback = 0.0f;
        defaultColorPlug.child(0).getValue(fallback);
        shaderSchema.CreateInput(_tokens->paramDefault, SdfValueTypeNames->Float)
            .Set(fallback, usdTime);

    } break;
    case 2: {
        GfVec2f fallback(0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(_tokens->paramDefault, SdfValueTypeNames->Float2)
            .Set(fallback, usdTime);
    } break;
    case 3: {
        GfVec3f fallback(0.0f, 0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(_tokens->paramDefault, SdfValueTypeNames->Color3f)
            .Set(fallback, usdTime);
    } break;
    case 4: {
        GfVec4f fallback(0.0f, 0.0f, 0.0f, 1.0f);
        for (size_t i = 0u; i < 3; ++i) { // defaultColor is a 3Float
            defaultColorPlug.child(i).getValue(fallback[i]);
        }
        shaderSchema.CreateInput(_tokens->paramDefault, SdfValueTypeNames->Color4f)
            .Set(fallback, usdTime);
    } break;
    default: TF_CODING_ERROR("Unsupported format for default"); return;
    }

    // uaddressmode type="string" value="periodic" enum="constant,clamp,periodic,mirror"
    // vaddressmode type="string" value="periodic" enum="constant,clamp,periodic,mirror"
    const TfToken wrapMirror[2][3] { { _tokens->wrapU, _tokens->mirrorU, _tokens->uaddressmode },
                                     { _tokens->wrapV, _tokens->mirrorV, _tokens->vaddressmode } };
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
            outputValue = _tokens->clamp.GetString();
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
            outputValue = mirrorVal ? _tokens->mirror.GetString() : _tokens->periodic.GetString();
        }
        shaderSchema.CreateInput(addressModeToken, SdfValueTypeNames->String)
            .Set(outputValue, usdTime);
    }

    // filtertype type="string" value="linear" enum="closest,linear,cubic"
    const MPlug filterTypePlug = depNodeFn.findPlug(
        _tokens->filterType.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    int filterType = 0;
    filterTypePlug.getValue(filterType);
    // blend is the default as "linear"
    std::string filterTypeString = _tokens->linear.GetString();
    if (filterType == 0) {
        // No filter means "closest"
        filterTypeString = _tokens->closest.GetString();
    } else if (filterType > 1) {
        // box, quadratic, quartic, gauss all map to "cubic" (lossy)
        filterTypeString = _tokens->cubic.GetString();
    }

    shaderSchema.CreateInput(_tokens->filtertype, SdfValueTypeNames->String)
        .Set(filterTypeString, usdTime);
}

/* virtual */
UsdAttribute MaterialXTranslators_FileTextureWriter::GetShadingAttributeForMayaAttrName(
    const TfToken& mayaAttrName)
{
    UsdShadeShader nodeSchema(_usdPrim);
    if (!nodeSchema) {
        return UsdAttribute();
    }

    if (mayaAttrName == _tokens->outColor) {
        switch (_numChannels) {
        case 1: return AddSwizzle("rrr", _numChannels);
        case 2:
            // Monochrome + alpha: use xxx swizzle of ND_image_vector2
            return AddSwizzle("xxx", _numChannels);
        case 3:
            // Non-swizzled:
            return nodeSchema.GetOutput(_tokens->out);
        case 4: return AddSwizzle("rgb", _numChannels);
        default: TF_CODING_ERROR("Unsupported format for outColor"); return UsdAttribute();
        }
    }

    // Starting here, we handle subcomponent requests:
    
    if (_numChannels == 2) {
        // This will be ND_image_vector2, so requires xyz swizzles:
        if (mayaAttrName == _tokens->outColorR) {
            return AddSwizzle("x", _numChannels);
        }
        if (mayaAttrName == _tokens->outColorG || mayaAttrName == _tokens->outAlpha) {
            return AddSwizzle("y", _numChannels);
        }
    }

    if (mayaAttrName == _tokens->outColorR) {
        return AddSwizzle("r", _numChannels);
    }

    if (mayaAttrName == _tokens->outColorG) {
        return AddSwizzle("g", _numChannels);
    }

    if (mayaAttrName == _tokens->outColorB) {
        return AddSwizzle("b", _numChannels);
    }

    if (mayaAttrName == _tokens->outAlpha) {
        bool                    alphaIsLuminance = false;
        MStatus                 status;
        const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
        if (status == MS::kSuccess) {
            MPlug plug = depNodeFn.findPlug("alphaIsLuminance");
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

PXR_NAMESPACE_CLOSE_SCOPE
