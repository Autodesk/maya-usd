//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include "GlslFragmentGenerator.h"

#include "Nodes/MayaDarkClosureNode.h"
#include "Nodes/MayaShaderGraph.h"
#include "Nodes/SurfaceNodeMaya.h"
#include "Nodes/TexcoordNodeMaya.h"
#if MX_COMBINED_VERSION < 13809
#include "Nodes/MayaTransformNormalNodeGlsl.h"
#include "Nodes/MayaTransformPointNodeGlsl.h"
#include "Nodes/MayaTransformVectorNodeGlsl.h"
#endif
#ifdef FIX_NODEGRAPH_UDIM_SCALE_OFFSET
#include "Nodes/MayaCompoundNode.h"
#include "Nodes/MayaHwImageNode.h"
#endif
#ifdef FIX_DUPLICATE_INCLUDED_SHADER_CODE
#include "Nodes/MayaSourceCodeNode.h"
#endif
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
#include "Nodes/MayaClosureSourceCodeNode.h"
#endif

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>
#include <mayaUsd/render/MaterialXGenOgsXml/GlslOcioNodeImpl.h>
#include <mayaUsd/render/MaterialXGenOgsXml/LobePruner.h>
#include <mayaUsd/render/MaterialXGenOgsXml/OgsXmlGenerator.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>

#include <regex>

MATERIALX_NAMESPACE_BEGIN

namespace Stage {
const string UNIFORMS = "uniforms";
}

namespace {
// Lighting support found in the materialXLightDataBuilder fragment found in
// source\MaterialXContrib\MaterialXMaya\vp2ShaderFragments.
const string LIGHT_LOOP_RESULT = "lightLoopResult";
const string MAYA_ENV_IRRADIANCE_SAMPLE = "diffuseI";
const string MAYA_ENV_RADIANCE_SAMPLE = "specularI";
const string MAYA_ENV_ROUGHNESS = "roughness";

// The Apple shader compiler found on M1/M2 machines does not allow using a global variable as a
// temporary buffer. We will need to handle these inputs differently.
void fixupVertexDataInstance(ShaderStage& stage)
{
    // All the strings start with $. This is an anchor character in regex, so skip it.
    auto d = [](auto const& s) { return s.substr(1); };

    // Find keywords:                   (as text)
    //
    //  vec3 HW::T_POSITION_WORLD       vec3 $normalWorld
    //
    // And replace with:
    //
    // vec3 unused_T_POSITION_WORLD     vec3 unused_normalWorld
    //

    static const std::string paramSource = "vec3 [$](" + d(HW::T_POSITION_WORLD) + "|"
        + d(HW::T_POSITION_OBJECT) + "|" + d(HW::T_NORMAL_WORLD) + "|" + d(HW::T_NORMAL_OBJECT)
        + "|" + d(HW::T_TANGENT_WORLD) + "|" + d(HW::T_TANGENT_OBJECT) + "|"
        + d(HW::T_BITANGENT_WORLD) + "|" + d(HW::T_BITANGENT_OBJECT) + ")";

    static const std::regex paramRegex(paramSource.c_str());

    // Find keywords:                                       (as text)
    //
    //  HW::T_VERTEX_DATA_INSTANCE.HW::T_POSITION_WORLD     $vd.$normalWorld
    //
    // And replace with:
    //
    // HW::T_POSITION_WORLD(PIX_IN.HW::T_POSITION_WORLD)    $normalWorld( PIX_IN.$normalWorld )
    //

    static const std::string vtxSource = "[$]" + d(HW::T_VERTEX_DATA_INSTANCE) + "[.][$]("
        + d(HW::T_POSITION_WORLD) + "|" + d(HW::T_POSITION_OBJECT) + "|" + d(HW::T_NORMAL_WORLD)
        + "|" + d(HW::T_NORMAL_OBJECT) + "|" + d(HW::T_TANGENT_WORLD) + "|"
        + d(HW::T_TANGENT_OBJECT) + "|" + d(HW::T_BITANGENT_WORLD) + "|" + d(HW::T_BITANGENT_OBJECT)
        + ")";

    static const std::regex vtxRegex(vtxSource.c_str());

    // Find keywords:                        (as text)
    //
    //  vec[23] (HW::T_IN_GEOMPROP_NAME)     vec2 $inGeomprop_st
    //
    // And replace with:
    //
    // vec2 unused_(HW::T_IN_GEOMPROP_NAME)  vec2 unused_inGeomprop_st
    //

    static const std::string primvarParamSource
        = "vec([23]) [$](" + d(HW::T_IN_GEOMPROP) + "_[A-Za-z0-9_]+)";
    static const std::regex  primvarParamRegex(primvarParamSource.c_str());
    static const std::string texcoordParamSource
        = "vec([23]) [$](" + d(HW::T_TEXCOORD) + "_[0-9]+)";
    static const std::regex texcoordParamRegex(texcoordParamSource.c_str());

    // Find keywords:                                       (as text)
    //
    //  HW::T_VERTEX_DATA_INSTANCE.T_IN_GEOMPROP_(NAME)     $vd.$inGeomprop_st
    //
    // And replace with:
    //
    //  PIX_IN.(NAME)                                       PIX_IN.st
    //

    static const std::string vdCleanGeoSource = "[$]" + d(HW::T_VERTEX_DATA_INSTANCE) + "[.][$]"
        + d(HW::T_IN_GEOMPROP) + "_([A-Za-z0-9_]+)";
    static const std::regex vdCleanGeoRegex(vdCleanGeoSource.c_str());

    static const std::string vdCleanTexSource
        = "[$]" + d(HW::T_VERTEX_DATA_INSTANCE) + "[.][$](" + d(HW::T_TEXCOORD) + "_[0-9]+)";
    static const std::regex vdCleanTexRegex(vdCleanTexSource.c_str());

    // Find keywords:                                (as text)
    //
    //  HW::T_VERTEX_DATA_INSTANCE.(T_COLOR_INDEX)   $vd.$color_0;
    //
    // And replace with:
    //
    //  (T_COLOR_INDEX)                              color_0;
    //

    static const std::string vdCleanColorSource
        = "[$]" + d(HW::T_VERTEX_DATA_INSTANCE) + "[.][$](" + d(HW::T_COLOR) + "_[0-9]+)";
    static const std::regex vdCleanColorRegex(vdCleanColorSource.c_str());

    std::string code = stage.getSourceCode();
    code = std::regex_replace(code, paramRegex, "vec3 unused_$1");
    code = std::regex_replace(code, vtxRegex, "$$$1( PIX_IN.$$$1 )");
    code = std::regex_replace(code, primvarParamRegex, "vec$1 unused_$2");
    code = std::regex_replace(code, texcoordParamRegex, "vec$1 unused_$2");
    code = std::regex_replace(code, vdCleanGeoRegex, "PIX_IN.$1");
    code = std::regex_replace(code, vdCleanTexRegex, "PIX_IN.$1");
    code = std::regex_replace(code, vdCleanColorRegex, "$1");

#if MX_COMBINED_VERSION >= 13804
    stage.setSourceCode(code);
#else
    // setSourceCode was added in 1.38.4. Use const casting in previous versions to modify the
    // member variable directly (since getSourceCode returns by reference).
    std::string& ncCode(const_cast<std::string&>(stage.getSourceCode()));
    ncCode = code;
#endif
}
} // namespace

