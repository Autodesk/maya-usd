#include "LobePruner.h"

#include "MaterialXCore/Library.h"
#include "MaterialXCore/Types.h"

#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Exception.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace MaterialXMaya {
namespace ShaderGenUtil {

namespace {

const auto kBasePbrNodes = std::set<std::string> {
    "oren_nayar_diffuse_bsdf", "compensating_oren_nayar_diffuse_bsdf",
    "burley_diffuse_bsdf",     "conductor_bsdf",
    "subsurface_bsdf",         "translucent_bsdf",
};

const auto kLayerPbrNodes = std::set<std::string> {
    "dielectric_bsdf",    "generalized_schlick_bsdf",       "sheen_bsdf",
    "dielectric_tf_bsdf", "generalized_schlick_tf_82_bsdf", "sheen_zeltner_bsdf"
};
} // namespace

class LobePrunerImpl
{
    // Data layout:
    //    map< nodeDefName,
    //        {                             // NodeDefData
    //            nodeGraphName,
    //            map< attributeName,       // AttributeMap
    //                map< attributeValue,  // OptimizableValueMap
    //                     NodeVector
    //                >
    //            >
    //        }
    //    >
    using NodeVector = std::vector<PXR_NS::TfToken>;
    using OptimizableValueMap = std::map<float, NodeVector>;
    // We want attributes alphabetically sorted:
    using AttributeMap = std::map<PXR_NS::TfToken, OptimizableValueMap>;
    struct NodeDefData
    {
        PXR_NS::TfToken _nodeGraphName;
        AttributeMap    _attributeData;
    };

    // Also helps if we have a reverse connection map from source node to dest node:
    using Destinations = std::vector<std::string>;
    using ReverseCnxMap = std::map<std::string, Destinations>;

public:
    explicit LobePrunerImpl(const mx::DocumentPtr& library);
    ~LobePrunerImpl() = default;
    LobePrunerImpl() = delete;
    LobePrunerImpl(const LobePrunerImpl&) = delete;
    LobePrunerImpl& operator=(const LobePrunerImpl&) = delete;
    LobePrunerImpl(LobePrunerImpl&&) = delete;
    LobePrunerImpl& operator=(LobePrunerImpl&&) = delete;

    bool          getOptimizedNodeCategory(const mx::Node& node, std::string& nodeCategory);
    mx::StringVec getOptimizedAttributeNames(const mx::NodeDefPtr& nodeDef) const;

    PXR_NS::TfToken getOptimizedNodeId(const PXR_NS::HdMaterialNode2& node);

    static const std::string ND_PREFIX;
    static const std::string DARK_BASE;
    static const std::string DARK_LAYER;
    static const std::string ND_DARK_BASE;
    static const std::string ND_DARK_LAYER;
    static const std::string IM_DARK_BASE;
    static const std::string IM_DARK_LAYER;

private:
    bool isLobeInput(const mx::InputPtr& input, const mx::NodeDefPtr& nd) const;
    void addOptimizableValue(
        float                   value,
        const mx::InputPtr&     input,
        const mx::NodeGraphPtr& ng,
        const mx::NodeDefPtr&   nd);
    void
         ensureLibraryHasOptimizedShader(const PXR_NS::TfToken& nodeDefName, const std::string& flags);
    void optimizeZeroValue(
        mx::NodeGraphPtr&          newNg,
        const OptimizableValueMap& optimizationMap,
        ReverseCnxMap&             reverseMap);
    void optimizeOneValue(
        mx::NodeGraphPtr&          newNg,
        const OptimizableValueMap& optimizationMap,
        ReverseCnxMap&             reverseMap);
    void addDarkShaders();

