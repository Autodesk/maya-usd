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

namespace {
const unsigned int INVALID_INDEX = -1;
}

// Very simple delegating converter for intermediate nodes added between an image node and
// a shader parameter when swizzling/conversion is required.
class MtlxUsd_ConverterReader : public UsdMayaShaderReader
{
public:
    MtlxUsd_ConverterReader(const UsdMayaPrimReaderArgs& readArgs)
        : UsdMayaShaderReader(readArgs) {};

    boost::optional<IsConverterResult> IsConverter() override
    {
        const UsdPrim& prim = _GetArgs().GetUsdPrim();
        UsdShadeShader shaderSchema = UsdShadeShader(prim);
        if (!shaderSchema) {
            return {};
        }

        TfToken shaderId;
        shaderSchema.GetIdAttr().Get(&shaderId);

        const UsdShadeInput& input = shaderSchema.GetInput(TrMtlxTokens->in);
        if (!input) {
            return {};
        }

        UsdShadeConnectableAPI source;
        TfToken                sourceOutputName;
        UsdShadeAttributeType  sourceType;
        if (!UsdShadeConnectableAPI::GetConnectedSource(
                input, &source, &sourceOutputName, &sourceType)) {
            return {};
        }

        UsdShadeShader downstreamSchema = UsdShadeShader(source.GetPrim());
        if (!downstreamSchema) {
            return {};
        }

        // No refinement necessary for ND_convert_color3_vector3 and ND_normalmap.

        if (shaderId.GetString().rfind("ND_luminance_", 0) != std::string::npos) {
            // Luminance is an alpha output.
            _setAlphaIsLuminance = true;
            _childIndex = 3;
        } else if (shaderId.GetString().rfind("ND_swizzle_", 0) != std::string::npos) {
            const UsdShadeInput& channelsAttr = shaderSchema.GetInput(TrMtlxTokens->channels);
            VtValue              val;
            if (channelsAttr.Get(&val, UsdTimeCode::Default()) && val.IsHolding<std::string>()) {
                const std::string& channels = val.UncheckedGet<std::string>();
                if (channels.size() == 1) {
                    // Single channel swizzles refines to a subcomponent
                    switch (channels[0]) {
                    case 'r':
                    case 'x': _childIndex = 0; break;
                    case 'g':
                    case 'y': _childIndex = 1; break;
                    case 'b':
                    case 'z': _childIndex = 2; break;
                    case 'a':
                    case 'w': _childIndex = 3; break;
                    default: TF_CODING_ERROR("Unsupported swizzle");
                    }
                }
            }
        }
        _downstreamPrim = source.GetPrim();

        if (sourceOutputName == TrMayaTokens->outColor && _childIndex == 3) {
            // Special case for RGBA outColor which never happens in Maya and are indeed connections
            // on outAlpha instead. Happens because we merged the channels on the MaterialX side to
            // work around multi-output issues.
            _downstreamOutputName
                = UsdShadeUtils::GetFullName(TrMayaTokens->outAlpha, UsdShadeAttributeType::Output);
            _childIndex = INVALID_INDEX;
        } else {
            _downstreamOutputName
                = UsdShadeUtils::GetFullName(sourceOutputName, UsdShadeAttributeType::Output);
        }
        return IsConverterResult { downstreamSchema, sourceOutputName };
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
            mayaPlug
                = _downstreamReader->GetMayaPlugForUsdAttrName(_downstreamOutputName, mayaObject);

            if (mayaPlug.isNull() || _childIndex == INVALID_INDEX) {
                // Nothing tho refine.
                return mayaPlug;
            }

            if (_childIndex != INVALID_INDEX && !mayaPlug.numChildren()) {
                // Already refined. Do not refine twice.
                return mayaPlug;
            }

            if (_setAlphaIsLuminance) {
                MFnDependencyNode depNodeFn(mayaPlug.node());
                MPlug             alphaIsLuminancePlug
                    = depNodeFn.findPlug(TrMayaTokens->alphaIsLuminance.GetText());
                alphaIsLuminancePlug.setValue(true);
            }

            if (_childIndex != INVALID_INDEX) {
                mayaPlug = mayaPlug.child(_childIndex);
            }
        }
        return mayaPlug;
    }

private:
    std::shared_ptr<UsdMayaShaderReader> _downstreamReader;
    unsigned int                         _childIndex = INVALID_INDEX;
    UsdPrim                              _downstreamPrim;
    TfToken                              _downstreamOutputName;
    bool                                 _setAlphaIsLuminance = false;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_luminance_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_luminance_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_float_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector2_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector3_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector3_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector3_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector3_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector3_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector4_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector4_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector4_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector4_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_vector4_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color3_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_float, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_swizzle_color4_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_float_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_float_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_float_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_float_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_color3_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_color3_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_color4_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_color4_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_vector3_vector2, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_vector3_color3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_vector3_vector4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_vector4_color4, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_convert_vector4_vector3, MtlxUsd_ConverterReader);
PXRUSDMAYA_REGISTER_SHADER_READER(ND_normalmap, MtlxUsd_ConverterReader);

PXR_NAMESPACE_CLOSE_SCOPE
