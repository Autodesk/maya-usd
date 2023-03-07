//
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an “AS IS” BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <mayaHydraLib/sceneIndex/registration.h>

#include <pxr/imaging/hd/dataSourceTypeDefs.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPlugin.h>
#include <pxr/imaging/hd/sceneIndexPluginRegistry.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
constexpr char kDataSourceEntryName[] = { "object" };
constexpr char kSceneIndexPluginSuffix[] = {
    "MayaNodeSceneIndexPlugin"
}; // every scene index plugin compatible with the hydra viewport require this suffix
constexpr char kDagNodeMessageName[] = { "dagNode" };
} // namespace

/* To add a custom scene index, a customer plugin must:
 1. Define a Maya dag node via the MPxNode interface, and register it MFnPlugin::registerNode. This
is typically done inside a Maya plug-in initialize function.
 2. Define a HdSceneIndexPlugin which contains an _AppendSceneIndex method. The _AppendSceneIndex
method will be called for every Maya node added into the scene. A customer is responsible for type
checking the node for the one defined and also instantiate the corresponding scene index inside
_AppendSceneIndex. The scene index returned by _AppendSceneIndex is then added to the render index
by Maya.

For example here is how we do that process in the Maya USD plug-in :
// Some context :
// MayaUsdProxyShapeMayaNodeSceneIndexPlugin is a subclass of HdSceneIndexPlugin.
// MayaUsdProxyShapeBase is the base class for our Maya USD node.
// MayaUsd::MayaUsdProxyShapeSceneIndex is the scene index class for Maya USD where we load the
// stages.

HdSceneIndexBaseRefPtr MayaUsdProxyShapeMayaNodeSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr&      inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    using HdMObjectDataSource = HdRetainedTypedSampledDataSource<MObject>;
    using HdMObjectDataSourceHandle = HdRetainedTypedSampledDataSource<MObject>::Handle;
    static TfToken         dataSourceNodePathEntry("object");
    HdDataSourceBaseHandle dataSourceEntryPathHandle = inputArgs->Get(dataSourceNodePathEntry);
    if (HdMObjectDataSourceHandle dataSourceEntryPath
        = HdMObjectDataSource::Cast(dataSourceEntryPathHandle)) {
        MObject           dagNode = dataSourceEntryPath->GetTypedValue(0.0f);
        MStatus           status;
        MFnDependencyNode dependNodeFn(dagNode, &status);
        if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
            return nullptr;
        }

        // Check that this node is a MayaUsdProxyShapeBase which is the base class for our Maya USD
        // node
        auto proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(dependNodeFn.userNode());
        if (TF_VERIFY(proxyShape, "Error getting MayaUsdProxyShapeBase")) {
            auto psSceneIndex = MayaUsd::MayaUsdProxyShapeSceneIndex::New(proxyShape);
            // Flatten transforms, visibility, purpose, model, and material
            // bindings over hierarchies.
            return HdFlatteningSceneIndex::New(psSceneIndex);
        }
    }

    return nullptr;
}

You may want to see the full source code in the Maya USD open source repository :
https://github.com/Autodesk/maya-usd/tree/dev/lib/mayaUsd/sceneIndex
*/

// MayaHydraSceneIndexRegistration is used to register a scene index for a given dag node type.
MayaHydraSceneIndexRegistration::MayaHydraSceneIndexRegistration(HdRenderIndex* renderIndex)
    : _renderIndex(renderIndex)
{
    // Begin registering of custom scene indices for given node types
    HdSceneIndexPluginRegistry& sceneIndexPluginRegistry
        = HdSceneIndexPluginRegistry::GetInstance();
    // Ensure scene index plugin registration
    std::vector<HfPluginDesc> customSceneIndexDescs;
    sceneIndexPluginRegistry.GetPluginDescs(&customSceneIndexDescs);
    if (customSceneIndexDescs.size() != 0) {
        MCallbackId id;
        MStatus     status;
        id = MDGMessage::addNodeAddedCallback(
            _CustomSceneIndexNodeAddedCallback, kDagNodeMessageName, this, &status);
        if (status == MS::kSuccess)
            _customSceneIndexAddedCallbacks.append(id);

        // Iterate over scene to find out existing node which will miss eventual dagNode added
        // callbacks
        MItDag nodesDagIt(MItDag::kDepthFirst, MFn::kInvalid);
        for (; !nodesDagIt.isDone(); nodesDagIt.next()) {
            MStatus status;
            MObject dagNode(nodesDagIt.item(&status));
            if (TF_VERIFY(status == MS::kSuccess)) {
                _AddCustomSceneIndexForNode(dagNode);
            }
        }
    }
}