    std::unordered_map<PXR_NS::TfToken, NodeDefData, PXR_NS::TfToken::HashFunctor> _prunerData;
    mx::DocumentPtr                                                                _library;
};

const std::string LobePrunerImpl::ND_PREFIX = "LPOPTIND_";
const std::string LobePrunerImpl::DARK_BASE = "lobe_pruner_optimization_dark_base_bsdf";
const std::string LobePrunerImpl::DARK_LAYER = "lobe_pruner_optimization_dark_layer_bsdf";
const std::string LobePrunerImpl::ND_DARK_BASE = "ND_" + LobePrunerImpl::DARK_BASE;
const std::string LobePrunerImpl::ND_DARK_LAYER = "ND_" + LobePrunerImpl::DARK_LAYER;
const std::string LobePrunerImpl::IM_DARK_BASE
    = "IM_" + LobePrunerImpl::DARK_BASE + mx::GlslShaderGenerator::TARGET;
const std::string LobePrunerImpl::IM_DARK_LAYER
    = "IM_" + LobePrunerImpl::DARK_LAYER + mx::GlslShaderGenerator::TARGET;

LobePrunerImpl::LobePrunerImpl(const mx::DocumentPtr& library)
    : _library(library)
{
    if (!library) {
        throw mx::Exception("Requires a library");
    }

    addDarkShaders();

    // Browse all surface shaders and identify prunable lobes:
    for (const auto& nd : _library->getNodeDefs()) {
        const auto outputs = nd->getActiveOutputs();
        if (outputs.size() != 1 || outputs.front()->getType() != "surfaceshader") {
            continue;
        }

        const auto impl = nd->getImplementation(mx::GlslShaderGenerator::TARGET);
        if (!impl) {
            continue;
        }
        const auto ng = impl->isA<mx::NodeGraph>()
            ? impl->asA<mx::NodeGraph>()
            : _library->getNodeGraph(impl->asA<mx::Implementation>()->getNodeGraph());
        if (!ng) {
            continue;
        }

        for (const auto& node : ng->getNodes()) {
            if (node->getCategory() == "mix") {
                const auto nodeInput = node->getActiveInput("mix");
                if (nodeInput && isLobeInput(nodeInput, nd)) {
                    addOptimizableValue(0.0F, nodeInput, ng, nd);
                    addOptimizableValue(1.0F, nodeInput, ng, nd);
                }
            } else if (node->getCategory() == "multiply") {
                for (const auto& nodeInput : node->getActiveInputs()) {
                    if (nodeInput && isLobeInput(nodeInput, nd)) {
                        addOptimizableValue(0.0F, nodeInput, ng, nd);
                    }
                }
            } else if (
                kBasePbrNodes.count(node->getCategory())
                || kLayerPbrNodes.count(node->getCategory())) {
                const auto nodeInput = node->getActiveInput("weight");
                if (nodeInput && isLobeInput(nodeInput, nd)) {
                    addOptimizableValue(0.0F, nodeInput, ng, nd);
                }
            }
        }
    }
}

bool LobePrunerImpl::isLobeInput(const mx::InputPtr& input, const mx::NodeDefPtr& nd) const
{
    if (!input->hasInterfaceName() || input->getType() != "float") {
        return false;
    }
    const auto& ndInput = nd->getActiveInput(input->getInterfaceName());
    if (!ndInput || !ndInput->hasAttribute("uimin") || !ndInput->hasAttribute("uimax")) {
        return false;
    }
    const auto minVal = std::stof(ndInput->getAttribute("uimin"));
    if (minVal != 0.0F) {
        return false;
    }
    const auto maxVal = std::stof(ndInput->getAttribute("uimax"));
    if (maxVal != 1.0F) {
        return false;
    }
    return true;
}

void LobePrunerImpl::addOptimizableValue(
    float                   value,
    const mx::InputPtr&     input,
    const mx::NodeGraphPtr& ng,
    const mx::NodeDefPtr&   nd)
{
    const auto& nodeDefName = PXR_NS::TfToken(nd->getName());
    if (!_prunerData.count(nodeDefName)) {
        _prunerData.emplace(
            nodeDefName, NodeDefData { PXR_NS::TfToken(ng->getName()), AttributeMap {} });
    }
    auto& attrMap = _prunerData.find(nodeDefName)->second;

    const auto& interfaceName = PXR_NS::TfToken(input->getInterfaceName());
    if (!attrMap._attributeData.count(interfaceName)) {
        attrMap._attributeData.emplace(interfaceName, OptimizableValueMap {});
    }
    auto& valueMap = attrMap._attributeData.find(interfaceName)->second;

    if (!valueMap.count(value)) {
        valueMap.emplace(value, NodeVector {});
    }

    valueMap.find(value)->second.push_back(PXR_NS::TfToken(input->getParent()->getName()));
}

bool LobePrunerImpl::getOptimizedNodeCategory(const mx::Node& node, std::string& nodeCategory)
{
    const auto& nd = node.getNodeDef();
    if (!nd) {
        return false;
    }

    const auto ndName = PXR_NS::TfToken(nd->getName());
    const auto ndIt = _prunerData.find(ndName);
    if (ndIt == _prunerData.end()) {
        return false;
    }

    std::string flags(ndIt->second._attributeData.size(), 'x');

    bool canOptimize = false;

    auto attrIt = ndIt->second._attributeData.cbegin();
    for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
        const auto nodeinput = node.getActiveInput(attrIt->first);
        // Can not optimize if connected in any way.
        if (nodeinput
            && (nodeinput->hasNodeName() || nodeinput->hasOutputString()
                || nodeinput->hasInterfaceName())) {
            continue;
        }
        float inputValue = 0.5F;
        if (nodeinput) {
            inputValue = nodeinput->getValue()->asA<float>();
        } else {
            const auto defInput = nd->getActiveInput(attrIt->first);
            inputValue = defInput->getValue()->asA<float>();
        }
        for (const auto& optimizableValue : attrIt->second) {
            if (optimizableValue.first == inputValue) {
                if (inputValue == 0.0F) {
                    flags[i] = '0';
                } else {
                    flags[i] = '1';
                }
                canOptimize = true;
            }
        }
    }

