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
#include "renderItemAdapter.h"

#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/mayaAttrs.h>

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/base/tf/registryManager.h>

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

TF_REGISTRY_FUNCTION(TfType) { TfType::Define<HdMayaRenderItemAdapter, TfType::Bases<HdMayaAdapter>>(); }

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (translate)
    (rotate)
    (scale)
    (instanceTransform)
    (instancer)
);
// clang-format on

namespace {

} // namespace

HdMayaRenderItemAdapter::HdMayaRenderItemAdapter(
    const SdfPath& id,
    HdMayaDelegateCtx* del,
	MGeometry::Primitive primitiveType
	)
    : HdMayaAdapter(MObject(), id, del)
{
    // We shouldn't call virtual functions in constructors.
 /*   _isVisible = GetDagPath().isVisible();
    _visibilityDirty = false;
    _isInstanced = _dagPath.isInstanced() && _dagPath.instanceNumber() == 0;*/
}

HDMAYA_API
TfToken HdMayaRenderItemAdapter::GetRenderTag() const
{
	return HdTokens->geometry;
}

HdDisplayStyle HdMayaRenderItemAdapter::GetDisplayStyle()
{
	return { 0, false, false };
//#if MAYA_APP_VERSION >= 2019
//	MStatus           status;
//	MFnDependencyNode node(GetNode(), &status);
//	if (ARCH_UNLIKELY(!status)) {
//		return { 0, false, false };
//	}
//	const auto displaySmoothMesh
//		= node.findPlug(MayaAttrs::mesh::displaySmoothMesh, true).asShort();
//	if (displaySmoothMesh == 0) {
//		return { 0, false, false };
//	}
//	const auto smoothLevel
//		= std::max(0, node.findPlug(MayaAttrs::mesh::smoothLevel, true).asInt());
//	return { smoothLevel, false, false };
//#else
//	return { 0, false, false };
//#endif
}



void HdMayaRenderItemAdapter::UpdateTransform(MRenderItem& ri)
{

}

void HdMayaRenderItemAdapter::UpdateGeometry(MRenderItem& ri)
{
	MGeometry* geom = ri.geometry();
	VtIntArray faceVertexIndices;
	VtIntArray faceVertexCounts;
	// TODO : Multiple streams
	// for now assume first is position
	if (geom && geom->vertexBufferCount() > 0) {		
		if (MVertexBuffer* verts = geom->vertexBuffer(0)) {
			int vertCount = verts->vertexCount();
			_vertexPositions.clear();
			_vertexPositions.reserve(vertCount);
			float* vertexPositions = (float*)verts->map();
			_vertexPositions.assign(vertexPositions, vertexPositions + vertCount);			
			verts->unmap();
		}
		if (MIndexBuffer* indices = geom->indexBuffer(0))
		{
			int indexCount = indices->size();
			faceVertexIndices.reserve(indexCount);
			faceVertexCounts.reserve(indexCount/3);
			std::memcpy(faceVertexIndices.data(), indices->map(), indexCount *sizeof(int));
			for (int i = 0; i < 0; i++) faceVertexCounts[i] = 3;
			indices->unmap();
		}		
	}

	// TODO: Maybe we could use the flat shading of the display style?
	_meshTopology = HdMeshTopology(
#if MAYA_APP_VERSION >= 2019
		(GetDelegate()->GetParams().displaySmoothMeshes || GetDisplayStyle().refineLevel > 0)
		? PxOsdOpenSubdivTokens->catmullClark
		: PxOsdOpenSubdivTokens->none,
#else
		GetDelegate()->GetParams().displaySmoothMeshes ? PxOsdOpenSubdivTokens->catmullClark
		: PxOsdOpenSubdivTokens->none,
#endif

		UsdGeomTokens->rightHanded,
		faceVertexCounts,
		faceVertexIndices);
}

HdMeshTopology HdMayaRenderItemAdapter::GetMeshTopology()
{
	return _meshTopology;
}

VtValue HdMayaRenderItemAdapter::Get(const TfToken& key)
{
	if (key == HdTokens->points) {

		return VtValue(_vertexPositions);
	}
	// TODO
	//else if (key == HdMayaAdapterTokens->st) {
	//	return GetUVs();
	//}
	return {};
}

