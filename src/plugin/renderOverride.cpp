//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "renderOverride.h"

#include <pxr/base/gf/matrix4d.h>

#include <pxr/base/tf/instantiateSingleton.h>

#include <pxr/imaging/glf/contextCaps.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/imaging/hdx/tokens.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MEventMessage.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MTimerMessage.h>

#include <atomic>
#include <chrono>
#include <exception>

#include <hdmaya/delegates/delegateRegistry.h>
#include <hdmaya/delegates/sceneDelegate.h>

#include <hdmaya/utils.h>

#include "pluginDebugCodes.h"
#include "renderOverrideUtils.h"
#include "tokens.h"
#include "utils.h"

#if HDMAYA_UFE_BUILD
#include <maya/MFileIO.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/selectionNotification.h>
#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdStreamRendererPlugin));

namespace {

std::vector<MtohRenderOverride*> _allInstances;

#if HDMAYA_UFE_BUILD

// Observe UFE scene items for transformation changed only when they are
// selected.
class UfeSelectionObserver : public UFE_NS::Observer {
public:
    UfeSelectionObserver(MtohRenderOverride& mtohRenderOverride)
        : UFE_NS::Observer(), _mtohRenderOverride(mtohRenderOverride) {}

    //~UfeSelectionObserver() override {}

    void operator()(const UFE_NS::Notification& notification) override {
        // During Maya file read, each node will be selected in turn, so we get
        // notified for each node in the scene.  Prune this out.
        if (MFileIO::isOpeningFile()) { return; }

        auto selectionChanged =
            dynamic_cast<const UFE_NS::SelectionChanged*>(&notification);
        if (selectionChanged == nullptr) { return; }

        TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
            .Msg(
                "UfeSelectionObserver triggered (ufe selection change "
                "triggered)\n");
        _mtohRenderOverride.SelectionChanged();
    }

private:
    MtohRenderOverride& _mtohRenderOverride;
};

#endif // HDMAYA_UFE_BUILD

} // namespace

MtohRenderOverride::MtohRenderOverride(const MtohRendererDescription& desc)
    : MHWRender::MRenderOverride(desc.overrideName.GetText()),
      _rendererDesc(desc),
      _selectionTracker(new HdxSelectionTracker),
      _renderCollection(
          HdTokens->geometry,
          HdReprSelector(
#if MAYA_APP_VERSION >= 2019
              HdReprTokens->refined
#else
              HdReprTokens->smoothHull
#endif
              ),
          SdfPath::AbsoluteRootPath()),
      _selectionCollection(
          HdReprTokens->wire, HdReprSelector(HdReprTokens->wire)),
      _isUsingHdSt(desc.rendererName == _tokens->HdStreamRendererPlugin) {
    _needsClear.store(false);
    HdMayaDelegateRegistry::InstallDelegatesChangedSignal(
        [this]() { _needsClear.store(true); });
    _ID = SdfPath("/HdMayaViewportRenderer")
              .AppendChild(TfToken(TfStringPrintf(
                  "_HdMaya_%s_%p", desc.rendererName.GetText(), this)));

    MStatus status;
    auto id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew, _ClearHydraCallback, this, &status);
    if (status) { _callbacks.push_back(id); }
    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, _ClearHydraCallback, this, &status);
    if (status) { _callbacks.push_back(id); }
    id = MEventMessage::addEventCallback(
        MString("SelectionChanged"), _SelectionChangedCallback, this, &status);
    if (status) { _callbacks.push_back(id); }

    id = MTimerMessage::addTimerCallback(
        1.0f / 10.0f, _TimerCallback, this, &status);
    if (status) { _callbacks.push_back(id); }

    id = MTimerMessage::addTimerCallback(
        5.0f, _ClearResourcesCallback, this, &status);
    if (status) { _callbacks.push_back(id); }

    _defaultLight.SetSpecular(GfVec4f(0.0f));
    _defaultLight.SetAmbient(GfVec4f(0.0f));
    _allInstances.push_back(this);

    _globals = MtohGetRenderGlobals();

#if HDMAYA_UFE_BUILD
    const UFE_NS::GlobalSelection::Ptr& ufeSelection =
        UFE_NS::GlobalSelection::get();
    if (ufeSelection) {
        _ufeSelectionObserver = std::make_shared<UfeSelectionObserver>(*this);
        ufeSelection->addObserver(_ufeSelectionObserver);
    }