    if (canOptimize) {
        ensureLibraryHasOptimizedShader(ndName, flags);
        nodeCategory = node.getCategory() + "_" + flags;
        return true;
    }

    return false;
}

mx::StringVec LobePrunerImpl::getOptimizedAttributeNames(const mx::NodeDefPtr& nodeDef) const
{
    mx::StringVec retVal;

    const auto ndIt = _prunerData.find(PXR_NS::TfToken(nodeDef->getName()));
    if (ndIt == _prunerData.end()) {
        return {};
    }

    retVal.reserve(ndIt->second._attributeData.size());
    for (const auto& attrData : ndIt->second._attributeData) {
        retVal.push_back(attrData.first);
    }

    return retVal;
}

PXR_NS::TfToken LobePrunerImpl::getOptimizedNodeId(const PXR_NS::HdMaterialNode2& node)
{
    PXR_NS::TfToken retVal;

    const auto ndIt = _prunerData.find(node.nodeTypeId);
    if (ndIt == _prunerData.end()) {
        return retVal;
    }

    const auto* nodeDef
        = PXR_NS::SdrRegistry::GetInstance().GetShaderNodeByIdentifier(node.nodeTypeId);

    std::string flags(ndIt->second._attributeData.size(), 'x');

    bool canOptimize = false;

    auto attrIt = ndIt->second._attributeData.cbegin();
    for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
        // Can not optimize if connected in any way.
        if (node.inputConnections.find(attrIt->first) != node.inputConnections.end()) {
            continue;
        }
        float      inputValue = 0.5F;
        const auto valueIt = node.parameters.find(attrIt->first);
        if (valueIt != node.parameters.end()) {
            inputValue = valueIt->second.UncheckedGet<float>();
        } else {
            const auto* defInput = nodeDef->GetShaderInput(attrIt->first);
            inputValue = defInput->GetDefaultValueAsSdfType().UncheckedGet<float>();
        }
        for (const auto& optimizableValue : attrIt->second) {
            if (optimizableValue.first == inputValue) {
                if (inputValue == 0.0F) {
                    flags[i] = '0';
                } else {
                    flags[i] = '1';
                }
                canOptimize = true;
            }
        }
    }

    if (canOptimize) {
        ensureLibraryHasOptimizedShader(node.nodeTypeId, flags);
        return PXR_NS::TfToken(
            ND_PREFIX + nodeDef->GetFamily().GetString() + "_" + flags + "_surfaceshader");
    }

    return retVal;
}

