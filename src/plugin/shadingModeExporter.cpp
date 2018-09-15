//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <pxr/pxr.h>

#include <pxr/imaging/glf/glslfx.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/shader.h>

#include <usdMaya/shadingModeExporter.h>
#include <usdMaya/shadingModeRegistry.h>

#include <hdmaya/adapters/materialNetworkConverter.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((defaultOutputName, "outputs:out"))
    ((glslfxSurface, "glslfx:surface"))
);

class MtohShadingModeExporter : public UsdMayaShadingModeExporter {
public:
    MtohShadingModeExporter() = default;
    ~MtohShadingModeExporter() = default;

protected:
    bool _ExportNode(UsdStagePtr& stage, HdMaterialNode& hdNode) {
        UsdShadeShader shaderSchema = UsdShadeShader::Define(
                stage, hdNode.path);
        if (!TF_VERIFY(shaderSchema)) { return false; }
        if (!TF_VERIFY(shaderSchema.CreateIdAttr(VtValue(hdNode.identifier)))) {
            return false;
        }
        bool success = true;
        for(auto& paramNameVal: hdNode.parameters) {
            auto& paramName = paramNameVal.first;
            auto& paramVal = paramNameVal.second;
            UsdShadeInput input = shaderSchema.CreateInput(
                    paramName, SdfGetValueTypeNameForValue(paramVal));
            if (!TF_VERIFY(input)) {
                success = false;
                continue;
            }
            if (!TF_VERIFY(input.Set(paramVal))) {
                success = false;
            }
        }
        return success;
    }

    bool _ExportRelationship(
            UsdStagePtr& stage,
            HdMaterialRelationship& relationship) {
        // TODO: come up with a better way for determining type rather than
        // relying on the input or output to already be set, so we can read it's
        // type...
        // probably use the shader registry (?), though I don't think the
        // PreviewSurface is actually registered there yet
        SdfValueTypeName typeName;

        auto inputPrim = stage->GetPrimAtPath(relationship.inputId);
        if (!TF_VERIFY(inputPrim)) { return false; }
        UsdShadeShader inputShader(inputPrim);
        if (!TF_VERIFY(inputShader)) { return false; }
        UsdShadeInput input = inputShader.GetInput(relationship.inputName);
        if (input) {
            typeName = input.GetTypeName();
        }

        auto outputPrim = stage->GetPrimAtPath(relationship.outputId);
        if (!TF_VERIFY(outputPrim)) { return false; }
        UsdShadeShader outputShader(outputPrim);
        if (!TF_VERIFY(outputShader)) { return false; }
        UsdShadeInput output = outputShader.GetInput(relationship.outputName);
        if (output) {
            if (!typeName) {
                typeName = output.GetTypeName();
            } else if (typeName != output.GetTypeName()) {
                TF_WARN("Types of inputs and outputs did not match: "
                        "input %s.%s was %s, output %s.%s was %s",
                        relationship.inputId.GetText(),
                        relationship.inputName.GetText(),
                        typeName.GetAsToken().GetText(),
                        relationship.outputId.GetText(),
                        relationship.outputName.GetText(),
                        output.GetTypeName().GetAsToken().GetText());
                return false;
            }
        }

        if (!typeName) {
            typeName = SdfValueTypeNames->Token;
        }

        if (!input) {
            input = inputShader.CreateInput(relationship.inputName, typeName);
            if (!TF_VERIFY(input)) { return false; }
        }
        if (output) {
            return UsdShadeConnectableAPI::ConnectToSource(input, output);
        }
        return UsdShadeConnectableAPI::ConnectToSource(input,
                outputShader, relationship.outputName,
                UsdShadeAttributeType::Output, typeName);
    }

public:
    virtual void Export(
        const UsdMayaShadingModeExportContext& context,
        UsdShadeMaterial* const mat,
        SdfPathSet* const boundPrimPaths) override {
        const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
                context.GetAssignments();
        if (assignments.empty()) {
            return;
        }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments,
                std::string(), boundPrimPaths);
        UsdShadeMaterial material(materialPrim);
        if (!material) {
            return;
        }

        if (mat != nullptr) {
            *mat = material;
        }

        HdMaterialNetwork materialNetwork;
        HdMayaMaterialNetworkConverter converter(materialNetwork,
                materialPrim.GetPath());
        SdfPath hdSurf = converter.GetMaterial(context.GetSurfaceShader());

        // TODO: add support for volume / displacement
        //SdfPath hdVol = converter.GetMaterial(context.GetVolumeShader());
        //SdfPath hdDisp = converter.GetMaterial(context.GetDisplacementShader());

        if (hdSurf.IsEmpty()) {
            return;
        }

        UsdStagePtr stage = materialPrim.GetStage();

        // Generate nodes
        for (auto& hdNode : materialNetwork.nodes) {
            if (!TF_VERIFY(_ExportNode(stage, hdNode))) {
                continue;
            }
            if (hdNode.path == hdSurf) {
                UsdShadeOutput surfaceOutput = material
                        .CreateSurfaceOutput(GlfGLSLFXTokens->glslfx);
                if (TF_VERIFY(surfaceOutput)) {
                    UsdShadeConnectableAPI::ConnectToSource(
                        surfaceOutput,
                        hdNode.path.IsPropertyPath() ? hdNode.path :
                                hdNode.path.AppendProperty(_tokens->defaultOutputName));
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
        "mtoh", []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(new MtohShadingModeExporter());
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
