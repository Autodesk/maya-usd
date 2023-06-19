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
constexpr const char TAG_SAMPLER[] = "sampler";
constexpr const char TAG_SOURCE[] = "source";

// Lengths where needed:
constexpr auto TAG_TEXTURE2_LEN = sizeof(TAG_TEXTURE2) / sizeof(TAG_TEXTURE2[0]);

// Expected XML attribute names for an OCIO fragment:
constexpr const char ATTR_NAME[] = "name";
constexpr const char ATTR_LANGUAGE[] = "language";
constexpr const char ATTR_VAL[] = "val";

std::map<std::string, pugi::xml_document> knownOCIOFragments;
std::vector<std::string>                  knownOCIOImplementations;

std::string getUntypedNodeDefName(const std::string& nodeName)
{
    return OCIO_ND_PREFIX + nodeName + "_";
}

std::string getUntypedImplementationName(const std::string& nodeName)
{
    return OCIO_IM_PREFIX + nodeName + "_";
}

const pugi::xml_document& docFromImplementationName(const std::string& implName)
{
    // Would be so much faster with removeprefix and removesuffix:
    auto nodeName = implName.substr(
        OCIO_IM_PREFIX_LEN - 1, implName.size() - OCIO_IM_PREFIX_LEN - OCIO_COLOR3_LEN + 1);

    auto docIt = knownOCIOFragments.find(nodeName);
    if (docIt != knownOCIOFragments.end()) {
        return docIt->second;
    }
    static const pugi::xml_document emptyDoc;
    return emptyDoc;
}