#endif // HDMAYA_UFE_BUILD
}

MtohRenderOverride::~MtohRenderOverride() {
    ClearHydraResources();

    for (auto operation : _operations) { delete operation; }

    for (auto callback : _callbacks) { MMessage::removeCallback(callback); }
    _allInstances.erase(
        std::remove(_allInstances.begin(), _allInstances.end(), this),
        _allInstances.end());

#if HDMAYA_UFE_BUILD
    const UFE_NS::GlobalSelection::Ptr& ufeSelection =
        UFE_NS::GlobalSelection::get();
    if (ufeSelection) {
        ufeSelection->removeObserver(_ufeSelectionObserver);
        _ufeSelectionObserver = nullptr;
    }
#endif // HDMAYA_UFE_BUILD
}

void MtohRenderOverride::UpdateRenderGlobals() {
    for (auto* instance : _allInstances) {
        instance->_renderGlobalsHaveChanged = true;
    }
}

void MtohRenderOverride::_DetectMayaDefaultLighting(
    const MHWRender::MDrawContext& drawContext) {
    constexpr auto considerAllSceneLights =
        MHWRender::MDrawContext::kFilteredIgnoreLightLimit;

    const auto numLights =
        drawContext.numberOfActiveLights(considerAllSceneLights);
    auto foundMayaDefaultLight = false;
    if (numLights == 1) {
        auto* lightParam =
            drawContext.getLightParameterInformation(0, considerAllSceneLights);
        if (lightParam != nullptr && !lightParam->lightPath().isValid()) {
            // This light does not exist so it must be the
            // default maya light
            MFloatPointArray positions;
            MFloatVector direction;
            auto intensity = 0.0f;
            MColor color;
            auto hasDirection = false;
            auto hasPosition = false;

            // Maya default light has no position, only direction
            drawContext.getLightInformation(
                0, positions, direction, intensity, color, hasDirection,
                hasPosition, considerAllSceneLights);

            if (hasDirection && !hasPosition) {
                _defaultLight.SetPosition(
                    {-direction.x, -direction.y, -direction.z, 0.0f});
                _defaultLight.SetDiffuse({intensity * color.r,
                                          intensity * color.g,
                                          intensity * color.b, 1.0f});
                foundMayaDefaultLight = true;
            }
        }
    }

    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg(
            "MtohRenderOverride::"
            "_DetectMayaDefaultLighting() "
            "foundMayaDefaultLight=%i\n",
            foundMayaDefaultLight);

    if (foundMayaDefaultLight != _hasDefaultLighting) {
        _hasDefaultLighting = foundMayaDefaultLight;
        _needsClear.store(true);
        TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
            .Msg(
                "MtohRenderOverride::"
                "_DetectMayaDefaultLighting() clearing! "
                "_hasDefaultLighting=%i\n",
                _hasDefaultLighting);
    }
}

void MtohRenderOverride::_UpdateRenderGlobals() {
    if (!_renderGlobalsHaveChanged) { return; }
    _renderGlobalsHaveChanged = false;
    _globals = MtohGetRenderGlobals();
    _UpdateRenderDelegateOptions();
    if (_isUsingHdSt && !_operations.empty()) {
        const auto vp2Overlay = _globals.selectionOverlay == MtohTokens->UseVp2;
        auto* mayaRender = reinterpret_cast<HdMayaSceneRender*>(_operations[0]);
        if (mayaRender->_drawSelectionOverlay != vp2Overlay) {
            mayaRender->_drawSelectionOverlay = vp2Overlay;
            MGlobal::executeCommandOnIdle("refresh -f;");
        }
    }
}

void MtohRenderOverride::_UpdateRenderDelegateOptions() {
#ifdef USD_001901_BUILD
    if (_renderIndex == nullptr) { return; }
    auto* renderDelegate = _renderIndex->GetRenderDelegate();
    if (renderDelegate == nullptr) { return; }
    const auto* settings =
        TfMapLookupPtr(_globals.rendererSettings, _rendererDesc.rendererName);
    if (settings == nullptr) { return; }
    // TODO: Which is better? Set everything blindly or only set settings that
    //  have changed? This is not performance critical, and render delegates
    //  might track changes internally.
    for (const auto& setting : *settings) {
        const auto v = renderDelegate->GetRenderSetting(setting.key);
        if (v != setting.value) {
            renderDelegate->SetRenderSetting(setting.key, setting.value);
        }
    }
#endif
}

