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
#include "shadingAsset.h"
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
#include <pxr/usd/sdf/layerUtils.h>
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

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdUVTexture, PxrMayaUsdUVTexture_Reader)

PxrMayaUsdUVTexture_Reader::PxrMayaUsdUVTexture_Reader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrMayaUsdUVTexture_Reader::Read(UsdMayaPrimReaderContext& context)
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
        // we need to make sure those types are loaded..
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

    if (!ResolveTextureAssetPath(prim, shaderSchema, depFn, _GetArgs().GetJobArguments())) {
        return false;
    }

    // The Maya file node's 'colorGain' and 'alphaGain' attributes map to the
    // UsdUVTexture's scale input.
    VtValue       val;
    UsdShadeInput usdInput = shaderSchema.GetInput(TrUsdTokens->scale);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f scale = val.UncheckedGet<GfVec4f>();
            MPlug   mayaAttr = depFn.findPlug(TrMayaTokens->colorGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(scale[0], scale[1], scale[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(TrMayaTokens->alphaGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = scale[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // The Maya file node's 'colorOffset' and 'alphaOffset' attributes map to
    // the UsdUVTexture's bias input.
    usdInput = shaderSchema.GetInput(TrUsdTokens->bias);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f bias = val.UncheckedGet<GfVec4f>();
            MPlug   mayaAttr = depFn.findPlug(TrMayaTokens->colorOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(bias[0], bias[1], bias[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(TrMayaTokens->alphaOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                // The alpha is not scaled
                val = bias[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // Default color
    usdInput = shaderSchema.GetInput(TrUsdTokens->fallback);
    MPlug mayaAttr = depFn.findPlug(TrMayaTokens->defaultColor.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f fallback = val.UncheckedGet<GfVec4f>();
            val = GfVec3f(fallback[0], fallback[1], fallback[2]);
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
        }
    }

    // Wrap U/V
    const TfToken wrapMirrorTriples[2][3] {
        { TrMayaTokens->wrapU, TrMayaTokens->mirrorU, TrUsdTokens->wrapS },
        { TrMayaTokens->wrapV, TrMayaTokens->mirrorV, TrUsdTokens->wrapT }
    };
    for (auto wrapMirrorTriple : wrapMirrorTriples) {
        auto wrapUVToken = wrapMirrorTriple[0];
        auto mirrorUVToken = wrapMirrorTriple[1];
        auto wrapSTToken = wrapMirrorTriple[2];

        usdInput = shaderSchema.GetInput(wrapSTToken);
        if (usdInput) {
            if (usdInput.Get(&val) && val.IsHolding<TfToken>()) {
                TfToken wrapVal = val.UncheckedGet<TfToken>();
                TfToken plugName;

                if (wrapVal == TrUsdTokens->repeat) {
                    // do nothing - will repeat by default
                    continue;
                } else if (wrapVal == TrUsdTokens->mirror) {
                    plugName = mirrorUVToken;
                    val = true;
                } else {
                    plugName = wrapUVToken;
                    val = false;
                }
                MPlug mayaAttr = uvDepFn.findPlug(plugName.GetText(), true, &status);
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
TfToken PxrMayaUsdUVTexture_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdOutputName;
    UsdShadeAttributeType attrType;
    std::tie(usdOutputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output) {
        if (usdOutputName == TrUsdTokens->RGBOutputName) {
            return TrMayaTokens->outColor;
        } else if (usdOutputName == TrUsdTokens->RedOutputName) {
            return TrMayaTokens->outColorR;
        } else if (usdOutputName == TrUsdTokens->GreenOutputName) {
            return TrMayaTokens->outColorG;
        } else if (usdOutputName == TrUsdTokens->BlueOutputName) {
            return TrMayaTokens->outColorB;
        } else if (usdOutputName == TrUsdTokens->AlphaOutputName) {
            return TrMayaTokens->outAlpha;
        }
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
