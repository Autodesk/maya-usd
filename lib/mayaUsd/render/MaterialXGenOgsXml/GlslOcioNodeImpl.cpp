//
// Copyright 2023 Autodesk
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

#include "GlslOcioNodeImpl.h"

#include "PugiXML/pugixml.hpp"

#include <maya/MFragmentManager.h>
#include <maya/MGlobal.h>

#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderStage.h>

#include <cstring>
#include <map>

MATERIALX_NAMESPACE_BEGIN

namespace {
// Internal OCIO strings:
constexpr const char OCIO_COLOR3[] = "color3";
constexpr const char OCIO_COLOR4[] = "color4";
constexpr const char OCIO_GLSL[] = "GLSL";
constexpr const char OCIO_IM_PREFIX[] = "IMMayaOCIO_";
constexpr const char OCIO_ND_PREFIX[] = "NDMayaOCIO_";

// Lengths where needed:
constexpr auto OCIO_COLOR3_LEN = sizeof(OCIO_COLOR3) / sizeof(OCIO_COLOR3[0]);
constexpr auto OCIO_GLSL_LEN = sizeof(OCIO_GLSL) / sizeof(OCIO_GLSL[0]);
constexpr auto OCIO_IM_PREFIX_LEN = sizeof(OCIO_IM_PREFIX) / sizeof(OCIO_IM_PREFIX[0]);

// Expected XML tag names for an OCIO fragment:
constexpr const char TAG_FLOAT3[] = "float3";
constexpr const char TAG_FRAGMENT[] = "fragment";
constexpr const char TAG_FUNCTION_NAME[] = "function_name";
constexpr const char TAG_PROPERTIES[] = "properties";
constexpr const char TAG_VALUES[] = "values";
constexpr const char TAG_OUTPUTS[] = "outputs";
constexpr const char TAG_IMPLEMENTATION[] = "implementation";
constexpr const char TAG_TEXTURE2[] = "texture2";
constexpr const char TAG_SOURCE[] = "source";

// Lengths where needed:
constexpr auto TAG_TEXTURE2_LEN = sizeof(TAG_TEXTURE2) / sizeof(TAG_TEXTURE2[0]);

// Expected XML attribute names for an OCIO fragment:
constexpr const char ATTR_NAME[] = "name";
constexpr const char ATTR_LANGUAGE[] = "language";
constexpr const char ATTR_VAL[] = "val";

struct OcioData
{
    std::string              fragName;
    std::string              colorInput;
    std::string              colorOutput;
    std::vector<std::string> extraParams;
    std::vector<std::string> samplerNames;
    std::string              functionName;
    std::string              sourceCode;
};

std::map<std::string, OcioData> knownOCIOFragments;
std::vector<std::string>        knownOCIOImplementations;
DocumentPtr                     knownLibrary;

std::string getUntypedNodeDefName(const std::string& nodeName)
{
    return OCIO_ND_PREFIX + nodeName + "_";
}

std::string getUntypedImplementationName(const std::string& nodeName)
{
    return OCIO_IM_PREFIX + nodeName + "_";
}

const OcioData& getOcioData(const std::string& implName)
{
    // Would be so much faster with removeprefix and removesuffix:
    auto nodeName = implName.substr(
        OCIO_IM_PREFIX_LEN - 1, implName.size() - OCIO_IM_PREFIX_LEN - OCIO_COLOR3_LEN + 1);

    auto it = knownOCIOFragments.find(nodeName);
    if (it == knownOCIOFragments.end()) {
        throw std::runtime_error("Missing OCIO data");
    }
    return it->second;
}

void addOCIONodeDef(const OcioData& ocioData, const std::string& output_type)
{
    if (!knownLibrary) {
        knownLibrary = createDocument();
    }

    std::string defName = getUntypedNodeDefName(ocioData.fragName) + output_type;
    std::string implName = getUntypedImplementationName(ocioData.fragName) + output_type;

    auto nodeDef = knownLibrary->addNodeDef(defName, "", ocioData.fragName);

    nodeDef->addInput(ocioData.colorInput, output_type);
    nodeDef->addOutput(ocioData.colorOutput, output_type);

    auto implementation = knownLibrary->addImplementation(implName);
    implementation->setTarget(GlslShaderGenerator::TARGET);
    implementation->setNodeDef(nodeDef);

    knownOCIOImplementations.push_back(implName);
}

class GlslOcioNodeData;
using GlslOcioNodeDataPtr = shared_ptr<class GlslOcioNodeData>;
class GlslOcioNodeData : public GenUserData
{
public:
    static const std::string& name()
    {
        static const string OCIO_DATA = "GlslOcioNodeData";
        return OCIO_DATA;
    }

