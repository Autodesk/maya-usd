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

class MtlxUsd_PreviewSurfaceReader : public MtlxUsd_BaseReader
{
public:
    MtlxUsd_PreviewSurfaceReader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_UsdPreviewSurface_surfaceshader, MtlxUsd_PreviewSurfaceReader);

MtlxUsd_PreviewSurfaceReader::MtlxUsd_PreviewSurfaceReader(const UsdMayaPrimReaderArgs& readArgs)
    : MtlxUsd_BaseReader(readArgs)
{
}

/* virtual */
bool MtlxUsd_PreviewSurfaceReader::Read(UsdMayaPrimReaderContext& context)
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
              TrMayaTokens->usdPreviewSurface.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        TF_RUNTIME_ERROR(
            "Could not create node of type %s for shader '%s'.\n",
            TrMayaTokens->standardSurface.GetText(),
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
        if (!input.GetAttr().Get(&inputVal)) {
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
TfToken MtlxUsd_PreviewSurfaceReader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               baseName;
    UsdShadeAttributeType attrType;
    std::tie(baseName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Input) {
        return baseName;
    } else if (attrType == UsdShadeAttributeType::Output && baseName == UsdShadeTokens->surface) {
        return TrMayaTokens->outColor;
    }
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
