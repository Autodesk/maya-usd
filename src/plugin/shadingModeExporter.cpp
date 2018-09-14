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
    void _ExportNode(UsdStagePtr& stage, HdMaterialNode& hdNode) {
        UsdShadeShader shaderSchema = UsdShadeShader::Define(
                stage, hdNode.path);
        shaderSchema.CreateIdAttr(VtValue(hdNode.identifier));
        for(auto& paramNameVal: hdNode.parameters) {
            auto& paramName = paramNameVal.first;
            auto& paramVal = paramNameVal.second;
            UsdShadeInput input = shaderSchema.CreateInput(
                    paramName, SdfGetValueTypeNameForValue(paramVal));
            input.Set(paramVal);
        }
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

        for (auto& hdNode : materialNetwork.nodes) {
            _ExportNode(stage, hdNode);
            if (hdNode.path == hdSurf) {
                UsdShadeOutput surfaceOutput = material
                        .CreateSurfaceOutput(GlfGLSLFXTokens->glslfx);
                UsdShadeConnectableAPI::ConnectToSource(
                    surfaceOutput,
                    hdNode.path.IsPropertyPath() ? hdNode.path :
                            hdNode.path.AppendProperty(_tokens->defaultOutputName));
            }
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
