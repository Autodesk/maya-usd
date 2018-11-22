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

void _HierarchyChanged(MDagPath& child, MDagPath& parent, void* clientData) {
    TF_UNUSED(child);
    TF_UNUSED(parent);
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->GetDelegate()->RecreateAdapter(
        adapter->GetID(), adapter->GetNode());
}

template <typename T>
VtValue _GetPerInstanceValues(
    const MObject& obj,
    const std::function<void(const MTransformationMatrix&, T&)>& f,
    const T& defValue = T()) {
    MDagPathArray dags;
    if (!MDagPath::getAllPathsTo(obj, dags)) { return {}; }
    const auto numDags = dags.length();
    VtArray<T> ret;
    ret.assign(numDags, defValue);
    for (auto i = decltype(numDags){0}; i < numDags; ++i) {
        MTransformationMatrix matrix(dags[i].inclusiveMatrix());
        f(matrix, ret[i]);
    }
    return VtValue(ret);
}

const auto _instancePrimvarDescriptors = HdPrimvarDescriptorVector{
    /*{_tokens->translate, HdInterpolationInstance, HdPrimvarRoleTokens->none},
    {_tokens->rotate, HdInterpolationInstance, HdPrimvarRoleTokens->none},
    {_tokens->scale, HdInterpolationInstance, HdPrimvarRoleTokens->none},*/
    {_tokens->instanceTransform, HdInterpolationInstance, HdPrimvarRoleTokens->none},
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
        _transform = GetGfMatrixFromMaya(_dagPath.inclusiveMatrix());
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
            auto id = MNodeMessage::addNodeDirtyPlugCallback(
                obj, _TransformNodeDirty, this, &status);
            if (status) { AddCallback(id); }
            _AddHierarchyChangedCallback(dag);
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
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
    VtIntArray ret; ret.reserve(numDags);
    for (auto i = decltype(numDags){0}; i < numDags; ++i) {
        ret.push_back(i);
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

    const auto& id = GetID().AppendProperty(_tokens->instancer);
    auto& renderIndex = GetDelegate()->GetRenderIndex();
    if (renderIndex.GetInstancer(id) == nullptr) {
        renderIndex.InsertInstancer(GetDelegate(), id);
        renderIndex.GetChangeTracker().InstancerInserted(id);
    }
    return id;
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
    /*if (key == _tokens->translate) {
        return _GetPerInstanceValues<GfVec3f>(
            GetDagPath().node(),
            [](const MTransformationMatrix& matrix, GfVec3f& out) {
                const auto translation = matrix.getTranslation(MSpace::kWorld);
                out[0] = static_cast<float>(translation.x);
                out[1] = static_cast<float>(translation.y);
                out[2] = static_cast<float>(translation.z);
            },
            GfVec3f(0.0f, 0.0f, 0.0f));
    } else if (key == _tokens->rotate) {
        return _GetPerInstanceValues<GfVec4f>(
            GetDagPath().node(),
            [](const MTransformationMatrix& matrix, GfVec4f& out) {
                GfVec4d rotation;
                matrix.getRotationQuaternion(
                    rotation[0], rotation[1], rotation[2], rotation[3]);
                out[0] = static_cast<float>(rotation[0]);
                out[1] = static_cast<float>(rotation[1]);
                out[2] = static_cast<float>(rotation[2]);
                out[3] = static_cast<float>(rotation[3]);
                const auto translation = matrix.getTranslation(MSpace::kWorld);
                out[0] = static_cast<float>(translation.x);
                out[1] = static_cast<float>(translation.y);
                out[2] = static_cast<float>(translation.z);
            },
            GfVec4f(0.0f, 0.0f, 0.0f, 0.0f));
    } else if (key == _tokens->scale) {
        return _GetPerInstanceValues<GfVec3f>(
            GetDagPath().node(),
            [](const MTransformationMatrix& matrix, GfVec3f& out) {
                GfVec3d scale;
                matrix.getScale(scale.data(), MSpace::kWorld);
                out[0] = static_cast<float>(scale[0]);
                out[1] = static_cast<float>(scale[1]);
                out[2] = static_cast<float>(scale[2]);
            },
            GfVec3f(1.0f, 1.0f, 1.0f));
    } else*/ if (key == _tokens->instanceTransform) {
        return _GetPerInstanceValues<GfMatrix4d>(
            GetDagPath().node(),
            [](const MTransformationMatrix& matrix, GfMatrix4d& out) {
                out = GetGfMatrixFromMaya(matrix.asMatrix());
            });
    }
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
