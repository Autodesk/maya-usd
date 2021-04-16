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

#include <functional>

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
}

TfToken HdMayaRenderItemAdapter::GetRenderTag() const
{
	switch (_primitive)
	{		
		case MHWRender::MGeometry::Primitive::kLines:
			//return HdRenderTagTokens->guide;
			// TODO: Why must render tag match  for both primitive type?
			// Otherwise renderIndex.cpp _DirtyRprimIdsFilterPredicate will fail from filterParam->renderTags[tagNum] == primRenderTag 
		case MHWRender::MGeometry::Primitive::kTriangles:
		default:
			return HdRenderTagTokens->geometry;
	}
}

void HdMayaRenderItemAdapter::UpdateTransform(MRenderItem& ri)
{
	MMatrix matrix;
	if (ri.getMatrix(matrix) == MStatus::kSuccess)
	{
		_transform[0] = GetGfMatrixFromMaya(matrix);
		if (GetDelegate()->GetParams().enableMotionSamples) 
		{
			MDGContextGuard guard(MAnimControl::currentTime() + 1.0);
			_transform[1] = GetGfMatrixFromMaya(matrix);
		}
		else 
		{
			_transform[1] = _transform[0];
		}
	}
}

bool HdMayaRenderItemAdapter::IsSupported() const
{
	switch (_primitive)
	{
		case MHWRender::MGeometry::Primitive::kTriangles:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);			
		case MHWRender::MGeometry::Primitive::kLines:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);			
		default:
			return false;
	}
}

void HdMayaRenderItemAdapter::UpdateTopology(MRenderItem& ri)
{
	MGeometry* geom = ri.geometry();
	VtIntArray vertexIndices;
	VtIntArray vertexCounts;	
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
			vertexIndices.resize(indexCount);
			int* indicesData = (int*)indices->map();
			for (int i = 0; i < indexCount; i++)
			{
				vertexIndices[i] = indicesData[i];
			}

			switch (_primitive)
			{
			case MHWRender::MGeometry::Primitive::kTriangles:
				vertexCounts.resize(indexCount / 3);
				for (int i = 0; i < indexCount / 3; i++) vertexCounts[i] = 3;
				break;
			case MHWRender::MGeometry::Primitive::kLines:
				vertexCounts.resize(1);
				vertexCounts[0] = vertexIndices.size();
				break;
				/*				case MHWRender::MGeometry::Primitive::kLines:
									vertexCounts.resize(indexCount);
									for (int i = 0; i < indexCount; i++) vertexCounts[i] = 1;
									break;		*/
			}
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
			switch (_primitive)
			{
			case MGeometry::Primitive::kTriangles:
				// TODO: Maybe we could use the flat shading of the display style?
				_topology.reset(new HdMeshTopology(
					(GetDelegate()->GetParams().displaySmoothMeshes || GetDisplayStyle().refineLevel > 0)
					? PxOsdOpenSubdivTokens->catmullClark
					: PxOsdOpenSubdivTokens->none,
					UsdGeomTokens->rightHanded,
					vertexCounts,
					vertexIndices));

				MarkDirty(
/*					HdChangeTracker::DirtyTransform |
					HdChangeTracker::DirtyTopology |
					HdChangeTracker::DirtyPrimvar |
					HdChangeTracker::DirtyPoints		*/			
					HdChangeTracker::Clean
					| HdChangeTracker::InitRepr
					| HdChangeTracker::DirtyExtent
					| HdChangeTracker::DirtyNormals
					| HdChangeTracker::DirtyPoints
					| HdChangeTracker::DirtyPrimID
					| HdChangeTracker::DirtyPrimvar
					| HdChangeTracker::DirtyDisplayStyle
					| HdChangeTracker::DirtyRepr
					| HdChangeTracker::DirtyMaterialId
					| HdChangeTracker::DirtyTopology
					| HdChangeTracker::DirtyTransform
					| HdChangeTracker::DirtyVisibility
					| HdChangeTracker::DirtyWidths
					| HdChangeTracker::DirtyComputationPrimvarDesc
					| HdChangeTracker::DirtyInstancer
				);
				break;
			case MGeometry::Primitive::kLines:
				// This will allow us to output geometry to the effect of GL_LINES
				_topology.reset(new HdBasisCurvesTopology(
					HdTokens->linear,
					// basis type is ignored, due to linear curve type
					{},
					HdTokens->segmented,
					vertexCounts,
					vertexIndices));
				MarkDirty(
					//HdChangeTracker::DirtyTransform |
					//HdChangeTracker::DirtyTopology |
					//HdChangeTracker::DirtyPrimvar |
					//HdChangeTracker::DirtyPoints
					HdChangeTracker::Clean
					| HdChangeTracker::InitRepr
					| HdChangeTracker::DirtyExtent
					| HdChangeTracker::DirtyNormals
					| HdChangeTracker::DirtyPoints
					| HdChangeTracker::DirtyPrimID
					| HdChangeTracker::DirtyPrimvar
					| HdChangeTracker::DirtyDisplayStyle
					| HdChangeTracker::DirtyRepr
					| HdChangeTracker::DirtyMaterialId
					| HdChangeTracker::DirtyTopology
					| HdChangeTracker::DirtyTransform
					| HdChangeTracker::DirtyVisibility
					| HdChangeTracker::DirtyWidths
					| HdChangeTracker::DirtyComputationPrimvarDesc
					| HdChangeTracker::DirtyInstancer
				);
				break;

			}
		}
	}
}

std::shared_ptr<HdTopology> HdMayaRenderItemAdapter::GetTopology()
{
	return _topology;
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
    if (dirtyBits != 0) 
	{
		GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
    }
}

void HdMayaRenderItemAdapter::Populate()
{
	if (_isPopulated) 
	{
		return;
	}
	
	switch (_primitive)
	{
		case MHWRender::MGeometry::Primitive::kTriangles:
			GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), {}/* TODO : GetInstancerID() */);
			break;
		case MHWRender::MGeometry::Primitive::kLines:
			GetDelegate()->InsertRprim(HdPrimTypeTokens->basisCurves, GetID(), {}/* TODO : GetInstancerID() */);
			break;
	}
	_isPopulated = true;
}

void HdMayaRenderItemAdapter::RemovePrim()
{
    if (!_isPopulated) 
	{
        return;
    }
    GetDelegate()->RemoveRprim(GetID());

    _isPopulated = false;
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
