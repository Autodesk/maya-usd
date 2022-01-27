//
// Copyright 2021 Animal Logic
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

#include <pxr/usd/usdShade/shader.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPrimvarReader_float3_Reader : public UsdMayaShaderReader
{
public:
    PxrMayaUsdPrimvarReader_float3_Reader(const UsdMayaPrimReaderArgs& readArgs);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPrimvarReader_float3, PxrMayaUsdPrimvarReader_float3_Reader)

PxrMayaUsdPrimvarReader_float3_Reader::PxrMayaUsdPrimvarReader_float3_Reader(
    const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrMayaUsdPrimvarReader_float3_Reader::Read(UsdMayaPrimReaderContext& context)
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MObject       obj;
    VtValue       val;
    UsdShadeInput usdInput = shaderSchema.GetInput(TrUsdTokens->varname);
    if (usdInput && usdInput.Get(&val) && val.IsHolding<std::string>()
        && val.UncheckedGet<std::string>() == TrUsdTokens->displayColor.GetString()) {
        // Create a CPV color node in place of USD's displayColor primvar reader
        MStatus           status;
        MFnDependencyNode depFn;
        obj = depFn.create(TrMayaTokens->cpvColor.GetText(), &status);
        if (status != MS::kSuccess) {
            TF_RUNTIME_ERROR(
                "Could not create node of type %s for prim '%s'. ",
                TrMayaTokens->cpvColor.GetText(),
                prim.GetPath().GetText());
            return false;
        }
    }

    if (obj.isNull()) {
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), obj);
    return true;
}

/* virtual */
TfToken
PxrMayaUsdPrimvarReader_float3_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               baseName;
    UsdShadeAttributeType attrType;
    std::tie(baseName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output) {
        if (baseName == TrUsdTokens->result) {
            return TrMayaTokens->outColor;
        }
    }
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE