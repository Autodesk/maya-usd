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
#include <hdmaya/adapters/dagAdapter.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MAnimControl.h>
#include <maya/MDGContext.h>
#include <maya/MDGContextGuard.h>
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

void _HierarchyChanged(MDagPath& child, MDagPath& parent, void* clientData) {
    TF_UNUSED(child);
    TF_UNUSED(parent);
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->RemoveCallbacks();
    adapter->RemovePrim();
    adapter->GetDelegate()->RecreateAdapterOnIdle(
        adapter->GetID(), adapter->GetNode());
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

void _InstancerNodePreRemoval(MObject& node, void* clientData) {
    TF_UNUSED(node);
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->MarkDirty(
        HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex |
        HdChangeTracker::DirtyPrimvar);
    adapter->RemoveCallbacks();
    adapter->RemovePrim();
    adapter->GetDelegate()->RecreateAdapterOnIdle(
        adapter->GetID(), adapter->GetNode());
}

void _MasterNodePreRemoval(MObject& node, void* clientData) {
    // The node points to the transform above the shape, not the shape itself.
    TF_UNUSED(node);
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    MDagPathArray dags;
    MDagPath::getAllPathsTo(adapter->GetNode(), dags);
    if (dags.length() < 2) { return; }
    adapter->RemoveCallbacks();
    adapter->RemovePrim();
    adapter->GetDelegate()->RecreateAdapterOnIdle(
        adapter->GetID(), dags[1].node());
}

const auto _instancePrimvarDescriptors = HdPrimvarDescriptorVector{
    {_tokens->instanceTransform, HdInterpolationInstance,
     HdPrimvarRoleTokens->none},
};

} // namespace

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath)
    : HdMayaAdapter(dagPath.node(), id, delegate), _dagPath(dagPath) {
    // We shouldn't call virtual functions in constructors.
    _isVisible = GetDagPath().isVisible();
    _isInstanced = _dagPath.isInstanced() && _dagPath.instanceNumber() == 0;
}

void HdMayaDagAdapter::_CalculateTransform() {
    if (_invalidTransform) {
        if (IsInstanced()) {
            _transform[0].SetIdentity();
            _transform[1].SetIdentity();
        } else {
            _transform[0] = GetGfMatrixFromMaya(_dagPath.inclusiveMatrix());
            if (GetDelegate()->GetParams().enableMotionSamples) {
                MDGContextGuard guard(MAnimControl::currentTime() + 1.0);
                _transform[1] = GetGfMatrixFromMaya(_dagPath.inclusiveMatrix());
            } else {
                _transform[1] = _transform[0];
            }
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
    return _transform[0];
}

size_t HdMayaDagAdapter::SampleTransform(
    size_t maxSampleCount, float* times, GfMatrix4d* samples) {
    _CalculateTransform();
    if (maxSampleCount < 1) { return 0; }
    times[0] = 0.0f;
    samples[0] = _transform[0];
    if (maxSampleCount == 1 || !GetDelegate()->GetParams().enableMotionSamples) {
        return 1;
    } else {
        times[1] = 1.0f;
        samples[1] = _transform[1];
        return 2;
    }
}

void HdMayaDagAdapter::CreateCallbacks() {
    MStatus status;
    if (IsInstanced()) {
        auto obj = GetDagPath().transform();
        auto id = MNodeMessage::addNodePreRemovalCallback(
            obj, _MasterNodePreRemoval, this, &status);
        if (status) { AddCallback(id); }
        MDagPathArray dags;
        if (MDagPath::getAllPathsTo(GetDagPath().node(), dags)) {
            const auto numDags = dags.length();
            for (auto i = decltype(numDags){0}; i < numDags; ++i) {
                auto cdag = dags[i];
                cdag.pop();
                obj = cdag.node();
                id = MNodeMessage::addNodePreRemovalCallback(
                    obj, _InstancerNodePreRemoval, this, &status);
                if (status) { AddCallback(id); }
                for (; cdag.length() > 0; cdag.pop()) {
                    obj = cdag.node();
                    if (obj != MObject::kNullObj) {
                        id = MNodeMessage::addNodeDirtyPlugCallback(
                            obj, _InstancerNodeDirty, this, &status);
                        if (status) { AddCallback(id); }
                        _AddHierarchyChangedCallback(cdag);
                    }
                }
            }
        }
    } else {
        auto dag = GetDagPath();
        if (dag.node() != dag.transform()) { dag.pop(); }
        for (; dag.length() > 0; dag.pop()) {
            MObject obj = dag.node();
            if (obj != MObject::kNullObj) {
                if (!IsInstanced()) {
                    auto id = MNodeMessage::addNodeDirtyPlugCallback(
                        obj, _TransformNodeDirty, this, &status);
                    if (status) { AddCallback(id); }
                }
                _AddHierarchyChangedCallback(dag);
            }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
        if (IsInstanced()) {
            GetDelegate()->GetChangeTracker().MarkInstancerDirty(
                GetInstancerID(), dirtyBits);
        }
    }
}

void HdMayaDagAdapter::RemovePrim() {
    if (!_isPopulated) { return; }
    GetDelegate()->RemoveRprim(GetID());
    if (_isInstanced) {
        GetDelegate()->RemoveInstancer(
            GetID().AppendProperty(_tokens->instancer));
    }
    _isPopulated = false;
}

bool HdMayaDagAdapter::UpdateVisibility() {
    if (ARCH_UNLIKELY(!GetDagPath().isValid())) { return false; }
    const auto visible = _GetVisibility();
    if (visible != _isVisible) {
        _isVisible = visible;
        return true;
    }
    return false;
}

VtIntArray HdMayaDagAdapter::GetInstanceIndices(const SdfPath& prototypeId) {
    if (!IsInstanced()) { return {}; }
    MDagPathArray dags;
    if (!MDagPath::getAllPathsTo(GetDagPath().node(), dags)) { return {}; }
    const auto numDags = dags.length();
    VtIntArray ret;
    ret.reserve(numDags);
    for (auto i = decltype(numDags){0}; i < numDags; ++i) {
        if (dags[i].isValid() && dags[i].isVisible()) {
            ret.push_back(static_cast<int>(ret.size()));
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

SdfPath HdMayaDagAdapter::GetInstancerID() const {
    if (!_isInstanced) { return {}; }

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

bool HdMayaDagAdapter::_GetVisibility() const {
    return GetDagPath().isVisible();
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
