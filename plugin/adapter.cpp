#include "adapter.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaAdapter::HdMayaAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate) :
    _id(id), _delegate(delegate) { }

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

PXR_NAMESPACE_CLOSE_SCOPE
