//
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <mayaHydraLib/sceneIndex/registration.h>
#include <mayaHydraLib/utils.h>

#include <pxr/imaging/hd/dataSourceTypeDefs.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPlugin.h>
#include <pxr/imaging/hd/sceneIndexPluginRegistry.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <ufe/rtid.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
constexpr char kDataSourceEntryNodePathName[] = { "object" };
constexpr char kDataSourceEntryRuntimeName[] = { "runtime" };
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
MayaHydraSceneIndexRegistry::MayaHydraSceneIndexRegistry(HdRenderIndex* renderIndex)
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
            _SceneIndexNodeAddedCallback, kDagNodeMessageName, this, &status);
        if (TF_VERIFY(status == MS::kSuccess, "NodeAdded callback registration failed."))
            _sceneIndexDagNodeMessageCallbacks.append(id);
        id = MDGMessage::addNodeRemovedCallback(
            _SceneIndexNodeRemovedCallback, kDagNodeMessageName, this, &status);
        if (TF_VERIFY(status == MS::kSuccess, "NodeRemoved callback registration failed."))
            _sceneIndexDagNodeMessageCallbacks.append(id);

        // Iterate over scene to find out existing node which will miss eventual dagNode added
        // callbacks
        MItDag nodesDagIt(MItDag::kDepthFirst, MFn::kInvalid);
        for (; !nodesDagIt.isDone(); nodesDagIt.next()) {
            MStatus status;
            MObject dagNode(nodesDagIt.item(&status));
            if (TF_VERIFY(status == MS::kSuccess)) {
                _AddSceneIndexForNode(dagNode);
            }
        }
    }
}

static MayaHydraSceneIndexRegistration kInvalidSceneIndexUfeData;

// Retrieve information relevant to registration such as UFE compatibility of a particular scene
// index
const MayaHydraSceneIndexRegistration&
MayaHydraSceneIndexRegistry::GetSceneIndexRegistrationForRprim(const SdfPath& rprimPath) const
{
    // Retrieve the rprimPath prefix. Composed of the scene index plugin path, and a name used as a
    // disambiguator. MAYA-128179: Revisit this operation. SdfPath operation is slow. No way to get
    // first component only
    SdfPath sceneIndexPluginPath = rprimPath.GetParentPath();
    while (sceneIndexPluginPath.GetPathElementCount() != 2)
        sceneIndexPluginPath = sceneIndexPluginPath.GetParentPath();
    auto it = _registrations.find(sceneIndexPluginPath);
    if (it != _registrations.end()) {
        return *it->second;
    }

    return kInvalidSceneIndexUfeData;
}

MayaHydraSceneIndexRegistry::~MayaHydraSceneIndexRegistry()
{
    MDGMessage::removeCallbacks(_sceneIndexDagNodeMessageCallbacks);
    _sceneIndexDagNodeMessageCallbacks.clear();
    _registrationsByObjectHandle.clear();
    _registrations.clear();
}

bool MayaHydraSceneIndexRegistry::_RemoveSceneIndexForNode(const MObject& dagNode)
{
    MObjectHandle dagNodeHandle(dagNode);
    auto          it = _registrationsByObjectHandle.find(dagNodeHandle);
    if (it != _registrationsByObjectHandle.end()) {
        MayaHydraSceneIndexRegistrationPtr registration(it->second);
        _renderIndex->RemoveSceneIndex(registration->sceneIndex);
        _registrationsByObjectHandle.erase(dagNodeHandle);
        _registrations.erase(registration->sceneIndexPathPrefix);
        return true;
    }
    return false;
}

