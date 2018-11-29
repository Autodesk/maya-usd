//
// Copyright 2018 Luma Pictures
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
#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdStreamRendererPlugin));

TF_INSTANTIATE_SINGLETON(MtohRenderOverride);

namespace {

constexpr auto MTOH_RENDER_OVERRIDE_NAME = "hydraViewportOverride";

// I don't think there is an easy way to detect if the viewport was changed,
// so I'm adding a 5 second timeout.
// There MUST be a better way to do this!
std::mutex _convergenceMutex;
bool _isConverged;
std::chrono::system_clock::time_point _lastRenderTime =
    std::chrono::system_clock::now();

std::atomic<bool> _needsClear;

void _TimerCallback(float, float, void*) {
    std::lock_guard<std::mutex> lock(_convergenceMutex);
    if (!_isConverged && (std::chrono::system_clock::now() - _lastRenderTime) <
                             std::chrono::seconds(5)) {
        MGlobal::executeCommandOnIdle("refresh -f");
    }
}

void _ClearResourcesCallback(float, float, void*) {
    const auto num3dViews = M3dView::numberOf3dViews();
    for (auto i = decltype(num3dViews){0}; i < num3dViews; ++i) {
        M3dView view;
        M3dView::get3dView(i, view);
        if (view.renderOverrideName() == MString(MTOH_RENDER_OVERRIDE_NAME)) {
            return;
        }
    }
    MtohRenderOverride::GetInstance().ClearHydraResources();
}

class HdMayaSceneRender : public MHWRender::MSceneRender {
public:
    HdMayaSceneRender(const MString& name) : MHWRender::MSceneRender(name) {}

    MUint64 getObjectTypeExclusions() override {
        return ~(
            MHWRender::MFrameContext::kExcludeSelectHandles |
            // MHWRender::MFrameContext::kExcludeCameras |
            MHWRender::MFrameContext::kExcludeLights);
    }

    MHWRender::MClearOperation& clearOperation() override {
        auto* renderer = MHWRender::MRenderer::theRenderer();
        const auto gradient = renderer->useGradient();
        const auto color1 = renderer->clearColor();
        const auto color2 = renderer->clearColor2();

        float c1[4] = {color1[0], color1[1], color1[2], 1.0f};
        float c2[4] = {color2[0], color2[1], color2[2], 1.0f};

        mClearOperation.setClearColor(c1);
        mClearOperation.setClearColor2(c2);
        mClearOperation.setClearGradient(gradient);
        return mClearOperation;
    }
};

class HdMayaManipulatorRender : public MHWRender::MSceneRender {
public:
    HdMayaManipulatorRender(const MString& name)
        : MHWRender::MSceneRender(name) {}

    MUint64 getObjectTypeExclusions() override {
        return ~MHWRender::MFrameContext::kExcludeManipulators;
    }

    MHWRender::MClearOperation& clearOperation() override {
        mClearOperation.setMask(MHWRender::MClearOperation::kClearNone);
        return mClearOperation;
    }
};

class HdMayaRender : public MHWRender::MUserRenderOperation {
public:
    HdMayaRender(const MString& name, MtohRenderOverride* override)
        : MHWRender::MUserRenderOperation(name), _override(override) {}

    MStatus execute(const MHWRender::MDrawContext& drawContext) override {
        return _override->Render(drawContext);
    }

    bool hasUIDrawables() const override { return false; }

    bool requiresLightData() const override { return false; }

private:
    MtohRenderOverride* _override;
};

class SetRenderGLState {
public:
    SetRenderGLState() {
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &_oldBlendFunc);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &_oldBlendEquation);
        glGetBooleanv(GL_BLEND, &_oldBlend);
        glGetBooleanv(GL_CULL_FACE, &_oldCullFace);

        if (_oldBlendFunc != BLEND_FUNC) {
            glBlendFunc(GL_SRC_ALPHA, BLEND_FUNC);
        }

