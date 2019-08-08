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

#include <hdmaya/hdmaya.h>

#include <pxr/base/gf/matrix4d.h>

#include <pxr/base/tf/instantiateSingleton.h>

#include <pxr/imaging/glf/contextCaps.h>

#include <pxr/imaging/hd/rprim.h>
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
#include <maya/MUiMessage.h>

#include <atomic>
#include <chrono>
#include <exception>

#include "../../usd/hdmaya/delegates/delegateRegistry.h"
#include "../../usd/hdmaya/delegates/sceneDelegate.h"

#include "../../usd/hdmaya/utils.h"

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

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (HdStreamRendererPlugin)
);
// clang-format on

namespace {

// Not sure if we actually need a mutex guarding _allInstances, but
// everywhere that uses it isn't a "frequent" operation, so the
// extra speed loss should be fine, and I'd rather be safe.
std::mutex _allInstancesMutex;
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

        TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
            .Msg(
                "UfeSelectionObserver triggered (ufe selection change "
                "triggered)\n");
        _mtohRenderOverride.SelectionChanged();
    }

private:
    MtohRenderOverride& _mtohRenderOverride;
};

#endif // HDMAYA_UFE_BUILD

/// Simple RAII class to save uniform buffer bindings, to deal with a maya
/// issue.
///
/// As originally explained by Pixar in their usdmaya plugin:
///
/// XXX: When Maya is using OpenGL Core Profile as the rendering engine (in
/// either compatibility or strict mode), batch renders like those done in
/// the "Render View" window or through the ogsRender command do not
/// properly track uniform buffer binding state. This was causing issues
/// where the first batch render performed would look correct, but then all
/// subsequent renders done in that Maya session would be completely black
/// (no alpha), even if the frame contained only Maya-native geometry or if
/// a new scene was created/opened.
///
/// To avoid this problem, we need to save and restore Maya's bindings
/// across Hydra calls. We try not to bog down performance by saving and
/// restoring *all* GL_MAX_UNIFORM_BUFFER_BINDINGS possible bindings, so
/// instead we only do just enough to avoid issues. Empirically, the
/// problematic binding has been the material binding at index 4.
class UBOBindingsSaver {
public:
    static constexpr size_t UNIFORM_BINDINGS_TO_SAVE = 5u;

    UBOBindingsSaver() {
        for (size_t i = 0u; i < _uniformBufferBindings.size(); ++i) {
            glGetIntegeri_v(
                GL_UNIFORM_BUFFER_BINDING, (GLuint)i,
                &_uniformBufferBindings[i]);
        }
    }

    ~UBOBindingsSaver() {
        for (size_t i = 0u; i < _uniformBufferBindings.size(); ++i) {
            glBindBufferBase(
                GL_UNIFORM_BUFFER, (GLuint)i, _uniformBufferBindings[i]);
        }
    }

private:
    std::array<GLint, UNIFORM_BINDINGS_TO_SAVE> _uniformBufferBindings;
};

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
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride created (%s - %s - %s)\n",
            _rendererDesc.rendererName.GetText(),
            _rendererDesc.overrideName.GetText(),
            _rendererDesc.displayName.GetText());
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

    _defaultLight.SetSpecular(GfVec4f(0.0f));
    _defaultLight.SetAmbient(GfVec4f(0.0f));

    {
        std::lock_guard<std::mutex> lock(_allInstancesMutex);
        _allInstances.push_back(this);
    }

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
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride destroyed (%s - %s - %s)\n",
            _rendererDesc.rendererName.GetText(),
            _rendererDesc.overrideName.GetText(),
            _rendererDesc.displayName.GetText());

    ClearHydraResources();

    for (auto operation : _operations) { delete operation; }

    for (auto callback : _callbacks) { MMessage::removeCallback(callback); }
    for (auto& panelAndCallbacks : _renderPanelCallbacks) {
        MMessage::removeCallbacks(panelAndCallbacks.second);
    }

    {
        std::lock_guard<std::mutex> lock(_allInstancesMutex);
        _allInstances.erase(
            std::remove(_allInstances.begin(), _allInstances.end(), this),
            _allInstances.end());
    }

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
    std::lock_guard<std::mutex> lock(_allInstancesMutex);
    for (auto* instance : _allInstances) {
        instance->_renderGlobalsHaveChanged = true;
    }
}

