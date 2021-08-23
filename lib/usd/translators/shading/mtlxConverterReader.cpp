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
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
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
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION(
    TrMtlxTokens->conversionName,
    TrMtlxTokens->contextName,
    TrMtlxTokens->niceName,
    TrMtlxTokens->importDescription);

// Very simple delegating converter for intermediate nodes added between an image node and
// a shader parameter when swizzling/conversion is required.
class MtlxUsd_ConverterReader : public UsdMayaShaderReader
{
public:
    MtlxUsd_ConverterReader(const UsdMayaPrimReaderArgs& readArgs)
        : UsdMayaShaderReader(readArgs) {};

    bool IsConverter(UsdShadeShader& downstreamSchema, TfToken& downstreamOutputName) override
    {
        const UsdPrim& prim = _GetArgs().GetUsdPrim();
        UsdShadeShader shaderSchema = UsdShadeShader(prim);
        if (!shaderSchema) {
            return false;
        }

        TfToken shaderId;
        shaderSchema.GetIdAttr().Get(&shaderId);

        const UsdShadeInput& input = shaderSchema.GetInput(TrMtlxTokens->in);
        if (!input) {
            return false;
        }

        UsdShadeConnectableAPI source;
        TfToken                sourceOutputName;
        UsdShadeAttributeType  sourceType;
        if (!UsdShadeConnectableAPI::GetConnectedSource(
                input, &source, &sourceOutputName, &sourceType)) {
            return false;
        }

        downstreamSchema = UsdShadeShader(source.GetPrim());
        if (!downstreamSchema) {
            return false;
        }

        // No refinement necessary for ND_convert_color3_vector3 and ND_normalmap.

        if (shaderId.GetString().rfind("ND_luminance_", 0) != std::string::npos) {
            // Luminance is an alpha output.
            _setAlphaIsLuminance = true;
            _refinedOutputToken = TrMayaTokens->outAlpha;
        } else if (shaderId.GetString().rfind("ND_swizzle_", 0) != std::string::npos) {
            const UsdShadeInput& channelsAttr = shaderSchema.GetInput(TrMtlxTokens->channels);
            VtValue              val;
            if (channelsAttr.Get(&val, UsdTimeCode::Default()) && val.IsHolding<std::string>()) {
                const std::string& channels = val.UncheckedGet<std::string>();
                if (channels.size() == 1) {
                    // Single channel swizzles refines to a subcomponent
                    switch (channels[0]) {
                    case 'r':
                    case 'x': _refinedOutputToken = TrMayaTokens->outColorR; break;
                    case 'g': _refinedOutputToken = TrMayaTokens->outColorG; break;
                    case 'y':
                        if (shaderSchema.GetOutput(TrMtlxTokens->out).GetTypeName()
                            == SdfValueTypeNames->Float) {
                            _refinedOutputToken = TrMayaTokens->outAlpha;
                        } else {
                            _refinedOutputToken = TrMayaTokens->outColorG;
                        }
                        break;
                    case 'b':
                    case 'z': _refinedOutputToken = TrMayaTokens->outColorB; break;
                    case 'a':
                    case 'w': _refinedOutputToken = TrMayaTokens->outAlpha; break;
                    default: TF_CODING_ERROR("Unsupported swizzle");
                    }
                } else if (channels.size() == 3) {
                    // Triple channel swizzles must go to outColor:
                    _refinedOutputToken = TrMayaTokens->outColor;
                }
            }
        }
        _downstreamPrim = source.GetPrim();
        downstreamOutputName = sourceOutputName;

        return true;
    }

    void SetDownstreamReader(std::shared_ptr<UsdMayaShaderReader> downstreamReader) override
    {
        _downstreamReader = downstreamReader;
    }

    MObject GetCreatedObject(const UsdMayaShadingModeImportContext& context, const UsdPrim& prim)
        const override
    {
        if (_downstreamReader) {
            return _downstreamReader->GetCreatedObject(context, _downstreamPrim);
        }
        return MObject();
    }

    bool Read(UsdMayaPrimReaderContext& context) override
    {
        if (_downstreamReader) {
            return _downstreamReader->Read(context);
        }
        return false;
    }

    MPlug
    GetMayaPlugForUsdAttrName(const TfToken& usdAttrName, const MObject& mayaObject) const override
    {
        MPlug mayaPlug;
        if (_downstreamReader) {
            mayaPlug = _downstreamReader->GetMayaPlugForUsdAttrName(usdAttrName, mayaObject);

            if (mayaPlug.isNull() || _refinedOutputToken.IsEmpty()) {
                // Nothing tho refine.
                return mayaPlug;
            }

            if (_refinedOutputToken != TrMayaTokens->outColor
                && UsdMayaShadingUtil::GetStandardAttrName(mayaPlug, false)
                    != TrMayaTokens->outColor.GetString()) {
                // Already refined. Do not refine twice.
                return mayaPlug;
            }

            MFnDependencyNode depNodeFn(mayaPlug.node());

            if (_setAlphaIsLuminance) {
                MPlug alphaIsLuminancePlug
                    = depNodeFn.findPlug(TrMayaTokens->alphaIsLuminance.GetText());
                alphaIsLuminancePlug.setValue(true);
            }

            if (!_refinedOutputToken.IsEmpty()) {
                mayaPlug = depNodeFn.findPlug(_refinedOutputToken.GetText());
            }
        }
        return mayaPlug;
    }

private:
    std::shared_ptr<UsdMayaShaderReader> _downstreamReader;
    TfToken                              _refinedOutputToken;
    UsdPrim                              _downstreamPrim;
    bool                                 _setAlphaIsLuminance = false;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_luminance_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_luminance_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_color3_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_normalmap, MtlxUsd_ConverterReader);

PXR_NAMESPACE_CLOSE_SCOPE
