//
// Copyright 2018 Luma Pictures
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
#include <hdmaya/adapters/dagAdapter.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MDagMessage.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MTransformationMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaDagAdapter, TfType::Bases<HdMayaAdapter> >();
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (translate)(rotate)(scale)(instanceTransform)(instancer));

namespace {

void _TransformNodeDirty(MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY)
        .Msg(
            "Dag adapter marking prim (%s) dirty because %s plug was "
            "dirtied.\n",
            adapter->GetID().GetText(), plug.partialName().asChar());
    if (plug == MayaAttrs::dagNode::visibility) {
        if (adapter->UpdateVisibility()) {
            // Transform can change while dag path is hidden.
            adapter->MarkDirty(
                HdChangeTracker::DirtyVisibility |
                HdChangeTracker::DirtyTransform);
            adapter->InvalidateTransform();
        }
    } else if (adapter->IsVisible()) {
        adapter->MarkDirty(HdChangeTracker::DirtyTransform);
        adapter->InvalidateTransform();
    }
}

void _InstancerNodeDirty(MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY)
        .Msg(
            "Dag instancer adapter marking prim (%s) dirty because %s plug was "
            "dirtied.\n",
            adapter->GetID().GetText(), plug.partialName().asChar());
    adapter->MarkDirty(
        HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex |
        HdChangeTracker::DirtyPrimvar);
}

void _InstancerNodeDestroyed(
    void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->MarkDirty(
        HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex |
        HdChangeTracker::DirtyPrimvar);
}

void _HierarchyChanged(MDagPath& child, MDagPath& parent, void* clientData) {
    TF_UNUSED(child);
    TF_UNUSED(parent);
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->GetDelegate()->RecreateAdapter(
        adapter->GetID(), adapter->GetNode());
}

const auto _instancePrimvarDescriptors = HdPrimvarDescriptorVector{
    {_tokens->instanceTransform, HdInterpolationInstance,
     HdPrimvarRoleTokens->none},
};

} // namespace

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath)
    : HdMayaAdapter(dagPath.node(), id, delegate), _dagPath(dagPath) {
    UpdateVisibility();
    _isMasterInstancer =
        _dagPath.isInstanced() && _dagPath.instanceNumber() == 0;
}

void HdMayaDagAdapter::_CalculateTransform() {
    if (_invalidTransform) {
        if (IsMasterInstancer()) {
            _transform.SetIdentity();
        } else {
            _transform = GetGfMatrixFromMaya(_dagPath.inclusiveMatrix());
        }
        _invalidTransform = false;
    }
};

const GfMatrix4d& HdMayaDagAdapter::GetTransform() {
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "Called HdMayaDagAdapter::GetTransform() - %s\n",
            _dagPath.partialPathName().asChar());
    _CalculateTransform();
    return _transform;
}

void HdMayaDagAdapter::CreateCallbacks() {
    MStatus status;
    auto dag = GetDagPath();
    if (dag.node() != dag.transform()) { dag.pop(); }
    for (; dag.length() > 0; dag.pop()) {
        MObject obj = dag.node();
        if (obj != MObject::kNullObj) {
            if (!IsMasterInstancer()) {
                auto id = MNodeMessage::addNodeDirtyPlugCallback(
                    obj, _TransformNodeDirty, this, &status);
                if (status) { AddCallback(id); }
            }
            _AddHierarchyChangedCallback(dag);
        }
    }
    if (IsMasterInstancer()) {
        MDagPathArray dags;
        if (MDagPath::getAllPathsTo(GetDagPath().node(), dags)) {
            const auto numDags = dags.length();
            for (auto i = decltype(numDags){0}; i < numDags; ++i) {
                auto cdag = dags[i];
                MObject obj = cdag.node();
                auto id = MNodeMessage::addNodeDestroyedCallback(
                    obj, _InstancerNodeDestroyed, this, &status);
                if (status) { AddCallback(id); }
                cdag.pop();
                for (; cdag.length() > 0; cdag.pop()) {
                    obj = cdag.node();
                    if (obj != MObject::kNullObj) {
                        id = MNodeMessage::addNodeDirtyPlugCallback(
                            obj, _InstancerNodeDirty, this, &status);
                        if (status) { AddCallback(id); }
                    }
                }
            }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
        if (IsMasterInstancer()) {
            GetDelegate()->GetChangeTracker().MarkInstancerDirty(
                _GetInstancerID(), dirtyBits);
        }
    }
}

void HdMayaDagAdapter::RemovePrim() {
    if (_isMasterInstancer) {
        GetDelegate()->RemoveInstancer(
            GetID().AppendProperty(_tokens->instancer));
    }
    GetDelegate()->RemoveRprim(GetID());
}

void HdMayaDagAdapter::PopulateSelection(
    const HdSelection::HighlightMode& mode, HdSelection* selection) {
    selection->AddRprim(mode, GetID());
}

bool HdMayaDagAdapter::UpdateVisibility() {
    if (ARCH_UNLIKELY(!GetDagPath().isValid())) { return false; }
    const auto visible = GetDagPath().isVisible();
    if (visible != _isVisible) {
        _isVisible = visible;
        return true;
    }
    return false;
}

VtIntArray HdMayaDagAdapter::GetInstanceIndices(const SdfPath& prototypeId) {
    if (!IsMasterInstancer()) { return {}; }
    MDagPathArray dags;
    if (!MDagPath::getAllPathsTo(GetDagPath().node(), dags)) { return {}; }
    const auto numDags = dags.length();
    VtIntArray ret;
    ret.reserve(numDags);
    for (auto i = decltype(numDags){0}; i < numDags; ++i) {
        if (dags[i].isValid() && dags[i].isVisible()) {
            ret.push_back(ret.size());
        }
    }
    return ret;
}

void HdMayaDagAdapter::_AddHierarchyChangedCallback(MDagPath& dag) {
    MStatus status;
    auto id = MDagMessage::addParentAddedDagPathCallback(
        dag, _HierarchyChanged, this, &status);
    if (status) { AddCallback(id); }
}

SdfPath HdMayaDagAdapter::_GetInstancerID() {
    if (!_isMasterInstancer) { return {}; }

    return GetID().AppendProperty(_tokens->instancer);
}

HdPrimvarDescriptorVector HdMayaDagAdapter::GetInstancePrimvarDescriptors(
    HdInterpolation interpolation) const {
    if (interpolation == HdInterpolationInstance) {
        return _instancePrimvarDescriptors;
    } else {
        return {};
    }
}

VtValue HdMayaDagAdapter::GetInstancePrimvar(const TfToken& key) {
    if (key == _tokens->instanceTransform) {
        MDagPathArray dags;
        if (!MDagPath::getAllPathsTo(GetDagPath().node(), dags)) { return {}; }
        const auto numDags = dags.length();
        VtArray<GfMatrix4d> ret;
        ret.reserve(numDags);
        for (auto i = decltype(numDags){0}; i < numDags; ++i) {
            if (dags[i].isValid() && dags[i].isVisible()) {
                ret.push_back(GetGfMatrixFromMaya(dags[i].inclusiveMatrix()));
            }
        }
        return VtValue(ret);
    }
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
