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

#ifndef HDMAYA_RENDER_ITEM_ADAPTER_H
#define HDMAYA_RENDER_ITEM_ADAPTER_H

#include <hdMaya/adapters/adapter.h>
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/materialNetworkConverter.h>

#include <hdMaya/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>

#include <pxr/imaging/hdx/renderTask.h>

#include <maya/MMatrix.h>

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
	std::string kRenderItemTypeName = "renderItem";

	static constexpr const char* kPointSize = "pointSize";

	static const SdfPath kInvalidMaterial = SdfPath("");
}

using HdMayaRenderItemAdapterPtr = std::shared_ptr<class HdMayaRenderItemAdapter>;
using HdMayaShaderAdapterPtr = std::shared_ptr<class HdMayaShapeUIShaderAdapter>;
using HdMayaMaterialShaderAdapterPtr = std::shared_ptr<class HdMayaMaterialShaderAdapter>;

///////////////////////////////////////////////////////////////////////
// HdMayaShaderInstanceData
///////////////////////////////////////////////////////////////////////

struct HdMayaRenderItemShaderParam
{
	TfToken name = TfToken("");
	VtValue value;
	SdfValueTypeName type;	
};

struct HdMayaShaderData
{	
	TfToken Name = TfToken("");
	TfToken ReprSelector = TfToken("");	
};

struct HdMayaShaderInstanceData
{	
	const HdMayaShaderData* ShapeUIShader = nullptr;
	
	SdfPath Material = kInvalidMaterial;
	
	std::map<TfToken, HdMayaRenderItemShaderParam> Params;
};

class HdMayaRenderItemShaderConverter
{
public:
	static bool ExtractShapeUIShaderData(
		const MRenderItem& renderItem,		
		HdMayaShaderInstanceData& shaderData);
};

class HdMayaShapeUIShaderAdapter : public HdMayaAdapter
{
public:
	HDMAYA_API
		HdMayaShapeUIShaderAdapter(
			HdMayaDelegateCtx* del,
			const HdMayaShaderData& shader
		);

	HDMAYA_API
		virtual ~HdMayaShapeUIShaderAdapter();

	// override
	/////////////

	HDMAYA_API
		VtValue Get(const TfToken& key) override;

	HDMAYA_API
		bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

	HDMAYA_API
		virtual bool IsSupported() const override { return true; }

	HDMAYA_API
		virtual bool GetDoubleSided() const { return true; };

	

	HDMAYA_API
		virtual void MarkDirty(HdDirtyBits dirtyBits) override;

	// TODO: maybe this should not be an adapter??
	// no rprim, sprim, instead a render task!
	HDMAYA_API
		virtual void RemovePrim() override {}
	HDMAYA_API
		virtual void Populate() override {}

	const HdMayaShaderData& GetShaderData() { return _shader; }

private:
	const HdMayaShaderData& _shader;
	HdRprimCollection _rprimCollection;

};


///////////////////////////////////////////////////////////////////////
// HdMayaRenderItemAdapter
///////////////////////////////////////////////////////////////////////

class HdMayaRenderItemAdapter : public HdMayaAdapter
{
public:
	HDMAYA_API
	HdMayaRenderItemAdapter(
		const SdfPath& id,
		HdMayaDelegateCtx* del,
		const MRenderItem& ri,
		const HdMayaShaderInstanceData& sd
		);

	HDMAYA_API
	virtual ~HdMayaRenderItemAdapter();

	// override
	/////////////

	HDMAYA_API
		virtual void RemovePrim() override {}
	HDMAYA_API
		virtual void Populate() override {}

	HDMAYA_API
	bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

	HDMAYA_API
	virtual bool IsSupported() const override;

	HDMAYA_API
	virtual bool GetDoubleSided() const { return true; };

	

	HDMAYA_API
	virtual void MarkDirty(HdDirtyBits dirtyBits) override;
	


	HDMAYA_API
	VtValue Get(const TfToken& key) override;

	// this
	///////////

	HDMAYA_API
	VtValue GetMaterialResource();

	HDMAYA_API
    virtual bool GetVisible() { return IsVisible(); }
    
	HDMAYA_API
    const GfMatrix4d& GetTransform();
    
	HDMAYA_API
	bool UpdateVisibility() { return true; }
	
	HDMAYA_API	
	bool IsVisible(bool checkDirty = true) { return true; }   	

	HDMAYA_API
    void InvalidateTransform() { }

	HDMAYA_API
	bool IsInstanced() const { return false; }
	
	HDMAYA_API
	HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation);

	HDMAYA_API
	void UpdateTransform(MRenderItem& ri);

	HDMAYA_API
	void UpdateTopology(MRenderItem& ri);

	HDMAYA_API
	virtual std::shared_ptr<HdTopology> GetTopology();

	// TODO : Different smooth levels
	HDMAYA_API
	HdDisplayStyle GetDisplayStyle() { return { 0, false, false }; }

	HDMAYA_API
	virtual TfToken GetRenderTag() const;

	HDMAYA_API
	virtual void IsStale(bool stale) { _isStale = stale; }
	
	HDMAYA_API
	virtual bool IsStale() const { return _isStale; }

	HDMAYA_API
	HdMayaShaderInstanceData& GetShaderData() { return _shaderInstance; }

private:
	HdMayaShaderInstanceData _shaderInstance;
	std::shared_ptr<HdTopology> _topology = nullptr;
	VtVec3fArray _vertexPositions = {};
	VtVec2fArray _uvs = {};
	MGeometry::Primitive _primitive;
	MString _name;
    GfMatrix4d _transform[2];
    bool _isVisible = true;
	bool _isStale = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_RENDER_ITEM_ADAPTER_H