    /// Create and return a new instance.
    static GlslOcioNodeDataPtr create() { return std::make_shared<GlslOcioNodeData>(); }

    /// All OCIO code blocks already emitted:
    std::set<std::string> emittedOcioBlocks;
};
} // namespace

std::string GlslOcioNodeImpl::registerOCIOFragment(const std::string& fragName)
{
    if (knownOCIOFragments.count(fragName)) {
        return getUntypedNodeDefName(fragName);
    }

    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) {
        return {};
    }

    MHWRender::MFragmentManager* fragmentManager = theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return {};
    }

    MString fragText;
    if (!fragmentManager->getFragmentXML(fragName.c_str(), fragText)) {
        return {};
    }

    pugi::xml_document     doc;
    pugi::xml_parse_result result = doc.load_string(fragText.asChar());

    if (!result) {
        MString errorMsg = "XML error parsing fragment for ";
        errorMsg += fragName.c_str();
        errorMsg += " at character ";
        errorMsg += static_cast<int>(result.offset);
        errorMsg += ": ";
        errorMsg += result.description();
        MGlobal::displayError(errorMsg);
        return {};
    }

    OcioData ocioData;

    // Validate that the fragment structure is 100% as expected. We need the properties, values,
    // outputs, and GLSL source to proceed:
    auto fragment = doc.child(TAG_FRAGMENT);
    if (!fragment) {
        return {};
    }

    ocioData.fragName = fragment.attribute(ATTR_NAME).as_string();
    if (ocioData.fragName.empty()) {
        return {};
    }

    auto properties = fragment.child(TAG_PROPERTIES);
    if (!properties) {
        return {};
    }
    bool skipColorInput = true;
    for (auto&& property : fragment.child(TAG_PROPERTIES).children()) {
        if (skipColorInput) {
            skipColorInput = false;
            continue;
        }
        if (std::strncmp(property.name(), TAG_TEXTURE2, TAG_TEXTURE2_LEN) == 0) {
            std::string samplerName = property.attribute(ATTR_NAME).as_string();
            samplerName += "Sampler";
            ocioData.samplerNames.emplace_back(samplerName);
            continue;
        }
        ocioData.extraParams.emplace_back(property.attribute(ATTR_NAME).as_string());
    }

    auto values = fragment.child(TAG_VALUES);
    if (!values) {
        return {};
    }
    auto inputInfo = fragment.child(TAG_VALUES).child(TAG_FLOAT3);
    ocioData.colorInput = inputInfo.attribute(ATTR_NAME).as_string();

    auto outputs = fragment.child(TAG_OUTPUTS);
    if (!outputs) {
        return {};
    }
    auto outputInfo = fragment.child(TAG_OUTPUTS).child(TAG_FLOAT3);
    ocioData.colorOutput = outputInfo.attribute(ATTR_NAME).as_string();

    auto implementations = fragment.child(TAG_IMPLEMENTATION);
    if (!implementations) {
        return {};
    }
    for (auto&& implementation : implementations.children()) {
        auto language = implementation.attribute(ATTR_LANGUAGE);
        if (std::strncmp(language.as_string(), OCIO_GLSL, OCIO_GLSL_LEN) == 0) {
            ocioData.sourceCode = implementation.child_value(TAG_SOURCE);
            ocioData.functionName
                = implementation.child(TAG_FUNCTION_NAME).attribute(ATTR_VAL).as_string();
            break;
        }
    }
    if (ocioData.sourceCode.empty() || ocioData.functionName.empty()) {
        return {};
    }

    // We now have all the data we need. Preserve the info and add our new OCIO NodeDef in the
    // library.
    addOCIONodeDef(ocioData, OCIO_COLOR3);
    addOCIONodeDef(ocioData, OCIO_COLOR4);
    knownOCIOFragments.emplace(fragName, std::move(ocioData));

    return getUntypedNodeDefName(fragName);
}

