//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "adapter.h"
#include "adapterDebugCodes.h"

#include <pxr/base/tf/type.h>

#include <maya/MNodeMessage.h>

#include "materialNetworkConverter.h"
#include "mayaAttrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) { TfType::Define<HdMayaAdapter>(); }

namespace {

void _preRemoval(MObject& node, void* clientData) {
    TF_UNUSED(node);
    auto* adapter = reinterpret_cast<HdMayaAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg(
            "Pre-removal callback triggered for prim (%s)\n",
            adapter->GetID().GetText());
    adapter->GetDelegate()->RemoveAdapter(adapter->GetID());
}

void _nameChanged(MObject& node, const MString& str, void* clientData) {
    TF_UNUSED(node);
    TF_UNUSED(str);
    auto* adapter = reinterpret_cast<HdMayaAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg(
            "Name-changed callback triggered for prim (%s)\n",
            adapter->GetID().GetText());
    adapter->RemoveCallbacks();
    adapter->GetDelegate()->RecreateAdapterOnIdle(
        adapter->GetID(), adapter->GetNode());
}

} // namespace

HdMayaAdapter::HdMayaAdapter(
    const MObject& node, const SdfPath& id, HdMayaDelegateCtx* delegate)
    : _id(id), _delegate(delegate), _node(node) {}

HdMayaAdapter::~HdMayaAdapter() { RemoveCallbacks(); }

void HdMayaAdapter::AddCallback(MCallbackId callbackId) {
    _callbacks.push_back(callbackId);
}

void HdMayaAdapter::RemoveCallbacks() {
    if (_callbacks.empty()) { return; }

    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg(
            "Removing all adapter callbacks for prim (%s).\n",
            GetID().GetText());
    for (auto c : _callbacks) { MMessage::removeCallback(c); }
    std::vector<MCallbackId>().swap(_callbacks);
}

VtValue HdMayaAdapter::Get(const TfToken& key) {
    TF_UNUSED(key);
    return {};
};

bool HdMayaAdapter::HasType(const TfToken& typeId) const {
    TF_UNUSED(typeId);
    return false;
}

void HdMayaAdapter::CreateCallbacks() {
    if (_node != MObject::kNullObj) {
        TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
            .Msg(
                "Creating generic adapter callbacks for prim (%s).\n",
                GetID().GetText());

        MStatus status;
        auto id = MNodeMessage::addNodePreRemovalCallback(
            _node, _preRemoval, this, &status);
        if (status) { AddCallback(id); }
        id = MNodeMessage::addNameChangedCallback(
            _node, _nameChanged, this, &status);
        if (status) { AddCallback(id); }
    }
}

MStatus HdMayaAdapter::Initialize() {
    auto status = MayaAttrs::initialize();
    if (status) { HdMayaMaterialNetworkConverter::initialize(); }
    return MayaAttrs::initialize();
}

PXR_NAMESPACE_CLOSE_SCOPE
