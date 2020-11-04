//
// Copyright 2019 Luma Pictures
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
#include <hdMaya/adapters/materialNetworkConverter.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <pxr/imaging/hio/glslfx.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/shader.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((defaultOutputName, "outputs:out"))
    ((glslfxSurface, "glslfx:surface"))
);
// clang-format off

class MtohShadingModeExporter : public UsdMayaShadingModeExporter {
public:
    MtohShadingModeExporter() = default;
    ~MtohShadingModeExporter() = default;

protected:
    bool _ExportNode(UsdStagePtr& stage, HdMaterialNode& hdNode) {
        UsdShadeShader shaderSchema =
            UsdShadeShader::Define(stage, hdNode.path);
        if (!TF_VERIFY(shaderSchema)) { return false; }
        if (!TF_VERIFY(shaderSchema.CreateIdAttr(VtValue(hdNode.identifier)))) {
            return false;
        }
        bool success = true;
        for (auto& paramNameVal : hdNode.parameters) {
            auto& paramName = paramNameVal.first;
            auto& paramVal = paramNameVal.second;
            UsdShadeInput input = shaderSchema.CreateInput(
                paramName, SdfGetValueTypeNameForValue(paramVal));
            if (!TF_VERIFY(input)) {
                success = false;
                continue;
            }
            if (!TF_VERIFY(input.Set(paramVal))) { success = false; }
        }
        return success;
    }

    bool _ExportRelationship(
        UsdStagePtr& stage, HdMaterialRelationship& relationship) {
        // TODO: come up with a better way for determining type rather than
        // relying on the input or output to already be set, so we can read it's
        // type...
        // probably use the shader registry (?), though I don't think the
        // PreviewSurface is actually registered there yet
        SdfValueTypeName typeName;

        // The following segment can be confusing at first. Output and input
        // have two different meaning. In a Hydra context, like
        // HdMaterialRelationship connections go from input to output.
        // On USD primitives connections go from parameters in the
        // outputs namespace to parameters in the inputs namespace.
        // This is why the meaning is seemingly reversed, even though
        // they represent two different concepts.
        // Hydra is using input and output for connections, while USD is
        // using inputs and outputs for the role of parameters on a prim.
        auto inputPrim = stage->GetPrimAtPath(relationship.outputId);
        if (!TF_VERIFY(inputPrim)) { return false; }
        UsdShadeShader inputShader(inputPrim);
        if (!TF_VERIFY(inputShader)) { return false; }
        UsdShadeInput input = inputShader.GetInput(relationship.outputName);
        if (input) { typeName = input.GetTypeName(); }

        auto outputPrim = stage->GetPrimAtPath(relationship.inputId);
        if (!TF_VERIFY(outputPrim)) { return false; }
        UsdShadeShader outputShader(outputPrim);
        if (!TF_VERIFY(outputShader)) { return false; }
        UsdShadeInput output = outputShader.GetInput(relationship.inputName);
        if (output) {
            if (!typeName) {
                typeName = output.GetTypeName();
            } else if (typeName != output.GetTypeName()) {
                TF_WARN(
                    "Types of inputs and outputs did not match: "
                    "input %s.%s was %s, output %s.%s was %s",
                    relationship.outputId.GetText(),
                    relationship.outputName.GetText(),
                    typeName.GetAsToken().GetText(),
                    relationship.inputId.GetText(),
                    relationship.inputName.GetText(),
                    output.GetTypeName().GetAsToken().GetText());
                return false;
            }
        }

        if (!typeName) { typeName = SdfValueTypeNames->Token; }

        if (!input) {
            input = inputShader.CreateInput(relationship.outputName, typeName);
            if (!TF_VERIFY(input)) { return false; }
        }
        if (output) {
            return UsdShadeConnectableAPI::ConnectToSource(input, output);
        }
        return UsdShadeConnectableAPI::ConnectToSource(
            input, outputShader, relationship.inputName,
            UsdShadeAttributeType::Output, typeName);
    }

public:
    void Export(
        const UsdMayaShadingModeExportContext& context,
        UsdShadeMaterial* const mat,
        SdfPathSet* const boundPrimPaths) override {
        const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
            context.GetAssignments();
        if (assignments.empty()) { return; }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments);
        context.BindStandardMaterialPrim(materialPrim, assignments, boundPrimPaths);
        UsdShadeMaterial material(materialPrim);
        if (!material) { return; }

        if (mat != nullptr) { *mat = material; }

        HdMaterialNetwork materialNetwork;
        HdMayaMaterialNetworkConverter converter(
            materialNetwork, materialPrim.GetPath());
        auto* hdSurfMat = converter.GetMaterial(context.GetSurfaceShader());
        if(!hdSurfMat) { return; }
        SdfPath hdSurfPath = hdSurfMat->path;

        // TODO: add support for volume / displacement
        // SdfPath hdVol = converter.GetMaterial(context.GetVolumeShader()).path;
        // SdfPath hdDisp =
        // converter.GetMaterial(context.GetDisplacementShader()).path;

        if (hdSurfPath.IsEmpty()) { return; }

        UsdStagePtr stage = materialPrim.GetStage();

        // Generate nodes
        for (auto& hdNode : materialNetwork.nodes) {
            if (!TF_VERIFY(_ExportNode(stage, hdNode))) { continue; }
            if (hdNode.path == hdSurfPath) {
                // Hook up the glslfx surface output
                UsdShadeOutput surfaceOutput =
                    material.CreateSurfaceOutput(HioGlslfxTokens->glslfx);
                auto outputProperty = hdNode.path.IsPropertyPath()
                        ? hdNode.path
                        : hdNode.path.AppendProperty(
                                _tokens->defaultOutputName);
                if (TF_VERIFY(surfaceOutput)) {
                    UsdShadeConnectableAPI::ConnectToSource(
                        surfaceOutput, outputProperty);
                }

                // If there is no universal surface output, hook that up too
                if ( (surfaceOutput = material.GetSurfaceOutput()) ) {
                    UsdShadeConnectableAPI::ConnectToSource(
                        surfaceOutput, outputProperty);
                }

            }
        }

        // Make connections
        for (auto& relationship : materialNetwork.relationships) {
            _ExportRelationship(stage, relationship);
        }
    }
};

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, mtoh) {
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "mtoh",
        "MtoH",
        "",
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(new MtohShadingModeExporter());
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
