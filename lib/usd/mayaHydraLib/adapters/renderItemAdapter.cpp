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

#include <mayaHydraLib/delegates/sceneDelegate.h>
#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/adapters/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/base/tf/registryManager.h>
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/sdr/registry.h>

#include "pxr/imaging/hdx/renderTask.h"

#include <maya/MAnimControl.h>
#include <maya/MDGContext.h>
#include <maya/MDGContextGuard.h>
#include <maya/MHWGeometry.h>
#include <maya/MShaderManager.h>
#include <maya/MViewport2Renderer.h>

#include <functional>


/*
TODO:
---------------------------------

* Stipple lines (dotted lines)
* materials
* Depth priority


*/


PXR_NAMESPACE_OPEN_SCOPE


#define PLUG_THIS_PLUGIN \
    PlugRegistry::GetInstance().GetPluginWithName(\
        TF_PP_STRINGIZE(MFB_PACKAGE_NAME))



///////////////////////////////////////////////////////////////////////
// MayaHydraRenderItemAdapter
///////////////////////////////////////////////////////////////////////

MayaHydraRenderItemAdapter::MayaHydraRenderItemAdapter(
    const SdfPath& slowId,
	int fastId,
    MayaHydraDelegateCtx* del,
	const MRenderItem& ri
	)
    : MayaHydraAdapter(MObject(), slowId, del)
	, _primitive(ri.primitive())
	, _name(ri.name())
	, _fastId(fastId)
{
	_InsertRprim();
}

MayaHydraRenderItemAdapter::~MayaHydraRenderItemAdapter()
{
	_RemoveRprim();
}

TfToken MayaHydraRenderItemAdapter::GetRenderTag() const
{
	return HdRenderTagTokens->geometry;
}

void MayaHydraRenderItemAdapter::UpdateTransform(MRenderItem& ri)
{
	MMatrix matrix;
	if (ri.getMatrix(matrix) == MStatus::kSuccess)
	{
		_transform[0] = GetGfMatrixFromMaya(matrix);
		if (GetDelegate()->GetParams().motionSamplesEnabled())
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

bool MayaHydraRenderItemAdapter::IsSupported() const
{
	switch (_primitive)
	{
		case MHWRender::MGeometry::Primitive::kTriangles:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);			
		case MHWRender::MGeometry::Primitive::kLines:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);	
		case MHWRender::MGeometry::Primitive::kPoints:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->points);
		default:
			return false;
	}
}

void MayaHydraRenderItemAdapter::_InsertRprim()
{
	switch (GetPrimitive())
	{
	case MHWRender::MGeometry::Primitive::kTriangles:
		GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), {});
		break;
	case MHWRender::MGeometry::Primitive::kLines:
		GetDelegate()->InsertRprim(HdPrimTypeTokens->basisCurves, GetID(), {});
		break;
	case MHWRender::MGeometry::Primitive::kPoints:
		GetDelegate()->InsertRprim(HdPrimTypeTokens->points, GetID(), {});
		break;
	default:
		assert(false); // unexpected/unsupported primitive type
		break;
	}
}

void MayaHydraRenderItemAdapter::_RemoveRprim()
{
	GetDelegate()->RemoveRprim(GetID());
}



