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
#include "usdMaterialReader.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReader.h>
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
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

PxrUsdTranslators_MaterialReader::PxrUsdTranslators_MaterialReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrUsdTranslators_MaterialReader::Read(UsdMayaPrimReaderContext& context)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    const TfToken&    mayaNodeTypeName = _GetMayaNodeTypeName();
    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              mayaNodeTypeName.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            mayaNodeTypeName.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        if (input.GetBaseName() == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName) {
            // We need a displacementShader:
            std::string shaderName = prim.GetName().GetString();
            shaderName += "_displacement";
            UsdMayaTranslatorUtil::CreateShaderNode(
                shaderName.c_str(),
                "displacementShader",
                UsdMayaShadingNodeType::Shader,
                &status,
                &_displacementShader);
            // this displacement shader will get connected as we explore
            // the displacement input of the UsdShadeMaterial.
        }

        TfToken baseName = GetMayaNameForUsdAttrName(input.GetFullName());
        if (baseName.IsEmpty()) {
            continue;
        }
        _OnBeforeReadAttribute(baseName, depFn);
        MPlug mayaAttr = GetMayaPlugForUsdAttrName(input.GetFullName(), mayaObject);
        if (mayaAttr.isNull()) {
            continue;
        }
        VtValue val;
        if (input.Get(&val)) {
            _ConvertToMaya(baseName, val);
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
        }
    }

    return true;
}

/* virtual */
MPlug PxrUsdTranslators_MaterialReader::GetMayaPlugForUsdAttrName(
    const TfToken& usdAttrName,
    const MObject& mayaObject) const
{
    TfToken baseName = UsdShadeUtils::GetBaseNameAndType(usdAttrName).first;
    // We return the same R/W plug for input and output if it is the displacement attribute.
    if (baseName == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName
        && !_displacementShader.isNull()) {
        MStatus           status;
        MFnDependencyNode depFn(_displacementShader, &status);
        if (status != MS::kSuccess) {
            return MPlug();
        }

        return depFn.findPlug(GetMayaNameForUsdAttrName(usdAttrName).GetText());
    }

    return UsdMayaShaderReader::GetMayaPlugForUsdAttrName(usdAttrName, mayaObject);
}

/* virtual */
TfToken
PxrUsdTranslators_MaterialReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdOutputName;
    UsdShadeAttributeType attrType;
    std::tie(usdOutputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (usdOutputName == UsdShadeTokens->displacement && !_displacementShader.isNull()) {
        return usdOutputName;
    } else if (attrType == UsdShadeAttributeType::Output) {
        if (usdOutputName == UsdShadeTokens->surface) {
            return TrMayaTokens->outColor;
        }
    }

    return TfToken();
}

void PxrUsdTranslators_MaterialReader::_ConvertToMaya(const TfToken&, VtValue&) const
{
    // Nothing to do
    return;
}

void PxrUsdTranslators_MaterialReader::_OnBeforeReadAttribute(const TfToken&, MFnDependencyNode&)
    const
{
    // Nothing to do
    return;
}

PXR_NAMESPACE_CLOSE_SCOPE
