#include "adapter.h"

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

void _aboutToDelete(MObject& node, MDGModifier& modifier, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaAdapter*>(clientData);
    adapter->GetDelegate()->RemoveAdapter(adapter->GetID());
}

}

HdMayaAdapter::HdMayaAdapter(MObject node, const SdfPath& id, HdMayaDelegateCtx* delegate) :
    _node(node), _id(id), _delegate(delegate) { }

HdMayaAdapter::~HdMayaAdapter() {
    for (auto c : _callbacks) {
        MMessage::removeCallback(c);
    }
}

void
HdMayaAdapter::AddCallback(MCallbackId callbackId) {
    _callbacks.push_back(callbackId);
}

VtValue
HdMayaAdapter::Get(const TfToken& /*key*/) {
    return {};
};

void
HdMayaAdapter::CreateCallbacks() {
    if (_node != MObject::kNullObj) {
        MStatus status;
        auto id = MNodeMessage::addNodeAboutToDeleteCallback(_node, _aboutToDelete, this, &status);
        if (status) { AddCallback(id); }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