void LobePrunerImpl::ensureLibraryHasOptimizedShader(
    const PXR_NS::TfToken& nodeDefName,
    const std::string&     flags)
{
    const auto ndIt = _prunerData.find(nodeDefName);
    if (ndIt == _prunerData.end()) {
        return;
    }

    const auto        nd = _library->getNodeDef(nodeDefName.GetString());
    const auto        ng = _library->getNodeGraph(ndIt->second._nodeGraphName);
    const std::string nodeName = nd->getNodeString() + "_" + flags;
    const std::string ndName = ND_PREFIX + nodeName + "_surfaceshader";
    if (_library->getNodeDef(ndName)) {
        // Already there
        return;
    }

    auto newNd = _library->addNodeDef(ndName, "surfaceshader", nodeName);
    newNd->copyContentFrom(nd);
    newNd->setSourceUri("");
    newNd->setNodeString(nodeName);

    auto newNg = _library->addNodeGraph("NG_" + nodeName + "_surfaceshader");
    newNg->copyContentFrom(ng);
    newNg->setSourceUri("");
    newNg->setNodeDefString(ndName);

    ReverseCnxMap reverseMap;
    for (const auto& node : newNg->getNodes()) {
        for (const auto& input : node->getActiveInputs()) {
            if (input->hasNodeName()) {
                const auto& sourceNodeName = input->getNodeName();
                if (!reverseMap.count(sourceNodeName)) {
                    reverseMap.emplace(sourceNodeName, Destinations {});
                }
                reverseMap.find(sourceNodeName)->second.push_back(node->getName());
            }
        }
    }

    auto attrIt = ndIt->second._attributeData.cbegin();
    for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
        switch (flags[i]) {
        case '0': optimizeZeroValue(newNg, attrIt->second, reverseMap); break;
        case '1': optimizeOneValue(newNg, attrIt->second, reverseMap); break;
        default: continue;
        }
    }
}

void LobePrunerImpl::optimizeZeroValue(
    mx::NodeGraphPtr&          newNg,
    const OptimizableValueMap& optimizationMap,
    ReverseCnxMap&             reverseMap)
{
    for (const auto& nodeName : optimizationMap.find(0.0F)->second) {
        auto node = newNg->getNode(nodeName);
        if (!node) {
            continue;
        }
        if (node->getCategory() == "mix") {
            auto bgInput = node->getInput("bg");
            if (!bgInput) {
                continue;
            }
            for (const auto& destNodeName : reverseMap.find(node->getName())->second) {
                auto destNode = newNg->getNode(destNodeName);
                if (!destNode) {
                    continue;
                }
                for (auto input : destNode->getInputs()) {
                    if (input->getNodeName() == node->getName()) {
                        input->removeAttribute(mx::PortElement::NODE_NAME_ATTRIBUTE);
                        if (bgInput->hasNodeName()) {
                            input->setNodeName(bgInput->getNodeName());
                            auto& nodeVector = reverseMap.find(bgInput->getNodeName())->second;
                            nodeVector.push_back(destNodeName);
                            nodeVector.erase(
                                std::remove_if(
                                    nodeVector.begin(),
                                    nodeVector.end(),
                                    [node](const std::string& s) { return s == node->getName(); }),
                                nodeVector.end());
                        }
                        if (bgInput->hasInterfaceName()) {
                            input->setInterfaceName(bgInput->getInterfaceName());
                        }
                        if (bgInput->hasOutputString()) {
                            input->setOutputString(bgInput->getOutputString());
                        }
                        if (bgInput->hasValueString()) {
                            input->setValueString(bgInput->getValueString());
                        }
                    }
                }
            }
            newNg->removeNode(node->getName());
        } else if (node->getCategory() == "multiply") {
            // Result will be a zero value of the type it requests:
            for (const auto& destNodeName : reverseMap.find(node->getName())->second) {
                auto destNode = newNg->getNode(destNodeName);
                for (auto input : destNode->getInputs()) {
                    if (input->getNodeName() == node->getName()) {
                        input->removeAttribute(mx::PortElement::NODE_NAME_ATTRIBUTE);
                        if (input->getType() == "float") {
                            input->setValueString("0");
                        } else if (input->getType() == "color3") {
                            input->setValueString("0, 0, 0");
                        }
                        // TODO: Pad all remaining types
                    }
                }
            }
            newNg->removeNode(node->getName());
        } else if (kBasePbrNodes.count(node->getCategory())) {
            // Prune all inputs.
            for (const auto& input : node->getInputs()) {
                node->removeInput(input->getName());
            }
            // Change node category:
            node->setCategory(DARK_BASE);
            if (node->hasNodeDefString()) {
                node->setNodeDefString(ND_DARK_BASE);
            }
        } else if (kLayerPbrNodes.count(node->getCategory())) {
            // Prune all inputs.
            for (const auto& input : node->getInputs()) {
                node->removeInput(input->getName());
            }
            // Change node category:
            node->setCategory(DARK_LAYER);
            if (node->hasNodeDefString()) {
                node->setNodeDefString(ND_DARK_LAYER);
            }
        }
    }
}