MayaHydraSceneIndexRegistration::~MayaHydraSceneIndexRegistration()
{
    MDGMessage::removeCallbacks(_customSceneIndexAddedCallbacks);
    for (auto it : _customSceneIndexNodePreRemovalCallbacks) {
        MNodeMessage::removeCallback(it.second);
    }
    _customSceneIndexAddedCallbacks.clear();
    _customSceneIndexNodePreRemovalCallbacks.clear();
    // Since render index is deleted above, scene indices must be recreated.
    _customSceneIndices.clear();
}

bool MayaHydraSceneIndexRegistration::_RemoveCustomSceneIndexForNode(const MObject& dagNode)
{
    MObjectHandle dagNodeHandle(dagNode);
    auto          customSceneIndex = _customSceneIndices.find(dagNodeHandle);
    if (customSceneIndex != _customSceneIndices.end()) {
        _renderIndex->RemoveSceneIndex(customSceneIndex->second);
        _customSceneIndices.erase(dagNodeHandle);
        auto preRemovalCallback = _customSceneIndexNodePreRemovalCallbacks.find(dagNodeHandle);
        if (TF_VERIFY(preRemovalCallback != _customSceneIndexNodePreRemovalCallbacks.end())) {
            MNodeMessage::removeCallback(preRemovalCallback->second);
            _customSceneIndexNodePreRemovalCallbacks.erase(dagNodeHandle);
        }
        return true;
    }
    return false;
}

void MayaHydraSceneIndexRegistration::_AddCustomSceneIndexForNode(MObject& dagNode)
{
    MFnDependencyNode dependNodeFn(dagNode);
    // Name must match Plugin TfType registration thus must begin with upper case
    std::string pluginName(dependNodeFn.typeName().asChar());
    pluginName[0] = toupper(pluginName[0]);
    pluginName += kSceneIndexPluginSuffix;
    TfToken pluginId(pluginName);

    static HdSceneIndexPluginRegistry& sceneIndexPluginRegistry
        = HdSceneIndexPluginRegistry::GetInstance();
    if (sceneIndexPluginRegistry.IsRegisteredPlugin(pluginId)) {
        using HdMObjectDataSource = HdRetainedTypedSampledDataSource<MObject>;
        TfToken                names[1] { TfToken(kDataSourceEntryName) };
        HdDataSourceBaseHandle values[1] { HdMObjectDataSource::New(dagNode) };
        HdSceneIndexBaseRefPtr sceneIndex = sceneIndexPluginRegistry.AppendSceneIndex(
            pluginId, nullptr, HdRetainedContainerDataSource::New(1, names, values));
        if (TF_VERIFY(
                sceneIndex,
                "HdSceneIndexBase::AppendSceneIndex failed to create %s scene index from given "
                "node type.",
                pluginName.c_str())) {
            MStatus     status;
            MCallbackId preRemovalCallback = MNodeMessage::addNodePreRemovalCallback(
                dagNode, _CustomSceneIndexNodeRemovedCallback, this, &status);
            if (TF_VERIFY(
                    status != MS::kFailure, "MNodeMessage::addNodePreRemovalCallback failed")) {
                _renderIndex->InsertSceneIndex(sceneIndex, SdfPath::AbsoluteRootPath());
                static SdfPath maya126790Workaround("maya126790Workaround");
                sceneIndex->GetPrim(maya126790Workaround);
                MObjectHandle dagNodeHandle(dagNode);
                _customSceneIndices.insert({ dagNodeHandle, sceneIndex });
                _customSceneIndexNodePreRemovalCallbacks.insert(
                    { dagNodeHandle, preRemovalCallback });
            }
        }
    }
}

void MayaHydraSceneIndexRegistration::_CustomSceneIndexNodeAddedCallback(
    MObject& dagNode,
    void*    clientData)
{
    if (dagNode.isNull() || dagNode.apiType() != MFn::kPluginShape)
        return;
    auto renderOverride = static_cast<MayaHydraSceneIndexRegistration*>(clientData);
    renderOverride->_AddCustomSceneIndexForNode(dagNode);
}

void MayaHydraSceneIndexRegistration::_CustomSceneIndexNodeRemovedCallback(
    MObject& dagNode,
    void*    clientData)
{
    if (dagNode.isNull() || dagNode.apiType() != MFn::kPluginShape)
        return;
    auto renderOverride = static_cast<MayaHydraSceneIndexRegistration*>(clientData);
    renderOverride->_RemoveCustomSceneIndexForNode(dagNode);
}

PXR_NAMESPACE_CLOSE_SCOPE