void HdMayaRenderItemAdapter::_CalculateTransform()
{
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

const GfMatrix4d& HdMayaRenderItemAdapter::GetTransform()
{
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg("Called HdMayaRenderItemAdapter::GetTransform() - %s\n", _dagPath.partialPathName().asChar());
    _CalculateTransform();
    return _transform[0];
}

size_t HdMayaRenderItemAdapter::SampleTransform(size_t maxSampleCount, float* times, GfMatrix4d* samples)
{
    _CalculateTransform();
    if (maxSampleCount < 1) {
        return 0;
    }
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

void HdMayaRenderItemAdapter::CreateCallbacks()
{
    //MStatus status;

    //TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
    //    .Msg("Creating dag adapter callbacks for prim (%s).\n", GetID().GetText());

    //MDagPathArray dags;
    //if (MDagPath::getAllPathsTo(GetDagPath().node(), dags)) {
    //    const auto numDags = dags.length();
    //    auto       dagNodeDirtyCallback = numDags > 1 ? _InstancerNodeDirty : _TransformNodeDirty;
    //    for (auto i = decltype(numDags) { 0 }; i < numDags; ++i) {
    //        auto dag = dags[i];
    //        for (; dag.length() > 0; dag.pop()) {
    //            MObject obj = dag.node();
    //            if (obj != MObject::kNullObj) {
    //                auto id = MNodeMessage::addNodeDirtyPlugCallback(
    //                    obj, dagNodeDirtyCallback, this, &status);
    //                if (status) {
    //                    AddCallback(id);
    //                }
    //                TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
    //                    .Msg(
    //                        "- Added _InstancerNodeDirty callback for "
    //                        "dagPath (%s).\n",
    //                        dag.partialPathName().asChar());
    //                _AddHierarchyChangedCallbacks(dag);
    //            }
    //        }
    //    }
    //}
    //HdMayaAdapter::CreateCallbacks();
}

void HdMayaRenderItemAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
        if (IsInstanced()) {
            GetDelegate()->GetChangeTracker().MarkInstancerDirty(GetInstancerID(), dirtyBits);
        }
        if (dirtyBits & HdChangeTracker::DirtyVisibility) {
            _visibilityDirty = true;
        }
    }
}

void HdMayaRenderItemAdapter::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    GetDelegate()->RemoveRprim(GetID());
    if (_isInstanced) {
        GetDelegate()->RemoveInstancer(GetID().AppendProperty(_tokens->instancer));
    }
    _isPopulated = false;
}

bool HdMayaRenderItemAdapter::UpdateVisibility()
{
    if (ARCH_UNLIKELY(!GetDagPath().isValid())) {
        return false;
    }
    const auto visible = _GetVisibility();
    _visibilityDirty = false;
    if (visible != _isVisible) {
        _isVisible = visible;
        return true;
    }
    return false;
}

bool HdMayaRenderItemAdapter::IsVisible(bool checkDirty)
{
    if (checkDirty && _visibilityDirty) {
        UpdateVisibility();
    }
    return _isVisible;
}

VtIntArray HdMayaRenderItemAdapter::GetInstanceIndices(const SdfPath& prototypeId)
{
    if (!IsInstanced()) {
        return {};
    }
    MDagPathArray dags;
    if (!MDagPath::getAllPathsTo(GetDagPath().node(), dags)) {
        return {};
    }
    const auto numDags = dags.length();
    VtIntArray ret;
    ret.reserve(numDags);
    for (auto i = decltype(numDags) { 0 }; i < numDags; ++i) {
        if (dags[i].isValid() && dags[i].isVisible()) {
            ret.push_back(static_cast<int>(ret.size()));
        }
    }
    return ret;
}


SdfPath HdMayaRenderItemAdapter::GetInstancerID() const
{
    if (!_isInstanced) {
        return {};
    }

    return GetID().AppendProperty(_tokens->instancer);
}

HdPrimvarDescriptorVector
HdMayaRenderItemAdapter::GetInstancePrimvarDescriptors(HdInterpolation interpolation) const
{
	return {};
    //if (interpolation == HdInterpolationInstance) {
    //    return _instancePrimvarDescriptors;
    //} else {
    //    return {};
    //}
}

bool HdMayaRenderItemAdapter::_GetVisibility() const { return GetDagPath().isVisible(); }

VtValue HdMayaRenderItemAdapter::GetInstancePrimvar(const TfToken& key)
{
    if (key == _tokens->instanceTransform) {
        MDagPathArray dags;
        if (!MDagPath::getAllPathsTo(GetDagPath().node(), dags)) {
            return {};
        }
        const auto          numDags = dags.length();
        VtArray<GfMatrix4d> ret;
        ret.reserve(numDags);
        for (auto i = decltype(numDags) { 0 }; i < numDags; ++i) {
            if (dags[i].isValid() && dags[i].isVisible()) {
                ret.push_back(GetGfMatrixFromMaya(dags[i].inclusiveMatrix()));
            }
        }
        return VtValue(ret);
    }
    return {};
}

TF_REGISTRY_FUNCTION(TfType)
{
	TfType::Define<HdMayaRenderItemAdapter, TfType::Bases<HdMayaAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, renderItem)
{	
	HdMayaAdapterRegistry::RegisterRenderItemAdapter(
		TfToken(gsRenderItemTypeName),
		[](HdMayaDelegateCtx* del, const MRenderItem& ri) -> HdMayaRenderItemAdapterPtr
	{
		return HdMayaRenderItemAdapterPtr(
			new HdMayaRenderItemAdapter(
				del->GetPrimPath(ri, false),
				del,
				ri.primitive()));
	});
}


PXR_NAMESPACE_CLOSE_SCOPE
