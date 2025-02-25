//
// Copyright 2024 Autodesk
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
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>

#include <usdUfe/base/tokens.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdMtlx/utils.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>
#include <pxr/usd/usdUI/nodeGraphNodeAPI.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>

#include <MaterialXFormat/XmlIo.h>

PXR_NAMESPACE_OPEN_SCOPE

// Retrieves the standard library document for MaterialX.
MaterialX::ConstDocumentPtr _GetStandardLibraryDocument()
{
    static auto standardDoc = UsdMtlxGetDocument("");
    return standardDoc;
}

// Gets the node definition string for a given node and UFE path.
// If the Node already has it's nodeDefString set, use that.
// Otherwise, get the nodeDef via UFE.
std::string _GetNodeDefString(const MaterialX::NodePtr& node, const Ufe::Path& ufePath)
{
    auto nodeDefString = node->getNodeDefString();
    if (!nodeDefString.empty()) {
        return nodeDefString;
    }

    auto nodeDefHandler = Ufe::RunTimeMgr::instance().nodeDefHandler(ufePath.runTimeId());
    auto sceneItem = Ufe::Hierarchy::createItem(ufePath);
    auto nodeDef = nodeDefHandler->definition(sceneItem);
    if (nodeDef) {
        return nodeDef->type();
    }
    TF_WARN("Could not find nodeDef for node '%s'", node->getName().c_str());
    return std::string();
}

// Retrieves the node definition for a given node and UFE path.
MaterialX::ConstNodeDefPtr _GetNodeDef(const MaterialX::NodePtr& node, const Ufe::Path& ufePath)
{
    auto nodeDefName = _GetNodeDefString(node, ufePath);
    auto nodeDefPtr = node->getDocument()->getNodeDef(nodeDefName);
    if (!nodeDefPtr) {
        // Get the standard library document and check that.
        nodeDefPtr = _GetStandardLibraryDocument()->getNodeDef(nodeDefName);
    }
    return nodeDefPtr;
}

// Sets shader info:id attribute on a USD ShadeShader based on a MaterialX node.
void _SetShaderInfoAttributes(
    const MaterialX::NodePtr& node,
    UsdShadeShader&           usdShader,
    const Ufe::Path&          ufePath)
{
    auto nodeDefString = _GetNodeDefString(node, ufePath);
    usdShader.CreateIdAttr(VtValue(TfToken(nodeDefString)));
}

// Checks if the input type supports color space.
bool _TypeSupportsColorSpace(const MaterialX::InputPtr& mxElem, const Ufe::Path& ufePath)
{
    // ColorSpaces are supported on
    //  - inputs of type color3 or color4
    //  - filename inputs on image nodes with color3 or color4 outputs
    const std::string& type = mxElem->getType();
    const bool         colorInput = type == "color3" || type == "color4";

    bool colorImageNode = false;
    if (type == "filename") {
        // verify the output is color3 or color4
        auto node = mxElem->getParent()->asA<MaterialX::Node>();
        if (auto parentNodeDef = _GetNodeDef(node, ufePath)) {
            for (const MaterialX::OutputPtr& output : parentNodeDef->getOutputs()) {
                const std::string& type = output->getType();
                colorImageNode |= type == "color3" || type == "color4";
            }
        }
    }
    return colorInput || colorImageNode;
}

// Sets UI attributes for a USD input based on a MaterialX input.
void _SetInputUIAttributes(const MaterialX::InputPtr mtlxInput, UsdShadeInput& usdInput)
{
    auto attr = usdInput.GetAttr();
    for (const auto& key : UsdUfe::MetadataTokens->allTokens) {
        if (mtlxInput->hasAttribute(key.GetString())) {
            auto value = mtlxInput->getAttribute(key);
            if (key == UsdUfe::MetadataTokens->UIDoc) {
                attr.SetDocumentation(value);
            } else if (key == UsdUfe::MetadataTokens->UIEnumLabels) {
                const auto   enumStrings = UsdUfe::splitString(value, ",");
                VtTokenArray allowedTokens;
                allowedTokens.reserve(enumStrings.size());
                for (const auto& tokenString : enumStrings) {
                    allowedTokens.push_back(TfToken(TfStringTrim(tokenString, " ")));
                }
                attr.SetMetadata(SdfFieldKeys->AllowedTokens, allowedTokens);
            } else if (key == UsdUfe::MetadataTokens->UIFolder) {
                std::replace(value.begin(), value.end(), '/', ':');
                attr.SetDisplayGroup(value);
            } else if (key == UsdUfe::MetadataTokens->UIName) {
                attr.SetDisplayName(value);
            } else if (SdfSchema::GetInstance().IsRegistered(key)) {
                attr.SetMetadata(key, VtValue(value));
            } else {
                attr.SetCustomDataByKey(key, VtValue(value));
            }
        }
    }
}

