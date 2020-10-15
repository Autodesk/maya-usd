//
// Copyright 2020 Autodesk
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
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdUVTexture_Reader : public UsdMayaShaderReader
{
public:
    PxrMayaUsdUVTexture_Reader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext* context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdUVTexture, PxrMayaUsdUVTexture_Reader)

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya "file" node attribute names
    (file)(alphaGain)(alphaOffset)(colorGain)(colorOffset)(defaultColor)(fileTextureName)(outAlpha)(
        outColor)(outColorR)(outColorG)(outColorB)(place2dTexture)(coverage)(translateFrame)(
        rotateFrame)(mirrorU)(mirrorV)(stagger)(wrapU)(wrapV)(repeatUV)(offset)(rotateUV)(noiseUV)(
        vertexUvOne)(vertexUvTwo)(vertexUvThree)(vertexCameraOne)

    // UsdUVTexture Input Names
    (bias)(fallback)(scale)(wrapS)(wrapT)

    // Values for wrapS and wrapT
    (black)(repeat)

    // UsdUVTexture Output Names
    ((RGBOutputName, "rgb"))((RedOutputName, "r"))((GreenOutputName, "g"))((BlueOutputName, "b"))(
        (AlphaOutputName, "a")));

static const TfTokenVector _Place2dTextureConnections = {
    _tokens->coverage,    _tokens->translateFrame, _tokens->rotateFrame,   _tokens->mirrorU,
    _tokens->mirrorV,     _tokens->stagger,        _tokens->wrapU,         _tokens->wrapV,
    _tokens->repeatUV,    _tokens->offset,         _tokens->rotateUV,      _tokens->noiseUV,
    _tokens->vertexUvOne, _tokens->vertexUvTwo,    _tokens->vertexUvThree, _tokens->vertexCameraOne
};

PxrMayaUsdUVTexture_Reader::PxrMayaUsdUVTexture_Reader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrMayaUsdUVTexture_Reader::Read(UsdMayaPrimReaderContext* context)
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
              _tokens->file.GetText(),
              UsdMayaShadingNodeType::Texture,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            _tokens->file.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context->RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    // Create place2dTexture:
    MObject           uvObj;
    MFnDependencyNode uvDepFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              _tokens->place2dTexture.GetText(),
              _tokens->place2dTexture.GetText(),
              UsdMayaShadingNodeType::Utility,
              &status,
              &uvObj)
          && uvDepFn.setObject(uvObj))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            _tokens->place2dTexture.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    // Connect manually (fileTexturePlacementConnect is not available in batch):
    MString connectCmd;
    for (const TfToken& uvName : _Place2dTextureConnections) {
        MPlug uvPlug = uvDepFn.findPlug(uvName.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(uvName.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }

    VtValue val;
    MPlug   mayaAttr;

    // File
    UsdShadeInput usdInput = shaderSchema.GetInput(_tokens->file);
    if (usdInput && usdInput.Get(&val) && val.IsHolding<SdfAssetPath>()) {
        std::string filePath = val.UncheckedGet<SdfAssetPath>().GetResolvedPath();
        if (!filePath.empty()) {
            // Maya has issues with relative paths, especially if deep inside a
            // nesting of referenced assets. Use absolute path instead if USD was
            // able to resolve. A better fix will require providing an asset
            // resolver to Maya that can resolve the file correctly using the
            // MPxFileResolver API.
            val = SdfAssetPath(filePath);
        }
        mayaAttr = depFn.findPlug(_tokens->fileTextureName.GetText(), true, &status);
        if (status == MS::kSuccess) {
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
        }
    }

    // The Maya file node's 'colorGain' and 'alphaGain' attributes map to the
    // UsdUVTexture's scale input.
    usdInput = shaderSchema.GetInput(_tokens->scale);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f scale = val.UncheckedGet<GfVec4f>();
            mayaAttr = depFn.findPlug(_tokens->colorGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(scale[0], scale[1], scale[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(_tokens->alphaGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = scale[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // The Maya file node's 'colorOffset' and 'alphaOffset' attributes map to
    // the UsdUVTexture's bias input.
    usdInput = shaderSchema.GetInput(_tokens->bias);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f bias = val.UncheckedGet<GfVec4f>();
            mayaAttr = depFn.findPlug(_tokens->colorOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(bias[0], bias[1], bias[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(_tokens->alphaOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                // The alpha is not scaled
                val = bias[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // Default color
    usdInput = shaderSchema.GetInput(_tokens->fallback);
    mayaAttr = depFn.findPlug(_tokens->defaultColor.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f fallback = val.UncheckedGet<GfVec4f>();
            val = GfVec3f(fallback[0], fallback[1], fallback[2]);
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
        }
    }

    // Wrap U
    usdInput = shaderSchema.GetInput(_tokens->wrapS);
    mayaAttr = uvDepFn.findPlug(_tokens->wrapU.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess) {
        if (usdInput.Get(&val) && val.IsHolding<TfToken>()) {
            TfToken wrapS = val.UncheckedGet<TfToken>();
            val = wrapS == _tokens->repeat;
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
        }
    }

    // Wrap V
    usdInput = shaderSchema.GetInput(_tokens->wrapT);
    mayaAttr = uvDepFn.findPlug(_tokens->wrapV.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess) {
        if (usdInput.Get(&val) && val.IsHolding<TfToken>()) {
            TfToken wrapT = val.UncheckedGet<TfToken>();
            val = wrapT == _tokens->repeat;
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
        }
    }

    return true;
}

/* virtual */
TfToken PxrMayaUsdUVTexture_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdOutputName;
    UsdShadeAttributeType attrType;
    std::tie(usdOutputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output) {
        if (usdOutputName == _tokens->RGBOutputName) {
            return _tokens->outColor;
        } else if (usdOutputName == _tokens->RedOutputName) {
            return _tokens->outColorR;
        } else if (usdOutputName == _tokens->GreenOutputName) {
            return _tokens->outColorG;
        } else if (usdOutputName == _tokens->BlueOutputName) {
            return _tokens->outColorB;
        } else if (usdOutputName == _tokens->AlphaOutputName) {
            return _tokens->outAlpha;
        }
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