const string& HwSpecularEnvironmentSamples::name()
{
    static const string USER_DATA_ENV_SAMPLES = "HwSpecularEnvironmentSamples";
    return USER_DATA_ENV_SAMPLES;
}

#if MX_COMBINED_VERSION >= 13900

GlslFragmentSyntax::GlslFragmentSyntax(TypeSystemPtr typeSystem)
    : GlslSyntax(typeSystem)
{
}

SyntaxPtr GlslFragmentSyntax::create(TypeSystemPtr typeSystem)
{
    return std::make_shared<GlslFragmentSyntax>(typeSystem);
}

#endif

string GlslFragmentSyntax::getVariableName(
    const string& name,
#if MX_COMBINED_VERSION < 13900
    const TypeDesc* type,
#else
    TypeDesc type,
#endif
    IdentifierMap& identifiers) const
{
    string variable = GlslSyntax::getVariableName(name, type, identifiers);
    // A filename input corresponds to a texture sampler uniform
    // which requires a special suffix in OGS XML fragments.
    if (type == Type::FILENAME) {
        // Make sure it's not already used.
        if (!OgsXmlGenerator::isSamplerName(variable)) {
            variable = OgsXmlGenerator::textureToSamplerName(variable);
        }
    }
    return variable;
}

const string GlslFragmentGenerator::MATRIX3_TO_MATRIX4_POSTFIX = "4";

GlslFragmentGenerator::GlslFragmentGenerator()
#if MX_COMBINED_VERSION < 13903
    : GlslShaderGenerator()
#else
    : GlslShaderGenerator(TypeSystem::create())
#endif
{
    // Use our custom syntax class
#if MX_COMBINED_VERSION < 13903
    _syntax = std::make_shared<GlslFragmentSyntax>();
#else
    _syntax = GlslFragmentSyntax::create(_typeSystem);
#endif

    // Set identifier names to match OGS naming convention.
    _tokenSubstitutions[HW::T_POSITION_WORLD] = "Pw";
    _tokenSubstitutions[HW::T_POSITION_OBJECT] = "Pm";
    _tokenSubstitutions[HW::T_NORMAL_WORLD] = "Nw";
    _tokenSubstitutions[HW::T_NORMAL_OBJECT] = "Nm";
    _tokenSubstitutions[HW::T_TANGENT_WORLD] = "Tw";
    _tokenSubstitutions[HW::T_TANGENT_OBJECT] = "Tm";
    _tokenSubstitutions[HW::T_BITANGENT_WORLD] = "Bw";
    _tokenSubstitutions[HW::T_BITANGENT_OBJECT] = "Bm";
    if (OgsXmlGenerator::useLightAPI() >= 2) {
        // Use a Maya 2022.1-aware surface node implementation.
        registerImplementation(
            "IM_surface_" + GlslShaderGenerator::TARGET, SurfaceNodeMaya::create);
    } else {
        _tokenSubstitutions[HW::T_LIGHT_DATA_INSTANCE]
            = "g_lightData"; // Store Maya lights in global non-const
        _tokenSubstitutions[HW::T_NUM_ACTIVE_LIGHT_SOURCES] = "g_numActiveLightSources";
    }

    if (!OgsXmlGenerator::getPrimaryUVSetName().empty()) {
        registerImplementation(
            "IM_texcoord_vector2_" + GlslShaderGenerator::TARGET, TexcoordNodeGlslMaya::create);
        registerImplementation(
            "IM_texcoord_vector3_" + GlslShaderGenerator::TARGET, TexcoordNodeGlslMaya::create);
    }

    registerImplementation(
        MaterialXMaya::ShaderGenUtil::LobePruner::getDarkBaseImplementationName(),
        MayaDarkClosureNode::create);
    registerImplementation(
        MaterialXMaya::ShaderGenUtil::LobePruner::getDarkLayerImplementationName(),
        MayaDarkClosureNode::create);

    // The MaterialX transform node will crash if one of the "space" inputs is empty. This will be
    // fixed in 1.38.9. In the meantime we use patched nodes to replace those previously added in
    // the base class.
#if MX_COMBINED_VERSION < 13809
    // <!-- <ND_transformpoint> ->
    registerImplementation(
        "IM_transformpoint_vector3_" + GlslShaderGenerator::TARGET,
        MayaTransformPointNodeGlsl::create);

    // <!-- <ND_transformvector> ->
    registerImplementation(
        "IM_transformvector_vector3_" + GlslShaderGenerator::TARGET,
        MayaTransformVectorNodeGlsl::create);

    // <!-- <ND_transformnormal> ->
    registerImplementation(
        "IM_transformnormal_vector3_" + GlslShaderGenerator::TARGET,
        MayaTransformNormalNodeGlsl::create);
#endif

#ifdef FIX_NODEGRAPH_UDIM_SCALE_OFFSET
    // Locally fixing the UV scale and offset for UDIMs. We will submit to 1.39 later.
    StringVec elementNames;
    elementNames = {
        "IM_image_float_" + GlslShaderGenerator::TARGET,
        "IM_image_color3_" + GlslShaderGenerator::TARGET,
        "IM_image_color4_" + GlslShaderGenerator::TARGET,
        "IM_image_vector2_" + GlslShaderGenerator::TARGET,
        "IM_image_vector3_" + GlslShaderGenerator::TARGET,
        "IM_image_vector4_" + GlslShaderGenerator::TARGET,
    };
    registerImplementation(elementNames, MayaHwImageNode::create);
#endif

    for (auto&& implName : GlslOcioNodeImpl::getOCIOImplementations()) {
        registerImplementation(implName, GlslOcioNodeImpl::create);
    }
}