void MayaHydraRenderItemAdapter::UpdateFromDelta(const UpdateFromDeltaData& data)
{ 
    if (_primitive != MHWRender::MGeometry::Primitive::kTriangles
        && _primitive != MHWRender::MGeometry::Primitive::kLines) {
        return;
    }

	const bool positionsHaveBeenReset = (0 == _positions.size());//when positionsHaveBeenReset is true we need to recompute the geometry and topology as our data has been cleared
	using MVS = MDataServerOperation::MViewportScene;
	//const bool isNew = flags & MViewportScene::MVS_new;  //not used yet
	const bool visible			= data._flags & MVS::MVS_visible;
	const bool matrixChanged    = data._flags & MVS::MVS_changedMatrix;
    const bool geomChanged		= (data._flags & MVS::MVS_changedGeometry)	|| positionsHaveBeenReset;
    const bool topoChanged		= (data._flags & MVS::MVS_changedTopo)		|| positionsHaveBeenReset;
	const bool visibChanged		= data._flags & MVS::MVS_changedVisibility;
	const bool effectChanged	= data._flags & MVS::MVS_changedEffect;
	
	HdDirtyBits dirtyBits = 0;

	if (data._wireframeColor != _wireframeColor) {
		_wireframeColor = data._wireframeColor;
		dirtyBits |= HdChangeTracker::DirtyPrimvar; // displayColor primVar
	}
	
	_displayStatus				= data._displayStatus;
	const bool hideOnPlayback	= data._ri.isHideOnPlayback();
	if (hideOnPlayback != _isHideOnPlayback){
		_isHideOnPlayback	= hideOnPlayback;
		dirtyBits			|= HdChangeTracker::DirtyVisibility;
	}

	if (visibChanged)
	{
		SetVisible(visible);
		dirtyBits |= HdChangeTracker::DirtyVisibility;
	}

	if (effectChanged) {
		dirtyBits |= HdChangeTracker::DirtyMaterialId;
	}
    if (matrixChanged) {
        dirtyBits |= HdChangeTracker::DirtyTransform;
    }
    if (geomChanged) {
        dirtyBits |= HdChangeTracker::DirtyPoints;
    }
    if (topoChanged) {
        dirtyBits |= (HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPrimvar);
    }

    MGeometry* geom = nullptr;
    if (geomChanged | topoChanged) {
        geom = data._ri.geometry();
    }
    VtIntArray vertexIndices;
    VtIntArray vertexCounts;
    // TODO : Multiple streams
    // for now assume first is position

 	// Vertices
    MVertexBuffer* verts = nullptr;
    if (geomChanged && geom && geom->vertexBufferCount() > 0) {
        verts = geom->vertexBuffer(0);
        if (verts) {
            int vertCount = 0;
			const unsigned int originalVertexCount = verts->vertexCount();
            if ( topoChanged) {
                vertCount = originalVertexCount;
            } else {
                // Keep the previously-determined vertex count in case it was truncated.
				const size_t positionSize = _positions.size() ;
				if (positionSize > 0 && positionSize <= originalVertexCount){
					vertCount = positionSize;
				}else{
					vertCount = originalVertexCount;
				}
            }

            _positions.clear();
            //_positions.resize(vertCount);
            // map() is usually just reading from the software copy of the vp2 buffers.  It was also
            // showing up in vtune that it was sometimes mapping OpenGL buffers to read from, which
            // is slow.  Disabling processing of non-triangle render made that disappear.  Maybe
            // something like joint render items point to hardware only buffers?
            const auto* vertexPositions = reinterpret_cast<const GfVec3f*>(verts->map());
            // NOTE: Looking at MayaHydraMeshAdapter::GetPoints notice assign(vertexPositions,
            // vertexPositions + vertCount) Why are we not multiplying with sizeof(GfVec3f) to
            // calculate the offset ? The following happens when I try to do it : Invalid Hydra prim
            // - Vertex primvar points has 288 elements, while its topology references only upto
            // element index 24.
            if (TF_VERIFY(vertexPositions)) {
                _positions.assign(vertexPositions, vertexPositions + vertCount);
            }
            verts->unmap();
        }
    }

    // Indices
    MIndexBuffer* indices = nullptr;
    if (topoChanged && geom && geom->vertexBufferCount() > 0) {
        indices = geom->indexBuffer(0);
        if (indices) {
            int indexCount = indices->size();
            vertexIndices.resize(indexCount);
            int* indicesData = (int*)indices->map();
            // USD spamming the "topology references only upto element" message is super
            // slow.  Scanning the index array to look for an incompletely used vertex
            // buffer is innefficient, but it's better than the spammy warning. Cause of
            // the incompletely used vertex buffer is unclear.  Maya scene data just is
            // that way sometimes.
            int maxIndex = 0;
            for (int i = 0; i < indexCount; i++) {
                if (indicesData[i] > maxIndex) {
                    maxIndex = indicesData[i];
                }
            }

            // VtArray operator[] is oddly expensive, ~10ms per frame here. Replace with assign().
            // for (int i = 0; i < indexCount; i++) vertexIndices[i] = indicesData[i];
            vertexIndices.assign(indicesData, indicesData + indexCount);

            if (maxIndex < (int64_t)_positions.size() - 1) {
                _positions.resize(maxIndex + 1);
            }

            switch (_primitive) {
            case MHWRender::MGeometry::Primitive::kTriangles:
                vertexCounts.resize(indexCount / 3);
                // for (int i = 0; i < indexCount / 3; i++) vertexCounts[i] = 3;
                vertexCounts.assign(indexCount / 3, 3);


				// UVs
				if(indexCount > 0)
				{
					MVertexBuffer* mvb = nullptr;
					for (int vbIdx = 0; vbIdx < geom->vertexBufferCount(); vbIdx++)
					{
						mvb = geom->vertexBuffer(vbIdx);
						if (!mvb) continue;

						const MVertexBufferDescriptor& desc = mvb->descriptor();

						if (desc.semantic() != MGeometry::Semantic::kTexture) continue;

						// Hydra expects a uv coordinate for each face-index (total of 36), not 1 per vertex.
						// not for every vertex. e.g. a cube expects 36 uvs not 24.
						// Note that ASSERT(mvb->vertexCount() == 24)
						// See HdStMesh::_PopulateFaceVaryingPrimvars
						_uvs.clear();
						_uvs.resize(indices->size());
						float* uvs = (float*)mvb->map();
						for (int i = 0; i < indexCount; i++)
						{
							_uvs[i].Set(&uvs[indicesData[i]*2]);
						}
						mvb->unmap();
						break;
					}
				}
                break;

            case MHWRender::MGeometry::Primitive::kLines:
                vertexCounts.resize(indexCount);
                // for (int i = 0; i < indexCount; i++) vertexCounts[i] = 1;
                vertexCounts.assign(indexCount / 2, 2);
                break;

            default:
                assert(false); // unexpected/unsupported primitive type
                break;
            }
            indices->unmap();
        }
    }

    if (indices && verts) {
        if (topoChanged) {
            if (_primitive == MHWRender::MGeometry::Primitive::kTriangles) {

                // TODO: Maybe we could use the flat shading of the display style?
                _topology.reset(new HdMeshTopology(
                    (GetDelegate()->GetParams().displaySmoothMeshes
                     || GetDisplayStyle().refineLevel > 0)
                        ? PxOsdOpenSubdivTokens->catmullClark
                        : PxOsdOpenSubdivTokens->none,
                    UsdGeomTokens->rightHanded,
                    vertexCounts,
                    vertexIndices));
            } else if (_primitive == MHWRender::MGeometry::Primitive::kLines) {
                _topology.reset(new HdBasisCurvesTopology(
                    HdTokens->linear,
                    // basis type is ignored, due to linear curve type
                    {},
                    HdTokens->segmented,
                    vertexCounts,
                    vertexIndices));
            }
        }
    }

    MarkDirty(dirtyBits);
}