// Gets the output name of the mxNode connected to a portElement
// If the portElement has an outputString, use that.
// Otherwise, look for the output name in the NodeDef
// If that fails, use the default output name.
std::string _GetOutputName(
    const MaterialX::PortElementPtr& portElement,
    const MaterialX::NodePtr&        mxNode,
    const Ufe::Path&                 ufePath)
{
    std::string outputName;
    if (portElement->hasOutputString()) {
        outputName = portElement->getOutputString();
    } else if (auto nodeDef = _GetNodeDef(mxNode, ufePath)) {
        auto outVec = nodeDef->getOutputs();
        if (!outVec.empty()) {
            outputName = outVec[0]->getName();
        }
    }
    if (outputName.empty()) {
        outputName = UsdMtlxTokens->DefaultOutputName.GetString();
    }
    return outputName;
}

// Connects a USD input to a node output based on a MaterialX input.
void _ConnectToNode(
    const MaterialX::InputPtr& input,
    UsdShadeInput&             usdInput,
    const SdfPath&             parentPath,
    const Ufe::Path&           ufePath,
    const UsdStagePtr&         stage)
{
    auto connectedNode = input->getConnectedNode();

    std::string outputName = _GetOutputName(input, connectedNode, ufePath);
    auto        nodeOutput = UsdShadeShader(stage->GetPrimAtPath(
                                         parentPath.AppendPath(SdfPath(connectedNode->getName()))))
                          .GetOutput(TfToken(outputName));
    if (nodeOutput.IsDefined()) {
        usdInput.ConnectToSource(nodeOutput);
    }
}

// Connect a USD input to a NodeGraph input based on a MaterialX input.
void _ConnectToInterfaceInput(
    const MaterialX::InputPtr& interfaceInput,
    UsdShadeInput&             usdInput,
    const SdfPath&             parentPath,
    const UsdStagePtr&         stage)
{
    auto interfaceInputNode = interfaceInput->getParent();
    auto interfaceInputNodePrim = stage->GetPrimAtPath(parentPath);
    if (interfaceInputNode->getName() == interfaceInputNodePrim.GetName()) {
        auto interfaceInputNodeGraph = UsdShadeNodeGraph(interfaceInputNodePrim);
        auto valType = UsdMtlxGetUsdType(interfaceInput->getType());
        auto interfaceInputNodeOutput = interfaceInputNodeGraph.CreateInput(
            TfToken(interfaceInput->getName()), valType.valueTypeName);
        if (interfaceInputNodeOutput.IsDefined()) {
            usdInput.ConnectToSource(interfaceInputNodeOutput);
        }
    }
}

// Connects a USD input to a node graph output based on a MaterialX input.
void _ConnectToNodeGraph(
    const MaterialX::InputPtr& input,
    UsdShadeInput&             usdInput,
    const SdfPath&             parentPath,
    const UsdStagePtr&         stage)
{
    auto output = input->getConnectedOutput();
    auto nodeGraphName = output->getParent()->getName();
    auto nodeGraphPrim = stage->GetPrimAtPath(parentPath.AppendPath(SdfPath(nodeGraphName)));
    if (nodeGraphPrim.IsDefined()) {
        auto nodeGraph = UsdShadeNodeGraph(nodeGraphPrim);
        auto usdOutput = nodeGraph.GetOutput(TfToken(output->getName()));
        if (usdOutput.IsDefined()) {
            usdInput.ConnectToSource(usdOutput);
        }
    }
}

// Sets the value of a USD input based on a MaterialX input.
void _SetInputValue(
    const MaterialX::InputPtr& input,
    UsdShadeInput&             usdInput,
    const Ufe::Path&           ufePath)
{
    usdInput.Set(UsdMtlxGetUsdValue(input));
    if (_TypeSupportsColorSpace(input, ufePath)) {
        auto colorSpace = input->getActiveColorSpace();
        if (!colorSpace.empty()) {
            usdInput.GetAttr().SetColorSpace(TfToken(colorSpace));
        }
    }
}

