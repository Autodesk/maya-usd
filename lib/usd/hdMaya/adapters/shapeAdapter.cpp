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
#include "shapeAdapter.h"

#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/type.h>

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaShapeAdapter, TfType::Bases<HdMayaDagAdapter>>();
}

HdMayaShapeAdapter::HdMayaShapeAdapter(
    const SdfPath&     id,
    HdMayaDelegateCtx* delegate,
    const MDagPath&    dagPath)
    : HdMayaDagAdapter(id, delegate, dagPath)
{
    _CalculateExtent();
}

void HdMayaShapeAdapter::_CalculateExtent()
{
    MStatus    status;
    MFnDagNode dagNode(GetDagPath(), &status);
    if (ARCH_LIKELY(status)) {
        const auto bb = dagNode.boundingBox();
        const auto mn = bb.min();
        const auto mx = bb.max();
        _extent.SetMin({ mn.x, mn.y, mn.z });
        _extent.SetMax({ mx.x, mx.y, mx.z });
        _extentDirty = false;
    }
};

size_t HdMayaShapeAdapter::SamplePrimvar(
    const TfToken& key,
    size_t         maxSampleCount,
    float*         times,
    VtValue*       samples)
{
    if (maxSampleCount < 1) {
        return 0;
    }
    times[0] = 0.0f;
    samples[0] = Get(key);
    return 1;
}

HdMeshTopology HdMayaShapeAdapter::GetMeshTopology() { return {}; };

HdBasisCurvesTopology HdMayaShapeAdapter::GetBasisCurvesTopology() { return {}; };

HdDisplayStyle HdMayaShapeAdapter::GetDisplayStyle() { return { 0, false, false }; }

PxOsdSubdivTags HdMayaShapeAdapter::GetSubdivTags() { return {}; }

void HdMayaShapeAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    HdMayaDagAdapter::MarkDirty(dirtyBits);
    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        _extentDirty = true;
    }
}

MObject HdMayaShapeAdapter::GetMaterial()
{
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "Called HdMayaShapeAdapter::GetMaterial() - %s\n",
            GetDagPath().partialPathName().asChar());

    MStatus    status;
    MFnDagNode dagNode(GetDagPath(), &status);
    if (!status) {
        return MObject::kNullObj;
    }

    auto instObjGroups = dagNode.findPlug(MayaAttrs::dagNode::instObjGroups, true);
    if (instObjGroups.isNull()) {
        return MObject::kNullObj;
    }

    MPlugArray conns;
    // TODO: deal with instancing properly.
    instObjGroups.elementByLogicalIndex(0).connectedTo(conns, false, true);

    const auto numConnections = conns.length();
    if (numConnections == 0) {
        return MObject::kNullObj;
    }
    for (auto i = decltype(numConnections) { 0 }; i < numConnections; ++i) {
        auto sg = conns[i].node();
        if (sg.apiType() == MFn::kShadingEngine) {
            return sg;
        }
    }

    return MObject::kNullObj;
}

const GfRange3d& HdMayaShapeAdapter::GetExtent()
{
    if (_extentDirty) {
        _CalculateExtent();
    }
    return _extent;
}

TfToken HdMayaShapeAdapter::GetRenderTag() const { return HdTokens->geometry; }

void HdMayaShapeAdapter::PopulateSelectedPaths(
    const MDagPath&                             selectedDag,
    SdfPathVector&                              selectedSdfPaths,
    std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
    const HdSelectionSharedPtr&                 selection)
{
    VtIntArray indices(1);
    if (IsInstanced()) {
        indices[0] = selectedDag.instanceNumber();
        selection->AddInstance(HdSelection::HighlightModeSelect, _id, indices);
        if (selectedMasters.find(_id) == selectedMasters.end()) {
            selectedSdfPaths.push_back(_id);
            selectedMasters.insert(_id);
        }
    } else {
        selection->AddRprim(HdSelection::HighlightModeSelect, _id);
        selectedSdfPaths.push_back(_id);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