ShaderGeneratorPtr GlslFragmentGenerator::create()
{
    return std::make_shared<GlslFragmentGenerator>();
}

ShaderPtr GlslFragmentGenerator::createShader(
    const string& name,
    ElementPtr    element,
    GenContext&   context) const
{
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    // Create the root shader graph
    ShaderGraphPtr graph = MayaShaderGraph::create(nullptr, name, element, context);
    ShaderPtr      shader = std::make_shared<Shader>(name, graph);

    // Check if there are inputs with default geomprops assigned. In order to bind the
    // corresponding data to these inputs we insert geomprop nodes in the graph.
    bool geomNodeAdded = false;
    for (ShaderGraphInputSocket* socket : graph->getInputSockets()) {
        if (!socket->getGeomProp().empty()) {
            ConstDocumentPtr doc = element->getDocument();
            GeomPropDefPtr   geomprop = doc->getGeomPropDef(socket->getGeomProp());
            if (geomprop) {
                // A default geomprop was assigned to this graph input.
                // For all internal connections to this input, break the connection
                // and assign a geomprop node that generates this data.
                // Note: If a geomprop node exists already it is reused,
                // so only a single node per geometry type is created.
                ShaderInputVec connections = socket->getConnections();
                for (auto connection : connections) {
                    connection->breakConnection();
                    graph->addDefaultGeomNode(connection, *geomprop, context);
                    geomNodeAdded = true;
                }
            }
        }
    }
    // If nodes were added we need to re-sort the nodes in topological order.
    if (geomNodeAdded) {
        graph->topologicalSort();
    }

    // Create vertex stage.
    ShaderStagePtr vs = createStage(Stage::VERTEX, *shader);
    vs->createInputBlock(HW::VERTEX_INPUTS, "i_vs");

    // Each Stage must have three types of uniform blocks:
    // Private, Public and Sampler blocks
    // Public uniforms are inputs that should be published in a user interface for user interaction,
    // while private uniforms are internal variables needed by the system which should not be
    // exposed in UI. So when creating these uniforms for a shader node, if the variable is
    // user-facing it should go into the public block, and otherwise the private block. All texture
    // based objects should be added to Sampler block

    vs->createUniformBlock(HW::PRIVATE_UNIFORMS, "u_prv");
    vs->createUniformBlock(HW::PUBLIC_UNIFORMS, "u_pub");

    // Create required variables for vertex stage
    VariableBlock& vsInputs = vs->getInputBlock(HW::VERTEX_INPUTS);
    vsInputs.add(Type::VECTOR3, HW::T_IN_POSITION);
    VariableBlock& vsPrivateUniforms = vs->getUniformBlock(HW::PRIVATE_UNIFORMS);
    vsPrivateUniforms.add(Type::MATRIX44, HW::T_WORLD_MATRIX);
    vsPrivateUniforms.add(Type::MATRIX44, HW::T_VIEW_PROJECTION_MATRIX);

    // Create pixel stage.
    ShaderStagePtr   ps = createStage(Stage::PIXEL, *shader);
    VariableBlockPtr psOutputs = ps->createOutputBlock(HW::PIXEL_OUTPUTS, "o_ps");

    // Create required Uniform blocks and any additonal blocks if needed.
    VariableBlockPtr psPrivateUniforms = ps->createUniformBlock(HW::PRIVATE_UNIFORMS, "u_prv");
    VariableBlockPtr psPublicUniforms = ps->createUniformBlock(HW::PUBLIC_UNIFORMS, "u_pub");
    VariableBlockPtr lightData = ps->createUniformBlock(HW::LIGHT_DATA, HW::T_LIGHT_DATA_INSTANCE);
    lightData->add(Type::INTEGER, "type");

    // Add a block for data from vertex to pixel shader.
    addStageConnectorBlock(HW::VERTEX_DATA, HW::T_VERTEX_DATA_INSTANCE, *vs, *ps);

    // Add uniforms for transparent rendering.
    if (context.getOptions().hwTransparency) {
        psPrivateUniforms->add(Type::FLOAT, HW::T_ALPHA_THRESHOLD, Value::createValue(0.001f));
    }

    // Add uniforms for shadow map rendering.
    if (context.getOptions().hwShadowMap) {
        psPrivateUniforms->add(Type::FILENAME, HW::T_SHADOW_MAP);
        psPrivateUniforms->add(
            Type::MATRIX44, HW::T_SHADOW_MATRIX, Value::createValue(Matrix44::IDENTITY));
    }

    // Add inputs and uniforms for ambient occlusion.
    if (context.getOptions().hwAmbientOcclusion) {
        addStageInput(HW::VERTEX_INPUTS, Type::VECTOR2, HW::T_IN_TEXCOORD + "_0", *vs);
        addStageConnector(HW::VERTEX_DATA, Type::VECTOR2, HW::T_TEXCOORD + "_0", *vs, *ps);
        psPrivateUniforms->add(Type::FILENAME, HW::T_AMB_OCC_MAP);
        psPrivateUniforms->add(Type::FLOAT, HW::T_AMB_OCC_GAIN, Value::createValue(1.0f));
    }

    // Add uniforms for environment lighting.
    bool lighting = graph->hasClassification(
                        ShaderNode::Classification::SHADER | ShaderNode::Classification::SURFACE)
        || graph->hasClassification(ShaderNode::Classification::BSDF);
    if (lighting && context.getOptions().hwSpecularEnvironmentMethod != SPECULAR_ENVIRONMENT_NONE) {
        const Matrix44 yRotationPI = Matrix44::createScale(Vector3(-1, 1, -1));
        psPrivateUniforms->add(Type::MATRIX44, HW::T_ENV_MATRIX, Value::createValue(yRotationPI));
        psPrivateUniforms->add(Type::FILENAME, HW::T_ENV_RADIANCE);
        psPrivateUniforms->add(Type::FLOAT, HW::T_ENV_LIGHT_INTENSITY, Value::createValue(1.0f));
        psPrivateUniforms->add(Type::INTEGER, HW::T_ENV_RADIANCE_MIPS, Value::createValue<int>(1));
        psPrivateUniforms->add(
            Type::INTEGER, HW::T_ENV_RADIANCE_SAMPLES, Value::createValue<int>(16));
        psPrivateUniforms->add(Type::FILENAME, HW::T_ENV_IRRADIANCE);
        psPrivateUniforms->add(Type::BOOLEAN, HW::T_REFRACTION_TWO_SIDED);
    }

    // Add uniforms for the directional albedo table.
    if (context.getOptions().hwDirectionalAlbedoMethod == DIRECTIONAL_ALBEDO_TABLE
        || context.getOptions().hwWriteAlbedoTable) {
        psPrivateUniforms->add(Type::FILENAME, HW::T_ALBEDO_TABLE);
        psPrivateUniforms->add(Type::INTEGER, HW::T_ALBEDO_TABLE_SIZE, Value::createValue<int>(64));
    }

    // Add uniforms for environment prefiltering.
    if (context.getOptions().hwWriteEnvPrefilter) {
        psPrivateUniforms->add(Type::FILENAME, HW::T_ENV_RADIANCE);
        psPrivateUniforms->add(Type::FLOAT, HW::T_ENV_LIGHT_INTENSITY, Value::createValue(1.0f));
        psPrivateUniforms->add(Type::INTEGER, HW::T_ENV_PREFILTER_MIP, Value::createValue<int>(1));
        const Matrix44 yRotationPI = Matrix44::createScale(Vector3(-1, 1, -1));
        psPrivateUniforms->add(Type::MATRIX44, HW::T_ENV_MATRIX, Value::createValue(yRotationPI));
        psPrivateUniforms->add(Type::INTEGER, HW::T_ENV_RADIANCE_MIPS, Value::createValue<int>(1));
    }

    // Create uniforms for the published graph interface
    for (ShaderGraphInputSocket* inputSocket : graph->getInputSockets()) {
        // Only for inputs that are connected/used internally,
        // and are editable by users.
        if (!inputSocket->getConnections().empty() && graph->isEditable(*inputSocket)) {
            psPublicUniforms->add(inputSocket->getSelf());
        }
    }

    // Add the pixel stage output. This needs to be a color4 for rendering,
    // so copy name and variable from the graph output but set type to color4.
    // TODO: Improve this to support multiple outputs and other data types.
    ShaderGraphOutputSocket* outputSocket = graph->getOutputSocket();
    ShaderPort*              output = psOutputs->add(Type::COLOR4, outputSocket->getName());
    output->setVariable(outputSocket->getVariable());
    output->setPath(outputSocket->getPath());

    // Create shader variables for all nodes that need this.
    createVariables(graph, context, *shader);

    HwLightShadersPtr lightShaders
        = context.getUserData<HwLightShaders>(HW::USER_DATA_LIGHT_SHADERS);

    // For surface shaders we need light shaders
    if (lightShaders
        && graph->hasClassification(
            ShaderNode::Classification::SHADER | ShaderNode::Classification::SURFACE)) {
        // Create shader variables for all bound light shaders
        for (const auto& it : lightShaders->get()) {
            ShaderNode* node = it.second.get();
            node->getImplementation().createVariables(*node, context, *shader);
        }
    }

    //
    // For image textures we need to convert filenames into uniforms (texture samplers).
    // Any unconnected filename input on file texture nodes needs to have a corresponding
    // uniform.
    //

    // Start with top level graphs.
    vector<ShaderGraph*> graphStack = { graph.get() };
    if (lightShaders) {
        for (const auto& it : lightShaders->get()) {
            ShaderNode*  node = it.second.get();
            ShaderGraph* lightGraph = node->getImplementation().getGraph();
            if (lightGraph) {
                graphStack.push_back(lightGraph);
            }
        }
    }

    while (!graphStack.empty()) {
        ShaderGraph* g = graphStack.back();
        graphStack.pop_back();

        for (ShaderNode* node : g->getNodes()) {
            if (node->hasClassification(ShaderNode::Classification::FILETEXTURE)) {
                for (ShaderInput* input : node->getInputs()) {
#if MX_COMBINED_VERSION < 13900
                    if (!input->getConnection() && *input->getType() == *Type::FILENAME) {
#else
                    if (!input->getConnection() && input->getType() == Type::FILENAME) {
#endif
                        // Create the uniform using the filename type to make this uniform into a
                        // texture sampler.
                        ShaderPort* filename = psPublicUniforms->add(
                            Type::FILENAME, input->getVariable(), input->getValue());
                        filename->setPath(input->getPath());

                        // Assing the uniform name to the input value
                        // so we can reference it during code generation.
                        input->setValue(Value::createValue(input->getVariable()));
                    }
                }
            }
            // Push subgraphs on the stack to process these as well.
            ShaderGraph* subgraph = node->getImplementation().getGraph();
            if (subgraph) {
                graphStack.push_back(subgraph);
            }
        }
    }

    if (context.getOptions().hwTransparency) {
        // Flag the shader as being transparent.
        shader->setAttribute(HW::ATTR_TRANSPARENT);
    }
#else
    ShaderPtr shader = GlslShaderGenerator::createShader(name, element, context);
    ShaderGraph& graph = shader->getGraph();
#endif
    ShaderStage& pixelStage = shader->getStage(Stage::PIXEL);

    // Add uniforms for environment lighting.
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    if (requiresLighting(*graph) && OgsXmlGenerator::useLightAPI() < 2) {
#else
    if (requiresLighting(graph) && OgsXmlGenerator::useLightAPI() < 2) {
#endif
        VariableBlock& psPrivateUniforms = pixelStage.getUniformBlock(HW::PUBLIC_UNIFORMS);
        psPrivateUniforms.add(
            Type::COLOR3, LIGHT_LOOP_RESULT, Value::createValue(Color3(0.0f, 0.0f, 0.0f)));
        psPrivateUniforms.add(
            Type::COLOR3, MAYA_ENV_IRRADIANCE_SAMPLE, Value::createValue(Color3(0.0f, 0.0f, 0.0f)));
        psPrivateUniforms.add(
            Type::COLOR3, MAYA_ENV_RADIANCE_SAMPLE, Value::createValue(Color3(0.0f, 0.0f, 0.0f)));
        psPrivateUniforms.add(Type::FLOAT, MAYA_ENV_ROUGHNESS, Value::createValue(0.0f));
    }

    createStage(Stage::UNIFORMS, *shader);
    return shader;
}

ShaderPtr GlslFragmentGenerator::generate(
    const string& fragmentName,
    ElementPtr    element,
    GenContext&   context) const
{
    ShaderPtr shader = createShader(fragmentName, element, context);

    ShaderStage& pixelStage = shader->getStage(Stage::PIXEL);
    ShaderGraph& graph = shader->getGraph();

    // Turn on fixed float formatting to make sure float values are
    // emitted with a decimal point and not as integers, and to avoid
    // any scientific notation which isn't supported by all OpenGL targets.
    ScopedFloatFormatting floatFormatting(Value::FloatFormatFixed);

    const VariableBlock& vertexData = pixelStage.getInputBlock(HW::VERTEX_DATA);

    // Add global constants and type definitions
    const unsigned int maxLights = std::max(1u, context.getOptions().hwMaxActiveLightSources);
    emitLine("#define MAX_LIGHT_SOURCES " + std::to_string(maxLights), pixelStage, false);
    emitLineBreak(pixelStage);
    emitTypeDefinitions(context, pixelStage);

    // Add all constants
    const VariableBlock& constants = pixelStage.getConstantBlock();
    if (!constants.empty()) {
        emitVariableDeclarations(
            constants, _syntax->getConstantQualifier(), Syntax::SEMICOLON, context, pixelStage);
        emitLineBreak(pixelStage);
    }

    const bool lighting = requiresLighting(graph);

#if MX_COMBINED_VERSION == 13804
    // 1.38.4 is the only version requiring "libraries" in the path
    std::string libRoot = "libraries/";
#else
    // No libRoot for other versions:
    std::string libRoot;
#endif

#if MX_COMBINED_VERSION <= 13804
#define MX_EMIT_INCLUDE emitInclude
#else
#define MX_EMIT_INCLUDE emitLibraryInclude
#endif

    // Emit common math functions
    MX_EMIT_INCLUDE(libRoot + "stdlib/genglsl/lib/mx_math.glsl", context, pixelStage);
    emitLineBreak(pixelStage);

    if (lighting) {
        int specularMethod = context.getOptions().hwSpecularEnvironmentMethod;
        if (specularMethod == SPECULAR_ENVIRONMENT_FIS) {
            emitLine(
                "#define DIRECTIONAL_ALBEDO_METHOD "
                    + std::to_string(int(context.getOptions().hwDirectionalAlbedoMethod)),
                pixelStage,
                false);
            emitLineBreak(pixelStage);
            HwSpecularEnvironmentSamplesPtr pSamples
                = context.getUserData<HwSpecularEnvironmentSamples>(
                    HwSpecularEnvironmentSamples::name());
            if (pSamples) {
                emitLine(
                    "#define MX_NUM_FIS_SAMPLES "
                        + std::to_string(pSamples->hwSpecularEnvironmentSamples),
                    pixelStage,
                    false);
            } else {
                emitLine("#define MX_NUM_FIS_SAMPLES 64", pixelStage, false);
            }
            emitLineBreak(pixelStage);
            MX_EMIT_INCLUDE(
                libRoot + "pbrlib/genglsl/ogsxml/mx_lighting_maya_v3.glsl", context, pixelStage);
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
            emitLine("#define MAYA_MX39_USING_ENVIRONMENT_FIS", pixelStage, false);
#endif
        } else if (specularMethod == SPECULAR_ENVIRONMENT_PREFILTER) {
            if (OgsXmlGenerator::useLightAPI() < 2) {
                MX_EMIT_INCLUDE(
                    libRoot + "pbrlib/genglsl/ogsxml/mx_lighting_maya_v1.glsl",
                    context,
                    pixelStage);
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
                emitLine("#define MAYA_MX39_USING_ENVIRONMENT_PREFILTER_V1", pixelStage, false);
#endif
            } else {
                MX_EMIT_INCLUDE(
                    libRoot + "pbrlib/genglsl/ogsxml/mx_lighting_maya_v2.glsl",
                    context,
                    pixelStage);
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
                emitLine("#define MAYA_MX39_USING_ENVIRONMENT_PREFILTER_V2", pixelStage, false);
#endif
            }
        } else if (specularMethod == SPECULAR_ENVIRONMENT_NONE) {
            MX_EMIT_INCLUDE(
                libRoot + "pbrlib/genglsl/ogsxml/mx_lighting_maya_none.glsl", context, pixelStage);
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
            emitLine("#define MAYA_MX39_USING_ENVIRONMENT_NONE", pixelStage, false);
#endif
        } else {
            throw ExceptionShaderGenError(
                "Invalid hardware specular environment method specified: '"
                + std::to_string(specularMethod) + "'");
        }
    }
    emitLineBreak(pixelStage);

#if MX_COMBINED_VERSION >= 13805
    emitTransmissionRender(context, pixelStage);
#endif

    // Set the include file to use for uv transformations,
    // depending on the vertical flip flag.
#if MX_COMBINED_VERSION <= 13804
    _tokenSubstitutions[ShaderGenerator::T_FILE_TRANSFORM_UV] = string(libRoot + "stdlib/genglsl")
        + (context.getOptions().fileTextureVerticalFlip ? "/lib/mx_transform_uv_vflip.glsl"
                                                        : "/lib/mx_transform_uv.glsl");
#else
    _tokenSubstitutions[ShaderGenerator::T_FILE_TRANSFORM_UV]
        = (context.getOptions().fileTextureVerticalFlip ? "mx_transform_uv_vflip.glsl"
                                                        : "mx_transform_uv.glsl");
#if MX_COMBINED_VERSION <= 13806
    _tokenSubstitutions[HW::T_REFRACTION_ENV] = MX_REFRACTION_SUBSTITUTION;
#else
    // Renamed in 1.38.7
    _tokenSubstitutions[HW::T_REFRACTION_TWO_SIDED] = MX_REFRACTION_SUBSTITUTION;
#endif // <= 13806
#endif // <= 13804

    // Add all functions for node implementations
    emitFunctionDefinitions(graph, context, pixelStage);

    const ShaderGraphOutputSocket* outputSocket = graph.getOutputSocket();

    // Add the function signature for the fragment's root function

    // Keep track of arguments we changed from matrix3 to matrix4 as additional
    // code must be inserted to get back the matrix3 version
    StringVec convertMatrixStrings;

    string functionName = shader->getName();
    _syntax->makeIdentifier(functionName, graph.getIdentifierMap());
    setFunctionName(functionName, pixelStage);

    emitLine(
        (context.getOptions().hwTransparency ? "vec4 " : "vec3 ") + functionName,
        pixelStage,
        false);

    // Emit public uniforms and vertex data as function arguments
    //
    emitScopeBegin(pixelStage, Syntax::PARENTHESES);
    {
        bool firstArgument = true;

        auto emitArgument
            = [this, &firstArgument, &pixelStage, &context](const ShaderPort* shaderPort) -> void {
            if (firstArgument) {
                firstArgument = false;
            } else {
                emitString(Syntax::COMMA, pixelStage);
                emitLineEnd(pixelStage, false);
            }

            emitLineBegin(pixelStage);
            emitVariableDeclaration(shaderPort, EMPTY_STRING, context, pixelStage, false);
        };

        const VariableBlock& publicUniforms = pixelStage.getUniformBlock(HW::PUBLIC_UNIFORMS);
        for (size_t i = 0; i < publicUniforms.size(); ++i) {
            const ShaderPort* const shaderPort = publicUniforms[i];

            if (shaderPort->getType() == Type::MATRIX33)
                convertMatrixStrings.push_back(shaderPort->getVariable());

            emitArgument(shaderPort);
        }

        for (size_t i = 0; i < vertexData.size(); ++i)
            emitArgument(vertexData[i]);

        if (context.getOptions().hwTransparency) {
            // A dummy argument not used in the generated shader code but necessary to
            // map onto an OGS fragment parameter and a shading node DG attribute with
            // the same name that can be set to a non-0 value to let Maya know that the
            // surface is transparent.

            if (!firstArgument) {
                emitString(Syntax::COMMA, pixelStage);
                emitLineEnd(pixelStage, false);
            }

            emitLineBegin(pixelStage);
            emitString("float ", pixelStage);
            emitString(OgsXmlGenerator::VP_TRANSPARENCY_NAME, pixelStage);
            emitLineEnd(pixelStage, false);
        }
    }
    emitScopeEnd(pixelStage);

    // Add function body
    emitScopeBegin(pixelStage);

    if (graph.hasClassification(ShaderNode::Classification::CLOSURE)
        && !graph.hasClassification(ShaderNode::Classification::SHADER)) {
        // Handle the case where the graph is a direct closure.
        // We don't support rendering closures without attaching
        // to a surface shader, so just output black.
        emitLine("return vec3(0.0)", pixelStage);
    } else {
        if (lighting && OgsXmlGenerator::useLightAPI() < 2) {
            // Store environment samples from light rig:
            emitLine(
                "g_" + MAYA_ENV_IRRADIANCE_SAMPLE + " = " + MAYA_ENV_IRRADIANCE_SAMPLE, pixelStage);
            emitLine(
                "g_" + MAYA_ENV_RADIANCE_SAMPLE + " = " + MAYA_ENV_RADIANCE_SAMPLE, pixelStage);
        }

        // Insert matrix converters
        for (const string& argument : convertMatrixStrings) {
            emitLine(
                "mat3 " + argument + " = mat3(" + argument
                    + GlslFragmentGenerator::MATRIX3_TO_MATRIX4_POSTFIX + ")",
                pixelStage,
                true);
        }

        // Add all function calls (varies greatly by MaterialX version)
#if MX_COMBINED_VERSION >= 13805
        // Surface shaders need special handling.
        if (graph.hasClassification(
                ShaderNode::Classification::SHADER | ShaderNode::Classification::SURFACE)) {
            // Emit all texturing nodes. These are inputs to any
            // closure/shader nodes and need to be emitted first.
            emitFunctionCalls(graph, context, pixelStage, ShaderNode::Classification::TEXTURE);

            // Emit function calls for "root" closure/shader nodes.
            // These will internally emit function calls for any dependent closure nodes upstream.
            for (ShaderGraphOutputSocket* socket : graph.getOutputSockets()) {
                if (socket->getConnection()) {
                    const ShaderNode* upstream = socket->getConnection()->getNode();
                    if (upstream->getParent() == &graph
                        && (upstream->hasClassification(ShaderNode::Classification::CLOSURE)
                            || upstream->hasClassification(ShaderNode::Classification::SHADER))) {
                        emitFunctionCall(*upstream, context, pixelStage);
                    }
                }
            }
        } else {
            // No surface shader graph so just generate all
            // function calls in order.
            emitFunctionCalls(graph, context, pixelStage);
        }
#elif MX_COMBINED_VERSION >= 13803
        // Surface shaders need special handling.
        if (graph.hasClassification(
                ShaderNode::Classification::SHADER | ShaderNode::Classification::SURFACE)) {
            // Emit all texturing nodes. These are inputs to any
            // closure/shader nodes and need to be emitted first.
            emitFunctionCalls(graph, context, pixelStage, ShaderNode::Classification::TEXTURE);

            // Emit function calls for all surface shader nodes.
            // These will internally emit their closure function calls.
            emitFunctionCalls(
                graph,
                context,
                pixelStage,
                ShaderNode::Classification::SHADER | ShaderNode::Classification::SURFACE);
        } else {
            // No surface shader graph so just generate all
            // function calls in order.
            emitFunctionCalls(graph, context, pixelStage);
        }
#else
        emitFunctionCalls(graph, context, pixelStage);
#endif

        // Emit final result
        //
        // TODO: Emit a mayaSurfaceShaderOutput result to plug into mayaComputeSurfaceFinal
        //
        if (const ShaderOutput* const outputConnection = outputSocket->getConnection()) {
            string finalOutput = outputConnection->getVariable();
#if MX_COMBINED_VERSION < 13900
            const string& channels = outputSocket->getChannels();
            if (!channels.empty()) {
                finalOutput = _syntax->getSwizzledVariable(
                    finalOutput, outputConnection->getType(), channels, outputSocket->getType());
            }
#endif
            if (graph.hasClassification(ShaderNode::Classification::SURFACE)) {
                if (context.getOptions().hwTransparency) {
                    emitLine(
                        "return vec4(" + finalOutput + ".color, clamp(1.0 - dot(" + finalOutput
                            + ".transparency, vec3(0.3333)), 0.0, 1.0))",
                        pixelStage);
                } else {
                    emitLine("return " + finalOutput + ".color", pixelStage);
                }
            } else {
#if MX_COMBINED_VERSION < 13900
                if (context.getOptions().hwTransparency && !outputSocket->getType()->isFloat4()) {
                    toVec4(outputSocket->getType(), finalOutput);
                } else if (
                    !context.getOptions().hwTransparency && !outputSocket->getType()->isFloat3()) {
                    toVec3(outputSocket->getType(), finalOutput);
                }
#else
                if (context.getOptions().hwTransparency && !outputSocket->getType().isFloat4()) {
                    toVec4(outputSocket->getType(), finalOutput);
                } else if (
                    !context.getOptions().hwTransparency && !outputSocket->getType().isFloat3()) {
                    toVec3(outputSocket->getType(), finalOutput);
                }
#endif
                emitLine("return " + finalOutput, pixelStage);
            }
        } else {
            const string outputValue = outputSocket->getValue()
                ? _syntax->getValue(outputSocket->getType(), *outputSocket->getValue())
                : _syntax->getDefaultValue(outputSocket->getType());

            if (!context.getOptions().hwTransparency &&
#if MX_COMBINED_VERSION < 13900
                !outputSocket->getType()->isFloat3()) {
#else
                !outputSocket->getType().isFloat3()) {
#endif
                string finalOutput = outputSocket->getVariable() + "_tmp";
                emitLine(
                    _syntax->getTypeName(outputSocket->getType()) + " " + finalOutput + " = "
                        + outputValue,
                    pixelStage);
                toVec3(outputSocket->getType(), finalOutput);
                emitLine("return " + finalOutput, pixelStage);
            } else if (
                context.getOptions().hwTransparency &&
#if MX_COMBINED_VERSION < 13900
                !outputSocket->getType()->isFloat4()) {
#else
                !outputSocket->getType().isFloat4()) {
#endif
                string finalOutput = outputSocket->getVariable() + "_tmp";
                emitLine(
                    _syntax->getTypeName(outputSocket->getType()) + " " + finalOutput + " = "
                        + outputValue,
                    pixelStage);
                toVec4(outputSocket->getType(), finalOutput);
                emitLine("return " + finalOutput, pixelStage);
            } else {
                emitLine("return " + outputValue, pixelStage);
            }
        }
    }

    // End function
    emitScopeEnd(pixelStage);

    fixupVertexDataInstance(pixelStage);

    // Replace all tokens with real identifier names
    replaceTokens(_tokenSubstitutions, pixelStage);

    // Now emit uniform definitions to a special stage which is only
    // consumed by the HLSL cross-compiler.
    //
    ShaderStage& uniformsStage = shader->getStage(Stage::UNIFORMS);

    auto emitUniformBlock
        = [this, &uniformsStage, &context](const VariableBlock& uniformBlock) -> void {
        for (size_t i = 0; i < uniformBlock.size(); ++i) {
            const ShaderPort* const shaderPort = uniformBlock[i];

            const string& originalName = shaderPort->getVariable();
            const string  textureName = OgsXmlGenerator::samplerToTextureName(originalName);
            if (!textureName.empty()) {
                // GLSL for OpenGL (which MaterialX generates) uses combined
                // samplers where the texture is implicit in the sampler.
                // HLSL shader model 5.0 required by DX11 uses separate samplers
                // and textures.
                // We use a naming convention compatible with both OGS and
                // SPIRV-Cross where we derive sampler names from texture names
                // with a prefix and a suffix.
                // Unfortunately, the cross-compiler toolchain converts GLSL
                // samplers to HLSL textures preserving the original names and
                // generates HLSL samplers with derived names. For this reason,
                // we rename GLSL samplers with macros here to follow the
                // texture naming convention which preserves the correct
                // mapping.

                emitLineBegin(uniformsStage);
                emitString("#define ", uniformsStage);
                emitString(originalName, uniformsStage);
                emitString(" ", uniformsStage);
                emitString(textureName, uniformsStage);
                emitLineEnd(uniformsStage, false);
            }

            emitLineBegin(uniformsStage);
            emitVariableDeclaration(
                shaderPort, _syntax->getUniformQualifier(), context, uniformsStage);
            emitString(Syntax::SEMICOLON, uniformsStage);
            emitLineEnd(uniformsStage, false);
        }

        if (!uniformBlock.empty())
            emitLineBreak(uniformsStage);
    };

    emitUniformBlock(pixelStage.getUniformBlock(HW::PRIVATE_UNIFORMS));
    emitUniformBlock(pixelStage.getUniformBlock(HW::PUBLIC_UNIFORMS));

    replaceTokens(_tokenSubstitutions, uniformsStage);

    return shader;
}

#if MX_COMBINED_VERSION < 13900
void GlslFragmentGenerator::toVec3(const TypeDesc* type, string& variable)
{
    if (type->isFloat2()) {
        variable = "vec3(" + variable + ", 0.0)";
    } else if (type->isFloat4()) {
        variable = variable + ".xyz";
#else
void GlslFragmentGenerator::toVec3(const TypeDesc& type, string& variable)
{
    if (type.isFloat2()) {
        variable = "vec3(" + variable + ", 0.0)";
    } else if (type.isFloat4()) {
        variable = variable + ".xyz";
#endif
    } else if (type == Type::FLOAT || type == Type::INTEGER) {
        variable = "vec3(" + variable + ", " + variable + ", " + variable + ")";
    } else if (type == Type::BSDF || type == Type::EDF) {
        variable = "vec3(" + variable + ")";
    } else {
        // Can't understand other types. Just return black.
        variable = "vec3(0.0, 0.0, 0.0)";
    }
}

void GlslFragmentGenerator::emitVariableDeclaration(
    const ShaderPort* variable,
    const string&     qualifier,
    GenContext&       context,
    ShaderStage&      stage,
    bool              assignValue) const
{
    // We change matrix3 to matrix4 input arguments
    if (variable->getType() == Type::MATRIX33) {
        string qualifierPrefix = qualifier.empty() ? EMPTY_STRING : qualifier + " ";
        emitString(
            qualifierPrefix + "mat4 " + variable->getVariable() + MATRIX3_TO_MATRIX4_POSTFIX,
            stage);
    } else {
        GlslShaderGenerator::emitVariableDeclaration(
            variable, qualifier, context, stage, assignValue);
    }
}

#ifdef FIX_NODEGRAPH_UDIM_SCALE_OFFSET
// Locally fixing the UV scale and offset for UDIMs. We will submit to 1.39 later.
ShaderNodeImplPtr
GlslFragmentGenerator::getImplementation(const NodeDef& nodedef, GenContext& context) const
{
    InterfaceElementPtr implElement = nodedef.getImplementation(getTarget());
    if (!implElement) {
        return nullptr;
    }

    const string& name = implElement->getName();

    // Check if it's created and cached already.
    ShaderNodeImplPtr impl = context.findNodeImplementation(name);
    if (impl) {
        return impl;
    }

    vector<OutputPtr> outputs = nodedef.getActiveOutputs();
    if (outputs.empty()) {
        throw ExceptionShaderGenError("NodeDef '" + nodedef.getName() + "' has no outputs defined");
    }

#if MX_COMBINED_VERSION < 13900
    const TypeDesc* outputType = TypeDesc::get(outputs[0]->getType());
#elif MX_COMBINED_VERSION < 13903
    const TypeDesc outputType = TypeDesc::get(outputs[0]->getType());
#else
    const TypeDesc outputType = context.getTypeDesc(outputs[0]->getType());
#endif

#if MX_COMBINED_VERSION < 13900
    if (implElement->isA<NodeGraph>() && outputType->getName() != Type::LIGHTSHADER->getName()
        && !outputType->isClosure()) {
#else
    if (implElement->isA<NodeGraph>() && outputType.getName() != Type::LIGHTSHADER.getName()
        && !outputType.isClosure()) {
#endif
        // Use a compound implementation that can propagate UDIM inputs:
        impl = MayaCompoundNode::create();
        impl->initialize(*implElement, context);

        // Cache it.
        context.addNodeImplementation(name, impl);

        return impl;
#ifdef FIX_DUPLICATE_INCLUDED_SHADER_CODE
    } else if (
        implElement->isA<Implementation>() && !_implFactory.classRegistered(name)
        && !outputType->isClosure()) {
        // Backporting 1.39 fix done in
        //  https://github.com/AcademySoftwareFoundation/MaterialX/pull/1754
        impl = MayaSourceCodeNode::create();
        impl->initialize(*implElement, context);

        // Cache it.
        context.addNodeImplementation(name, impl);

        return impl;
#endif
#ifdef USD_HAS_BACKPORTED_MX39_OPENPBR
    } else if (
        implElement->getName() == "IM_dielectric_tf_bsdf_genglsl"
        || implElement->getName() == "IM_generalized_schlick_tf_82_bsdf_genglsl") {
        // We need to inject lighting code into the backported OpenPBR:
        impl = MayaClosureSourceCodeNode::create();
        impl->initialize(*implElement, context);

        // Cache it.
        context.addNodeImplementation(name, impl);

        return impl;
#endif
    }
    return GlslShaderGenerator::getImplementation(nodedef, context);
}
#endif

MATERIALX_NAMESPACE_END