// Adds a USD input based on a MaterialX input.
void _AddInput(
    const MaterialX::InputPtr& input,
    UsdShadeInput&             usdInput,
    const SdfPath&             parentPath,
    const Ufe::Path&           ufePath,
    const UsdStagePtr&         stage)
{
    if (input->hasNodeGraphString()) {
        _ConnectToNodeGraph(input, usdInput, parentPath, stage);
    } else if (input->hasNodeName()) {
        _ConnectToNode(input, usdInput, parentPath, ufePath, stage);
    } else if (input->hasInterfaceName()) {
        auto interfaceInput = input->getInterfaceInput();
        _ConnectToInterfaceInput(interfaceInput, usdInput, parentPath, stage);
    } else if (!input->hasOutputString()) {
        _SetInputValue(input, usdInput, ufePath);
    }
}

// Adds a shader input to a USD shader based on a MaterialX input.
void _AddShaderInput(
    const MaterialX::InputPtr& input,
    UsdShadeShader&            usdShader,
    const SdfPath&             parentPath,
    const Ufe::Path&           ufePath,
    const UsdStagePtr&         stage)
{
    auto typeStr = input->getType();
    auto valType = UsdMtlxGetUsdType(typeStr);
    auto usdInput = usdShader.CreateInput(TfToken(input->getName()), valType.valueTypeName);
    if (usdInput.IsDefined()) {
        _AddInput(input, usdInput, parentPath, ufePath, stage);
    }
}

// Adds a node graph input to a USD node graph based on a MaterialX input.
void _AddNodeGraphInput(
    const MaterialX::InputPtr& input,
    UsdShadeNodeGraph&         usdNodeGraph,
    const SdfPath&             parentPath,
    const Ufe::Path&           ufePath,
    const UsdStagePtr&         stage)
{
    auto typeStr = input->getType();
    auto valType = UsdMtlxGetUsdType(typeStr);
    auto usdInput = usdNodeGraph.CreateInput(TfToken(input->getName()), valType.valueTypeName);
    if (usdInput.IsDefined()) {
        _AddInput(input, usdInput, parentPath, ufePath, stage);
        _SetInputUIAttributes(input, usdInput);
    }
}

// Sets UI attributes for a prim based on a MaterialX node.
void _SetShaderUIAttribute(const MaterialX::InterfaceElementPtr& node, UsdPrim& prim)
{
    if (!prim.HasAPI<UsdUINodeGraphNodeAPI>()) {
        UsdUINodeGraphNodeAPI::Apply(prim);
    }

    if (auto nodeGraphApi = UsdUINodeGraphNodeAPI(prim)) {
        if (node->hasAttribute("ypos") && node->hasAttribute("xpos")) {
            nodeGraphApi.CreatePosAttr(VtValue(GfVec2f(
                std::stof(node->getAttribute("xpos")), std::stof(node->getAttribute("ypos")))));
        }
    }
}

// Retrieves the output of a USD prim.
UsdShadeOutput _GetPrimOutput(UsdPrim& prim, const TfToken& outputName)
{
    auto           shader = UsdShadeShader(prim);
    UsdShadeOutput output;
    if (auto shader = UsdShadeShader(prim)) {
        output = shader.GetOutput(outputName);
    } else if (auto nodeGraph = UsdShadeNodeGraph(prim)) {
        output = nodeGraph.GetOutput(outputName);
    }
    return output;
}

// Adds a Shader prim to the USD stage based on a MaterialX node.
void _AddNode(
    const MaterialX::NodePtr& node,
    const UsdStagePtr&        stage,
    const SdfPath&            parentPath,
    const Ufe::Path&          ufePath)
{
    // Don't do anything for NodeGraphs, they are handled separately.
    if (node->getCategory() == "nodegraph") {
        return;
    }

    auto primPath = parentPath.AppendPath(SdfPath(node->getName()));
    auto shader = UsdShadeShader::Define(stage, primPath);

    if (!TF_VERIFY(shader, "Could not define UsdShadeShader at path '%s'\n", primPath.GetText())) {
        return;
    }

    auto shaderPrim = shader.GetPrim();
    _SetShaderUIAttribute(node, shaderPrim);
    auto shaderUfePath = ufePath + node->getName();
    _SetShaderInfoAttributes(node, shader, shaderUfePath);

    if (auto nodeDef = _GetNodeDef(node, shaderUfePath)) {
        for (auto output : nodeDef->getOutputs()) {
            auto outName
                = output->getName().empty() ? UsdMtlxTokens->DefaultOutputName : output->getName();
            shader.CreateOutput(
                TfToken(outName), UsdMtlxGetUsdType(output->getType()).valueTypeName);
        }
    }

    for (auto input : node->getInputs()) {
        _AddShaderInput(input, shader, parentPath, shaderUfePath, stage);
    }
}

