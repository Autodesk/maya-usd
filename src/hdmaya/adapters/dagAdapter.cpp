#include <hdmaya/adapters/dagAdapter.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MNodeMessage.h>
#include <maya/MNodeClass.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaDagAdapter, TfType::Bases<HdMayaAdapter> >();
}

namespace {

bool _xformAttrsInitialized = false;

MObject _xformVisibilityAttribute; // Will hold "visibility" attribute when initialized

void _TransformNodeDirty(
    MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    if (plug == _xformVisibilityAttribute ){
        adapter->MarkDirty(HdChangeTracker::DirtyVisibility);
    }
}

void _ChangeTransformAttributes(
    MNodeMessage::AttributeMessage /*msg*/, MPlug& plug, MPlug& /*otherPlug*/, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    if (plug == _xformVisibilityAttribute ){
        adapter->MarkDirty(HdChangeTracker::DirtyVisibility);
    } else {
        adapter->MarkDirty(HdChangeTracker::DirtyTransform);
    }
}

}

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath) :
    HdMayaAdapter(dagPath.node(), id, delegate), _dagPath(dagPath) {
    if (!_xformAttrsInitialized) {
        MNodeClass xformNodeClass("transform");
        if (TF_VERIFY(xformNodeClass.typeId() != 0)) {
            MStatus status;
            _xformAttrsInitialized = true;
            _xformVisibilityAttribute = xformNodeClass.attribute("visibility", &status);
            _xformAttrsInitialized &= TF_VERIFY(status);
        }
    }
    CalculateTransform();
}

bool
HdMayaDagAdapter::GetVisible(){
    MDagPath path(GetDagPath());
    if (ARCH_UNLIKELY(!path.isValid())) { return {}; }
    return path.isVisible();
}

bool
HdMayaDagAdapter::CalculateTransform() {
    const auto _oldTransform = _transform;
    _transform = getGfMatrixFromMaya(_dagPath.inclusiveMatrix());
    return !GfIsClose(_oldTransform, _transform, 0.001);
};

void
HdMayaDagAdapter::CreateCallbacks() {
    MStatus status;
    for (auto dag = GetDagPath(); dag.length() > 0; dag.pop()) {
        MObject obj = dag.node();
        if (obj != MObject::kNullObj) {
            auto id = MNodeMessage::addAttributeChangedCallback(obj, _ChangeTransformAttributes, this, &status);
            if (status) { AddCallback(id); }
            id = MNodeMessage::addNodeDirtyCallback(obj, _TransformNodeDirty, this, &status);
            if (status) { AddCallback(id); }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void
HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits & HdChangeTracker::DirtyTransform) {
        // TODO: this will trigger dg evalutation during the dirty-bit-push
        // loop... refactor to avoid - we should just be able to only set the
        // DirtyTransform bit if the transform was actually dirtied - we will
        // miss the case where it was "changed" to the same value, but this
        // should be an infrequent enough case we can probably ignore - or
        // else delay the check until the renderOverride's "setup"
        if (!CalculateTransform()) {
            dirtyBits &= ~HdChangeTracker::DirtyTransform;
        }
    }

    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
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
