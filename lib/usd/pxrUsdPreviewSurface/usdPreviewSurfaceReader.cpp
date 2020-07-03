//
// Copyright 2018 Pixar
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
#include "usdPreviewSurface.h"

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
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPreviewSurface_Reader : public UsdMayaShaderReader {
public:
    PxrMayaUsdPreviewSurface_Reader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext* context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdPreviewSurface, PxrMayaUsdPreviewSurface_Reader)

PxrMayaUsdPreviewSurface_Reader::PxrMayaUsdPreviewSurface_Reader(
    const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrMayaUsdPreviewSurface_Reader::Read(UsdMayaPrimReaderContext* context)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MStatus           status;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              PxrMayaUsdPreviewSurfaceTokens->MayaTypeName.GetText(),
              UsdMayaShadingNodeType::Shader,
              &status,
              &_mayaObject)
          && depFn.setObject(_mayaObject))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'. "
            "Probably missing a loadPlugin.\n",
            PxrMayaUsdPreviewSurfaceTokens->MayaTypeName.GetText(),
            prim.GetName().GetText());
        return false;
    }

    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        auto mayaAttrName = GetMayaNameForUsdAttrName(input.GetFullName());
        if (mayaAttrName == TfToken()) {
            continue;
        }
        MPlug mayaAttr = depFn.findPlug(mayaAttrName.GetText(), true, &status);
        if (status == MS::kSuccess) {
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, input);
        }
    }

    return true;
}

/* virtual */
TfToken PxrMayaUsdPreviewSurface_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName)
{
    if (_mayaObject.isNull()) {
        return TfToken();
    }

    const auto& usdName(usdAttrName.GetString());
    if (TfStringStartsWith(usdName, UsdShadeTokens->inputs)) {
        TfToken mayaAttrName = TfToken(usdName.substr(UsdShadeTokens->inputs.GetString().size()));
        if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName
            || mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName) {
            return mayaAttrName;
        }
    } else if (TfStringStartsWith(usdName, UsdShadeTokens->outputs)) {
        TfToken mayaAttrName = TfToken(usdName.substr(UsdShadeTokens->outputs.GetString().size()));
        if (mayaAttrName == UsdShadeTokens->surface) {
            return PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName;
        }
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