// Adds a node and all its dependent nodes to the USD stage.
void _AddDependentNodes(
    const MaterialX::InterfaceElementPtr&     node,
    std::set<MaterialX::InterfaceElementPtr>& collectedNodes,
    const UsdStagePtr&                        stage,
    const SdfPath&                            parentPath,
    const Ufe::Path&                          ufePath)
{
    if (!node || collectedNodes.find(node) != collectedNodes.end()) {
        return;
    }
    collectedNodes.insert(node);

    SdfPath           targetPath = parentPath;
    Ufe::Path         targetUfePath = ufePath;
    bool              isNodeGraph = node->getCategory() == "nodegraph";
    UsdShadeNodeGraph usdNodeGraph;
    if (isNodeGraph) {
        // Define the NodeGraph
        targetPath = parentPath.AppendPath(SdfPath(node->getName()));
        targetUfePath = ufePath + node->getName();
        auto nodeGraphPrim = stage->DefinePrim(targetPath, TfToken("NodeGraph"));
        if (!TF_VERIFY(
                nodeGraphPrim, "Could not define NodeGraph at path '%s'\n", targetPath.GetText())) {
            return;
        }
        usdNodeGraph = UsdShadeNodeGraph(nodeGraphPrim);
        _SetShaderUIAttribute(node, nodeGraphPrim);

        auto nodeGraph = node->asA<MaterialX::NodeGraph>();
        for (auto graphNode : nodeGraph->getNodes()) {
            _AddDependentNodes(graphNode, collectedNodes, stage, targetPath, targetUfePath);
            _AddNode(graphNode, stage, targetPath, targetUfePath);
        }
        for (auto output : nodeGraph->getOutputs()) {
            auto usdOutput = usdNodeGraph.CreateOutput(
                TfToken(output->getName()), UsdMtlxGetUsdType(output->getType()).valueTypeName);
            if (auto targetOutput = output->getConnectedOutput()) {
                auto targetPrim = stage->GetPrimAtPath(
                    targetPath.AppendPath(SdfPath(targetOutput->getParent()->getName())));
                usdOutput.ConnectToSource(
                    _GetPrimOutput(targetPrim, TfToken(targetOutput->getName())));
            } else if (auto targetNode = output->getConnectedNode()) {
                if (targetNode->getParent() != node) {
                    TF_WARN(
                        "NodeGraph output '%s' is connected to a node outside the NodeGraph",
                        output->getName().c_str());
                    continue;
                }
                auto targetOutputName
                    = _GetOutputName(output, targetNode, targetUfePath + targetNode->getName());
                auto targetPrim
                    = stage->GetPrimAtPath(targetPath.AppendPath(SdfPath(targetNode->getName())));
                usdOutput.ConnectToSource(_GetPrimOutput(targetPrim, TfToken(targetOutputName)));
            }
        }
    }

    for (auto input : node->getInputs()) {
        // if it's connected to a NodeGraph, collect all the nodes in the NodeGraph
        if (!input->getNodeGraphString().empty()) {
            _AddDependentNodes(
                node->getDocument()->getNodeGraph(input->getNodeGraphString()),
                collectedNodes,
                stage,
                parentPath,
                ufePath);
        }
        // If it's connected to an "independent" node, add that node and its dependencies
        else if (MaterialX::NodePtr connectedNode = input->getConnectedNode()) {
            if (collectedNodes.find(connectedNode) == collectedNodes.end()) {
                _AddDependentNodes(connectedNode, collectedNodes, stage, targetPath, targetUfePath);
                _AddNode(connectedNode, stage, parentPath, ufePath);
            }
        }
        if (isNodeGraph) {
            _AddNodeGraphInput(input, usdNodeGraph, parentPath, targetUfePath, stage);
        }
    }
}

class MtlxMaterialXSurfaceShaderWriter : public UsdMayaShaderWriter
{
public:
    MtlxMaterialXSurfaceShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    static ContextSupport CanExport(const UsdMayaJobExportArgs&);
    void                  Write(const UsdTimeCode& usdTime) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(MaterialXSurfaceShader, MtlxMaterialXSurfaceShaderWriter);

UsdMayaShaderWriter::ContextSupport
MtlxMaterialXSurfaceShaderWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    if (!exportArgs.exportMaterials) {
        return ContextSupport::Unsupported;
    }

