//-
// ==========================================================================
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc.
// and are protected under applicable copyright and trade secret law.
// They may not be disclosed to, copied or used by any third party without
// the prior written consent of Autodesk, Inc.
// ==========================================================================
//+

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
        // TODO: This is traversing the whole Dag hierarchy looking for appropriate nodes. This
        // won't scale to large scenes; perhaps something like what the MEL command "ls -type"
        // is doing would be more appropriate. We can save this for later.
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
                // MAYA-126790 TODO: properly resolve missing PrimsAdded notification issue
                // https://github.com/PixarAnimationStudios/USD/blob/dev/pxr/imaging/hd/sceneIndex.cpp#L38
                // Pixar has discussed adding a missing overridable virtual function when an
                // observer is registered For now GetPrim called with magic string populates the
                // scene index
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
