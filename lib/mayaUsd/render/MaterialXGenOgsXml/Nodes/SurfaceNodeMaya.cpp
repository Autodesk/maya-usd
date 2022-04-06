//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include "SurfaceNodeMaya.h"

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>

MATERIALX_NAMESPACE_BEGIN

namespace {
const string MX_MAYA_EXTERNAL_LIGHTS = "mayaExternalLightFunctions";
}

SurfaceNodeMaya::SurfaceNodeMaya()
    : SurfaceNodeGlsl()
{
}

ShaderNodeImplPtr SurfaceNodeMaya::create() { return std::make_shared<SurfaceNodeMaya>(); }

void SurfaceNodeMaya::createVariables(const ShaderNode& node, GenContext& context, Shader& shader)
    const
{
    SurfaceNodeGlsl::createVariables(node, context, shader);
    ShaderStage& ps = shader.getStage(Stage::PIXEL);

    addStageUniform(HW::PRIVATE_UNIFORMS, Type::INTEGER, MX_MAYA_EXTERNAL_LIGHTS, ps);
}

void SurfaceNodeMaya::emitLightLoop(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage,
    const string&     outColor) const
{
    const GlslShaderGenerator& shadergen
        = static_cast<const GlslShaderGenerator&>(context.getShaderGenerator());

    const ShaderInput* bsdfInput = node.getInput("bsdf");
    const ShaderNode*  bsdf = bsdfInput->getConnectedSibling();

    shadergen.emitComment("Light loop", stage);

#if MAYA_LIGHTAPI_VERSION_2 == 3
    // MayaUSD issue #2121: Flat lighting not used in Maya 2022.3
    //
    // MaterialX has no concept of flat lighting or ambient lights, so we must find something that
    // looks like a diffuse node and see what color it has.
    auto getParam = [](ShaderInput* in) -> std::string {
        if (!in->getConnection() && in->getValue()) {
            return in->getValue()->getValueString();
        } else if (in->getConnection()) {
            return in->getConnection()->getVariable();
        } else {
            return {};
        }
    };

    ShaderInput* ncBsdfInput = const_cast<ShaderInput*>(bsdfInput);
    ShaderNode*  emittedDiffuseNode = nullptr;
    for (ShaderGraphEdge edge : ShaderGraph::traverseUpstream(ncBsdfInput->getConnection())) {
        const std::string& implName = edge.upstream->getNode()->getImplementation().getName();
        if (implName == "IM_oren_nayar_diffuse_bsdf_genglsl"
            || implName == "IM_burley_diffuse_bsdf_genglsl") {
            // Found a diffuse base which could potentially support ambient lighting:
            ShaderInput* weightInput = edge.upstream->getNode()->getInput("weight");
            ShaderInput* colorInput = edge.upstream->getNode()->getInput("color");
            if (!weightInput || !colorInput) {
                continue;
            }
            // Are they connected to something that either provides a variable name or a value?
            std::string weightParam = getParam(weightInput);
            std::string colorParam = getParam(colorInput);
            if (weightParam.empty() || colorParam.empty()) {
                continue;
            }

            // We can hit multiple diffuse nodes in layered shaders. How should that work for
            // flat lighting is currently undefined. Just report the issue.
            if (emittedDiffuseNode) {
#ifdef DEBUG
                if (emittedDiffuseNode != edge.upstream->getNode()) {
                    throw mx::Exception(
                        "Multiple diffuse nodes found: " + edge.upstream->getNode()->getName()
                        + " could also contribute to ambient lighting.");
                }
#endif
                continue;
            }

            shadergen.emitLine(
                outColor + " = mayaGetAmbientLightColor() * " + weightParam + " * " + colorParam,
                stage);
            emittedDiffuseNode = edge.upstream->getNode();
        }
    }
#endif

    shadergen.emitLine("int numLights = mayaGetNumLights()", stage);
    shadergen.emitLine("irradiance lightShader", stage);
    shadergen.emitLine(
        "for (int activeLightIndex = 0; activeLightIndex < numLights; ++activeLightIndex)",
        stage,
        false);

    shadergen.emitScopeBegin(stage);

    shadergen.emitLine("lightShader = mayaGetLightIrradiance(activeLightIndex, P, N, V)", stage);
    shadergen.emitLine("vec3 L = lightShader.Ld", stage);
    shadergen.emitLineBreak(stage);

    shadergen.emitComment("Calculate the BSDF response for this light source", stage);
    context.pushClosureContext(&_callReflection);
    shadergen.emitFunctionCall(*bsdf, context, stage);
    context.popClosureContext();

    shadergen.emitComment("Accumulate the light's contribution", stage);
    shadergen.emitLine(
        outColor + " += lightShader.specularI * " + bsdf->getOutput()->getVariable() + ".response",
        stage);

    shadergen.emitScopeEnd(stage);
    shadergen.emitLineBreak(stage);
}

MATERIALX_NAMESPACE_END