    return exportArgs.convertMaterialsTo == TrMtlxTokens->conversionName
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

MtlxMaterialXSurfaceShaderWriter::MtlxMaterialXSurfaceShaderWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    // The shader writer is being called twice, once for the surface and once for the
    // displacement, but there is only one material
    // Skip the second call
    if (GetUsdStage()->GetPrimAtPath(GetUsdPath())) {
        return;
    }

    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }
    auto parentPath = GetUsdPrim().GetParent().GetPath();

    MStatus status;
    MPlug   childPlug = depNodeFn.findPlug("ufePath", true, &status);
    MString ufePathString;
    childPlug.getValue(ufePathString);

    // This is the material node
    auto ufePath = Ufe::PathString::path(ufePathString.asChar());
    // This is the document node
    auto                ufeParentPath = ufePath.pop();
    Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(ufeParentPath);

    // Render Document is the MaterialX document
    childPlug = depNodeFn.findPlug("renderDocument", true, &status);
    MString renderDocumentString;
    childPlug.getValue(renderDocumentString);

    auto mtlxDoc = MaterialX::createDocument();
    readFromXmlString(mtlxDoc, renderDocumentString.asChar());

    // surfaceMaterialNode
    auto MaterialNode = mtlxDoc->getNode(ufePath.back().string());
    if (MaterialNode == nullptr) {
        TF_WARN(
            "Material Node '%s' not found in the MaterialX Document",
            ufePath.back().string().c_str());
        return;
    }
    // Collection of the MaterialX nodes already processed, to avoid processing them again.
    std::set<MaterialX::InterfaceElementPtr> collectedNodes;

    // Handle the displacement shader output connection.
    // Usually this is done by the shadingModeUseRegistry, but since we are doing both surface and
    // displacement in one go, we need to handle it here.
    if (auto displacementNode = MaterialNode->getConnectedNode("displacementshader")) {
        UsdShadeOutput _mtlDisplacementOutput;
        if (jobCtx.GetArgs().allMaterialConversions.size() > 1) {
            _mtlDisplacementOutput
                = UsdShadeMaterial(GetUsdPrim().GetParent()).CreateDisplacementOutput();
            auto output = UsdShadeMaterial(GetUsdPrim().GetParent().GetParent())
                              .CreateDisplacementOutput(TfToken("mtlx"));
            output.ConnectToSource(_mtlDisplacementOutput);
        } else {
            _mtlDisplacementOutput = UsdShadeMaterial(GetUsdPrim().GetParent())
                                         .CreateDisplacementOutput(TfToken("mtlx"));
        }
        UsdShadeShader displacementShader;
        displacementShader = UsdShadeShader::Define(
            GetUsdStage(), parentPath.AppendPath(SdfPath(displacementNode->getName())));
        auto _ShaderDisplacementOutput = displacementShader.CreateOutput(
            UsdMtlxTokens->DefaultOutputName, _mtlDisplacementOutput.GetTypeName());
        _mtlDisplacementOutput.ConnectToSource(_ShaderDisplacementOutput);
        auto&     dispNodeName = displacementNode->getName();
        Ufe::Path ufeDispPath = ufeParentPath + dispNodeName;
        _SetShaderInfoAttributes(displacementNode, displacementShader, ufeDispPath);
        _AddDependentNodes(
            displacementNode, collectedNodes, GetUsdStage(), parentPath, ufeParentPath);
        for (auto input : displacementNode->getInputs()) {
            _AddShaderInput(input, displacementShader, parentPath, ufeDispPath, GetUsdStage());
        }
    }

    auto shaderNode = MaterialNode->getConnectedNode("surfaceshader");
    if (shaderNode == nullptr) {
        TF_WARN(
            "Surface Shader Node not found in the MaterialX Document, for Shader at path '%s'",
            _usdPrim.GetPrimPath().GetText());
        return;
    }
    _SetShaderInfoAttributes(shaderNode, shaderSchema, ufeParentPath + shaderNode->getName());
    _AddDependentNodes(shaderNode, collectedNodes, GetUsdStage(), parentPath, ufeParentPath);

    for (auto input : shaderNode->getInputs()) {
        _AddShaderInput(
            input, shaderSchema, parentPath, ufeParentPath + shaderNode->getName(), GetUsdStage());
    }
}

void MtlxMaterialXSurfaceShaderWriter::Write(const UsdTimeCode& usdTime)
{
    // Really nothing to do here
}

PXR_NAMESPACE_CLOSE_SCOPE