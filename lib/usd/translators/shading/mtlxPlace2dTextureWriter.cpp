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
#include "mtlxBaseWriter.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class MtlxUsd_Place2dTextureWriter : public MtlxUsd_BaseWriter
{
public:
    MtlxUsd_Place2dTextureWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

private:
    void _ConnectVarnameInput(UsdShadeShader& primvarReaderSchema);
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(place2dTexture, MtlxUsd_Place2dTextureWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Primvar reader suffix:
    ((PrimvarReaderSuffix, "_MayaGeomPropValue"))
);
// clang-format on

namespace {

static const TfTokenVector _inputNames
    = { TrMayaTokens->coverage, TrMayaTokens->translateFrame, TrMayaTokens->rotateFrame,
        TrMayaTokens->mirrorU,  TrMayaTokens->mirrorV,        TrMayaTokens->stagger,
        TrMayaTokens->wrapU,    TrMayaTokens->wrapV,          TrMayaTokens->repeatUV,
        TrMayaTokens->offset,   TrMayaTokens->rotateUV,       TrMayaTokens->noiseUV };

} // namespace

// Was there any modified values on the place2dTexture node. If not, we can use a plain
// geompropvalue reader instead.
bool MtlxUsd_BaseWriter::IsAuthoredPlace2dTexture(const MFnDependencyNode& p2dTxFn)
{
    for (const TfToken& inputName : _inputNames) {
        MPlug plug = p2dTxFn.findPlug(inputName.GetText());
        if (UsdMayaUtil::IsAuthored(plug)) {
            return true;
        }
    }

    return false;
}

MtlxUsd_Place2dTextureWriter::MtlxUsd_Place2dTextureWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    // Everything must be added in the material node graph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not get UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    SdfPath nodegraphPath = nodegraphSchema.GetPath();
    SdfPath p2dTexPath
        = nodegraphPath.AppendChild(TfToken(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar())));

    UsdShadeShader p2dTexSchema = UsdShadeShader::Define(GetUsdStage(), p2dTexPath);
    if (!TF_VERIFY(
            p2dTexSchema, "Could not define UsdShadeShader at path '%s'\n", p2dTexPath.GetText())) {
        return;
    }

    _usdPrim = p2dTexSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            p2dTexPath.GetText())) {
        return;
    }

    if (IsAuthoredPlace2dTexture(depNodeFn)) {
        p2dTexSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_place2dTexture_vector2));
        p2dTexSchema.CreateOutput(TrMayaTokens->outUV, SdfValueTypeNames->Float2);
    } else {
        // Just install a reader to save space.
        p2dTexSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_geompropvalue_vector2));
        p2dTexSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);
        _ConnectVarnameInput(p2dTexSchema);
        return;
    }

    // Base class has created all the necessary node and inputs. We only need to make sure there is
    // a geompropvalue reader attached.
    MString readerName = depNodeFn.name();
    readerName += _tokens->PrimvarReaderSuffix.GetText();

    // Only create a geompropvalue reader if there is no place2dtexture connected:
    SdfPath primvarReaderPath = nodegraphSchema.GetPath().AppendChild(
        TfToken(UsdMayaUtil::SanitizeName(readerName.asChar())));

    if (!GetUsdStage()->GetPrimAtPath(primvarReaderPath)) {
        UsdShadeShader primvarReaderSchema
            = UsdShadeShader::Define(GetUsdStage(), primvarReaderPath);
        primvarReaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_geompropvalue_vector2));

        _ConnectVarnameInput(primvarReaderSchema);

        UsdShadeOutput primvarReaderOutput
            = primvarReaderSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);

        // Connect the output of the primvar reader to the texture coordinate
        // input of the UV texture.
        UsdShadeShader(_usdPrim)
            .CreateInput(TrMayaTokens->uvCoord, SdfValueTypeNames->Float2)
            .ConnectToSource(primvarReaderOutput);
    }
}

void MtlxUsd_Place2dTextureWriter::_ConnectVarnameInput(UsdShadeShader& primvarReaderSchema)
{

    const MFnDependencyNode depNodeFn(GetMayaObject());

    // Find the node connected to outUV, since it will be the one used for primvar resolution.
    const MPlug outPlug = depNodeFn.findPlug(TrMayaTokens->outUV.GetText());
    MPlugArray  fileNodes;
    outPlug.destinations(fileNodes);

    if (!fileNodes.length()) {
        return;
    }

    // In case of multiple connections, take the shortest name that comes first in sort order. We
    // are aiming for reproducible results here since all names are equally correct to use.
    std::string fileNodeName;

    for (unsigned int i = 0; i < fileNodes.length(); ++i) {
        const MFnDependencyNode fileNodeFn(fileNodes[i].node());
        std::string             nodeName = fileNodeFn.name().asChar();
        if (fileNodeName.empty() || nodeName.size() < fileNodeName.size()
            || nodeName < fileNodeName) {
            fileNodeName = nodeName;
        }
    }

    UsdShadeInput varnameInput
        = primvarReaderSchema.CreateInput(TrMtlxTokens->geomprop, SdfValueTypeNames->String);
    TfToken inputName(
        TfStringPrintf("%s:%s", fileNodeName.c_str(), TrMtlxTokens->varnameStr.GetText()));

    // We expose the primvar reader varnameStr attribute to the material to allow
    // easy specialization based on UV mappings to geometries:
    UsdPrim          materialPrim = primvarReaderSchema.GetPrim().GetParent();
    UsdShadeMaterial materialSchema(materialPrim);
    while (!materialSchema && materialPrim) {
        UsdShadeNodeGraph intermediateNodeGraph(materialPrim);
        if (intermediateNodeGraph) {
            UsdShadeInput intermediateInput
                = intermediateNodeGraph.CreateInput(inputName, SdfValueTypeNames->String);
            varnameInput.ConnectToSource(intermediateInput);
            varnameInput = intermediateInput;
        }

        materialPrim = materialPrim.GetParent();
        materialSchema = UsdShadeMaterial(materialPrim);
    }

    if (materialSchema) {
        UsdShadeInput materialInput
            = materialSchema.CreateInput(inputName, SdfValueTypeNames->String);
        materialInput.Set(UsdUtilsGetPrimaryUVSetName().GetString());
        varnameInput.ConnectToSource(materialInput);
    } else {
        varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
    }
}

/* virtual */
void MtlxUsd_Place2dTextureWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    TfToken shaderID;
    shaderSchema.GetIdAttr().Get(&shaderID);
    if (shaderID == TrMtlxTokens->ND_geompropvalue_vector2) {
        return;
    }

    for (const TfToken& inputName : _inputNames) {
        AuthorShaderInputFromShadingNodeAttr(depNodeFn, inputName, shaderSchema, usdTime);
    }
}

/* virtual */
TfToken
MtlxUsd_Place2dTextureWriter::GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
{
    UsdShadeShader nodeSchema(_usdPrim);
    if (!nodeSchema) {
        return {};
    }

    TfToken shaderID;
    nodeSchema.GetIdAttr().Get(&shaderID);

    if (shaderID == TrMtlxTokens->ND_geompropvalue_vector2) {
        if (mayaAttrName == TrMayaTokens->outUV) {
            return UsdShadeUtils::GetFullName(TrMtlxTokens->out, UsdShadeAttributeType::Output);
        }
        return {};
    }

    if (mayaAttrName == TrMayaTokens->outUV) {
        return UsdShadeUtils::GetFullName(mayaAttrName, UsdShadeAttributeType::Output);
    }
    return UsdShadeUtils::GetFullName(mayaAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
