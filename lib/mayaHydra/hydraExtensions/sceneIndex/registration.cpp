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
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/usd/sdf/path.h>

// The SceneIndex plugin required for MaterialX binding to work will no longer be needed
// after 23.08 hopefully. Pixar has made changed to USD that should handle this case.
#if PXR_VERSION < 2308
// Temp workaround to get the code to compile.
// this will not be required once the pull request to USD header is complete.
// Details of the issue - https://groups.google.com/g/usd-interest/c/0kyY3Z2sgjo
// TODO: remove conditional #undefs after PR is complete.
#ifdef interface
#undef interface
#endif
#include <pxr/imaging/hdsi/terminalsResolvingSceneIndex.h>
#endif //PXR_VERSION

#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>

#include <ufeExtensions/Global.h>

PXR_NAMESPACE_OPEN_SCOPE

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

namespace {
constexpr char kDagNodeMessageName[] = { "dagNode" };
}
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

// Retrieve information relevant to registration such as UFE compatibility of a particular scene
// index
MayaHydraSceneIndexRegistrationPtr
MayaHydraSceneIndexRegistry::GetSceneIndexRegistrationForRprim(const SdfPath& rprimPath) const
{
    // Retrieve the rprimPath prefix. Composed of the scene index plugin path, and a name used as a
    // disambiguator. MAYA-128179: Revisit this operation. SdfPath operation is slow. No way to get
    // first component only
    SdfPath sceneIndexPluginPath = rprimPath.GetParentPath();
    while (sceneIndexPluginPath.GetPathElementCount() > 2)
        sceneIndexPluginPath = sceneIndexPluginPath.GetParentPath();
    auto it = _registrations.find(sceneIndexPluginPath);
    if (it != _registrations.end()) {
        return it->second;
    }

    return nullptr;
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
        _renderIndex->RemoveSceneIndex(registration->rootSceneIndex);
        _registrationsByObjectHandle.erase(dagNodeHandle);
        _registrations.erase(registration->sceneIndexPathPrefix);
        return true;
    }
    return false;
}

#if PXR_VERSION < 2308
HdSceneIndexBaseRefPtr
MayaHydraSceneIndexRegistry::_AppendTerminalRenamingSceneIndex(HdSceneIndexBaseRefPtr sceneIndex)
{    
    // Get the list of renderer supported material network implementations.
    TfTokenVector renderingContexts = _renderIndex->GetRenderDelegate()->
                                                    GetMaterialRenderContexts();    
    // Create remapping token pairs to help Hydra build the material networks.
    std::map<TfToken, TfToken> terminalRemapList;
    for (const auto& terminal : renderingContexts)
        terminalRemapList.emplace(TfToken(terminal.GetString()+":surface"), 
                                  HdMaterialTerminalTokens->surface);
    return HdsiTerminalsResolvingSceneIndex::New(sceneIndex, terminalRemapList);    
}
#endif // PXR_VERSION

constexpr char kSceneIndexPluginSuffix[] = {
    "MayaNodeSceneIndexPlugin"
}; // every scene index plugin compatible with the hydra viewport requires this suffix

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
        using MayaHydraMObjectDataSource = HdRetainedTypedSampledDataSource<MObject>;
        using MayaHydraVersionDataSource = HdRetainedTypedSampledDataSource<int>;
        // Functions retrieved from the scene index plugin
        using MayaHydraInterpretRprimPathDataSource
            = HdRetainedTypedSampledDataSource<MayaHydraInterpretRprimPath&>;

        // Create the registration record which is then added into the registry if everything
        // succeeds
        static TfToken sDataSourceEntryNames[] { TfToken("object"),
                                                 TfToken("version"),
                                                 TfToken("interpretRprimPath") };
        constexpr int  kDataSourceNumEntries = sizeof(sDataSourceEntryNames) / sizeof(TfToken);
        MayaHydraSceneIndexRegistrationPtr registration(new MayaHydraSceneIndexRegistration());
        HdDataSourceBaseHandle             values[] { MayaHydraMObjectDataSource::New(dagNode),
                                          MayaHydraVersionDataSource::New(MAYAHYDRA_API_VERSION),
                                          MayaHydraInterpretRprimPathDataSource::New(
                                              registration->interpretRprimPathFn) };
        static_assert(
            sizeof(values) / sizeof(HdDataSourceBaseHandle) == kDataSourceNumEntries,
            "Incorrect number of data source entries");
        registration->pluginSceneIndex = sceneIndexPluginRegistry.AppendSceneIndex(
            sceneIndexPluginId,
            nullptr,
            HdRetainedContainerDataSource::New(kDataSourceNumEntries, sDataSourceEntryNames, values));
        if (TF_VERIFY(
                registration->pluginSceneIndex,
                "HdSceneIndexBase::AppendSceneIndex failed to create %s scene index from given "
                "node type.",
                sceneIndexPluginName.c_str())) {

            MStatus  status;
            MDagPath dagPath(MDagPath::getAPathTo(dagNode, &status));
            if (TF_VERIFY(status == MS::kSuccess, "Incapable of finding dag path to given node")) {
                registration->dagNode = MObjectHandle(dagNode);
                // Construct the scene index path prefix appended to each rprim created by it.
                // It is composed of the "scene index plugin's name" + "dag node name" +
                // "disambiguator" The dag node name disambiguator is necessary in situation
                // where node name isn't unique and may clash with other node defined by the
                // same plugin.
                std::string dependNodeNameString (dependNodeFn.name().asChar());
                MAYAHYDRA_NS_DEF::SanitizeNameForSdfPath(dependNodeNameString);
                
                registration->sceneIndexPathPrefix = 
                          SdfPath::AbsoluteRootPath()
                          .AppendPath(SdfPath(sceneIndexPluginName))
                          .AppendPath(SdfPath(dependNodeNameString
                              + (dependNodeFn.hasUniqueName()
                                     ? ""
                                     : "__" + std::to_string(_incrementedCounterDisambiguator++))));

                #if PXR_VERSION < 2308
                // HYDRA-179
                // Inject TerminalsResolvingSceneIndex to get Hydra to handle material bindings.
                // A simple string replacement for Hydra to identify the terminals based on the render context.
                HdSceneIndexBaseRefPtr outSceneIndex = _AppendTerminalRenamingSceneIndex(registration->pluginSceneIndex);
                // Sanity check
                registration->rootSceneIndex = outSceneIndex ? outSceneIndex : registration->pluginSceneIndex;
                #endif // PXR_VERSION

                // By inserting the scene index was inserted into the render index using a custom
                // prefix, the chosen prefix will be prepended to rprims tied to that scene index
                // automatically.
                _renderIndex->InsertSceneIndex(
                    registration->rootSceneIndex, registration->sceneIndexPathPrefix);
                static SdfPath maya126790Workaround("maya126790Workaround");
                registration->pluginSceneIndex->GetPrim(maya126790Workaround);

                // Add registration record if everything succeeded
                _registrations.insert({ registration->sceneIndexPathPrefix, registration });
                _registrationsByObjectHandle.insert({ registration->dagNode, registration });
            }
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
