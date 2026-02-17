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

#include "mayaNodeObserver.h"

#include <maya/MDagMessage.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MNodeMessage.h>

#include <functional>

namespace MAYAUSD_NS_DEF {

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(MayaNodeObserver);

////////////////////////////////////////////////////////////////////////////
//
// External listener called when Maya notifications are received.
// Default implementation is to do nothing.

void MayaNodeObserver::Listener::processNodeRenamed(
    MObject& /*observedNode*/,
    const MString& /*str*/)
{
}
void MayaNodeObserver::Listener::processParentAdded(
    MObject& /*observedNode*/,
    MDagPath& /*child*/,
    MDagPath& /*parent*/)
{
}
void MayaNodeObserver::Listener::processPlugDirty(
    MObject& observedNode,
    MObject& /*dirtiedNode*/,
    MPlug& /*plug*/,
    bool /*pathChanged*/)
{
}

////////////////////////////////////////////////////////////////////////////
//
// Constructor and destructor.

MayaNodeObserver::MayaNodeObserver() { }

MayaNodeObserver::~MayaNodeObserver() { stopObserving(); }

////////////////////////////////////////////////////////////////////////////
//
// Observation begin and end.

void MayaNodeObserver::startObserving(const MObject& observedNode)
{
    stopObserving();

    _observedNode = observedNode;

    updateRenameCallbacks();
    updateAncestorCallbacks();
    updateDagPathCallbacks();
}

void MayaNodeObserver::stopObserving()
{
    removeRenameCallbacks();
    removeDagPathCallbacks();
    removeAncestorCallbacks();

    _observedNode = MObject();
}

void MayaNodeObserver::updateObserving()
{
    updateRenameCallbacks();
    updateAncestorCallbacks();
    updateDagPathCallbacks();
}

////////////////////////////////////////////////////////////////////////////
//
// Listener management.

void MayaNodeObserver::addListener(Listener& listener)
{
    // Start tracking and calling the given listener.
    _listeners.insert(&listener);
}

void MayaNodeObserver::removeListener(Listener& listener)
{
    // Stop tracking and calling the given listener.
    _listeners.erase(&listener);
}

////////////////////////////////////////////////////////////////////////////
//
// Maya callback helpers.

/* static */
void MayaNodeObserver::removeCallbackIds(std::vector<MCallbackId>& callbackIds)
{
    for (MCallbackId id : callbackIds)
        removeCallbackId(id);
    callbackIds.clear();
}

/* static */
void MayaNodeObserver::removeCallbackId(MCallbackId& callbackId)
{
    MMessage::removeCallback(callbackId);
    callbackId = 0;
}

////////////////////////////////////////////////////////////////////////////
//
// Maya callbacks registration and cleanup.

static void doForNodeAndAncestors(MObject& node, std::function<void(MObject&, MDagPath&)> func)
{
    // Make sure to call the call for the node itself even if it has no path
    // As nodes initially are not added to the DAG at creation time and this
    // might be called at creation time.
    {
        MDagPath path;
        MDagPath::getAPathTo(node, path);
        func(node, path);
    }

    MDagPathArray dags;
    if (MDagPath::getAllPathsTo(node, dags)) {
        const auto numDags = dags.length();
        for (auto i = decltype(numDags) { 0 }; i < numDags; ++i) {
            // Note: we lready processed the node itself above
            //       so don't process it multiple times. We would
            //       need to avoid processing multiple time anyway
            //       in the case it has multipke paths.
            MDagPath& dag = dags[i];
            for (dag.pop(); dag.length() > 0; dag.pop()) {
                MObject obj = dag.node();
                if (obj == MObject::kNullObj)
                    continue;
                func(obj, dag);
            }
        }
    }
}

void MayaNodeObserver::updateRenameCallbacks()
{
    removeRenameCallbacks();

    // We need to observe name change in the node and any ancestor.
    // This is to support listeners that track the node by full path
    // and need to update data related to this full path.
    doForNodeAndAncestors(_observedNode, [this](MObject& obj, MDagPath& /*dag*/) {
        _renameCallbackIds.push_back(
            MNodeMessage::addNameChangedCallback(obj, processNodeRenamed, this));
    });
}

void MayaNodeObserver::removeRenameCallbacks() { removeCallbackIds(_renameCallbackIds); }

void MayaNodeObserver::updateDagPathCallbacks()
{
    removeDagPathCallbacks();

    // We need to observe parenting change in the node and any ancestor.
    // This is to support listeners that track the node by full path
    // and need to update data related to this full path.
    doForNodeAndAncestors(_observedNode, [this](MObject& /*obj*/, MDagPath& dag) {
        _parentAddedCallbackIds.push_back(
            MDagMessage::addParentAddedDagPathCallback(dag, processParentAdded, this));
    });
}

void MayaNodeObserver::removeDagPathCallbacks() { removeCallbackIds(_parentAddedCallbackIds); }

void MayaNodeObserver::updateAncestorCallbacks()
{
    removeAncestorCallbacks();

    // Remember the path for which we are accumulating the listener
    {
        MDagPath ancestorPath;
        MDagPath::getAPathTo(_observedNode, ancestorPath);
        _ancestorCallbacksPath = ancestorPath.fullPathName();
    }

    // Add listener for all the ancestors
    doForNodeAndAncestors(_observedNode, [this](MObject& obj, MDagPath& /*dag*/) {
        _ancestorCallbackIds.push_back(
            MNodeMessage::addNodeDirtyPlugCallback(obj, processPlugDirty, this));
    });
}

void MayaNodeObserver::removeAncestorCallbacks() { removeCallbackIds(_ancestorCallbackIds); }

////////////////////////////////////////////////////////////////////////////
//
// Maya callbacks processing and forwarding to listeners.

namespace {

template <typename T> struct AutoValueRestore
{
    AutoValueRestore(T& value, T newValue)
        : _value(value)
        , _oldValue(value)
    {
        value = newValue;
    }

    ~AutoValueRestore() { _value = _oldValue; }

    T&      _value;
    const T _oldValue;
};

} // namespace

/* static */
void MayaNodeObserver::processPlugDirty(MObject& node, MPlug& plug, void* clientData)
{
    auto self = static_cast<MayaNodeObserver*>(clientData);
    if (!self)
        return;

    // This prevents recursion.
    if (self->_inAncestorCallback)
        return;

    AutoValueRestore<bool> restoreInAncestor(self->_inAncestorCallback, true);

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;

    // If the observed node's path has changed, update ancestor listener
    // and the DAG listener.
    MDagPath currentPath;
    MDagPath::getAPathTo(self->_observedNode, currentPath);
    const bool pathChanged = (currentPath.fullPathName() != self->_ancestorCallbacksPath);
    if (pathChanged) {
        // Note: we need to copy the old name before calling processNodeRename
        //       because it will call the updateObserving function which will
        //       change the _ancestorCallbacksPath value.
        const MString oldName = self->_ancestorCallbacksPath;
        self->processNodeRenamed(self->_observedNode, oldName, clientData);
    }

    for (Listener* cb : cachedListeners)
        cb->processPlugDirty(self->_observedNode, node, plug, pathChanged);
}

/* static */
void MayaNodeObserver::processNodeRenamed(
    MObject& /*node*/,
    const MString& oldName,
    void*          clientData)
{
    auto self = static_cast<MayaNodeObserver*>(clientData);
    if (!self)
        return;

    // Nodes only have a proper DAG path once renamed.
    // So, on rename, we update the listener.
    // Note that even then, they might still not have a parent...
    // So we will again possibly update on reparenting.
    self->updateObserving();

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processNodeRenamed(self->_observedNode, oldName);
}

/* static */
void MayaNodeObserver::processParentAdded(
    MDagPath& childPath,
    MDagPath& parentPath,
    void*     clientData)
{
    auto self = static_cast<MayaNodeObserver*>(clientData);
    if (!self)
        return;

    // Reparented, so listen to new hierarchy.
    self->updateObserving();

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processParentAdded(self->_observedNode, childPath, parentPath);
}

} // namespace MAYAUSD_NS_DEF
