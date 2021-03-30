//-
// ==========================================================================
// Copyright 2021 Autodesk, Inc. All rights reserved.
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc.
// and are protected under applicable copyright and trade secret law.
// They may not be disclosed to, copied or used by any third party without
// the prior written consent of Autodesk, Inc.
// ==========================================================================
//+

#include "renderItemAdapter.h"

#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/adapters/tokens.h>

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/base/tf/registryManager.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MAnimControl.h>
#include <maya/MDGContext.h>
#include <maya/MDGContextGuard.h>
#include <maya/MHWGeometry.h>

PXR_NAMESPACE_OPEN_SCOPE

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
	MGeometry::Primitive primitiveType,
	MString name
	)
    : HdMayaAdapter(MObject(), id, del)
	, _primitive(primitiveType)
	, _name(name)
{
    // We shouldn't call virtual functions in constructors.
 /*   _isVisible = GetDagPath().isVisible();
    _visibilityDirty = false;
    _isInstanced = _dagPath.isInstanced() && _dagPath.instanceNumber() == 0;*/
}

void HdMayaRenderItemAdapter::UpdateTransform(MRenderItem& ri)
{
	MMatrix matrix;
	if (ri.getMatrix(matrix) == MStatus::kSuccess)
	{
		_transform[0] = GetGfMatrixFromMaya(matrix);
		if (GetDelegate()->GetParams().enableMotionSamples) {
			MDGContextGuard guard(MAnimControl::currentTime() + 1.0);
			_transform[1] = GetGfMatrixFromMaya(matrix);
		}
		else {
			_transform[1] = _transform[0];
		}
	}
}

void HdMayaRenderItemAdapter::UpdateGeometry(MRenderItem& ri)
{
	MGeometry* geom = ri.geometry();
	VtIntArray faceVertexIndices;
	VtIntArray faceVertexCounts;	
	// TODO : Multiple streams
	// for now assume first is position
	if (geom && geom->vertexBufferCount() > 0) 
	{		
		// Vertices
		MVertexBuffer* verts = nullptr;
		if (verts = geom->vertexBuffer(0)) 
		{
			int vertCount = verts->vertexCount();
			_vertexPositions.clear();
			_vertexPositions.resize(vertCount);
			const auto* vertexPositions = reinterpret_cast<const GfVec3f*>(verts->map());
			// NOTE: Looking at HdMayaMeshAdapter::GetPoints notice assign(vertexPositions, vertexPositions + vertCount)
			// Why are we not multiplying with sizeof(GfVec3f) to calculate the offset ? 
			// The following happens when I try to do it :
			// Invalid Hydra prim - Vertex primvar points has 288 elements, while its topology references only upto element index 24.
			_vertexPositions.assign(vertexPositions, vertexPositions + vertCount);
			verts->unmap();
		}
		// Indices
		MIndexBuffer* indices = nullptr;		
		if (indices = geom->indexBuffer(0))
		{
			int indexCount = indices->size();
			faceVertexIndices.resize(indexCount);
			int* indicesData = (int*)indices->map();			
			for (int i = 0; i < indexCount; i++) faceVertexIndices[i] = indicesData[i];

			faceVertexCounts.resize(indexCount / 3);
			for (int i = 0; i < indexCount / 3; i++) faceVertexCounts[i] = 3;
			indices->unmap();
		}	
		// UVs
		//if(indices)
		//{
		//	_uvs.clear();
		//	int indexCount = indices->size();
		//	_uvs.resize(indexCount);
		//	// TODO : Fix this. these are bogus UVs but need to be there
		//	// to display anything at all
		//	for (int i = 0; i < indexCount; i++) _uvs[i] = (GfVec2f(0, 0));
		//}

		if (indices && verts)
		{
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

			MarkDirty(
				HdChangeTracker::DirtyTransform |
				HdChangeTracker::DirtyMaterialId |
				HdChangeTracker::DirtyTopology |
				HdChangeTracker::DirtyPrimvar |
				HdChangeTracker::DirtyPoints
			);
		}
	}
}

HdMeshTopology HdMayaRenderItemAdapter::GetMeshTopology()
{
	int deb = 0;
	if (_meshTopology.GetNumPoints() == 0)
	{
		deb = 1;
	}
	return _meshTopology;
}

VtValue HdMayaRenderItemAdapter::Get(const TfToken& key)
{
	if (key == HdTokens->points) 
	{
		return VtValue(_vertexPositions);
	}	 
	/*else if (key == HdMayaAdapterTokens->st) 
	{
		return VtValue(_uvs);
	}*/

	return {};
}

const GfMatrix4d& HdMayaRenderItemAdapter::GetTransform()
{    
    return _transform[0];
}

void HdMayaRenderItemAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
    }
}

void HdMayaRenderItemAdapter::Populate()
{
	if (_isPopulated) {
		return;
	}
	// TODO : GetInstancerID() instead of just {}
	GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), {});
	_isPopulated = true;
}

void HdMayaRenderItemAdapter::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    GetDelegate()->RemoveRprim(GetID());

    _isPopulated = false;
}

bool HdMayaRenderItemAdapter::UpdateVisibility()
{
	return true;
}


HdPrimvarDescriptorVector HdMayaRenderItemAdapter::GetPrimvarDescriptors(HdInterpolation interpolation)
{
	// Vertices
	if (interpolation == HdInterpolationVertex) 
	{
		HdPrimvarDescriptor desc;
		desc.name = UsdGeomTokens->points;
		desc.interpolation = interpolation;
		desc.role = HdPrimvarRoleTokens->point;
		return { desc };
	}
	// UVs
	//else if (interpolation == HdInterpolationFaceVarying) 
	//{
	//	HdPrimvarDescriptor desc;
	//	desc.name = HdMayaAdapterTokens->st;
	//	desc.interpolation = interpolation;
	//	desc.role = HdPrimvarRoleTokens->textureCoordinate;
	//	return { desc };
	//}

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
					ri.primitive(),
					ri.name()
					));
		});
}


PXR_NAMESPACE_CLOSE_SCOPE