void MayaHydraSceneIndexRegistry::_AddSceneIndexForNode(MObject& dagNode)
{
    MFnDependencyNode dependNodeFn(dagNode);
    // Name must match Plugin TfType registration thus must begin with upper case
    std::string sceneIndexPluginName(dependNodeFn.typeName().asChar());
    sceneIndexPluginName[0] = toupper(sceneIndexPluginName[0]);
    sceneIndexPluginName += kSceneIndexPluginSuffix;
    TfToken sceneIndexPluginId(sceneIndexPluginName);

    static HdSceneIndexPluginRegistry& sceneIndexPluginRegistry
        = HdSceneIndexPluginRegistry::GetInstance();
    if (sceneIndexPluginRegistry.IsRegisteredPlugin(sceneIndexPluginId)) {
        using HdMObjectDataSource = HdRetainedTypedSampledDataSource<MObject>;
        using HdRtidDataSource = HdRetainedTypedSampledDataSource<Ufe::Rtid&>;

        // Fetch the UFE runtime id using data source. A rtid reference data source is passed inside
        // of the _AppendSceneIndex function call and cached inside of MayaHydraSceneIndexRegistry.
        // It is retrieved using the GetSceneIndexUfeDataForRprim method call.
        Ufe::Rtid              ufeRtid(kInvalidUfeRtid);
        TfToken                names[2] { TfToken(kDataSourceEntryNodePathName),
                           TfToken(kDataSourceEntryRuntimeName) };
        HdDataSourceBaseHandle values[2] { HdMObjectDataSource::New(dagNode),
                                           HdRtidDataSource::New(ufeRtid) };
        HdSceneIndexBaseRefPtr sceneIndex = sceneIndexPluginRegistry.AppendSceneIndex(
            sceneIndexPluginId, nullptr, HdRetainedContainerDataSource::New(2, names, values));
        if (TF_VERIFY(
                sceneIndex,
                "HdSceneIndexBase::AppendSceneIndex failed to create %s scene index from given "
                "node type.",
                sceneIndexPluginName.c_str())) {
            MStatus       status;
            MObjectHandle dagNodeHandle(dagNode);
            // Construct the scene index path prefix appended to each rprim created by it. It is
            // composed of the "scene index plugin's name" + "dag node name" + "disambiguator"
            // The dag node name disambiguator is necessary in situation where node name isn't
            // unique and may clash with other node defined by the same plugin.
            SdfPath sceneIndexPathPrefix(
                SdfPath::AbsoluteRootPath()
                    .AppendPath(SdfPath(sceneIndexPluginName))
                    .AppendPath(SdfPath(
                        std::string(dependNodeFn.name().asChar())
                        + (dependNodeFn.hasUniqueName()
                               ? ""
                               : "__" + std::to_string(_incrementedCounterDisambiguator++)))));

            _renderIndex->InsertSceneIndex(sceneIndex, sceneIndexPathPrefix);
            static SdfPath maya126790Workaround("maya126790Workaround");
            sceneIndex->GetPrim(maya126790Workaround);

            MayaHydraSceneIndexRegistrationPtr registration(new MayaHydraSceneIndexRegistration {
                sceneIndex, sceneIndexPathPrefix, dagNodeHandle, ufeRtid });
            _registrations.insert({ sceneIndexPathPrefix, registration });
            _registrationsByObjectHandle.insert({ dagNodeHandle, registration });
        }
    }
}

void MayaHydraSceneIndexRegistry::_SceneIndexNodeAddedCallback(MObject& dagNode, void* clientData)
{
    if (dagNode.isNull() || dagNode.apiType() != MFn::kPluginShape)
        return;
    auto renderOverride = static_cast<MayaHydraSceneIndexRegistry*>(clientData);
    renderOverride->_AddSceneIndexForNode(dagNode);
}

void MayaHydraSceneIndexRegistry::_SceneIndexNodeRemovedCallback(MObject& dagNode, void* clientData)
{
    if (dagNode.isNull() || dagNode.apiType() != MFn::kPluginShape)
        return;
    auto renderOverride = static_cast<MayaHydraSceneIndexRegistry*>(clientData);
    renderOverride->_RemoveSceneIndexForNode(dagNode);
}

PXR_NAMESPACE_CLOSE_SCOPE
