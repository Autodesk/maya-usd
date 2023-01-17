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

#ifndef MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H
#define MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H

#include <mayaHydraLib/adapters/adapter.h>
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/materialNetworkConverter.h>

#include <mayaHydraLib/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>

#include <pxr/imaging/hdx/renderTask.h>

#include <maya/MMatrix.h>
#include <maya/MHWGeometryUtilities.h>

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
	std::string kRenderItemTypeName = "renderItem";

	static constexpr const char* kPointSize = "pointSize";

	static const SdfPath kInvalidMaterial = SdfPath("InvalidMaterial");
}

using MayaHydraRenderItemAdapterPtr = std::shared_ptr<class MayaHydraRenderItemAdapter>;
using MayaHydraShaderAdapterPtr = std::shared_ptr<class MayaHydraShapeUIShaderAdapter>;
using MayaHydraMaterialShaderAdapterPtr = std::shared_ptr<class MayaHydraMaterialShaderAdapter>;

///////////////////////////////////////////////////////////////////////
// MayaHydraShaderInstanceData
///////////////////////////////////////////////////////////////////////

struct MayaHydraRenderItemShaderParam
{
	TfToken name = TfToken("");
	VtValue value;
	SdfValueTypeName type;	
};

struct MayaHydraShaderData
{	
	TfToken Name = TfToken("");
	TfToken ReprSelector = TfToken("");	
};

struct MayaHydraShaderInstanceData
{	
	const MayaHydraShaderData* ShapeUIShader = nullptr;
	
	SdfPath Material = kInvalidMaterial;
	
	std::map<TfToken, MayaHydraRenderItemShaderParam> Params;
};

/*
class MayaHydraRenderItemShaderConverter
{
public:
	static bool ExtractShapeUIShaderData(
		const MRenderItem& renderItem,		
		MayaHydraShaderInstanceData& shaderData);
};
*/

// TODO: Remove, currently unused.
class MayaHydraShapeUIShaderAdapter : public MayaHydraAdapter
{
public:
	MAYAHYDRALIB_API
		MayaHydraShapeUIShaderAdapter(
			MayaHydraDelegateCtx* del,
			const MayaHydraShaderData& shader
		);

	MAYAHYDRALIB_API
		virtual ~MayaHydraShapeUIShaderAdapter();

	// override
	/////////////

	MAYAHYDRALIB_API
		VtValue Get(const TfToken& key) override;

	MAYAHYDRALIB_API
		bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

	MAYAHYDRALIB_API
		virtual bool IsSupported() const override { return true; }

	MAYAHYDRALIB_API
		virtual bool GetDoubleSided() const { return true; };



	MAYAHYDRALIB_API
		virtual void MarkDirty(HdDirtyBits dirtyBits) override;

	// TODO: maybe this should not be an adapter??
	// no rprim, sprim, instead a render task!
	MAYAHYDRALIB_API
		virtual void RemovePrim() override {}
	MAYAHYDRALIB_API
		virtual void Populate() override {}

	const MayaHydraShaderData& GetShaderData() { return _shader; }

private:
	const MayaHydraShaderData& _shader;
	HdRprimCollection _rprimCollection;

};


///////////////////////////////////////////////////////////////////////
// MayaHydraRenderItemAdapter
///////////////////////////////////////////////////////////////////////

// TODO: Remove dependency on MayaHydraAdapter.
//  None of it is used apart from cast inside generic method called in the scene delegate.
class MayaHydraRenderItemAdapter : public MayaHydraAdapter
{
public:
	MAYAHYDRALIB_API
	MayaHydraRenderItemAdapter(
		const SdfPath& slowId,
		int fastId,
		MayaHydraDelegateCtx* del,
		const MRenderItem& ri
		);

	MAYAHYDRALIB_API
	virtual ~MayaHydraRenderItemAdapter();

	// override
	/////////////

	MAYAHYDRALIB_API
		virtual void RemovePrim() override {}
	MAYAHYDRALIB_API
		virtual void Populate() override {}