void LobePrunerImpl::addDarkShaders()
{

    if (_library->getNodeDef(ND_DARK_BASE)) {
        return;
    }

    auto nd = _library->addNodeDef(ND_DARK_BASE, "BSDF", DARK_BASE);
    nd->setAttribute("bsdf", "R");
    nd->setNodeGroup("pbr");
    nd->setDocString("A completely dark base BSDF node.");

    auto im = _library->addImplementation(IM_DARK_BASE);
    im->setNodeDef(nd);

    nd = _library->addNodeDef(ND_DARK_LAYER, "BSDF", DARK_LAYER);
    nd->setNodeGroup("pbr");
    nd->setDocString("A completely dark layer BSDF node.");

    im = _library->addImplementation(IM_DARK_LAYER);
    im->setNodeDef(nd);
}

void LobePrunerImpl::optimizeOneValue(
    mx::NodeGraphPtr&          newNg,
    const OptimizableValueMap& optimizationMap,
    ReverseCnxMap&             reverseMap)
{
    for (const auto& nodeName : optimizationMap.find(1.0F)->second) {
        auto node = newNg->getNode(nodeName);
        if (!node) {
            continue;
        }
        if (node->getCategory() == "mix") {
            auto fgInput = node->getInput("fg");
            if (!fgInput) {
                continue;
            }
            for (const auto& destNodeName : reverseMap.find(node->getName())->second) {
                auto destNode = newNg->getNode(destNodeName);
                for (auto input : destNode->getInputs()) {
                    if (input->getNodeName() == node->getName()) {
                        input->removeAttribute(mx::PortElement::NODE_NAME_ATTRIBUTE);
                        if (fgInput->hasNodeName()) {
                            input->setNodeName(fgInput->getNodeName());
                            auto& nodeVector = reverseMap.find(fgInput->getNodeName())->second;
                            nodeVector.push_back(destNodeName);
                            nodeVector.erase(
                                std::remove_if(
                                    nodeVector.begin(),
                                    nodeVector.end(),
                                    [node](const std::string& s) { return s == node->getName(); }),
                                nodeVector.end());
                        }
                        if (fgInput->hasInterfaceName()) {
                            input->setInterfaceName(fgInput->getInterfaceName());
                        }
                        if (fgInput->hasOutputString()) {
                            input->setOutputString(fgInput->getOutputString());
                        }
                        if (fgInput->hasValueString()) {
                            input->setValueString(fgInput->getValueString());
                        }
                    }
                }
            }
            newNg->removeNode(node->getName());
        }
    }
}

LobePruner::Ptr LobePruner::create() { return std::make_shared<LobePruner>(); }

LobePruner::~LobePruner() = default;
LobePruner::LobePruner() = default;

bool LobePruner::getOptimizedNodeCategory(const mx::Node& node, std::string& nodeCategory)
{
    return _impl ? _impl->getOptimizedNodeCategory(node, nodeCategory) : false;
}

mx::StringVec LobePruner::getOptimizedAttributeNames(const mx::NodeDefPtr& nodeDef) const
{
    return _impl ? _impl->getOptimizedAttributeNames(nodeDef) : mx::StringVec {};
}

PXR_NS::TfToken LobePruner::getOptimizedNodeId(const PXR_NS::HdMaterialNode2& node)
{
    return _impl ? _impl->getOptimizedNodeId(node) : PXR_NS::TfToken {};
}

void LobePruner::setLibrary(const mx::DocumentPtr& library)
{
    _impl.reset(new LobePrunerImpl(library));
}

bool LobePruner::isOptimizedNodeId(const PXR_NS::TfToken& nodeId)
{
    return nodeId.GetString().rfind(LobePrunerImpl::ND_PREFIX, 0) == 0;
}

const std::string& LobePruner::getOptimizedNodeDefPrefix() { return LobePrunerImpl::ND_PREFIX; }

const std::string& LobePruner::getDarkBaseNodeName() { return LobePrunerImpl::DARK_BASE; }
const std::string& LobePruner::getDarkLayerNodeName() { return LobePrunerImpl::DARK_LAYER; }

const std::string& LobePruner::getDarkBaseNodeDefName() { return LobePrunerImpl::ND_DARK_BASE; }
const std::string& LobePruner::getDarkLayerNodeDefName() { return LobePrunerImpl::ND_DARK_LAYER; }

const std::string& LobePruner::getDarkBaseImplementationName()
{
    return LobePrunerImpl::IM_DARK_BASE;
}
const std::string& LobePruner::getDarkLayerImplementationName()
{
    return LobePrunerImpl::IM_DARK_LAYER;
}

} // namespace ShaderGenUtil
} // namespace MaterialXMaya
