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
#include <hdmaya/adapters/shapeAdapter.h>

#include <pxr/base/tf/type.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <hdmaya/adapters/adapterDebugCodes.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaShapeAdapter, TfType::Bases<HdMayaDagAdapter> >();
}

HdMayaShapeAdapter::HdMayaShapeAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath)
    : HdMayaDagAdapter(id, delegate, dagPath) {
    _CalculateExtent();
}

void
HdMayaShapeAdapter::_CalculateExtent() {
    MStatus status;
    MFnDagNode dagNode(GetDagPath(), &status);
    if (ARCH_LIKELY(status)) {
        const auto bb = dagNode.boundingBox();
        const auto mn = bb.min();
        const auto mx = bb.max();
        _extent.SetMin({mn.x, mn.y, mn.z});
        _extent.SetMax({mx.x, mx.y, mx.z});
        _extentDirty = false;
    }
};

HdMeshTopology
HdMayaShapeAdapter::GetMeshTopology() {
    return {};
};

HdPrimvarDescriptorVector
HdMayaShapeAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) {
    return {};
}

void
HdMayaShapeAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    HdMayaDagAdapter::MarkDirty(dirtyBits);
    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        _extentDirty = true;
    }
}

MObject
HdMayaShapeAdapter::GetMaterial() {
    TF_DEBUG(HDMAYA_ADAPTER_GET).Msg(
            "Called HdMayaShapeAdapter::GetMaterial() - %s\n",
            GetDagPath().partialPathName().asChar());

    MStatus status;
    MFnDagNode dagNode(GetDagPath(), &status);
    if (!status) { return MObject::kNullObj; }

    auto instObjGroups = dagNode.findPlug("instObjGroups");
    if (instObjGroups.isNull()) { return MObject::kNullObj; }

    MPlugArray conns;
    // TODO: deal with instancing properly.
    instObjGroups.elementByLogicalIndex(0).connectedTo(conns, false, true);

    const auto numConnections = conns.length();
    if (numConnections == 0) { return MObject::kNullObj; }
    for (auto i = decltype(numConnections){0}; i < numConnections; ++i) {
        auto sg = conns[i].node();
        if (sg.apiType() == MFn::kShadingEngine) {
            return sg;
        }
    }

    return MObject::kNullObj;
}

const GfRange3d&
HdMayaShapeAdapter::GetExtent() {

    if (_extentDirty) {
        _CalculateExtent();
    }
    return _extent;
}

PXR_NAMESPACE_CLOSE_SCOPE
