//-
// Copyright 2021 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+

//#include <maya/M3dView.h>
//#include <maya/MConditionMessage.h>
//#include <maya/MDrawContext.h>
//#include <maya/MEventMessage.h>
//#include <maya/MGlobal.h>
//#include <maya/MNodeMessage.h>
//#include <maya/MSceneMessage.h>
//#include <maya/MSelectionList.h>
//#include <maya/MTimerMessage.h>
//#include <maya/MUiMessage.h>

#include "pluginDebugCodes.h"
#include "renderOverride2.h"

#include "renderOverrideUtils.h"
#include "tokens.h"
#include "utils.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/tokens.h>


#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/delegates/sceneDelegate2.h>
#include <hdMaya/utils.h>
#include <mayaUsd/render/px_vp20/utils.h>


PXR_NAMESPACE_OPEN_SCOPE

namespace {

	// Not sure if we actually need a mutex guarding _allInstances, but
	// everywhere that uses it isn't a "frequent" operation, so the
	// extra speed loss should be fine, and I'd rather be safe.
	std::mutex                       _allInstancesMutex;
	std::vector<MtohRenderOverride2*> _allInstances;
}

// For override creation we return a UI name so that it shows up in as a
// renderer in the 3d viewport menus.
// 
MtohRenderOverride2::MtohRenderOverride2(
	const MtohRendererDescription& desc)
	: MtohRenderOverride(desc)
	, _rendererToken(desc.rendererName.GetString() + "2")
	, _currentOperation(-1)
{
}

// On destruction all operations are deleted.
//
MtohRenderOverride2::~MtohRenderOverride2()
{
}

MStatus MtohRenderOverride2::Render(const MHWRender::MDrawContext& drawContext)
{
	return MStatus::kSuccess;
}

MtohRenderOverride2* MtohRenderOverride2::_GetByName2(TfToken rendererName)
{
	std::lock_guard<std::mutex> lock(_allInstancesMutex);
	for (auto* instance : _allInstances) {
		if (instance->_rendererDesc.rendererName == rendererName) {
			return instance;
		}
	}
	return nullptr;
}


SdfPath MtohRenderOverride2::RendererSceneDelegateId(TfToken rendererName, TfToken sceneDelegateName)
{
	MtohRenderOverride2* instance = _GetByName2(rendererName);
	if (!instance) {
		return SdfPath();
	}

	for (auto& delegate : instance->_delegates) {
		if (delegate->GetName() == sceneDelegateName) {
			return delegate->GetMayaDelegateID();
		}
	}
	return SdfPath();
}

void MtohRenderOverride2::_InitHydraResources()
{
	TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
		.Msg("MtohRenderOverride::_InitHydraResources(%s)\n", _rendererToken.GetText());
#if USD_VERSION_NUM < 2102
	GlfGlewInit();
#endif
	GlfContextCaps::InitInstance();
	_rendererPlugin
		= HdRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererToken);
	auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
#if USD_VERSION_NUM > 2002
	_renderIndex = HdRenderIndex::New(renderDelegate, { &_hgiDriver });
#else
	_renderIndex = HdRenderIndex::New(renderDelegate);
