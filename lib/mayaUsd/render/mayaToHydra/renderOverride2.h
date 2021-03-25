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
#ifndef MTOH_VIEW_OVERRIDE_2_H
#define MTOH_VIEW_OVERRIDE_2_H

#include <pxr/pxr.h>
#if USD_VERSION_NUM < 2102
#include <pxr/imaging/glf/glew.h>
#endif

#include <pxr/base/tf/singleton.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MMessage.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

#if USD_VERSION_NUM > 2002
#include <pxr/imaging/hd/driver.h>
#endif
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hdSt/renderDelegate.h>
#include <pxr/imaging/hdx/taskController.h>

#if WANT_UFE_BUILD
#include <ufe/observer.h>
#endif // WANT_UFE_BUILD

#include "renderOverride.h"
#include "defaultLightDelegate.h"
#include "renderGlobals.h"
#include "utils.h"

#include <hdMaya/delegates/delegate.h>
#include <hdMaya/delegates/params.h>

PXR_NAMESPACE_OPEN_SCOPE

class viewportDataServerUserOp : public MHWRender::MUserRenderOperation
{
public:
	viewportDataServerUserOp(const MString &name);

	MStatus execute(const MDrawContext & drawContext) override { return MStatus::kSuccess; }

	MStatus execute2(const MDrawContext & drawContext, const MHWRender::MViewportScene& scene) override;

};

class MtohRenderOverride2 : public MtohRenderOverride
{
	typedef MtohRenderOverride ParentClass;
public:
	MtohRenderOverride2(
		const MtohRendererDescription& desc);
	~MtohRenderOverride2() override;

	// MtoH integration
	///////////////////

	MStatus Render(const MHWRender::MDrawContext& drawContext) override;

	void _InitHydraResources() override;

	static SdfPath RendererSceneDelegateId(TfToken rendererName, TfToken sceneDelegateName);
	
	static MtohRenderOverride2* _GetByName2(TfToken rendererName);

	// Viewport data server
	///////////////////////

	MHWRender::DrawAPI supportedDrawAPIs() const override;

	// Basic setup and cleanup
	MStatus setup(const MString & destination) override;
	MStatus cleanup() override;

	// Operation iteration methods
	bool startOperationIterator() override;
	MHWRender::MRenderOperation * renderOperation() override;
	bool nextRenderOperation() override;

	// UI name
	MString uiName() const override
	{
		return MString(_rendererToken.GetText());
	}

protected:
	// UI name 
	//MString mUIName;

	// Operations and operation names
	static constexpr int _operationCount = 4;
	std::unique_ptr<MHWRender::MRenderOperation> _operations[_operationCount];
	TfToken _rendererToken;

	// Temporary of operation iteration
	int _currentOperation;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_VIEW_OVERRIDE_H