DocumentPtr GlslOcioNodeImpl::getOCIOLibrary() { return knownLibrary; }

const std::vector<std::string>& GlslOcioNodeImpl::getOCIOImplementations()
{
    return knownOCIOImplementations;
}

ShaderNodeImplPtr GlslOcioNodeImpl::create() { return std::make_shared<GlslOcioNodeImpl>(); }

void GlslOcioNodeImpl::createVariables(const ShaderNode& node, GenContext& context, Shader& shader)
    const
{
    const OcioData& ocioData = getOcioData(getName());

    for (auto&& samplerName : ocioData.samplerNames) {
        addStageUniform(
            HW::PUBLIC_UNIFORMS, Type::FILENAME, samplerName, shader.getStage(Stage::PIXEL));
    }
}

void GlslOcioNodeImpl::emitFunctionDefinition(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    if (stage.getName() == Stage::PIXEL) {
        const OcioData& ocioData = getOcioData(getName());

        // Since the color3 and color4 implementations share the same code block, we need to make
        // sure the definition is only emitted once.
        auto pOcioData = context.getUserData<GlslOcioNodeData>(GlslOcioNodeData::name());
        if (!pOcioData) {
            context.pushUserData(GlslOcioNodeData::name(), GlslOcioNodeData::create());
            pOcioData = context.getUserData<GlslOcioNodeData>(GlslOcioNodeData::name());
        }

        if (pOcioData->emittedOcioBlocks.count(ocioData.fragName)) {
            return;
        }
        pOcioData->emittedOcioBlocks.insert(ocioData.fragName);

        stage.addString(ocioData.sourceCode);
        stage.endLine(false);
    }
}

void GlslOcioNodeImpl::emitFunctionCall(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    if (stage.getName() == Stage::PIXEL) {
        std::string     implName = getName();
        const OcioData& ocioData = getOcioData(implName);

        // Function call for color4: vec4 res = vec4(func(in.rgb, ...), in.a);
        // Function call for color3: vec3 res =      func(in, ...);
        bool isColor4 = implName[implName.size() - 1] == '4';

        const ShaderGenerator& shadergen = context.getShaderGenerator();
        shadergen.emitLineBegin(stage);

        const ShaderOutput* output = node.getOutput();
        const ShaderInput*  colorInput = node.getInput(0);

        shadergen.emitOutput(output, true, false, context, stage);
        shadergen.emitString(" =", stage);

        if (isColor4) {
            shadergen.emitString(" vec4(", stage);
        }

        shadergen.emitString(ocioData.functionName + "(", stage);
        shadergen.emitInput(colorInput, context, stage);
        if (isColor4) {
            shadergen.emitString(".rgb", stage);
        }

        for (auto&& extraParam : ocioData.extraParams) {
            shadergen.emitString(", ", stage);
            shadergen.emitString(extraParam, stage);
        }

        shadergen.emitString(")", stage);

        if (isColor4) {
            shadergen.emitString(", ", stage);
            shadergen.emitInput(colorInput, context, stage);
            shadergen.emitString(".a", stage);
            shadergen.emitString(")", stage);
        }

        shadergen.emitLineEnd(stage);
    }
}

MATERIALX_NAMESPACE_END
