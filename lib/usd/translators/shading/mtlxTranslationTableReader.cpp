//
// Copyright 2024 Autodesk
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
#include "mtlxTranslationTableReader.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
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
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

MtlxUsd_TranslationTableReader::MtlxUsd_TranslationTableReader(
    const UsdMayaPrimReaderArgs& readArgs)
    : MtlxUsd_BaseReader(readArgs)
{
}

namespace {

// Read a value that was set on a UsdShadeMaterial instead of the UsdShadeShader. This is something
// we see with materials imported by UsdMtlx:
bool _ReadFromMaterial(const UsdShadeInput& input, VtValue& inputVal)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceInputName;
    UsdShadeAttributeType  sourceType;
    if (!UsdShadeConnectableAPI::GetConnectedSource(
            input, &source, &sourceInputName, &sourceType)) {
        return false;
    }

    UsdShadeMaterial sourceMaterial(source.GetPrim());
    if (!sourceMaterial) {
        return false;
    }

    const UsdShadeInput& materialInput = sourceMaterial.GetInput(sourceInputName);
    if (!materialInput) {
        return false;
    }

    return materialInput.Get(&inputVal);
}

} // namespace
/* virtual */
bool MtlxUsd_TranslationTableReader::Read(UsdMayaPrimReaderContext& context)
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              getMaterialName().GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        TF_RUNTIME_ERROR(
            "Could not create node of type %s for shader '%s'.\n",
            getMaterialName().GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context.RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);
    RegisterConstructorNodes(context, mayaObject);

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        TfToken baseName = GetMayaNameForUsdAttrName(input.GetFullName());
        if (baseName.IsEmpty()) {
            continue;
        }
        MPlug mayaAttr = depFn.findPlug(baseName.GetText(), true, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        VtValue inputVal;
        if (!input.GetAttr().Get(&inputVal) && !_ReadFromMaterial(input, inputVal)) {
            continue;
        }

        if (UsdMayaReadUtil::SetMayaAttr(
                mayaAttr,
                inputVal,
                /* unlinearizeColors = */ false)) {
            UsdMayaReadUtil::SetMayaAttrKeyableState(mayaAttr, input.GetAttr().GetVariability());
        }
    }

    return true;
}

/* virtual */
TfToken MtlxUsd_TranslationTableReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               baseName;
    UsdShadeAttributeType attrType;
    std::tie(baseName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        auto it = getTranslationTable().find(baseName);
        if (it != getTranslationTable().end()) {
            return it->second;
        }
    } else if (attrType == UsdShadeAttributeType::Output && baseName == UsdShadeTokens->surface) {
        return getOutputName();
    }
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
