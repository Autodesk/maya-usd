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
#ifndef MAYAUSD_MAYA_NODE_OBSERVER_H
#define MAYAUSD_MAYA_NODE_OBSERVER_H

#include <mayaUsd/base/api.h>

#include <maya/MDagPath.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

#include <set>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Observer for a single Maya node that receives notifications when the node is renamed,
//  reparented, or any of its ancestor is renamed or reparented.
//
// It forwards those notifications to listerners.
class MayaNodeObserver
{
public:
    //! Listener triggered by the observation.
    struct MAYAUSD_CORE_PUBLIC Listener
    {
        //! Called when a given particular node is renamed.
        virtual void processNodeRenamed(MObject& observedNode, const MString& str);
        //! Called when a parent is added anywhere in the hierarchy of a given particular node.
        virtual void processParentAdded(MObject& observedNode, MDagPath& child, MDagPath& parent);
        //! Called when a plug is dirtied anywhere in the hierarchy of a given particular node.
        virtual void processPlugDirty(
            MObject& observedNode,
            MObject& dirtiedNode,
            MPlug&   plug,
            bool     pathChanged);
    };

    //! Create a Maya node observer. To start observing the node, call startObserving().
    MAYAUSD_CORE_PUBLIC
    MayaNodeObserver();

    MAYAUSD_CORE_PUBLIC
    ~MayaNodeObserver();

    //! Start observing the given node. Ends observing any previous observed node.
    MAYAUSD_CORE_PUBLIC
    void startObserving(const MObject& observedNode);

    //! Stop observing the given node.
    MAYAUSD_CORE_PUBLIC
    void stopObserving();

    //! Add a listener to be called when the node changes.
    //
    //  The caller is responsible to ensure the listener is valid
    //  until removed.
    MAYAUSD_CORE_PUBLIC
    void addListener(Listener& listener);

    //! Remove a listener.
    MAYAUSD_CORE_PUBLIC
    void removeListener(Listener& listener);

    //! Remove all listener in the given vector of listener and clear the vector.
    MAYAUSD_CORE_PUBLIC
    static void removeCallbackIds(std::vector<MCallbackId>& callbackIds);

    //! Remove the given callback and clear it, make it an invalid callback ID.
    MAYAUSD_CORE_PUBLIC
    static void removeCallbackId(MCallbackId& callbackId);

private:
    MayaNodeObserver(const MayaNodeObserver&) = delete;
    MayaNodeObserver& operator=(const MayaNodeObserver&) = delete;

    void updateRenameCallback();
    void removeRenameCallback();

    void updateAncestorCallbacks();
    void removeAncestorCallbacks();

    void updateDagPathCallbacks();
    void removeDagPathCallbacks();

    void updateAllNameRelatedCallbacks();

    static void processNodeRenamed(MObject& observedNode, const MString& str, void* clientData);
    static void processParentAdded(MDagPath& child, MDagPath& parent, void* clientData);
    static void processPlugDirty(MObject& dirtiedNode, MPlug& plug, void* clientData);

    MObject _observedNode;

    MCallbackId              _renameCallbackId = 0;
    std::vector<MCallbackId> _parentAddedCallbackIds;
    std::vector<MCallbackId> _ancestorCallbackIds;

    MString _ancestorCallbacksPath;
    bool    _inAncestorCallback = false;

    std::set<Listener*> _listeners;
};

} // namespace MAYAUSD_NS_DEF

#endif