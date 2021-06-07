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

#include "mtlxBaseWriter.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <unordered_map>

using namespace MAYAUSD_NS;

PXR_NAMESPACE_OPEN_SCOPE

// This is basically UsdMayaSymmetricShaderWriter but with a NodeGraph:
class MtlxUsd_PreviewSurfaceWriter : public MtlxUsd_BaseWriter
{
public:
    MtlxUsd_PreviewSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute
    GetShadingAttributeForMayaAttrName(const TfToken&, const SdfValueTypeName&) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(usdPreviewSurface, MtlxUsd_PreviewSurfaceWriter);

MtlxUsd_PreviewSurfaceWriter::MtlxUsd_PreviewSurfaceWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    shaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_UsdPreviewSurface_surfaceshader));

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not define UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    for (const TfToken& mayaAttrName : PxrMayaUsdPreviewSurfaceTokens->allTokens) {

        if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName) {
            // Done with inputs.
            break;
        }

        MPlug attrPlug = depNodeFn.findPlug(mayaAttrName.GetText(), true);

        // Keep our authoring sparse by ignoring attributes with no values set
        // and no connections. We know that the default value of base and base
        // color diverged between Maya and MaterialX in version 1.38.
        if (!UsdMayaUtil::IsAuthored(attrPlug) && !attrPlug.isConnected()) {
            continue;
        }

        const SdfValueTypeName valueTypeName = MayaUsd::Converter::getUsdTypeName(attrPlug);
        if (!valueTypeName) {
            // Unsupported Maya attribute type (e.g. "message" attributes).
            continue;
        }

        UsdShadeInput input = shaderSchema.CreateInput(mayaAttrName, valueTypeName);
        if (!input) {
            continue;
        }

        if (attrPlug.isElement()) {
            UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(), 0u);
        }

        // Add this input to the name/attrPlug map. We'll iterate through
        // these entries during Write() to set their values.
        _inputNameAttrMap.insert(std::make_pair(mayaAttrName, attrPlug));

        // All connections go directly to the node graph:
        if (attrPlug.isConnected()) {
            UsdShadeOutput ngOutput = nodegraphSchema.CreateOutput(mayaAttrName, valueTypeName);
            input.ConnectToSource(ngOutput);
        }
    }

    // Surface Output
    shaderSchema.CreateOutput(UsdShadeTokens->surface, SdfValueTypeNames->Token);
}

/* override */
void MtlxUsd_PreviewSurfaceWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    for (const auto& inputAttrPair : _inputNameAttrMap) {
        const TfToken& inputName = inputAttrPair.first;
        const MPlug&   attrPlug = inputAttrPair.second;

        UsdShadeInput input = shaderSchema.GetInput(inputName);
        if (!input || attrPlug.isConnected()) {
            continue;
        }

        // Color values are all linear on the shader, so do not re-linearize
        // them.
        VtValue value = UsdMayaWriteUtil::GetVtValue(
            attrPlug,
            Converter::getUsdTypeName(attrPlug),
            /* linearizeColors = */ false);

        input.Set(value, usdTime);
    }
}

/* override */
UsdAttribute MtlxUsd_PreviewSurfaceWriter::GetShadingAttributeForMayaAttrName(
    const TfToken& mayaAttrName,
    const SdfValueTypeName&)
{
    if (mayaAttrName == TrMayaTokens->outColor) {
        UsdShadeShader surfaceSchema(_usdPrim);
        if (!surfaceSchema) {
            return UsdAttribute();
        }

        // Surface output is on the shader itself
        return surfaceSchema.GetOutput(UsdShadeTokens->surface);
    }

    // All other are outputs of the NodeGraph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!nodegraphSchema) {
        return UsdAttribute();
    }

    return nodegraphSchema.GetOutput(mayaAttrName);
}

PXR_NAMESPACE_CLOSE_SCOPE