std::vector<MString> MtohRenderOverride::AllActiveRendererNames() {
    std::vector<MString> renderers;

    std::lock_guard<std::mutex> lock(_allInstancesMutex);
    for (auto* instance : _allInstances) {
        if (instance->_initializedViewport) {
            renderers.push_back(instance->_rendererDesc.rendererName.GetText());
        }
    }
    return renderers;
}

SdfPathVector MtohRenderOverride::RendererRprims(
    TfToken rendererName, bool visibleOnly) {
    MtohRenderOverride* instance = _GetByName(rendererName);
    if (!instance) { return SdfPathVector(); }

    auto* renderIndex = instance->_renderIndex;
    if (!renderIndex) { return SdfPathVector(); }
    auto primIds = renderIndex->GetRprimIds();
    if (visibleOnly) {
        primIds.erase(
            std::remove_if(
                primIds.begin(), primIds.end(),
                [renderIndex](const SdfPath& primId) {
                    auto* rprim = renderIndex->GetRprim(primId);
                    if (!rprim) return true;
                    return !rprim->IsVisible();
                }),
            primIds.end());
    }
    return primIds;
}

SdfPath MtohRenderOverride::RendererSceneDelegateId(
    TfToken rendererName, TfToken sceneDelegateName) {
    MtohRenderOverride* instance = _GetByName(rendererName);
    if (!instance) { return SdfPath(); }

    for (auto& delegate : instance->_delegates) {
        if (delegate->GetName() == sceneDelegateName) {
            return delegate->GetMayaDelegateID();
        }
    }
    return SdfPath();
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

    TF_DEBUG(HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING)
        .Msg(
            "MtohRenderOverride::"
            "_DetectMayaDefaultLighting() "
            "foundMayaDefaultLight=%i\n",
            foundMayaDefaultLight);

    if (foundMayaDefaultLight != _hasDefaultLighting) {
        _hasDefaultLighting = foundMayaDefaultLight;
        _needsClear.store(true);
        TF_DEBUG(HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING)
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
#ifdef HDMAYA_USD_001901_BUILD
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
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RENDER)
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
#ifdef HDMAYA_USD_001907_BUILD
        auto tasks = _taskController->GetRenderingTasks();
        _engine.Execute(_renderIndex, &tasks);
#else
#ifdef HDMAYA_USD_001901_BUILD
        _engine.Execute(*_renderIndex, _taskController->GetTasks());
#else
        _engine.Execute(
            *_renderIndex,
            _taskController->GetTasks(HdxTaskSetTokens->colorRender));
#endif // HDMAYA_USD_001901_BUILD
#endif // HDMAYA_USD_001907_BUILD
    };

    _UpdateRenderGlobals();

    _DetectMayaDefaultLighting(drawContext);
    if (_needsClear.exchange(false)) { ClearHydraResources(); }

    if (!_initializedViewport) {
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

    UBOBindingsSaver bindingsSaver;

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
#ifndef HDMAYA_OIT_ENABLED
        HdMayaSetRenderGLState state;
#endif
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

MtohRenderOverride* MtohRenderOverride::_GetByName(TfToken rendererName) {
    std::lock_guard<std::mutex> lock(_allInstancesMutex);
    for (auto* instance : _allInstances) {
        if (instance->_rendererDesc.rendererName == rendererName) {
            return instance;
        }
    }
    return nullptr;
}

void MtohRenderOverride::_InitHydraResources() {
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride::_InitHydraResources(%s)\n",
            _rendererDesc.rendererName.GetText());
    GlfGlewInit();
#ifdef HDMAYA_USD_001905_BUILD
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
        TfToken(), _engine, _renderIndex, _rendererPlugin, _taskController,
        SdfPath(), _isUsingHdSt);

    auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
    auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
    TF_VERIFY(delegateNames.size() == creators.size());
    for (size_t i = 0, n = creators.size(); i < n; ++i) {
        const auto& creator = creators[i];
        if (creator == nullptr) { continue; }
        delegateInitData.name = delegateNames[i];
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

    TF_DEBUG(HDMAYA_RENDEROVERRIDE_RESOURCES)
        .Msg(
            "MtohRenderOverride::ClearHydraResources(%s)\n",
            _rendererDesc.rendererName.GetText());

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

void MtohRenderOverride::_RemovePanel(MString panelName) {
    auto foundPanelCallbacks = _FindPanelCallbacks(panelName);
    if (foundPanelCallbacks != _renderPanelCallbacks.end()) {
        MMessage::removeCallbacks(foundPanelCallbacks->second);
        _renderPanelCallbacks.erase(foundPanelCallbacks);
    }

    if (_renderPanelCallbacks.empty()) {
        ClearHydraResources();
        _UpdateRenderGlobals();
    }
}

void MtohRenderOverride::SelectionChanged() { _selectionChanged = true; }

void MtohRenderOverride::_SelectionChanged() {
    if (!_selectionChanged) { return; }
    _selectionChanged = false;
    MSelectionList sel;
    if (!TF_VERIFY(MGlobal::getActiveSelectionList(sel))) { return; }
    SdfPathVector selectedPaths;
    auto selection = boost::make_shared<HdSelection>();

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
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
        .Msg(
            "MtohRenderOverride::_SelectionChanged - num selected: %lu\n",
            selectedPaths.size());
}

MHWRender::DrawAPI MtohRenderOverride::supportedDrawAPIs() const {
    return MHWRender::kOpenGLCoreProfile | MHWRender::kOpenGL;
}

MStatus MtohRenderOverride::setup(const MString& destination) {
    MStatus status;

    auto panelNameAndCallbacks = _FindPanelCallbacks(destination);
    if (panelNameAndCallbacks == _renderPanelCallbacks.end()) {
        // Install the panel callbacks
        MCallbackIdArray newCallbacks;

        auto id = MUiMessage::add3dViewDestroyMsgCallback(
            destination, _PanelDeletedCallback, this, &status);
        if (status) { newCallbacks.append(id); }

        id = MUiMessage::add3dViewRendererChangedCallback(
            destination, _RendererChangedCallback, this, &status);
        if (status) { newCallbacks.append(id); }

        id = MUiMessage::add3dViewRenderOverrideChangedCallback(
            destination, _RenderOverrideChangedCallback, this, &status);
        if (status) { newCallbacks.append(id); }

        _renderPanelCallbacks.emplace_back(destination, newCallbacks);
    }

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

void MtohRenderOverride::_PanelDeletedCallback(
    const MString& panelName, void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }

    instance->_RemovePanel(panelName);
}

void MtohRenderOverride::_RendererChangedCallback(
    const MString& panelName, const MString& oldRenderer,
    const MString& newRenderer, void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }

    if (newRenderer != oldRenderer) { instance->_RemovePanel(panelName); }
}

void MtohRenderOverride::_RenderOverrideChangedCallback(
    const MString& panelName, const MString& oldOverride,
    const MString& newOverride, void* data) {
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }

    if (newOverride != instance->name()) { instance->_RemovePanel(panelName); }
}

void MtohRenderOverride::_SelectionChangedCallback(void* data) {
    TF_DEBUG(HDMAYA_RENDEROVERRIDE_SELECTION)
        .Msg(
            "MtohRenderOverride::_SelectionChangedCallback() (normal maya "
            "selection triggered)\n");
    auto* instance = reinterpret_cast<MtohRenderOverride*>(data);
    if (!TF_VERIFY(instance)) { return; }
    instance->SelectionChanged();
}

PXR_NAMESPACE_CLOSE_SCOPE
