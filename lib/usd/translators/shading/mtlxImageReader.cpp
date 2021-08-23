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

class MtlxUsd_ImageReader : public UsdMayaShaderReader
{
public:
    MtlxUsd_ImageReader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

private:
    TfToken _shaderID;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_float, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_vector2, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_color3, MtlxUsd_ImageReader)
PXRUSDMAYA_REGISTER_SHADER_READER(ND_image_color4, MtlxUsd_ImageReader)

MtlxUsd_ImageReader::MtlxUsd_ImageReader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
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

    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              TrMayaTokens->file.GetText(),
              UsdMayaShadingNodeType::Texture,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            TrMayaTokens->file.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    // Create place2dTexture:
    MObject           uvObj = UsdMayaShadingUtil::CreatePlace2dTextureAndConnectTexture(mayaObject);
    MFnDependencyNode uvDepFn(uvObj);

    // TODO: Import UV SRT from ND_place2d_vector2 node.

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
    // The number of channels in the source can vary:
    shaderSchema.GetIdAttr().Get(&_shaderID);
    usdInput = shaderSchema.GetInput(TrMtlxTokens->paramDefault);
    mayaAttr = depFn.findPlug(TrMayaTokens->defaultColor.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess && usdInput.Get(&val)) {
        GfVec3f mayaVal(0.0f, 0.0f, 0.0f);
        if (_shaderID == TrMtlxTokens->ND_image_float && val.IsHolding<float>()) {
            // Mono: treat as rrr swizzle
            mayaVal[0] = val.UncheckedGet<float>();
            mayaVal[1] = mayaVal[0];
            mayaVal[2] = mayaVal[0];
        } else if (_shaderID == TrMtlxTokens->ND_image_vector2 && val.IsHolding<GfVec2f>()) {
            const GfVec2f& vecVal = val.UncheckedGet<GfVec2f>();
            // Mono + alpha: treat as rrr swizzle
            mayaVal[0] = vecVal[0];
            mayaVal[1] = vecVal[0];
            mayaVal[2] = vecVal[0];
        } else if (_shaderID == TrMtlxTokens->ND_image_color3 && val.IsHolding<GfVec3f>()) {
            mayaVal = val.UncheckedGet<GfVec3f>();
        } else if (_shaderID == TrMtlxTokens->ND_image_color4 && val.IsHolding<GfVec4f>()) {
            const GfVec4f& vecVal = val.UncheckedGet<GfVec4f>();
            mayaVal[0] = vecVal[0];
            mayaVal[1] = vecVal[1];
            mayaVal[2] = vecVal[2];
        }
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
    }

    // Wrap U/V
    const TfToken wrapMirrorTriples[2][3] {
        { TrMayaTokens->wrapU, TrMayaTokens->mirrorU, TrMtlxTokens->uaddressmode },
        { TrMayaTokens->wrapV, TrMayaTokens->mirrorV, TrMtlxTokens->vaddressmode }
    };
    for (auto wrapMirrorTriple : wrapMirrorTriples) {
        auto wrapUVToken = wrapMirrorTriple[0];
        auto mirrorUVToken = wrapMirrorTriple[1];
        auto wrapSTToken = wrapMirrorTriple[2];

        usdInput = shaderSchema.GetInput(wrapSTToken);
        if (usdInput) {
            if (usdInput.Get(&val) && val.IsHolding<std::string>()) {
                const std::string& wrapVal = val.UncheckedGet<std::string>();
                TfToken            plugName;

                if (wrapVal == TrMtlxTokens->periodic.GetString()) {
                    // do nothing - will repeat by default
                    continue;
                } else if (wrapVal == TrMtlxTokens->mirror.GetString()) {
                    plugName = mirrorUVToken;
                    val = true;
                } else {
                    plugName = wrapUVToken;
                    val = false;
                }
                mayaAttr = uvDepFn.findPlug(plugName.GetText(), true, &status);
                if (status != MS::kSuccess) {
                    continue;
                }
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
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

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
