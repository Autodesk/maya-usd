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

#ifndef MAYAHYDRALIB_SCENE_INDEX_REGISTRATION_H
#define MAYAHYDRALIB_SCENE_INDEX_REGISTRATION_H

#include <mayaHydraLib/api.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/pxr.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraSceneIndexRegistration is used to register a scene index for a given dag node
 * type.
 *
 * To add a custom scene index, a customer plugin must :
 *  1. Define a Maya dag node via the MPxNode interface, and register it MFnPlugin::registerNode.
 * This is typically node inside a Maya pluging initialize function.
 *  2. Define a HdSceneIndexPlugin which contains an _AppendSceneIndex method. The _AppendSceneIndex
 * method will be called for every Maya node added into the scene. A customer is responsible for
 * type checking the node for the one defined and also instantiate the corresponding scene index
 * inside _AppendSceneIndex. The scene index returned by _AppendSceneIndex is then added to the
 * render index by Maya.
 *
 * See registration.cpp file for a code snippet.
 */
class MayaHydraSceneIndexRegistration
{
public:
    MAYAHYDRALIB_API
    MayaHydraSceneIndexRegistration(HdRenderIndex* renderIndex);

    MAYAHYDRALIB_API
    ~MayaHydraSceneIndexRegistration();

private:
    void _AddCustomSceneIndexForNode(
        MObject& dagNode); // dagNode non-const because of callback registration
    bool        _RemoveCustomSceneIndexForNode(const MObject& dagNode);
    static void _CustomSceneIndexNodeAddedCallback(MObject& obj, void* clientData);
    static void _CustomSceneIndexNodeRemovedCallback(MObject& obj, void* clientData);

    HdRenderIndex*   _renderIndex = nullptr;
    MCallbackIdArray _customSceneIndexAddedCallbacks;
    struct _HashObjectHandle
    {
        unsigned long operator()(const MObjectHandle& handle) const { return handle.hashCode(); }
    };
    // MObjectHandle only used as opposed to MObject here because of their hashCode function.
    std::unordered_map<MObjectHandle, MCallbackId, _HashObjectHandle>
                                                                              _customSceneIndexNodePreRemovalCallbacks;
    std::unordered_map<MObjectHandle, HdSceneIndexBasePtr, _HashObjectHandle> _customSceneIndices;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_SCENE_INDEX_REGISTRATION_H
