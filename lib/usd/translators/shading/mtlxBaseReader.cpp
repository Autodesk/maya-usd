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

#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/readUtil.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <cmath>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_SHADING_MODE_IMPORT_MATERIAL_CONVERSION(
    TrMtlxTokens->conversionName,
    TrMtlxTokens->contextName,
    TrMtlxTokens->niceName,
    TrMtlxTokens->importDescription);

namespace {
bool _IsConstructorNode(UsdPrim prim)
{
    UsdShadeShader ctorShader(prim);

    TfToken shaderId;
    ctorShader.GetIdAttr().Get(&shaderId);

    if (shaderId.GetString().rfind(TrMtlxTokens->CombinePrefix.GetString(), 0) == 0
        && ctorShader.GetPath().GetName().rfind(TrMtlxTokens->ConstructorPrefix.GetString(), 0)
            == 0) {
        return true;
    }
    return false;
}
} // namespace

MtlxUsd_BaseReader::MtlxUsd_BaseReader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

bool MtlxUsd_BaseReader::ReadShaderInput(
    UsdShadeShader&          shaderSchema,
    const TfToken&           attributeName,
    const MFnDependencyNode& depNodeFn,
    const bool               unlinearizeColors)
{
    UsdShadeInput input = shaderSchema.GetInput(attributeName);
    if (!input) {
        return false;
    }

    TfToken baseName = GetMayaNameForUsdAttrName(input.GetFullName());
    if (baseName.IsEmpty()) {
        return false;
    }

    MStatus status;
    MPlug   mayaAttr = depNodeFn.findPlug(baseName.GetText(), true, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    VtValue inputVal;
    if (!input.GetAttr().Get(&inputVal)) {
        return false;
    }

    if (UsdMayaReadUtil::SetMayaAttr(mayaAttr, inputVal, unlinearizeColors)) {
        UsdMayaReadUtil::SetMayaAttrKeyableState(mayaAttr, input.GetAttr().GetVariability());
    }

    return true;
}

bool MtlxUsd_BaseReader::GetColorAndAlphaFromInput(
    const UsdShadeShader& shader,
    const TfToken&        inputName,
    GfVec3f&              color,
    float&                alpha)
{
    UsdShadeInput usdInput = shader.GetInput(inputName);

    VtValue val;
    if (usdInput && usdInput.Get(&val)) {
        if (val.IsHolding<float>()) {
            // Mono: treat as rrr swizzle
            color[0] = val.UncheckedGet<float>();
            color[1] = color[0];
            color[2] = color[0];
        } else if (val.IsHolding<GfVec2f>()) {
            const GfVec2f& vecVal = val.UncheckedGet<GfVec2f>();
            // Mono + alpha: treat as rrr swizzle
            color[0] = vecVal[0];
            color[1] = vecVal[0];
            color[2] = vecVal[0];
        } else if (val.IsHolding<GfVec3f>()) {
            color = val.UncheckedGet<GfVec3f>();
        } else if (val.IsHolding<GfVec4f>()) {
            const GfVec4f& vecVal = val.UncheckedGet<GfVec4f>();
            color[0] = vecVal[0];
            color[1] = vecVal[1];
            color[2] = vecVal[2];
            alpha = vecVal[3];
        } else {
            return false;
        }
        return true;
    }
    return false;
}

bool MtlxUsd_BaseReader::TraverseUnconnectableInput(const TfToken& usdAttrName)
{
    TfToken               usdPortName;
    UsdShadeAttributeType attrType;
    std::tie(usdPortName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output) {
        return false;
    }

    // Check for the presence of a CTOR node indicating connection to a subcomponent of a compound
    // input:
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);

    UsdShadeInput usdInput = shaderSchema.GetInput(usdPortName);
    if (usdInput) {
        UsdShadeConnectableAPI source;
        TfToken                sourceInputName;
        UsdShadeAttributeType  sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(
                usdInput, &source, &sourceInputName, &sourceType)) {

            // Surface nodes will connect to a NodeGraph. We must dig deeper in this case:
            UsdShadeNodeGraph nodeGraph(source.GetPrim());
            if (nodeGraph) {
                UsdShadeOutput graphOutput = source.GetOutput(sourceInputName);
                if (!graphOutput) {
                    // Not a NodeGraph we recognize.
                    return false;
                }
                UsdShadeConnectableAPI::GetConnectedSource(
                    graphOutput, &source, &sourceInputName, &sourceType);
            }

            return _IsConstructorNode(source.GetPrim());
        }
    }

    return false;
}

void MtlxUsd_BaseReader::RegisterConstructorNodes(
    UsdMayaPrimReaderContext& context,
    MObject                   mayaObject)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);

    for (auto&& usdInput : shaderSchema.GetInputs()) {
        UsdShadeConnectableAPI source;
        TfToken                sourceInputName;
        UsdShadeAttributeType  sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(
                usdInput, &source, &sourceInputName, &sourceType)) {

            // Surface nodes will connect to a NodeGraph. We must dig deeper in this case:
            UsdShadeNodeGraph nodeGraph(source.GetPrim());
            if (nodeGraph) {
                UsdShadeOutput graphOutput = source.GetOutput(sourceInputName);
                if (!graphOutput) {
                    // Not a NodeGraph we recognize.
                    continue;
                }
                UsdShadeConnectableAPI::GetConnectedSource(
                    graphOutput, &source, &sourceInputName, &sourceType);
            }

            if (_IsConstructorNode(source.GetPrim())) {
                context.RegisterNewMayaNode(source.GetPath().GetString(), mayaObject);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
