#include <hdmaya/adapters/dagAdapter.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaDagAdapter, TfType::Bases<HdMayaAdapter> >();
}

namespace {

void _changeTransformAttributes(MNodeMessage::AttributeMessage /*msg*/, MPlug& /*plug*/, MPlug& /*otherPlug*/, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->MarkDirty(HdChangeTracker::DirtyTransform);
}

}

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath) :
    HdMayaAdapter(dagPath.node(), id, delegate), _dagPath(dagPath) {
    CalculateTransform();
}

void
HdMayaDagAdapter::CalculateTransform() {
    _transform = getGfMatrixFromMaya(_dagPath.inclusiveMatrix());
};

void
HdMayaDagAdapter::CreateCallbacks() {
    MStatus status;
    for (auto dag = GetDagPath(); dag.length() > 0; dag.pop()) {
        MObject obj = dag.node();
        if (obj != MObject::kNullObj) {
            auto id = MNodeMessage::addAttributeChangedCallback(obj, _changeTransformAttributes, this, &status);
            if (status) { AddCallback(id); }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void
HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
    if (dirtyBits & HdChangeTracker::DirtyTransform) {
        CalculateTransform();
    }
}

void
HdMayaDagAdapter::RemovePrim() {
    GetDelegate()->RemoveRprim(GetID());
}

void
HdMayaDagAdapter::PopulateSelection(
    const HdSelection::HighlightMode& mode,
    HdSelection* selection) {
    selection->AddRprim(mode, GetID());
}

PXR_NAMESPACE_CLOSE_SCOPE