MStatus MtohRenderOverride::Render(const MHWRender::MDrawContext& drawContext) {
    // It would be good to clear the resources of the overrides that are
    // not in active use, but I'm not sure if we have a better way than
    // the idle time we use currently. The approach below would break if
    // two render overrides were used at the same time.
    // for (auto* override: _allInstances) {
    //     if (override != this) {
    //         override->ClearHydraResources();
    //     }
    // }
    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg("MtohRenderOverride::Render()\n");
    auto renderFrame = [&]() {
        const auto originX = 0;
        const auto originY = 0;
        int width = 0;
        int height = 0;
        drawContext.getRenderTargetSize(width, height);

        GfVec4d viewport(originX, originY, width, height);
        _taskController->SetCameraMatrices(
            GetGfMatrixFromMaya(
                drawContext.getMatrix(MHWRender::MFrameContext::kViewMtx)),
            GetGfMatrixFromMaya(drawContext.getMatrix(
                MHWRender::MFrameContext::kProjectionMtx)));
        _taskController->SetCameraViewport(viewport);
#ifdef USD_001901_BUILD
        _engine.Execute(*_renderIndex, _taskController->GetTasks());
#else
        _engine.Execute(
            *_renderIndex,
            _taskController->GetTasks(HdxTaskSetTokens->colorRender));
#endif
    };

    _UpdateRenderGlobals();

    _DetectMayaDefaultLighting(drawContext);
    if (_needsClear.exchange(false)) { ClearHydraResources(); }

    if (!_initializedViewport) {
        GlfGlewInit();
        _InitHydraResources();
// This was required to work around an issue in HdSt
// that didn't render lights the first time. Leaving it here
// for a while in case others run into the problem.
#if 0
        if (_isUsingHdSt) {
            _taskController->SetEnableShadows(false);
            renderFrame();
            _taskController->SetEnableShadows(true);
        }
#endif
    }

    _SelectionChanged();

    const auto displayStyle = drawContext.getDisplayStyle();
    _globals.delegateParams.displaySmoothMeshes =
        !(displayStyle & MHWRender::MFrameContext::kFlatShaded);

    if (_defaultLightDelegate != nullptr) {
        _defaultLightDelegate->SetDefaultLight(_defaultLight);
    }
    for (auto& it : _delegates) {
        it->SetParams(_globals.delegateParams);
        it->PreFrame(drawContext);
    }

    // TODO: Is there a way to improve this? Quite silly.
    auto enableShadows = true;
    auto* lightParam = drawContext.getLightParameterInformation(
        0, MHWRender::MDrawContext::kFilteredIgnoreLightLimit);
    if (lightParam != nullptr) {
        MIntArray intVals;
        if (lightParam->getParameter(
                MHWRender::MLightParameterInformation::kGlobalShadowOn,
                intVals) &&
            intVals.length() > 0) {
            enableShadows = intVals[0] != 0;
        }
    }
    _taskController->SetEnableShadows(enableShadows);

    HdxRenderTaskParams params;
    params.enableLighting = true;
    params.enableSceneMaterials = true;

    /* TODO: Find replacement
     * if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
        params.complexity = HdComplexityBoundingBox;
    } else {
        params.complexity = HdComplexityVeryHigh;
    }

    if (displayStyle & MHWRender::MFrameContext::kWireFrame) {
        params.geomStyle = HdGeomStyleLines;
    } else {
        params.geomStyle = HdGeomStylePolygons;
    }*/

    // TODO: separate color for normal wireframe / selected
    MColor colour = M3dView::leadColor();
    params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);

    params.cullStyle = HdCullStyleBackUnlessDoubleSided;

    _taskController->SetRenderParams(params);
    HdxShadowTaskParams shadowParams;
    shadowParams.cullStyle = HdCullStyleNothing;

    _taskController->SetShadowParams(shadowParams);

    // Default color in usdview.
    _taskController->SetSelectionColor(_globals.colorSelectionHighlightColor);
    _taskController->SetEnableSelection(_globals.colorSelectionHighlight);

    // This is required for HdStream to display transparency.
    // We should fix this upstream, so HdStream can setup
    // all the required states.
    _taskController->SetCollection(_renderCollection);
    if (_isUsingHdSt) {
        HdMayaSetRenderGLState state;
        renderFrame();
    } else {
        renderFrame();
    }

    // This causes issues with the embree delegate and potentially others.
    if (_globals.wireframeSelectionHighlight &&
        _globals.selectionOverlay == MtohTokens->UseHdSt && _isUsingHdSt) {
        if (!_selectionCollection.GetRootPaths().empty()) {
            _taskController->SetCollection(_selectionCollection);
            renderFrame();
            _taskController->SetCollection(_renderCollection);
        }
    }

    for (auto& it : _delegates) { it->PostFrame(); }

    std::lock_guard<std::mutex> lock(_convergenceMutex);
    _lastRenderTime = std::chrono::system_clock::now();
    _isConverged = _taskController->IsConverged();

    return MStatus::kSuccess;
}

