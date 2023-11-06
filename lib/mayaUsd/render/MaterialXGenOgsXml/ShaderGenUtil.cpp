#include "ShaderGenUtil.h"

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#include <MaterialXCore/Document.h>

#include <sstream>

namespace MaterialXMaya {
namespace ShaderGenUtil {

namespace {
const std::string SURFACEMATERIAL_CATEGORY("surfacematerial");
const std::string SURFACESHADER_TYPE("surfaceshader");

const std::set<std::string> _mtlxTopoNodeSet = {
    // Topo affecting nodes due to object/model/world space parameter
    "position",
    "normal",
    "tangent",
    "bitangent",
    // Topo affecting nodes due to channel index.
    "texcoord",
    // Color at vertices also affect topo, but we have not locked a naming scheme to go from index
    // based to name based as we did for UV sets. We will mark them as topo-affecting, but there is
    // nothing we can do to link them correctly to a primvar without specifying a naming scheme.
    "geomcolor",
    // Geompropvalue are the best way to reference a primvar by name. The primvar name is
    // topo-affecting. Note that boolean and string are not supported by the GLSL codegen.
    "geompropvalue",
    // Swizzles are inlined into the codegen and affect topology.
    "swizzle",
    // Conversion nodes:
    "convert",
    // Constants: they get inlined in the source.
    "constant",
#if MX_COMBINED_VERSION < 13808
    // Switch, unless all inputs are connected. Bug was fixed in 1.38.8.
    "switch",
#endif
#if MX_COMBINED_VERSION == 13807
    // Dot became topological in 1.38.7. Reverted in 1.38.8.
    // Still topological for filename though.
    "dot",
#endif
};
} // namespace

const std::string& TopoNeutralGraph::getMaterialName()
{
    // A material node is always the first node created and will be named N0
    static const std::string kMaterialName("N0");
    return kMaterialName;
}

TopoNeutralGraph::TopoNeutralGraph(const mx::ElementPtr& material)
{
    if (!material) {
        throw mx::Exception("Invalid material element");
    }
    std::string message;
    if (!material->validate(&message)) {
        const std::string step("Error in original graph:\n");
        throw mx::Exception(step + message);
    }
    _doc = mx::createDocument();

    auto inputDoc = material->getDocument();

    mx::NodePtr materialNode = material->asA<mx::Node>();
    if (!materialNode) {
        // We might handle standalone "Output" element at a later stage
        throw mx::Exception("Material element is not a node.");
    }

    std::list<mx::NodePtr> nodesToTraverse;
    mx::NodePtr            surfaceShader;
    if (materialNode->getCategory() == SURFACEMATERIAL_CATEGORY) {
        auto dupMaterial = cloneNode(*materialNode, *_doc);
        auto surfaceInput = materialNode->getInput(SURFACESHADER_TYPE);
        if (!surfaceInput || !surfaceInput->getConnectedNode()) {
            throw mx::Exception("Unconnected material node.");
        }
        surfaceShader = surfaceInput->getConnectedNode();
        if (!surfaceShader) {
            throw mx::Exception("Unconnected material node.");
        }
        auto dupSurfaceShader = cloneNode(*surfaceShader, *_doc);
        dupMaterial->addInput(SURFACESHADER_TYPE, SURFACESHADER_TYPE)
            ->setConnectedNode(dupSurfaceShader);
        nodesToTraverse.push_back(surfaceShader);
    } else {
        if (materialNode->getType() != SURFACESHADER_TYPE) {
            throw mx::Exception("Material shader node is not a surfaceshader.");
        }
        auto dupMaterial = _doc->addMaterialNode("N" + std::to_string(_nodeIndex));
        dupMaterial->setNodeDefString("ND_surfacematerial");
        ++_nodeIndex;
        surfaceShader = materialNode;
        auto dupSurfaceShader = cloneNode(*materialNode, *_doc);
        dupMaterial->addInput(SURFACESHADER_TYPE, SURFACESHADER_TYPE)
            ->setConnectedNode(dupSurfaceShader);
        nodesToTraverse.push_back(materialNode);
    }

    // Breadth-first traversal, in order of NodeDef attributes, to insure repeatability
    while (!nodesToTraverse.empty()) {
        const mx::Node& sourceNode = *nodesToTraverse.front();
        nodesToTraverse.pop_front();

        auto        destNodeIt = _nodeMap.find(sourceNode.getNamePath());
        mx::NodePtr destNode;
        if (destNodeIt != _nodeMap.end()) {
            destNode = destNodeIt->second;
        } else {
            if (!_nodeGraph) {
                _nodeGraph = _doc->addNodeGraph("NG0");
            }
            destNode = cloneNode(sourceNode, *_nodeGraph);
        }

        auto sourceNodeDef = sourceNode.getNodeDef();
        if (!sourceNodeDef) {
            throw mx::Exception("Could not find NodeDef.");
        }

        const bool isTopological = isTopologicalNodeDef(*sourceNodeDef);
        for (const auto& defInput : sourceNodeDef->getActiveInputs()) {
            auto sourceInput = sourceNode.getInput(defInput->getName());
            if (!sourceInput) {
                continue;
            }

            auto connectedNode = sourceInput->getConnectedNode();
            if (connectedNode) {
                auto        destConnectedIt = _nodeMap.find(connectedNode->getNamePath());
                mx::NodePtr destConnectedNode;
                if (destConnectedIt != _nodeMap.end()) {
                    destConnectedNode = destConnectedIt->second;
                } else {
                    if (!_nodeGraph) {
                        _nodeGraph = _doc->addNodeGraph("NG0");
                    }
                    destConnectedNode = cloneNode(*connectedNode, *_nodeGraph);
                    nodesToTraverse.push_back(connectedNode);
                }

                const std::string channelInfo = gatherChannels(*sourceInput);
                const std::string outputString = gatherOutput(*sourceInput);

                if (sourceNode != *surfaceShader) {
                    cloneConnection(
                        *sourceInput, *destNode, destConnectedNode, channelInfo, outputString);
                } else {
                    cloneNodeGraphConnection(
                        *sourceInput, *destNode, destConnectedNode, channelInfo, outputString);
                }
            } else if (isTopological) {
                std::string valueString = sourceInput->getValueString();
                if (valueString.empty()) {
                    const auto interfaceInput = sourceInput->getInterfaceInput();
                    if (interfaceInput) {
                        valueString = interfaceInput->getValueString();
                    }
                }
                if (!valueString.empty()) {
                    auto destInput
                        = destNode->addInput(sourceInput->getName(), sourceInput->getType());
                    destInput->setValueString(valueString);
                }
            }
        }
    }
}

mx::NodePtr TopoNeutralGraph::cloneNode(const mx::Node& node, mx::GraphElement& container)
{
    auto destNode
        = container.addNode(node.getCategory(), "N" + std::to_string(_nodeIndex), node.getType());
    ++_nodeIndex;
    _nodeMap.insert({ node.getNamePath(), destNode });
    _nodeMap.insert({ node.getNamePath(), destNode });
    _pathMap.insert({ destNode->getNamePath(), node.getNamePath() });
    // Always be explicit on the NodeDef:
    auto nodeDef = node.getNodeDef();
    if (!nodeDef) {
        throw mx::Exception("Ambiguous node is not fully resolvable");
    }
    destNode->setNodeDefString(nodeDef->getName());
    return destNode;
}

const std::string& TopoNeutralGraph::getOriginalPath(const std::string& topoPath) const
{
    auto it = _pathMap.find(topoPath);
    if (it == _pathMap.end()) {
        throw mx::Exception("Could not find original path for " + topoPath);
    }
    return it->second;
}

bool TopoNeutralGraph::isTopologicalNodeDef(const mx::NodeDef& nodeDef)
{
    // This is where we need to remove all these hardcoded names and instead ask the shadergen about
    // the info. Requires a shadergen that can tell if a node is topological (usually C++ nodes that
    // have custom shader code that varies when input value varies)

    // This is the hardcoded list for the GLSL shader generator:
#if MX_COMBINED_VERSION >= 13807
    // Dot filename is always topological to prevent creating extra OpenGL samplers in the
    // generated OpenGL code.
    if (nodeDef.getName() == "ND_dot_filename")
        return true;
#endif
    return _mtlxTopoNodeSet.find(nodeDef.getNodeString()) != _mtlxTopoNodeSet.cend();
}

mx::DocumentPtr TopoNeutralGraph::getDocument() const { return _doc; }

mx::OutputPtr
TopoNeutralGraph::findNodeGraphOutput(const mx::Input& input, const std::string& outputName)
{
    auto sourceNode = input.getParent();
    if (!sourceNode || !sourceNode->isA<mx::Node>()) {
        return nullptr;
    }

    auto scope = sourceNode->getParent();
    if (!scope || !scope->isA<mx::Document>()) {
        return nullptr;
    }

    auto nodeGraph = scope->asA<mx::Document>()->getNodeGraph(input.getNodeGraphString());
    if (!nodeGraph) {
        return nullptr;
    }

    return nodeGraph->getOutput(outputName);
}

std::string TopoNeutralGraph::gatherChannels(const mx::Input& input)
{
    // The info we seek might be on the interface of a standalone NodeGraph:
    const auto  interfaceInput = input.getInterfaceInput();
    const auto& ngInput = interfaceInput ? *interfaceInput : input;

    std::string channelInfo = ngInput.getChannels();

    if (!ngInput.hasNodeGraphString()) {
        if (ngInput.hasNodeName()) {
            return channelInfo;
        } else {
            throw mx::Exception("We do not support standalone Output elements");
        }
    }

    // See if we have extra channels on the NodeGraph output:
    auto output = findNodeGraphOutput(ngInput, ngInput.getOutputString());
    if (!output) {
        throw mx::Exception("Could not find nodegraph");
    }

    const std::string outputChannels = output->getChannels();
    if (outputChannels.empty()) {
        return channelInfo;
    } else if (channelInfo.empty()) {
        return outputChannels;
    }

    // Here we must combine the channels.
    std::string combinedChannels;
    combinedChannels.reserve(channelInfo.size());

    for (const char c : channelInfo) {
        switch (c) {
        case '0':
        case '1': combinedChannels.push_back(c); break;
        case 'r':
        case 'x':
            // We know from above the string is not empty.
            combinedChannels.push_back(outputChannels[0]);
            break;
        case 'g':
        case 'y':
            if (outputChannels.size() < 2) {
                throw mx::Exception("Missing channels in outputChannels");
            }
            combinedChannels.push_back(outputChannels[1]);
            break;
        case 'b':
        case 'z':
            if (outputChannels.size() < 3) {
                throw mx::Exception("Missing channels in outputChannels");
            }
            combinedChannels.push_back(outputChannels[2]);
            break;
        case 'a':
        case 'w':
            if (outputChannels.size() < 4) {
                throw mx::Exception("Missing channels in outputChannels");
            }
            combinedChannels.push_back(outputChannels[3]);
            break;
        default: throw mx::Exception("Invalid channel name");
        }
    }
    return combinedChannels;
}

std::string TopoNeutralGraph::gatherOutput(const mx::Input& input)
{
    // The info we seek might be on the interface of a standalone NodeGraph:
    const auto  interfaceInput = input.getInterfaceInput();
    const auto& ngInput = interfaceInput ? *interfaceInput : input;

    std::string outputString = ngInput.getOutputString();

    if (!ngInput.hasNodeGraphString()) {
        if (ngInput.hasNodeName()) {
            return outputString;
        } else {
            throw mx::Exception("We do not support standalone Output elements");
        }
    }

    // See if we have extra channels on the NodeGraph output:
    auto output = findNodeGraphOutput(ngInput, ngInput.getOutputString());
    if (!output) {
        throw mx::Exception("Could not find nodegraph");
    }

    return output->getOutputString();
}

void TopoNeutralGraph::cloneConnection(
    const mx::Input&   sourceInput,
    mx::Node&          destNode,
    mx::NodePtr&       destConnectedNode,
    const std::string& channelInfo,
    const std::string& output)
{
    auto destInput = destNode.addInput(sourceInput.getName(), sourceInput.getType());
    destInput->setConnectedNode(destConnectedNode);
    if (!channelInfo.empty()) {
        destInput->setChannels(channelInfo);
    }
    if (!output.empty()) {
        destInput->setOutputString(output);
    }
}

void TopoNeutralGraph::cloneNodeGraphConnection(
    const mx::Input&   sourceInput,
    mx::Node&          destNode,
    mx::NodePtr&       destConnectedNode,
    const std::string& channelInfo,
    const std::string& output)
{
    std::string outputKey = destConnectedNode->getName() + "(t)" + sourceInput.getType() + "(c)"
        + channelInfo + "(o)" + output;
    mx::OutputPtr graphOutput;
    auto          outputIt = _outputMap.find(outputKey);
    if (outputIt != _outputMap.end()) {
        graphOutput = outputIt->second;
    } else {
        graphOutput
            = _nodeGraph->addOutput("O" + std::to_string(_outputIndex), sourceInput.getType());
        if (!channelInfo.empty()) {
            graphOutput->setChannels(channelInfo);
        }
        if (!output.empty()) {
            graphOutput->setOutputString(output);
        }
        ++_outputIndex;
        _outputMap.insert({ outputKey, graphOutput });
        graphOutput->setConnectedNode(destConnectedNode);
        auto destInput = destNode.addInput(sourceInput.getName(), sourceInput.getType());
        destInput->setConnectedOutput(graphOutput);
    }
}

} // namespace ShaderGenUtil
} // namespace MaterialXMaya