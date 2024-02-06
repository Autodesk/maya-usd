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
#ifndef MAYAUSD_MAYA_NODE_TYPE_OBSERVER_H
#define MAYAUSD_MAYA_NODE_TYPE_OBSERVER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/utils/mayaNodeObserver.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObjectHandle.h>

#include <memory>
#include <unordered_map>

namespace MAYAUSD_NS_DEF {

//! Observer for a given type of Maya node. Receives notifications when
//  an instance of that node type is added or removed and start observing
//  that node for renaming, etc with a MayaNodeObserver.
//
// It forwards those notifications to listerners.
class MayaNodeTypeObserver
{
public:
    //! Listener triggered by the observation.
    struct MAYAUSD_CORE_PUBLIC Listener
    {
        //! Called when any node of the observed type is added to the Maya scene.
        virtual void processNodeAdded(MObject& node);
        //! Called when any node of the observed type is removed from the Maya scene.
        virtual void processNodeRemoved(MObject& node);
    };

    //! Create a Maya node type observer for the given node type.
    MAYAUSD_CORE_PUBLIC
    MayaNodeTypeObserver(const MString& nodeTypeName);
    MAYAUSD_CORE_PUBLIC
    ~MayaNodeTypeObserver();

    //! Add a node type listener to be called when the node changes.
    //
    //  The caller is responsible to ensure the listener is valid
    //  until removed.
    MAYAUSD_CORE_PUBLIC
    void addTypeListener(Listener& listener);

    //! Remove a node type listener.
    MAYAUSD_CORE_PUBLIC
    void removeTypeListener(Listener& listener);

    //! Add a node listener to all observed nodes.
    //
    //  Useful to initially setup to receive listener from nodes
    //  that may have already been added before a listener is ready.
    MAYAUSD_CORE_PUBLIC
    void addNodeListener(MayaNodeObserver::Listener& listener);

    //! Remove a node listener from all observed nodes.
    MAYAUSD_CORE_PUBLIC
    void removeNodeListener(MayaNodeObserver::Listener& listener);

    //! Add a node of the observed type to be observed.
    //  Return the node observer associated with the node.
    //  We trust the caller to only pass nodes of the correct type.
    //
    // Adding a node multipe times is safe, extra aditions are do nothing.
    //
    //  Used so that the C++ class of the node type can manually add
    //  node instances as early as they are created, if needed.
    MAYAUSD_CORE_PUBLIC
    MayaNodeObserver* addObservedNode(const MObject& node);

    //! Remove a node of the observed type tono longer be observed.
    //  We trust the caller to only pass nodes of the correct type.
    //
    //  Used so that the C++ class of the node type can manually remove
    //  node instances as early as they are destroyed, if needed.
    MAYAUSD_CORE_PUBLIC
    void removeObservedNode(const MObject& node);

    //! Retrieve the node observer for the given node, if any.
    //  Return null if the node is not being observed by this instance.
    MAYAUSD_CORE_PUBLIC
    MayaNodeObserver* getNodeObserver(const MObject& node);

private:
    void updateNodeAddedRemovedCallbacks();
    void removeNodeAddedRemovedCallbacks();

    static void processNodeAdded(MObject& node, void* clientData);
    static void processNodeRemoved(MObject& node, void* clientData);

    struct ObjectHandleHasher
    {
        unsigned long operator()(const MObjectHandle& handle) const { return handle.hashCode(); }
    };

    using ObservedNodeMap = std::unordered_map<MObjectHandle, MayaNodeObserver, ObjectHandleHasher>;

    MString                  _nodeTypeName;
    ObservedNodeMap          _observedNodes;
    std::vector<MCallbackId> _nodeAddedRemovedCallbackIds;
    std::set<Listener*>      _listeners;
};

} // namespace MAYAUSD_NS_DEF

#endif