void addOCIONodeDef(DocumentPtr lib, const pugi::xml_document& doc, const std::string& output_type)
{
    std::string nodeName = doc.child(TAG_FRAGMENT).attribute(ATTR_NAME).as_string();
    std::string defName = getUntypedNodeDefName(nodeName) + output_type;
    std::string implName = getUntypedImplementationName(nodeName) + output_type;

    auto nodeDef = lib->addNodeDef(defName, "", nodeName);

    auto inputInfo = doc.child(TAG_FRAGMENT).child(TAG_VALUES).child(TAG_FLOAT3);
    nodeDef->addInput(inputInfo.attribute(ATTR_NAME).as_string(), output_type);

    auto outputInfo = doc.child(TAG_FRAGMENT).child(TAG_OUTPUTS).child(TAG_FLOAT3);
    nodeDef->addOutput(outputInfo.attribute(ATTR_NAME).as_string(), output_type);

    auto implementation = lib->addImplementation(implName);
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

std::string
GlslOcioNodeImpl::registerOCIOFragment(const std::string& fragName, DocumentPtr mtlxLibrary)
{
    if (knownOCIOFragments.count(fragName)) {
        // Make sure the NodeDef are there:
        if (!mtlxLibrary->getNodeDef(getUntypedNodeDefName(fragName) + OCIO_COLOR3)) {
            addOCIONodeDef(mtlxLibrary, knownOCIOFragments[fragName], OCIO_COLOR3);
        }

        if (!mtlxLibrary->getNodeDef(getUntypedNodeDefName(fragName) + OCIO_COLOR4)) {
            addOCIONodeDef(mtlxLibrary, knownOCIOFragments[fragName], OCIO_COLOR4);
        }

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

    // Validate that the fragment structure is 100% as expected. We need the properties, values,
    // outputs, and GLSL source to proceed:
    auto fragment = doc.child(TAG_FRAGMENT);
    if (!fragment) {
        return {};
    }

    auto properties = fragment.child(TAG_PROPERTIES);
    if (!properties) {
        return {};
    }

    auto values = fragment.child(TAG_VALUES);
    if (!values) {
        return {};
    }

    auto outputs = fragment.child(TAG_OUTPUTS);
    if (!outputs) {
        return {};
    }

    auto implementations = fragment.child(TAG_IMPLEMENTATION);
    if (!implementations) {
        return {};
    }
    bool hasGLSL = false;
    for (auto&& implementation : implementations.children()) {
        auto language = implementation.attribute(ATTR_LANGUAGE);
        if (std::strncmp(language.as_string(), OCIO_GLSL, OCIO_GLSL_LEN) == 0) {
            hasGLSL = true;
            break;
        }
    }
    if (!hasGLSL) {
        return {};
    }

    // We now have all the data we need. Preserve the info and add our new OCIO NodeDef in the
    // library.
    addOCIONodeDef(mtlxLibrary, doc, OCIO_COLOR3);
    addOCIONodeDef(mtlxLibrary, doc, OCIO_COLOR4);
    knownOCIOFragments.emplace(fragName, std::move(doc));

    return getUntypedNodeDefName(fragName);
}

const std::vector<std::string>& GlslOcioNodeImpl::getOCIOImplementations()
{
    return knownOCIOImplementations;
}

ShaderNodeImplPtr GlslOcioNodeImpl::create() { return std::make_shared<GlslOcioNodeImpl>(); }

void GlslOcioNodeImpl::createVariables(const ShaderNode& node, GenContext& context, Shader& shader)
    const
{
    const auto& ocioDoc = docFromImplementationName(getName());
    if (ocioDoc.empty()) {
        return;
    }

    // Need to create texture inputs if required:
    for (auto&& property : ocioDoc.child(TAG_FRAGMENT).child(TAG_PROPERTIES).children()) {
        if (std::strncmp(property.name(), TAG_TEXTURE2, TAG_TEXTURE2_LEN) == 0) {
            std::string samplerName = property.attribute(ATTR_NAME).as_string();
            samplerName += "Sampler";
            addStageUniform(
                HW::PUBLIC_UNIFORMS, Type::FILENAME, samplerName, shader.getStage(Stage::PIXEL));
        }
    }
}

void GlslOcioNodeImpl::emitFunctionDefinition(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    if (stage.getName() == Stage::PIXEL) {
        const auto& ocioDoc = docFromImplementationName(getName());
        if (ocioDoc.empty()) {
            return;
        }

        // Since the color3 and color4 implementations share the same code block, we need to make
        // sure the definition is only emitted once.
        auto pOcioData = context.getUserData<GlslOcioNodeData>(GlslOcioNodeData::name());
        if (!pOcioData) {
            context.pushUserData(GlslOcioNodeData::name(), GlslOcioNodeData::create());
            pOcioData = context.getUserData<GlslOcioNodeData>(GlslOcioNodeData::name());
        }
        const auto* fragName = ocioDoc.child(TAG_FRAGMENT).attribute(ATTR_NAME).as_string();
        if (pOcioData->emittedOcioBlocks.count(fragName)) {
            return;
        }
        pOcioData->emittedOcioBlocks.insert(fragName);

        auto implementations = ocioDoc.child(TAG_FRAGMENT).child(TAG_IMPLEMENTATION);
        if (!implementations) {
            return;
        }
        for (auto&& implementation : implementations.children()) {
            auto language = implementation.attribute(ATTR_LANGUAGE);
            if (std::strncmp(language.as_string(), OCIO_GLSL, OCIO_GLSL_LEN) == 0) {
                stage.addString(implementation.child_value(TAG_SOURCE));
                stage.endLine(false);
                return;
            }
        }
    }
}

void GlslOcioNodeImpl::emitFunctionCall(
    const ShaderNode& node,
    GenContext&       context,
    ShaderStage&      stage) const
{
    if (stage.getName() == Stage::PIXEL) {
        std::string implName = getName();
        const auto& ocioDoc = docFromImplementationName(implName);
        if (ocioDoc.empty()) {
            return;
        }
        std::string functionName;

        auto implementations = ocioDoc.child(TAG_FRAGMENT).child(TAG_IMPLEMENTATION);
        if (!implementations) {
            return;
        }
        for (auto&& implementation : implementations.children()) {
            auto language = implementation.attribute(ATTR_LANGUAGE);
            if (std::strncmp(language.as_string(), OCIO_GLSL, OCIO_GLSL_LEN) == 0) {

                functionName
                    = implementation.child(TAG_FUNCTION_NAME).attribute(ATTR_VAL).as_string();
                break;
            }
        }

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

        shadergen.emitString(functionName + "(", stage);
        shadergen.emitInput(colorInput, context, stage);
        if (isColor4) {
            shadergen.emitString(".rgb", stage);
        }

        bool skipColorInput = true;
        for (auto&& prop : ocioDoc.child(TAG_FRAGMENT).child(TAG_PROPERTIES).children()) {
            if (skipColorInput) {
                // Skip the color input as it was processed above
                skipColorInput = false;
                continue;
            }
            if (std::strncmp(prop.name(), TAG_TEXTURE2, TAG_TEXTURE2_LEN) == 0) {
                // Skip texture inputs since we only need the sampler
                continue;
            }
            // Add all other parameters as-is since this will be the uniform parameter name
            shadergen.emitString(", ", stage);
            shadergen.emitString(prop.attribute(ATTR_NAME).as_string(), stage);
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
