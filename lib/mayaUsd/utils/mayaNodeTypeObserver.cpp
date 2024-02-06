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

#include "mayaNodeTypeObserver.h"

#include <mayaUsd/base/debugCodes.h>

#include <maya/MDGMessage.h>

namespace MAYAUSD_NS_DEF {

////////////////////////////////////////////////////////////////////////////
//
// External listener called when Maya notifications are received.
// Default implementation is to do nothing.

void MayaNodeTypeObserver::Listener::processNodeAdded(MObject& /*node*/) { }
void MayaNodeTypeObserver::Listener::processNodeRemoved(MObject& /*node*/) { }

////////////////////////////////////////////////////////////////////////////
//
// Constructor and destructor.

MayaNodeTypeObserver::MayaNodeTypeObserver(const MString& nodeTypeName)
    : _nodeTypeName(nodeTypeName)
{
    updateNodeAddedRemovedCallbacks();
}

MayaNodeTypeObserver::~MayaNodeTypeObserver()
{
    // Stop listening to Maya notifications.
    removeNodeAddedRemovedCallbacks();
}

////////////////////////////////////////////////////////////////////////////
//
// Listener management.

void MayaNodeTypeObserver::addTypeListener(Listener& listener)
{
    // Start tracking and calling the given node type listener.
    _listeners.insert(&listener);
}

void MayaNodeTypeObserver::removeTypeListener(Listener& listener)
{
    // Stop tracking and calling the given node type listener.
    _listeners.erase(&listener);
}

void MayaNodeTypeObserver::addNodeListener(MayaNodeObserver::Listener& listener)
{
    for (auto& handleAndObserver : _observedNodes)
        handleAndObserver.second.addListener(listener);
}

void MayaNodeTypeObserver::removeNodeListener(MayaNodeObserver::Listener& listener)
{
    for (auto& handleAndObserver : _observedNodes)
        handleAndObserver.second.removeListener(listener);
}

////////////////////////////////////////////////////////////////////////////
//
// Manual observed node management.

MayaNodeObserver* MayaNodeTypeObserver::addObservedNode(const MObject& node)
{
    MObjectHandle handle(node);
    auto          iter = _observedNodes.find(handle);
    if (iter != _observedNodes.end())
        return &(iter->second);

    MayaNodeObserver& observer = _observedNodes[handle];
    observer.startObserving(node);
    return &observer;
}

void MayaNodeTypeObserver::removeObservedNode(const MObject& node)
{
    MObjectHandle handle(node);
    _observedNodes.erase(handle);
}

MayaNodeObserver* MayaNodeTypeObserver::getNodeObserver(const MObject& node)
{
    MObjectHandle handle(node);
    auto          iter = _observedNodes.find(handle);
    if (iter == _observedNodes.end())
        return nullptr;

    return &(iter->second);
}

////////////////////////////////////////////////////////////////////////////
//
// Maya listener registration and cleanup.

void MayaNodeTypeObserver::updateNodeAddedRemovedCallbacks()
{
    removeNodeAddedRemovedCallbacks();

    _nodeAddedRemovedCallbackIds.push_back(
        MDGMessage::addNodeAddedCallback(processNodeAdded, _nodeTypeName, this));

    _nodeAddedRemovedCallbackIds.push_back(
        MDGMessage::addNodeRemovedCallback(processNodeRemoved, _nodeTypeName, this));
}

void MayaNodeTypeObserver::removeNodeAddedRemovedCallbacks()
{
    MayaNodeObserver::removeCallbackIds(_nodeAddedRemovedCallbackIds);
}

////////////////////////////////////////////////////////////////////////////
//
// Maya listener processing and forwarding.

/* static */
void MayaNodeTypeObserver::processNodeAdded(MObject& node, void* clientData)
{
    auto self = static_cast<MayaNodeTypeObserver*>(clientData);
    if (!self)
        return;

    self->addObservedNode(node);

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processNodeAdded(node);
}

/* static */
void MayaNodeTypeObserver::processNodeRemoved(MObject& node, void* clientData)
{
    auto self = static_cast<MayaNodeTypeObserver*>(clientData);
    if (!self)
        return;

    // Note: we make a copy of the set of listeners in case calling
    //       a listener adds or remove listeners.
    const auto cachedListeners = self->_listeners;
    for (Listener* cb : cachedListeners)
        cb->processNodeRemoved(node);

    self->removeObservedNode(node);
}

} // namespace MAYAUSD_NS_DEF