        if (_oldBlendEquation != BLEND_EQUATION) {
            glBlendEquation(BLEND_EQUATION);
        }

        if (_oldBlend != BLEND) { glEnable(GL_BLEND); }

        if (_oldCullFace != CULL_FACE) { glDisable(GL_CULL_FACE); }
    }

    ~SetRenderGLState() {
        if (_oldBlend != BLEND) { glDisable(GL_BLEND); }

        if (_oldBlendFunc != BLEND_FUNC) {
            glBlendFunc(GL_SRC_ALPHA, _oldBlendFunc);
        }

        if (_oldBlendEquation != BLEND_EQUATION) {
            glBlendEquation(_oldBlendEquation);
        }

        if (_oldCullFace != CULL_FACE) { glEnable(GL_CULL_FACE); }
    }

private:
    // non-odr
    constexpr static int BLEND_FUNC = GL_ONE_MINUS_SRC_ALPHA;
    constexpr static int BLEND_EQUATION = GL_FUNC_ADD;
    constexpr static GLboolean BLEND = GL_TRUE;
    constexpr static GLboolean CULL_FACE = GL_FALSE;

    int _oldBlendFunc = BLEND_FUNC;
    int _oldBlendEquation = BLEND_EQUATION;
    GLboolean _oldBlend = BLEND;
    GLboolean _oldCullFace = CULL_FACE;
};

} // namespace

MtohRenderOverride::MtohRenderOverride()
    : MHWRender::MRenderOverride(MTOH_RENDER_OVERRIDE_NAME),
      _selectionTracker(new HdxSelectionTracker),
      _renderCollection(
          HdTokens->geometry, HdReprSelector(HdReprTokens->smoothHull),
          SdfPath::AbsoluteRootPath()),
      _selectionCollection(
          HdReprTokens->wire, HdReprSelector(HdReprTokens->wire)),
      _colorSelectionHighlightColor(1.0f, 1.0f, 0.0f, 0.5f) {
    _needsClear.store(false);
    HdMayaDelegateRegistry::InstallDelegatesChangedSignal(
        []() { _needsClear.store(true); });
    _ID = SdfPath("/HdMayaViewportRenderer")
              .AppendChild(TfToken(TfStringPrintf("_HdMaya_%p", this)));
    _rendererName = MtohGetDefaultRenderer();
    // This is a critical error, so we don't allow the construction
    // of the viewport renderer src if there is no renderer src
    // present.
    if (_rendererName.IsEmpty()) {
        // TODO: this should be checked when loading the plugin.
        throw std::runtime_error("No default renderer is available!");
    }

    MStatus status;
    auto id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew, ClearHydraCallback, nullptr, &status);
    if (status) { _callbacks.push_back(id); }
    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, ClearHydraCallback, nullptr, &status);
    if (status) { _callbacks.push_back(id); }
    id = MEventMessage::addEventCallback(
        MString("SelectionChanged"), SelectionChangedCallback, nullptr,
        &status);
    if (status) { _callbacks.push_back(id); }

    id = MTimerMessage::addTimerCallback(1.0f / 10.0f, _TimerCallback, &status);
    if (status) { _callbacks.push_back(id); }

    id =
        MTimerMessage::addTimerCallback(5.0f, _ClearResourcesCallback, &status);
    if (status) { _callbacks.push_back(id); }

    _defaultLight.SetSpecular(GfVec4f(0.0f));
    _defaultLight.SetAmbient(GfVec4f(0.0f));
}

MtohRenderOverride::~MtohRenderOverride() {
    ClearHydraResources();

    for (auto operation : _operations) { delete operation; }

    for (auto callback : _callbacks) { MMessage::removeCallback(callback); }
}