std::shared_ptr<HdTopology> MayaHydraRenderItemAdapter::GetTopology()
{
	return _topology;
}

VtValue MayaHydraRenderItemAdapter::Get(const TfToken& key)
{
	if (key == HdTokens->points) 
	{
		return VtValue(_positions);
	}
	if (key == MayaHydraAdapterTokens->st)
	{
		return VtValue(_uvs);
	}
	if (key == HdTokens->displayColor)
	{
		return VtValue(GfVec4f(_wireframeColor[0],_wireframeColor[1],_wireframeColor[2],_wireframeColor[3]));
	}

	return {};
}

void MayaHydraRenderItemAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
	if (dirtyBits != 0)
	{
		GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
	}
}

HdPrimvarDescriptorVector MayaHydraRenderItemAdapter::GetPrimvarDescriptors(HdInterpolation interpolation)
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
	else if (interpolation == HdInterpolationFaceVarying) 
	{
		 // UVs are face varying in maya.
		if (_primitive == MGeometry::Primitive::kTriangles)
		// TODO: Check that we indeed have UVs on the given mesh
		{
			HdPrimvarDescriptor desc;
			desc.name = MayaHydraAdapterTokens->st;
			desc.interpolation = interpolation;
			desc.role = HdPrimvarRoleTokens->textureCoordinate;
			return { desc };
		}
	}
	else if (interpolation == HdInterpolationConstant) {
		HdPrimvarDescriptor desc;
		desc.name = HdTokens->displayColor;
		desc.interpolation = interpolation;
		desc.role = HdPrimvarRoleTokens->color;
		return { desc };
    }

	return {};
}

VtValue MayaHydraRenderItemAdapter::GetMaterialResource()
{
	return {};
}

bool MayaHydraRenderItemAdapter::GetVisible()
{ 
	//Assuming that, if the playback is in the active view only (MAnimControl::kPlaybackViewActive), we are called because we are in the active view
	if (_isHideOnPlayback){
		// MAYA-127216: Remove dependency on parent class MayaHydraAdapter. This will let us use MayaHydraSceneDelegate directly
		auto sceneDelegate = static_cast<MayaHydraSceneDelegate*>(GetDelegate());
		return !sceneDelegate->GetPlaybackRunning();
	}
	
	return _visible; 
}

void MayaHydraRenderItemAdapter::SetPlaybackChanged()
{
	//There was a change in the playblack state, it started or stopped running so update any primitive that is dependent on this
	if (_isHideOnPlayback){
		MarkDirty(HdChangeTracker::DirtyVisibility);
	}
}

///////////////////////////////////////////////////////////////////////
// TF_REGISTRY
///////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
	TfType::Define<MayaHydraRenderItemAdapter, TfType::Bases<MayaHydraAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraAdapterRegistry, renderItem)
{	
}


PXR_NAMESPACE_CLOSE_SCOPE