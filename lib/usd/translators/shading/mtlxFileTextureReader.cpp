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

#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/arch/hash.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/resolver.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <ghc/filesystem.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class MtlxUsd_FileTextureReader : public MtlxUsd_BaseReader
{
public:
    MtlxUsd_FileTextureReader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext& context) override;

    bool TraverseUnconnectableInput(const TfToken& usdAttrName) const override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_float, MtlxUsd_FileTextureReader)
PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_vector2, MtlxUsd_FileTextureReader)
PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_vector3, MtlxUsd_FileTextureReader)
PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_vector4, MtlxUsd_FileTextureReader)
PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_color3, MtlxUsd_FileTextureReader)
PXRUSDMAYA_REGISTER_SHADER_READER(MayaND_fileTexture_color4, MtlxUsd_FileTextureReader)

MtlxUsd_FileTextureReader::MtlxUsd_FileTextureReader(const UsdMayaPrimReaderArgs& readArgs)
    : MtlxUsd_BaseReader(readArgs)
{
}

/* virtual */
bool MtlxUsd_FileTextureReader::Read(UsdMayaPrimReaderContext& context)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MString mayaNodeName = prim.GetName().GetText();
    MObject mayaObject;
    SdfPath imageNodePath;

    // Try to get the connection on inColor to get the name of the image node:
    UsdShadeInput inColorInput = shaderSchema.GetInput(TrMayaTokens->inColor);
    if (inColorInput) {
        UsdShadeConnectableAPI source;
        TfToken                sourceInputName;
        UsdShadeAttributeType  sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(
                inColorInput, &source, &sourceInputName, &sourceType)) {

            UsdShadeShader imageShader(source.GetPrim());

            TfToken shaderId;
            imageShader.GetIdAttr().Get(&shaderId);

            if (shaderId.GetString().rfind("ND_image_", 0) == 0) {
                mayaNodeName = source.GetPrim().GetName().GetText();
                // See if that node exists.
                imageNodePath = source.GetPath();
                mayaObject = context.GetMayaNode(imageNodePath, false);
            }
        }
    }

    MStatus status;
    if (mayaObject.isNull()) {
        if (!UsdMayaTranslatorUtil::CreateShaderNode(
                mayaNodeName,
                TrMayaTokens->file.GetText(),
                UsdMayaShadingNodeType::Texture,
                &status,
                &mayaObject)) {
            // we need to make sure those types are loaded..
            TF_RUNTIME_ERROR(
                "Could not create node of type '%s' for shader '%s'.\n",
                TrMayaTokens->file.GetText(),
                prim.GetPath().GetText());
            return false;
        }
        context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);
        if (!imageNodePath.IsEmpty()) {
            context.RegisterNewMayaNode(imageNodePath.GetString(), mayaObject);
        }
    }

    MFnDependencyNode depFn(mayaObject, &status);
    if (!status) {
        return false;
    }

    // Default color
    //
    float   alphaVal = 1.0f;
    GfVec3f colorVal(0.0f, 0.0f, 0.0f);
    if (GetColorAndAlphaFromInput(shaderSchema, TrMayaTokens->defaultColor, colorVal, alphaVal)) {
        MPlug mayaAttr = depFn.findPlug(TrMayaTokens->defaultColor.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(colorVal), /*unlinearizeColors*/ false);
    }

    // Color gain
    //
    colorVal = GfVec3f { 1.0f, 1.0f, 1.0f };
    alphaVal = 1.0f;
    if (GetColorAndAlphaFromInput(shaderSchema, TrMayaTokens->colorGain, colorVal, alphaVal)) {
        MPlug mayaAttr = depFn.findPlug(TrMayaTokens->colorGain.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(colorVal), /*unlinearizeColors*/ false);
        mayaAttr = depFn.findPlug(TrMayaTokens->alphaGain.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(alphaVal));
    }

    // Color offset
    //
    colorVal = GfVec3f { 0.0f, 0.0f, 0.0f };
    alphaVal = 0.0f;
    if (GetColorAndAlphaFromInput(shaderSchema, TrMayaTokens->colorOffset, colorVal, alphaVal)) {
        MPlug mayaAttr = depFn.findPlug(TrMayaTokens->colorOffset.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(colorVal), /*unlinearizeColors*/ false);
        mayaAttr = depFn.findPlug(TrMayaTokens->alphaOffset.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(alphaVal));
    }

    // Invert
    //
    ReadShaderInput(shaderSchema, TrMayaTokens->invert, depFn);

    // Exposure
    //
    ReadShaderInput(shaderSchema, TrMayaTokens->exposure, depFn);

    // Color space
    //
    ReadShaderInput(shaderSchema, TrMayaTokens->colorSpace, depFn);

    return true;
}

/* virtual */
TfToken MtlxUsd_FileTextureReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdPortName;
    UsdShadeAttributeType attrType;
    std::tie(usdPortName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output && usdPortName == TrMayaTokens->outColor) {
        return usdPortName;
    }

    if (attrType == UsdShadeAttributeType::Input) {
        return usdPortName;
    }

    return {};
}

bool MtlxUsd_FileTextureReader::TraverseUnconnectableInput(const TfToken& usdAttrName) const
{
    TfToken               usdPortName;
    UsdShadeAttributeType attrType;
    std::tie(usdPortName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    return usdPortName == TrMayaTokens->inColor && attrType == UsdShadeAttributeType::Input;
}

PXR_NAMESPACE_CLOSE_SCOPE
