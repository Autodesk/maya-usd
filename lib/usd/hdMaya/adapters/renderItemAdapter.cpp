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

static const std::map<std::string, TfToken> sHdMayaMaterialNameMap
{
	//{ "mayaPhongSurface", HdMayaAdapterTokens->HdMayaPhongShader },
	//{ "mayaBlinnSurface", HdMayaAdapterTokens->HdMayaBlinnShader },
	//{ "mayaLambertSurface", HdMayaAdapterTokens->HdMayaLambertShader },
	{ "mayaStippleShader", HdMayaAdapterTokens->HdMayaStippleShader },
	// Default
	{ "mayaSolidColorShader",  sDefaultMaterial },
};

///////////////////////////////////////////////////////////////////////
// HdMayaShaderAdapter
///////////////////////////////////////////////////////////////////////

bool HdMayaRenderItemShaderConverter::ExtractShaderData(const MShaderInstance& shaderInstance, HdMayaShaderInstanceData& shaderData)
{
	MString shaderName;
	if (shaderInstance.internalShaderName(shaderName) == MS::kSuccess)
	{
		auto nameConv = sHdMayaMaterialNameMap.find(shaderName.asChar());
		shaderData.Identifier = nameConv == sHdMayaMaterialNameMap.end() ?
			sDefaultMaterial :
			nameConv->second;
	}

	MStringArray params;
	shaderInstance.parameterList(params);
	std::vector<std::string> myarray;
	std::vector< MShaderInstance::ParameterType> artypes;
	for (unsigned int i = 0; i < params.length(); i++) 
	{
		HdMayaRenderItemShaderParam param;
		std::string mayaParamName(params[i].asChar());
		myarray.push_back(mayaParamName);
		artypes.push_back(shaderInstance.parameterType(params[i]));
		auto paramConv = sHdMayaParamNameMap.find(mayaParamName);
		if (paramConv != sHdMayaParamNameMap.end())
		{
			param.isSupported = true;
			param.name = paramConv->second;
		}		
		else
		{
			// Maintain unsupported parameter type to control
			// aspects such as stippled lines, and point size
			param.isSupported = false;
			param.name = TfToken(mayaParamName);
		}
	
		switch (shaderInstance.parameterType(params[i])) 
		{
			case MShaderInstance::ParameterType::kBoolean:
			{
				bool value;
				shaderInstance.getParameter(params[i], value);
				param.value = VtValue(value);
				param.type = SdfValueTypeNames->Bool;
				shaderData.Params.insert({ param.name, param });
			}
			break;
			case MShaderInstance::ParameterType::kFloat:
			{
				float value;
				shaderInstance.getParameter(params[i], value);
				param.value = VtValue(value);
				param.type = SdfValueTypeNames->Float;
				shaderData.Params.insert({ param.name, param });				
			}
			break;
			case MShaderInstance::ParameterType::kFloat3:
			{
				MFloatVector value;
				shaderInstance.getParameter(params[i], value);
				param.value = VtValue(GfVec3f(value[0], value[1], value[2]));
				param.type = SdfValueTypeNames->Float3;
				shaderData.Params.insert({ param.name, param });
			}
			break;
			case MShaderInstance::ParameterType::kFloat4:
			{
				MFloatVector value;
				shaderInstance.getParameter(params[i], value);
				param.value = VtValue(GfVec4f(value[0], value[1], value[2], value[3]));
				param.type = SdfValueTypeNames->Float4;
				shaderData.Params.insert({ param.name, param });
			}
			break;			
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////
// HdMayaRenderItemAdapter
///////////////////////////////////////////////////////////////////////

static std::string
_GetShaderResourcePath(char const * resourceName = "")
{
	//static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
	//const std::string path = PlugFindPluginResource(plugin,
	//	TfStringCatPaths("shaders", resourceName));

	//TF_VERIFY(!path.empty(), "Could not find shader resource: %s\n",
	//	resourceName);

	//return path;
}

void InitializeShaders()
{
	/*NdrNodeDiscoveryResultVec result;

	static std::string shaderDefsFile = _GetShaderResourcePath(
		"shaderDefs.usda");
	if (shaderDefsFile.empty())
		return result;

	auto resolverContext = ArGetResolver().CreateDefaultContextForAsset(
		shaderDefsFile);

	const UsdStageRefPtr stage = UsdStage::Open(shaderDefsFile,
		resolverContext);

	if (!stage) {
		TF_RUNTIME_ERROR("Could not open file '%s' on a USD stage.",
			shaderDefsFile.c_str());
		return result;
	}

	ArResolverContextBinder binder(resolverContext);
	auto rootPrims = stage->GetPseudoRoot().GetChildren();
	for (const auto &shaderDef : rootPrims) {
		UsdShadeShader shader(shaderDef);
		if (!shader) {
			continue;
		}

		auto discoveryResults = UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
			shader, shaderDefsFile);

		result.insert(result.end(), discoveryResults.begin(),
			discoveryResults.end());

		if (discoveryResults.empty()) {
			TF_RUNTIME_ERROR("Found shader definition <%s> with no valid "
				"discovery results. This is likely because there are no "
				"resolvable info:sourceAsset values.",
				shaderDef.GetPath().GetText());
		}
	}

	return result;*/
}


// static initialize
void HdMayaRenderItemAdapter::Initialize()
{
}

HdMayaRenderItemAdapter::HdMayaRenderItemAdapter(
    const SdfPath& id,
    HdMayaDelegateCtx* del,
	const MRenderItem& ri
	)
    : HdMayaAdapter(MObject(), id, del)
	, _primitive(ri.primitive())
	, _name(ri.name())
{
}

HdMayaRenderItemAdapter::~HdMayaRenderItemAdapter()
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
		case MHWRender::MGeometry::Primitive::kPoints:
		default:
			return HdRenderTagTokens->geometry;
	}
}

void HdMayaRenderItemAdapter::UpdateShader(const HdMayaShaderInstanceData& shaderData)
{

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
				/*				case MHWRender::MGeometry::Primitive::kLines:
									vertexCounts.resize(indexCount);
									for (int i = 0; i < indexCount; i++) vertexCounts[i] = 1;
									break;		*/
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
			GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), {});
			break;
		case MHWRender::MGeometry::Primitive::kLines:
			GetDelegate()->InsertRprim(HdPrimTypeTokens->basisCurves, GetID(), {});
			break;
		case MHWRender::MGeometry::Primitive::kPoints:
			GetDelegate()->InsertRprim(HdPrimTypeTokens->points, GetID(), {});
			break;
	}

	GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
	_isPopulated = true;
}

void HdMayaRenderItemAdapter::RemovePrim()
{
    if (!_isPopulated) 
	{
        return;
    }
    GetDelegate()->RemoveRprim(GetID());
	GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());

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

VtValue HdMayaRenderItemAdapter::GetMaterialResource()
{	
	HdMaterialNetworkMap map;
	HdMaterialNetwork    network;
	// Describes a material node which is made of a path, an identifier and a list of parameters. More...
	// This corresponds to a material instance
	HdMaterialNode       node;
	node.path = GetID();
	node.identifier = _shader.Identifier;
	map.terminals.push_back(node.path);

	for (const auto& it : HdMayaMaterialNetworkConverter::GetShaderParams(_shader.Identifier)) 
	{
		auto& param = _shader.Params.find(it.name);		
		node.parameters.emplace(it.name, param == _shader.Params.end() ?
			it.fallbackValue :
			param->second.value);		
	}

	network.nodes.push_back(node);
	map.map.emplace(HdMaterialTerminalTokens->surface, network);
	return VtValue(map);
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