void MtohRenderOverride::_InitHydraResources() {
    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg("MtohRenderOverride::_InitHydraResources()\n");
#ifdef USD_001901_BUILD
    GlfContextCaps::InitInstance();
#endif
    _rendererPlugin =
        HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(
            _rendererDesc.rendererName);
    auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
    _renderIndex = HdRenderIndex::New(renderDelegate);

    _taskController = new HdxTaskController(
        _renderIndex,
        _ID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p",
            TfMakeValidIdentifier(_rendererDesc.rendererName.GetText()).c_str(),
            this))));
    _taskController->SetEnableShadows(true);

    HdMayaDelegate::InitData delegateInitData(
        _engine, _renderIndex, _rendererPlugin, _taskController, SdfPath(),
        _isUsingHdSt);

    auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
    auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
    TF_VERIFY(delegateNames.size() == creators.size());
    for (size_t i = 0, n = creators.size(); i < n; ++i) {
        const auto& creator = creators[i];
        if (creator == nullptr) { continue; }
        delegateInitData.delegateID = _ID.AppendChild(TfToken(TfStringPrintf(
            "_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
        auto newDelegate = creator(delegateInitData);
        if (newDelegate) {
            // Call SetLightsEnabled before the delegate is populated
            newDelegate->SetLightsEnabled(!_hasDefaultLighting);
            _delegates.push_back(newDelegate);
        }
    }
    if (_hasDefaultLighting) {
        delegateInitData.delegateID = _ID.AppendChild(
            TfToken(TfStringPrintf("_DefaultLightDelegate_%p", this)));
        _defaultLightDelegate.reset(
            new MtohDefaultLightDelegate(delegateInitData));
    }
    VtValue selectionTrackerValue(_selectionTracker);
    _engine.SetTaskContextData(
        HdxTokens->selectionState, selectionTrackerValue);
    for (auto& it : _delegates) { it->Populate(); }
    if (_defaultLightDelegate) { _defaultLightDelegate->Populate(); }

    _renderIndex->GetChangeTracker().AddCollection(
        _selectionCollection.GetName());
    _SelectionChanged();

    _initializedViewport = true;
    _UpdateRenderDelegateOptions();
}

void MtohRenderOverride::ClearHydraResources() {
    if (!_initializedViewport) { return; }
    _delegates.clear();
    _defaultLightDelegate.reset();

    if (_taskController != nullptr) {
        delete _taskController;
        _taskController = nullptr;
    }

    HdRenderDelegate* renderDelegate = nullptr;
    if (_renderIndex != nullptr) {
        renderDelegate = _renderIndex->GetRenderDelegate();
        delete _renderIndex;
        _renderIndex = nullptr;
    }

    if (_rendererPlugin != nullptr) {
        if (renderDelegate != nullptr) {
            _rendererPlugin->DeleteRenderDelegate(renderDelegate);
        }
        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
        _rendererPlugin = nullptr;
    }

    _initializedViewport = false;
    SelectionChanged();
}

void MtohRenderOverride::SelectionChanged() { _selectionChanged = true; }

void MtohRenderOverride::_SelectionChanged() {
    if (!_selectionChanged) { return; }
    _selectionChanged = false;
    MSelectionList sel;
    if (!TF_VERIFY(MGlobal::getActiveSelectionList(sel))) { return; }
    SdfPathVector selectedPaths;
    auto* selection = new HdSelection;

#if HDMAYA_UFE_BUILD
    const UFE_NS::GlobalSelection::Ptr& ufeSelection =
        UFE_NS::GlobalSelection::get();
#endif // HDMAYA_UFE_BUILD

    for (auto& it : _delegates) {
#if HDMAYA_UFE_BUILD
        if (it->SupportsUfeSelection()) {
            if (ufeSelection) {
                it->PopulateSelectedPaths(
                    *ufeSelection, selectedPaths, selection);
            }
            // skip non-ufe PopulateSelectedPaths call
            continue;
        }
#endif // HDMAYA_UFE_BUILD
        it->PopulateSelectedPaths(sel, selectedPaths, selection);
    }
    _selectionCollection.SetRootPaths(selectedPaths);
    _selectionTracker->SetSelection(HdSelectionSharedPtr(selection));
    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg(
            "MtohRenderOverride::_SelectionChanged - num selected: %lu\n",
            selectedPaths.size());
}

MHWRender::DrawAPI MtohRenderOverride::supportedDrawAPIs() const {
    return MHWRender::kOpenGLCoreProfile | MHWRender::kOpenGL;
}

MStatus MtohRenderOverride::setup(const MString& destination) {
    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer == nullptr) { return MStatus::kFailure; }

    if (_operations.empty()) {
        _operations.push_back(new HdMayaSceneRender(
            "HydraRenderOverride_Scene",
            !_isUsingHdSt || _globals.selectionOverlay == MtohTokens->UseVp2));
        _operations.push_back(
            new HdMayaRender("HydraRenderOverride_Hydra", this));
        _operations.push_back(
            new HdMayaManipulatorRender("HydraRenderOverride_Manipulator"));
        _operations.push_back(new MHWRender::MHUDRender());
        auto* presentTarget =
            new MHWRender::MPresentTarget("HydraRenderOverride_Present");
        presentTarget->setPresentDepth(true);
        presentTarget->setTargetBackBuffer(
            MHWRender::MPresentTarget::kCenterBuffer);
        _operations.push_back(presentTarget);
    }

    return MS::kSuccess;
}

MStatus MtohRenderOverride::MtohRenderOverride::cleanup() {
    _currentOperation = -1;
    return MS::kSuccess;
}

bool MtohRenderOverride::startOperationIterator() {
    _currentOperation = 0;
    return true;
}

MHWRender::MRenderOperation* MtohRenderOverride::renderOperation() {
    if (_currentOperation >= 0 &&
        _currentOperation < static_cast<int>(_operations.size())) {
        return _operations[_currentOperation];
    }
    return nullptr;
}

bool MtohRenderOverride::nextRenderOperation() {
    return ++_currentOperation < static_cast<int>(_operations.size());
}

void MtohRenderOverride::_ClearHydraCallback(void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }
    instance->ClearHydraResources();
}

void MtohRenderOverride::_TimerCallback(float, float, void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }
    std::lock_guard<std::mutex> lock(instance->_convergenceMutex);
    if (!instance->_isConverged &&
        (std::chrono::system_clock::now() - instance->_lastRenderTime) <
            std::chrono::seconds(5)) {
        MGlobal::executeCommandOnIdle("refresh -f");
    }
}

void MtohRenderOverride::_ClearResourcesCallback(float, float, void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }
    const auto num3dViews = M3dView::numberOf3dViews();
    for (auto i = decltype(num3dViews){0}; i < num3dViews; ++i) {
        M3dView view;
        M3dView::get3dView(i, view);
        if (view.renderOverrideName() ==
            instance->_rendererDesc.overrideName.GetText()) {
            return;
        }
    }
    instance->ClearHydraResources();
    instance->_UpdateRenderGlobals();
}

void MtohRenderOverride::_SelectionChangedCallback(void* data) {
    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg(
            "MtohRenderOverride::_SelectionChangedCallback() (normal maya "
            "selection triggered)\n");
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }
    instance->SelectionChanged();
}

PXR_NAMESPACE_CLOSE_SCOPE
