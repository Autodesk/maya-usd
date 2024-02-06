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

namespace MAYAUSD_NS_DEF {

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

    updateRenameCallback();
    updateAncestorCallbacks();
    updateDagPathCallbacks();
}

void MayaNodeObserver::stopObserving()
{
    removeRenameCallback();
    removeDagPathCallbacks();
    removeAncestorCallbacks();

    _observedNode = MObject();
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

void MayaNodeObserver::updateRenameCallback()
{
    removeRenameCallback();
    _renameCallbackId
        = MNodeMessage::addNameChangedCallback(_observedNode, processNodeRenamed, this);
}

void MayaNodeObserver::removeRenameCallback() { removeCallbackId(_renameCallbackId); }

void MayaNodeObserver::updateDagPathCallbacks()
{
    removeDagPathCallbacks();

    MDagPathArray dags;
    if (MDagPath::getAllPathsTo(_observedNode, dags)) {
        const auto numDags = dags.length();
        for (auto i = decltype(numDags) { 0 }; i < numDags; ++i) {
            MDagPath& dag = dags[i];
            for (; dag.length() > 0; dag.pop()) {
                MObject obj = dag.node();
                if (obj == MObject::kNullObj)
                    continue;
                _parentAddedCallbackIds.push_back(
                    MDagMessage::addParentAddedDagPathCallback(dag, processParentAdded, this));
            }
        }
    }
}

void MayaNodeObserver::removeDagPathCallbacks() { removeCallbackIds(_parentAddedCallbackIds); }

void MayaNodeObserver::updateAncestorCallbacks()
{
    removeAncestorCallbacks();

    // Add our own callback
    _ancestorCallbackIds.push_back(
        MNodeMessage::addNodeDirtyPlugCallback(_observedNode, processPlugDirty, this));

    // Remember the path for which we are accumulating the listener
    MDagPath ancestorPath;
    MDagPath::getAPathTo(_observedNode, ancestorPath);
    _ancestorCallbacksPath = ancestorPath.fullPathName();

    // Add listener for all the ancestors
    for (ancestorPath.pop(); ancestorPath.isValid() && ancestorPath.length() > 0;
         ancestorPath.pop()) {
        MObject ancestorObj = ancestorPath.node();
        _ancestorCallbackIds.push_back(
            MNodeMessage::addNodeDirtyPlugCallback(ancestorObj, processPlugDirty, this));
    }
}

void MayaNodeObserver::removeAncestorCallbacks() { removeCallbackIds(_ancestorCallbackIds); }

void MayaNodeObserver::updateAllNameRelatedCallbacks()
{
    updateAncestorCallbacks();
    updateDagPathCallbacks();
}

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

    // If the observed node's path has changed, update ancestor listener
    // and the DAG listener.
    MDagPath currentPath;
    MDagPath::getAPathTo(self->_observedNode, currentPath);
    const bool pathChanged = (currentPath.fullPathName() != self->_ancestorCallbacksPath);
    if (pathChanged)
        self->updateAllNameRelatedCallbacks();

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processPlugDirty(self->_observedNode, node, plug, pathChanged);
}

/* static */
void MayaNodeObserver::processNodeRenamed(MObject& node, const MString& oldName, void* clientData)
{
    auto self = static_cast<MayaNodeObserver*>(clientData);
    if (!self)
        return;

    // Nodes only have a proper DAG path once renamed.
    // So, on rename, we update the listener.
    self->updateAllNameRelatedCallbacks();

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processNodeRenamed(node, oldName);
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
    self->updateAllNameRelatedCallbacks();

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processParentAdded(self->_observedNode, childPath, parentPath);
}

} // namespace MAYAUSD_NS_DEF
