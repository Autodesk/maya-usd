// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "proxyRenderDelegate.h"
#include "render_delegate.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/imaging/hdx/taskController.h"

#include "../../nodes/proxyShapeBase.h"
#include "../../nodes/stageData.h"
#include "../../utils/util.h"

#include <maya/MFnPluginData.h>
#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
    int _profilerCategory = MProfiler::addCategory("HdVP2RenderDelegate", "HdVP2RenderDelegate");   //!< Profiler category

} // namespace

//! \brief  Draw classification used during plugin load to register in VP2
const MString ProxyRenderDelegate::drawDbClassification(
    TfStringPrintf("drawdb/subscene/vp2RenderDelegate/%s",
        MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText()).c_str());

//! \brief  Factory method registered at plugin load
MHWRender::MPxSubSceneOverride* ProxyRenderDelegate::Creator(const MObject& obj) {
    return new ProxyRenderDelegate(obj);
}

//! \brief  Constructor
ProxyRenderDelegate::ProxyRenderDelegate(const MObject& obj) 
 : MHWRender::MPxSubSceneOverride(obj)
 , _mObject(obj) {

}

//! \brief  Destructor
ProxyRenderDelegate::~ProxyRenderDelegate() {
    delete _sceneDelegate;
    delete _taskController;
	delete _renderIndex;
	delete _renderDelegate;
}

//! \brief  This drawing routine supports all devices (DirectX and OpenGL)
MHWRender::DrawAPI ProxyRenderDelegate::supportedDrawAPIs() const {
    return MHWRender::kAllDevices;
}

//! \brief  Always requires update since changes are tracked by Hydraw change tracker and it will guarantee minimal update
bool ProxyRenderDelegate::requiresUpdate(const MSubSceneContainer& container, const MFrameContext& frameContext) const {
    return true;
}

//! \brief  Return pointer to DG proxy shape node
MayaUsdProxyShapeBase* ProxyRenderDelegate::getProxyShape() const {
    MStatus status;
    const MFnDependencyNode depNodeFn(_mObject);
    MayaUsdProxyShapeBase* usdSubSceneShape = static_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    return usdSubSceneShape;
}

//! \brief  One time initialization of this drawing routine
void ProxyRenderDelegate::_InitRenderDelegate() {
    // No need to run all the checks if we got till the end
    if (_isInitialized())
        return;
    
    MStatus status;
    MayaUsdProxyShapeBase* usdSubSceneShape = getProxyShape();
    if (!usdSubSceneShape)
        return;
    
    if (!_usdStage) {
        MPlug stagePlug(_mObject, MayaUsdProxyShapeBase::outStageDataAttr);

        MObject stageObject;
        {
            MProfilingScope subProfilingScope(_profilerCategory, MProfiler::kColorD_L1, "Evaluate Stage");
            status = stagePlug.getValue(stageObject);
        }

        if (!status)
            return;

        MFnPluginData pluginDataFn(stageObject);
        UsdMayaStageData* stageData = reinterpret_cast<UsdMayaStageData*>(pluginDataFn.data(&status));

        if (!status)
            return;

        _usdStage = stageData->stage;
    }

    if (!_renderDelegate) {
        MProfilingScope subProfilingScope(_profilerCategory, MProfiler::kColorD_L1, "Allocate VP2RenderDelegate");
		_renderDelegate = new HdVP2RenderDelegate(*this);
    }

    if (!_renderIndex) {
        MProfilingScope subProfilingScope(_profilerCategory, MProfiler::kColorD_L1, "Allocate RenderIndex");
        _renderIndex = HdRenderIndex::New(_renderDelegate);
    }

    if (!_sceneDelegate) {
        MProfilingScope subProfilingScope(_profilerCategory, MProfiler::kColorD_L1, "Allocate SceneDelegate");

        SdfPath delegateID = SdfPath::AbsoluteRootPath().AppendChild(TfToken(TfStringPrintf(
            "Proxy_%s_%p", usdSubSceneShape->name().asChar(), usdSubSceneShape)));
        
        _sceneDelegate = new UsdImagingDelegate(_renderIndex, delegateID);

        _taskController = new HdxTaskController(_renderIndex,
            delegateID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p", TfMakeValidIdentifier("VP2").c_str(),this))) );
    }
}

//! \brief  Populate render index with prims coming from scene delegate.
void ProxyRenderDelegate::_Populate() {
    if (!_isInitialized())
        return;

    if (!_isPopulated) {
        MProfilingScope subProfilingScope(_profilerCategory, MProfiler::kColorD_L1, "Populate");
        _sceneDelegate->Populate(_usdStage->GetPseudoRoot());

        _isPopulated = true;
    }
}

//! \brief  Synchronize USD scene delegate time with Maya's scene time.
void ProxyRenderDelegate::_UpdateTime() {
    MProfilingScope profilingScope(_profilerCategory, MProfiler::kColorC_L1, "Update Time");

    MayaUsdProxyShapeBase* usdSubSceneShape = getProxyShape();
    if(usdSubSceneShape && _sceneDelegate) {
        UsdTimeCode timeCode = usdSubSceneShape->getTime();
        _sceneDelegate->SetTime(timeCode);
    }
}

//! \brief  Execute Hydra engine which will performe minimal update VP2 state update based on change tracker.
void ProxyRenderDelegate::_Execute() {
    MProfilingScope profilingScope(_profilerCategory, MProfiler::kColorC_L1, "Execute");

    _engine.Execute( *_renderIndex, _taskController->GetTasks());
}

//! \brief  Main update entry from subscene override.
void ProxyRenderDelegate::update(MSubSceneContainer& container, const MFrameContext& frameContext) {
    MProfilingScope profilingScope(_profilerCategory, MProfiler::kColorD_L1, "ProxyRenderDelegate::update");

    _InitRenderDelegate();
    _Populate();

    // Give access to current time and subscene container to the rest of render delegate world via render param's.
    auto* param = reinterpret_cast<HdVP2RenderParam*>(_renderDelegate->GetRenderParam());
    param->BeginUpdate(container, _sceneDelegate->GetTime());

    _UpdateTime();
    _Execute();
 
    param->EndUpdate();
}

PXR_NAMESPACE_CLOSE_SCOPE
