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

#include <functional>


/*
TODO:
---------------------------------

* Stipple lines (dotted lines)
* materials
* Depth priority


*/



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

#define PLUG_THIS_PLUGIN \
    PlugRegistry::GetInstance().GetPluginWithName(\
        TF_PP_STRINGIZE(MFB_PACKAGE_NAME))

static const TfToken sDefaultMaterial = UsdImagingTokens->UsdPreviewSurface;

// Maya to hydra shader parameter conversion
// See void HdMayaMaterialNetworkConverter::initialize()
static const std::map<std::string, TfToken> sHdMayaParamNameMap
{
	{"solidColor", TfToken("diffuseColor") },
	{"lambert_1color", TfToken("diffuseColor") },
	{"phong_1color", TfToken("diffuseColor") },
	{"blinn_1color", TfToken("diffuseColor") }
};

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
	_shaderTokens,
	(mayaLambertSurface)
	(mayaStippleShader)
	(mayaSolidColorShader)
	(mayaInvalidShader)
);
// clang-format on

static const std::map<TfToken, HdMayaShaderData> sHdMayaSupportedShaders
{
	{ 
		_shaderTokens->mayaStippleShader,
		{
			_shaderTokens->mayaStippleShader,
			HdReprTokens->refined			
		}
	}
};

///////////////////////////////////////////////////////////////////////
// HdMayaShaderAdapter
///////////////////////////////////////////////////////////////////////

// Extracts shape UI shader data
bool HdMayaRenderItemShaderConverter::ExtractShapeUIShaderData(
	const MRenderItem& renderItem,
	HdMayaShaderInstanceData& shaderData)
{
	MString shaderName;
	auto& end = sHdMayaSupportedShaders.end();
	auto entry = end;
	if (
		renderItem.getShaderName(shaderName) != MS::kSuccess ||
		(entry = sHdMayaSupportedShaders.find(TfToken(shaderName.asChar()))) == end
		)
	{
		return false;
	}

	shaderData.ShapeUIShader = &entry->second;

	MStringArray params;
	// TODO: Fix leaky api here :(
	const MShaderInstance* shaderInstance = renderItem.getShader();
	shaderInstance->parameterList(params);		
	for (unsigned int i = 0; i < params.length(); i++) 
	{
		HdMayaRenderItemShaderParam param;
		std::string mayaParamName(params[i].asChar());		
		switch (shaderInstance->parameterType(params[i])) 
		{
			case MShaderInstance::ParameterType::kBoolean:
			{
				bool value;
				shaderInstance->getParameter(params[i], value);
				param.value = VtValue(value);
				param.type = SdfValueTypeNames->Bool;
				shaderData.Params.insert({ param.name, param });
			}
			break;
			case MShaderInstance::ParameterType::kFloat:
			{
				float value;
				shaderInstance->getParameter(params[i], value);
				param.value = VtValue(value);
				param.type = SdfValueTypeNames->Float;
				shaderData.Params.insert({ param.name, param });				
			}
			break;
			case MShaderInstance::ParameterType::kFloat3:
			{
				MFloatVector value;
				shaderInstance->getParameter(params[i], value);
				param.value = VtValue(GfVec3f(value[0], value[1], value[2]));
				param.type = SdfValueTypeNames->Float3;
				shaderData.Params.insert({ param.name, param });
			}
			break;
			case MShaderInstance::ParameterType::kFloat4:
			{
				MFloatVector value;
				shaderInstance->getParameter(params[i], value);
				param.value = VtValue(GfVec4f(value[0], value[1], value[2], value[3]));
				param.type = SdfValueTypeNames->Float4;
				shaderData.Params.insert({ param.name, param });
			}
			break;			
		}
	}

	return true;
}

