//
// Copyright 2019 Luma Pictures
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
#include "adapter.h"

#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/materialNetworkConverter.h>
#include <hdMaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/type.h>

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) { TfType::Define<HdMayaAdapter>(); }

namespace {

void _preRemoval(MObject& node, void* clientData)
{
    TF_UNUSED(node);
    auto* adapter = reinterpret_cast<HdMayaAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg("Pre-removal callback triggered for prim (%s)\n", adapter->GetID().GetText());
    adapter->GetDelegate()->RemoveAdapter(adapter->GetID());
}

void _nameChanged(MObject& node, const MString& str, void* clientData)
{
    TF_UNUSED(node);
    TF_UNUSED(str);
    auto* adapter = reinterpret_cast<HdMayaAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg("Name-changed callback triggered for prim (%s)\n", adapter->GetID().GetText());
    adapter->RemoveCallbacks();
    adapter->GetDelegate()->RecreateAdapterOnIdle(adapter->GetID(), adapter->GetNode());
}

} // namespace

HdMayaAdapter::HdMayaAdapter(const MObject& node, const SdfPath& id, HdMayaDelegateCtx* delegate)
    : _id(id)
    , _delegate(delegate)
    , _node(node)
{
}

HdMayaAdapter::~HdMayaAdapter() { RemoveCallbacks(); }

void HdMayaAdapter::AddCallback(MCallbackId callbackId) { _callbacks.push_back(callbackId); }

void HdMayaAdapter::RemoveCallbacks()
{
    if (_callbacks.empty()) {
        return;
    }

    TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
        .Msg("Removing all adapter callbacks for prim (%s).\n", GetID().GetText());
    for (auto c : _callbacks) {
        MMessage::removeCallback(c);
    }
    std::vector<MCallbackId>().swap(_callbacks);
}

VtValue HdMayaAdapter::Get(const TfToken& key)
{
    TF_UNUSED(key);
    return {};
};

bool HdMayaAdapter::HasType(const TfToken& typeId) const
{
    TF_UNUSED(typeId);
    return false;
}

void HdMayaAdapter::CreateCallbacks()
{
    if (_node != MObject::kNullObj) {
        TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
            .Msg("Creating generic adapter callbacks for prim (%s).\n", GetID().GetText());

        MStatus status;
        auto    id = MNodeMessage::addNodePreRemovalCallback(_node, _preRemoval, this, &status);
        if (status) {
            AddCallback(id);
        }
        id = MNodeMessage::addNameChangedCallback(_node, _nameChanged, this, &status);
        if (status) {
            AddCallback(id);
        }
    }
}

MStatus HdMayaAdapter::Initialize()
{
    auto status = MayaAttrs::initialize();
    if (status) {
        HdMayaMaterialNetworkConverter::initialize();
    }
    return status;
}

PXR_NAMESPACE_CLOSE_SCOPE