	MAYAHYDRALIB_API
	bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

	MAYAHYDRALIB_API
	virtual bool IsSupported() const override;

	MAYAHYDRALIB_API
	virtual bool GetDoubleSided() const { return false; };

	

	MAYAHYDRALIB_API
	virtual void MarkDirty(HdDirtyBits dirtyBits) override;
	


	MAYAHYDRALIB_API
	VtValue Get(const TfToken& key) override;

	// this
	///////////

	MAYAHYDRALIB_API
	VtValue GetMaterialResource();

	MAYAHYDRALIB_API
	void SetPlaybackChanged();

	MAYAHYDRALIB_API
	bool GetVisible() override;
	
	MAYAHYDRALIB_API
	void SetVisible(bool val) { _visible = val; }

	MAYAHYDRALIB_API
	const MColor& GetWireframeColor() const { return _wireframeColor; }
	
	MAYAHYDRALIB_API
	MHWRender::DisplayStatus GetDisplayStatus() const { return _displayStatus; }
	
	// TODO:  move transform to common base class with MayaHydraDagAdapter
	MAYAHYDRALIB_API
    GfMatrix4d GetTransform() { return _transform[0]; }
    
	MAYAHYDRALIB_API
    void InvalidateTransform() { }

	MAYAHYDRALIB_API
	bool IsInstanced() const { return false; }
	
	MAYAHYDRALIB_API
	HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation);

	MAYAHYDRALIB_API
	void UpdateTransform(MRenderItem& ri);

	MAYAHYDRALIB_API
	void UpdateTopology(MRenderItem& ri);

	//Class used to pass data to the UpdateFromDelta method, so we can extend the parameters in the future if needed.
	class UpdateFromDeltaData
    {
    public:
		UpdateFromDeltaData(MRenderItem& ri, unsigned int flags, const MColor& wireframeColor, MHWRender::DisplayStatus displayStatus) :
			_ri(ri), _flags(flags), _wireframeColor(wireframeColor), _displayStatus(displayStatus)
		{
		}

        MRenderItem&				_ri;
        unsigned int                _flags;
        const MColor&               _wireframeColor; 
        MHWRender::DisplayStatus    _displayStatus;
    };

	MAYAHYDRALIB_API
    void UpdateFromDelta(const UpdateFromDeltaData& data);

	MAYAHYDRALIB_API
	virtual std::shared_ptr<HdTopology> GetTopology();

	// TODO : Different smooth levels
	MAYAHYDRALIB_API
	HdDisplayStyle GetDisplayStyle() { return { 0, false, false }; }

	MAYAHYDRALIB_API
	virtual TfToken GetRenderTag() const;

	MAYAHYDRALIB_API
	MayaHydraShaderInstanceData& GetShaderData() { return _shaderInstance; }
	
	MAYAHYDRALIB_API
	void SetShaderData(const MayaHydraShaderInstanceData& val) { _shaderInstance = val; }

	MAYAHYDRALIB_API
	int GetFastID() const { return _fastId; }

	MAYAHYDRALIB_API
	MGeometry::Primitive GetPrimitive() const { return _primitive; }

private:
	MAYAHYDRALIB_API
	void _RemoveRprim();

	MAYAHYDRALIB_API
	void _InsertRprim();

	MayaHydraShaderInstanceData _shaderInstance;
	std::shared_ptr<HdTopology> _topology = nullptr;
	VtVec3fArray _positions = {};
	VtVec2fArray _uvs = {};
	MGeometry::Primitive _primitive;
	MString _name;
    GfMatrix4d _transform[2];  // TODO:  move transform to common base class with MayaHydraDagAdapter
	int _fastId = 0;
	bool _visible = false;
	MColor _wireframeColor			= {1.f,1.f,1.f,1.f};
	bool _isHideOnPlayback = false;
	MHWRender::DisplayStatus _displayStatus = MHWRender::DisplayStatus::kNoStatus;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H