HdMayaShapeUIShaderAdapter::HdMayaShapeUIShaderAdapter(	
	HdMayaDelegateCtx* del,
	const HdMayaShaderData& shader
	)
	: HdMayaAdapter(MObject(), SdfPath(shader.Name), del)
	, _shader(shader)
	, _rprimCollection(shader.Name, HdReprSelector(shader.ReprSelector))
{
	_isPopulated = true;
	GetDelegate()->GetRenderIndex().InsertTask<HdxRenderTask>(GetDelegate(), GetID());
}

HdMayaShapeUIShaderAdapter::~HdMayaShapeUIShaderAdapter()
{
}

void HdMayaShapeUIShaderAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
	GetDelegate()->GetRenderIndex().GetChangeTracker().MarkTaskDirty(GetID(), dirtyBits);
}

VtValue HdMayaShapeUIShaderAdapter::Get(const TfToken& key)
{
	if (key == HdTokens->collection)
	{
		return VtValue(_rprimCollection);
	}
	// TODO: Customize shader uniforms per render item..
	//else if (key == HdTokens->params)
	//	/*
	//		Rendering state management can be handled two ways: 
	//		1.) An application can create an HdxRenderTask and pass it the HdxRenderTaskParams struct as "params". 
	//		2.) An application can create an HdxRenderSetupTask and an HdxRenderTask, and pass params to the setup task. In this case the setup task must run first.	
	//		
	//		Parameter unpacking is handled by HdxRenderSetupTask; in case #1, HdxRenderTask creates a dummy setup task internally to manage the sync process.
	//		Case #2 introduces complexity; the benefit is that by changing which setup task you run before the render task, you can change the render parameters 
	//		without incurring a hydra sync or rebuilding any resources. 
	//		https://graphics.pixar.com/usd/docs/api/class_hdx_render_task.html
	//	*/
	//{
	//	HdxRenderTaskParams params;
	//	params.pointColor = GfVec4f(1, 0, 0, 1);
	//	params.overrideColor = GfVec4f(0, 1, 0, 1);
	//	params.wireframeColor = GfVec4f(0, 0, 1, 1);
	//	return VtValue(params);
	//}

	return {};
}


///////////////////////////////////////////////////////////////////////
// HdMayaRenderItemAdapter
///////////////////////////////////////////////////////////////////////

HdMayaRenderItemAdapter::HdMayaRenderItemAdapter(
    const SdfPath& id,
    HdMayaDelegateCtx* del,
	const MRenderItem& ri,
	const HdMayaShaderInstanceData& sd
	)
    : HdMayaAdapter(MObject(), id, del)
	, _shaderInstance(sd)
	, _primitive(ri.primitive())
	, _name(ri.name())
{
	_isPopulated = true;
	switch (_primitive)
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
	}	
}

HdMayaRenderItemAdapter::~HdMayaRenderItemAdapter()
{
	GetDelegate()->RemoveRprim(GetID());	
}