void MtohRenderOverride::ChangeRendererPlugin(const TfToken& id) {
    auto& instance = GetInstance();
    if (instance._rendererName == id) { return; }
    const auto renderers = MtohGetRendererPlugins();
    if (std::find(renderers.begin(), renderers.end(), id) == renderers.end()) {
        return;
    }
    instance._rendererName = id;
    if (instance._initializedViewport) { instance.ClearHydraResources(); }
}

int MtohRenderOverride::GetMaximumShadowMapResolution() {
    return GetInstance()._params.maximumShadowMapResolution;
}

void MtohRenderOverride::SetMaximumShadowMapResolution(int resolution) {
    GetInstance()._params.maximumShadowMapResolution = resolution;
}

int MtohRenderOverride::GetTextureMemoryPerTexture() {
    return static_cast<int>(GetInstance()._params.textureMemoryPerTexture);
}

void MtohRenderOverride::SetTextureMemoryPerTexture(int memory) {
    GetInstance()._params.textureMemoryPerTexture = static_cast<size_t>(memory);
}

bool MtohRenderOverride::GetWireframeSelectionHighlight() {
    return GetInstance()._wireframeSelectionHighlight;
}

void MtohRenderOverride::SetWireframeSelectionHighlight(bool value) {
    GetInstance()._wireframeSelectionHighlight = value;
}

bool MtohRenderOverride::GetColorSelectionHighlight() {
    return GetInstance()._colorSelectionHighlight;
}

void MtohRenderOverride::SetColorSelectionHighlight(bool value) {
    GetInstance()._colorSelectionHighlight = value;
}

GfVec4d MtohRenderOverride::GetColorSelectionHighlightColor() {
    return GetInstance()._colorSelectionHighlightColor;
}

void MtohRenderOverride::SetColorSelectionHighlightColor(const GfVec4d& color) {
    GetInstance()._colorSelectionHighlightColor = GfVec4f(color);
}

void MtohRenderOverride::DetectMayaDefaultLighting(
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
            "DetectMayaDefaultLightingAndClearIfChanged() "
            "foundMayaDefaultLight=%i\n",
            foundMayaDefaultLight);

    if (foundMayaDefaultLight != _hasDefaultLighting) {
        _hasDefaultLighting = foundMayaDefaultLight;
        _needsClear.store(true);
        TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
            .Msg(
                "MtohRenderOverride::"
                "DetectMayaDefaultLighting() clearing! "
                "_hasDefaultLighting=%i\n",
                _hasDefaultLighting);
    }
}

void MtohRenderOverride::ConfigureLighting() {
    if (_hasDefaultLighting) {
        GlfSimpleLightVector lights;
        lights.push_back(_defaultLight);
        _defaultLightingContext->SetLights(lights);
        _defaultLightingContext->SetUseLighting(true);
        _taskController->SetLightingState(_defaultLightingContext);
    }
}

