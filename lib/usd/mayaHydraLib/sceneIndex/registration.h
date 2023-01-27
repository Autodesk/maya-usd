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