TfToken HdMayaRenderItemAdapter::GetRenderTag() const
{
	return _shaderInstance.ShapeUIShader ?
		// Opt-in to the render pass which corresponds to this shader
		_shaderInstance.ShapeUIShader->Name :
		// Otherwise opt-in the default beauty pass
		HdRenderTagTokens->geometry;
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
		case MHWRender::MGeometry::Primitive::kPoints:
			return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->points);
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
		MVertexBuffer* mayaVertexBuffer = nullptr;		
		if (mayaVertexBuffer = geom->vertexBuffer(0))
		{
			int mayaVertexCount = mayaVertexBuffer->vertexCount();
			_vertexPositions.clear();
			_vertexPositions.resize(mayaVertexCount);
			const auto* vertexPositions = reinterpret_cast<const GfVec3f*>(mayaVertexBuffer->map());
			// NOTE: Looking at HdMayaMeshAdapter::GetPoints notice assign(vertexPositions, vertexPositions + vertCount)
			// Why are we not multiplying with sizeof(GfVec3f) to calculate the offset ? 
			// The following happens when I try to do it :
			// Invalid Hydra prim - Vertex primvar points has 288 elements, while its topology references only upto element index 24.
			_vertexPositions.assign(vertexPositions, vertexPositions + mayaVertexCount);
			mayaVertexBuffer->unmap();
		}
		// Uvs
		if (geom->vertexBufferCount() > 0)
		{
			MVertexBuffer* mayaUvsBuffer = geom->vertexBuffer(1);
			if (mayaUvsBuffer)
			{
				auto sem = mayaUvsBuffer->descriptor().semantic();
				if (sem == MGeometry::kTexture)
				{
					int uvCount = mayaUvsBuffer->vertexCount();
					_uvs.clear();
					_uvs.resize(uvCount);
					const auto* uvs = reinterpret_cast<const GfVec2f*>(mayaUvsBuffer->map());
					_uvs.assign(uvs, uvs + uvCount);
					mayaUvsBuffer->unmap();
				}
			}
		}
		// Indices
		MIndexBuffer* mayaIndexBuffer = nullptr;
		if (mayaIndexBuffer = geom->indexBuffer(0))
		{
			int mayaIndexCount = mayaIndexBuffer->size();
			vertexIndices.resize(mayaIndexCount);
			int* indicesData = (int*)mayaIndexBuffer->map();
			for (int i = 0; i < mayaIndexCount; i++)
			{
				vertexIndices[i] = indicesData[i];
			}

			switch (_primitive)
			{
			case MHWRender::MGeometry::Primitive::kTriangles:
				vertexCounts.resize(mayaIndexCount / 3);
				for (int i = 0; i < mayaIndexCount / 3; i++) vertexCounts[i] = 3;
				break;
			case MHWRender::MGeometry::Primitive::kPoints:
			case MHWRender::MGeometry::Primitive::kLines:
				vertexCounts.resize(1);
				vertexCounts[0] = vertexIndices.size();
				break;
			}
			mayaIndexBuffer->unmap();
		}

		if (mayaIndexBuffer && mayaVertexBuffer)
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
				MarkDirty(HdChangeTracker::AllDirty);
				break;
			case MGeometry::Primitive::kLines:
			case MHWRender::MGeometry::Primitive::kPoints:
				// This will allow us to output geometry to the effect of GL_LINES
				_topology.reset(new HdBasisCurvesTopology(
					HdTokens->linear,
					// basis type is ignored, due to linear curve type
					{},
					HdTokens->segmented,
					vertexCounts,
					vertexIndices));
				MarkDirty(HdChangeTracker::AllDirty);
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
	if (key == HdMayaAdapterTokens->st)
	{
		return VtValue(_uvs);
	}

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

VtValue HdMayaRenderItemAdapter::GetMaterialResource()
{	
	if (_shaderInstance.ShapeUIShader)
	{

		HdMaterialNetworkMap map;
		HdMaterialNetwork    network;
		// Describes a material node which is made of a path, an identifier and a list of parameters. More...
		// This corresponds to a material instance
		HdMaterialNode       node;
		node.path = GetID();
		node.identifier = _shaderInstance.ShapeUIShader->Name;
		map.terminals.push_back(node.path);

		for (const auto& it : HdMayaMaterialNetworkConverter::GetShaderParams(_shaderInstance.ShapeUIShader->Name))
		{
			auto& param = _shaderInstance.Params.find(it.name);
			node.parameters.emplace(it.name, param == _shaderInstance.Params.end() ?
				it.fallbackValue :
				param->second.value);
		}

		network.nodes.push_back(node);
		if (_shaderInstance.ShapeUIShader->Name == UsdImagingTokens->UsdPreviewSurface)
		{
			map.map.emplace(HdMaterialTerminalTokens->surface, network);
		}

		return VtValue(map);
	}

	return {};
};

///////////////////////////////////////////////////////////////////////
// TF_REGISTRY
///////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
	TfType::Define<HdMayaRenderItemAdapter, TfType::Bases<HdMayaAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, renderItem)
{	
}


PXR_NAMESPACE_CLOSE_SCOPE