MStatus MtohRenderOverride::Render(const MHWRender::MDrawContext& drawContext) {
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

    DetectMayaDefaultLighting(drawContext);
    if (_needsClear.exchange(false)) { ClearHydraResources(); }

    if (!_initializedViewport) {
        GlfGlewInit();
        InitHydraResources();
        if (_preferSimpleLight) {
            _taskController->SetEnableShadows(false);
            renderFrame();
            _taskController->SetEnableShadows(true);
        }
    }

    const auto displayStyle = drawContext.getDisplayStyle();
    _params.displaySmoothMeshes =
        !(displayStyle & MHWRender::MFrameContext::kFlatShaded);

    for (auto& it : _delegates) {
        it->SetParams(_params);
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

    ConfigureLighting();

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
    _taskController->SetSelectionColor(_colorSelectionHighlightColor);
    _taskController->SetEnableSelection(_colorSelectionHighlight);

    // This is required for HdStream to display transparency.
    // We should fix this upstream, so HdStream can setup
    // all the required states.
    if (_rendererName == _tokens->HdStreamRendererPlugin) {
        SetRenderGLState state;
        renderFrame();
    } else {
        renderFrame();
    }

    // This causes issues with the embree delegate and potentially others.
    if (_wireframeSelectionHighlight &&
        _rendererName == _tokens->HdStreamRendererPlugin) {
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

void MtohRenderOverride::InitHydraResources() {
    TF_DEBUG(HDMAYA_PLUGIN_RENDEROVERRIDE)
        .Msg("MtohRenderOverride::InitHydraResources()\n");
    _rendererPlugin =
        HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(
            _rendererName);
    auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
    _renderIndex = HdRenderIndex::New(renderDelegate);
    int delegateId = 0;
    for (const auto& creator : HdMayaDelegateRegistry::GetDelegateCreators()) {
        if (creator == nullptr) { continue; }
        auto newDelegate = creator(
            _renderIndex, _ID.AppendChild(TfToken(TfStringPrintf(
                              "_Delegate_%i_%p", delegateId++, this))));
        if (newDelegate) {
            // Call SetLightsEnabled before the delegate is populated
            newDelegate->SetLightsEnabled(!_hasDefaultLighting);
            _delegates.push_back(newDelegate);
        }
    }
    _taskController = new HdxTaskController(
        _renderIndex,
        _ID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p",
            TfMakeValidIdentifier(_rendererName.GetText()).c_str(), this))));
    if (_hasDefaultLighting) {
        _defaultLightingContext = GlfSimpleLightingContext::New();
        GlfSimpleMaterial material;
        material.SetAmbient(GfVec4f(0.f));
        material.SetDiffuse(GfVec4f(1.f));
        material.SetSpecular(GfVec4f(0.f));
        material.SetEmission(GfVec4f(0.f));
        material.SetShininess(0.0f);
        _defaultLightingContext->SetMaterial(material);
        _defaultLightingContext->SetSceneAmbient(GfVec4f(0.0f));
    }
    _taskController->SetEnableShadows(true);
    VtValue selectionTrackerValue(_selectionTracker);
    _engine.SetTaskContextData(
        HdxTokens->selectionState, selectionTrackerValue);
    _preferSimpleLight = _rendererName == _tokens->HdStreamRendererPlugin;
    for (auto& it : _delegates) {
        it->SetPreferSimpleLight(_preferSimpleLight);
        it->Populate();
    }

    _renderIndex->GetChangeTracker().AddCollection(
        _selectionCollection.GetName());
    SelectionChanged();

    _initializedViewport = true;
}

void MtohRenderOverride::ClearHydraResources() {
    if (!_initializedViewport) { return; }
    _delegates.clear();
    _defaultLightingContext.Reset();

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
}

void MtohRenderOverride::ClearHydraCallback(void*) {
    GetInstance().ClearHydraResources();
}

void MtohRenderOverride::SelectionChanged() {
    MSelectionList sel;
    if (!TF_VERIFY(MGlobal::getActiveSelectionList(sel))) { return; }
    SdfPathVector selectedPaths;
    for (auto& it : _delegates) {
        it->PopulateSelectedPaths(sel, selectedPaths);
    }
    _selectionCollection.SetRootPaths(selectedPaths);

    auto* selection = new HdSelection;
    for (auto& it : _delegates) { it->PopulateSelectedPaths(sel, selection); }
    _selectionTracker->SetSelection(HdSelectionSharedPtr(selection));
}

void MtohRenderOverride::SelectionChangedCallback(void*) {
    GetInstance().SelectionChanged();
}

MHWRender::DrawAPI MtohRenderOverride::supportedDrawAPIs() const {
    return MHWRender::kOpenGLCoreProfile | MHWRender::kOpenGL;
}

MStatus MtohRenderOverride::setup(const MString& destination) {
    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer == nullptr) { return MStatus::kFailure; }

    if (_operations.empty()) {
        _operations.push_back(
            new HdMayaSceneRender("HydraRenderOverride_Scene"));
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

PXR_NAMESPACE_CLOSE_SCOPE