#endif

	_taskController = new HdxTaskController(
		_renderIndex,
		_ID.AppendChild(TfToken(TfStringPrintf(
			"_UsdImaging_%s_%p",
			TfMakeValidIdentifier(_rendererToken.GetText()),
			this))));
	_taskController->SetEnableShadows(true);

	HdMayaDelegate::InitData delegateInitData(
		TfToken(),
		_engine,
		_renderIndex,
		_rendererPlugin,
		_taskController,
		SdfPath(),
		_isUsingHdSt);

	auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
	auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
	TF_VERIFY(delegateNames.size() == creators.size());
	for (size_t i = 0, n = creators.size(); i < n; ++i) {
		const auto& creator = creators[i];
		if (creator == nullptr) {
			continue;
		}
		delegateInitData.name = delegateNames[i];
		delegateInitData.delegateID = _ID.AppendChild(
			TfToken(TfStringPrintf("_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
		auto newDelegate = creator(delegateInitData);
		if (newDelegate) {
			// Call SetLightsEnabled before the delegate is populated
			newDelegate->SetLightsEnabled(!_hasDefaultLighting);
			_delegates.emplace_back(std::move(newDelegate));
		}
	}
	if (_hasDefaultLighting) {
		delegateInitData.delegateID
			= _ID.AppendChild(TfToken(TfStringPrintf("_DefaultLightDelegate_%p", this)));
		_defaultLightDelegate.reset(new MtohDefaultLightDelegate(delegateInitData));
	}
	VtValue selectionTrackerValue(_selectionTracker);
	_engine.SetTaskContextData(HdxTokens->selectionState, selectionTrackerValue);
	for (auto& it : _delegates) {
		it->Populate();
	}
	if (_defaultLightDelegate) {
		_defaultLightDelegate->Populate();
	}

	_renderIndex->GetChangeTracker().AddCollection(_selectionCollection.GetName());
	_SelectionChanged();

	_initializedViewport = true;
	if (auto* renderDelegate = _GetRenderDelegate()) {
		// Pull in any options that may have changed due file-open.
		// If the currentScene has defaultRenderGlobals we'll absorb those new settings,
		// but if not, fallback to user-defaults (current state) .
		const bool filterRenderer = true;
		const bool fallbackToUserDefaults = true;
		_globals.GlobalChanged(
			{ _rendererDesc.rendererName, filterRenderer, fallbackToUserDefaults });
		_globals.ApplySettings(renderDelegate, _rendererDesc.rendererName);
	}
}

// Drawing uses all internal code so will support all draw APIs
//
MHWRender::DrawAPI MtohRenderOverride2::supportedDrawAPIs() const
{
	return MHWRender::kAllDevices;
}

// Basic iterator methods which returns a list of operations in order
// The operations are not executed at this time only queued for execution
//
// - startOperationIterator() : to start iterating
// - renderOperation() : will be called to return the current operation
// - nextRenderOperation() : when this returns false we've returned all operations
//
bool MtohRenderOverride2::startOperationIterator()
{
	_currentOperation = 0;
	return true;
}

MHWRender::MRenderOperation*
MtohRenderOverride2::renderOperation()
{
	if (_currentOperation >= 0 && _currentOperation < _operationCount)
	{
		if (_operations[_currentOperation])
		{
			return _operations[_currentOperation].get();
		}
	}
	return nullptr;
}

bool
MtohRenderOverride2::nextRenderOperation()
{
	_currentOperation++;
	if (_currentOperation < _operationCount)
	{
		return true;
	}
	return false;
}

//
// On setup we make sure that we have created the appropriate operations
// These will be returned via the iteration code above.
//
// The only thing that is required here is to create:
//
//	- One scene render operation to draw the scene.
//	- One "stock" HUD render operation to draw the HUD over the scene
//	- One "stock" presentation operation to be able to see the results in the viewport
//
MStatus MtohRenderOverride2::setup(const MString & destination)
{
	ParentClass::setup(destination);

	MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
	if (!theRenderer)
		return MStatus::kFailure;

	// Create a new set of operations as required
	if (!_operations[0])
	{
		MHWRender::MClearOperation* clearOp = new MHWRender::MClearOperation("viewportDataServer_Clear");
		_operations[0].reset(clearOp);
		float c1[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float c2[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		clearOp->setClearColor(c1);
		clearOp->setClearColor2(c2);
		clearOp->setClearGradient(true);

		//_operations[0].reset(new viewportDataServerSceneRender("viewportDataServer_Scene"));
		_operations[1].reset(new viewportDataServerUserOp("viewportDataServer_User"));
		_operations[2].reset(new MHWRender::MHUDRender());
		_operations[3].reset(new MHWRender::MPresentTarget("viewportDataServer_Present"));
	}

	for (int i = 0; i < _operationCount; ++i) {
		if (!_operations[i]) {
			return MStatus::kFailure;
		}
	}

	return MStatus::kSuccess;
}

// On cleanup we just return for returning the list of operations for
// the next render
//
MStatus MtohRenderOverride2::cleanup()
{
	ParentClass::cleanup();

	_currentOperation = -1;
	return MStatus::kSuccess;
}

/*
// The only customization for the scene render (and hence derivation)
// is to be able to set the background color.
//
viewportDataServerSceneRender::viewportDataServerSceneRender(const MString &name)
: MSceneRender( name, MHWRender::MSceneRender::DataServerTag() )
{
}

// Background color override. We get the current colors from the
// renderer and use them
//
MHWRender::MClearOperation &
viewportDataServerSceneRender::clearOperation()
{
	//MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
	//bool gradient = renderer->useGradient();
	//MColor color1 = renderer->clearColor();
	//MColor color2 = renderer->clearColor2();
	//float c1[4] = { color1[0], color1[1], color1[2], 1.0f };
	//float c2[4] = { color2[0], color2[1], color2[2], 1.0f };

	float c1[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float c2[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	mClearOperation.setClearColor( c1 );
	mClearOperation.setClearColor2( c2 );
	mClearOperation.setClearGradient( true);
	return mClearOperation;
}
*/

viewportDataServerUserOp::viewportDataServerUserOp(const MString &name)
	: MUserRenderOperation(name, MHWRender::MUserRenderOperation::DataServerTag())
{
}

MStatus viewportDataServerUserOp::execute2(const MDrawContext & drawContext, const MHWRender::MViewportScene& scene)
{
	// Example to access render item geometry from MRenderItem
	int vertCount = 0;
	for (int i = 0; i < scene.mCount; ++i) {
		MRenderItem* item = scene.mItems[i];

		const MGeometry* geom = item->geometry();
		if (geom && geom->vertexBufferCount() > 0) {
			if (const MVertexBuffer* verts = geom->vertexBuffer(0)) {
				vertCount += verts->vertexCount();

				// float* vertexData = (float*)verts->map();
				// verts->unmap();
			}
		}
	}

	printf("ViewportDataServer: %zu render items, %d verts\n", scene.mCount, vertCount);
	return MStatus::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
