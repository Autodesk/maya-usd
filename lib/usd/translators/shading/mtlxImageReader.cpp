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

class MtlxUsd_ImageReader : public MtlxUsd_BaseReader
{
public:
    MtlxUsd_ImageReader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

    void PostConnectSubtree(UsdMayaPrimReaderContext* context) override;

private:
    TfToken _shaderID;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_float, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_vector2, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_vector3, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_vector4, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_color3, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_color4, MtlxUsd_ImageReader)

MtlxUsd_ImageReader::MtlxUsd_ImageReader(const UsdMayaPrimReaderArgs& readArgs)
    : MtlxUsd_BaseReader(readArgs)
{
}

/* virtual */
bool MtlxUsd_ImageReader::Read(UsdMayaPrimReaderContext& context)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    // It is possible the file node already exists if we encountered a post-processor:
    MObject mayaObject = context.GetMayaNode(prim.GetPath(), false);

    MStatus status;
    if (mayaObject.isNull()) {
        if (!UsdMayaTranslatorUtil::CreateShaderNode(
                MString(prim.GetName().GetText()),
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
    }

    MFnDependencyNode depFn(mayaObject, &status);
    if (!status) {
        return false;
    }

    // File
    VtValue val;
    MPlug   mayaAttr = depFn.findPlug(TrMayaTokens->fileTextureName.GetText(), true, &status);

    UsdShadeInput usdInput = shaderSchema.GetInput(TrMtlxTokens->file);
    if (status == MS::kSuccess && usdInput && usdInput.Get(&val) && val.IsHolding<SdfAssetPath>()) {
        std::string filePath = val.UncheckedGet<SdfAssetPath>().GetResolvedPath();
        if (!filePath.empty() && !ArIsPackageRelativePath(filePath)) {
            // Maya has issues with relative paths, especially if deep inside a
            // nesting of referenced assets. Use absolute path instead if USD was
            // able to resolve. A better fix will require providing an asset
            // resolver to Maya that can resolve the file correctly using the
            // MPxFileResolver API. We also make sure the path is not expressed
            // as a relationship like texture paths inside USDZ assets.
            val = SdfAssetPath(filePath);
        }

        // NOTE: Will need UDIM support and potentially USDZ support. When that happens, consider
        // refactoring existing code from usdUVTextureReader.cpp as shared utilities.
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);

        // colorSpace:
        if (usdInput.GetAttr().HasColorSpace()) {
            MString colorSpace = usdInput.GetAttr().GetColorSpace().GetText();
            mayaAttr = depFn.findPlug(TrMayaTokens->colorSpace.GetText(), true, &status);
            if (status == MS::kSuccess) {
                mayaAttr.setString(colorSpace);
            }
        }
    }

    // Default color
    //
    float   alphaVal = 1.0f;
    GfVec3f colorVal(0.0f, 0.0f, 0.0f);
    if (GetColorAndAlphaFromInput(shaderSchema, TrMayaTokens->defaultColor, colorVal, alphaVal)) {
        MPlug mayaAttr = depFn.findPlug(TrMayaTokens->defaultColor.GetText(), true, &status);
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(colorVal), /*unlinearizeColors*/ false);
    }

    // Filter type:
    mayaAttr = depFn.findPlug(TrMayaTokens->filterType.GetText(), true, &status);
    usdInput = shaderSchema.GetInput(TrMtlxTokens->filtertype);
    if (status == MS::kSuccess && usdInput && usdInput.Get(&val) && val.IsHolding<std::string>()) {
        std::string filterType = val.UncheckedGet<std::string>();
        if (filterType == TrMtlxTokens->closest.GetString()) {
            mayaAttr.setInt(0);
        } else if (filterType == TrMtlxTokens->linear.GetString()) {
            mayaAttr.setInt(1);
        } else {
            // Maya quadratic default for TrMtlxTokens->cubic:
            mayaAttr.setInt(3);
        }
    }

    return true;
}

/* virtual */
TfToken MtlxUsd_ImageReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdOutputName;
    UsdShadeAttributeType attrType;
    std::tie(usdOutputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output && usdOutputName == TrMtlxTokens->out) {
        if (_shaderID == TrMtlxTokens->ND_image_float) {
            return TrMayaTokens->outColorR;
        }

        return TrMayaTokens->outColor;
    }

    if (attrType == UsdShadeAttributeType::Input && usdOutputName == TrMtlxTokens->texcoord) {
        return TrMayaTokens->uvCoord;
    }

    return TfToken();
}

void MtlxUsd_ImageReader::PostConnectSubtree(UsdMayaPrimReaderContext* context)
{
    MObject           mayaObject = context->GetMayaNode(_GetArgs().GetUsdPrim().GetPath(), false);
    MFnDependencyNode depFn(mayaObject);

    MPlug uvCoordPlug = depFn.findPlug(TrMayaTokens->uvCoord.GetText());

    if (!uvCoordPlug.isDestination()) {
        // If there is no place2dTexture at this point, we can create a default one.
        UsdMayaShadingUtil::CreatePlace2dTextureAndConnectTexture(mayaObject);
    } else {
        // We imported a place2dTexture. Make sure it is fully connected.
        UsdMayaShadingUtil::ConnectPlace2dTexture(mayaObject, uvCoordPlug.source().node());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
