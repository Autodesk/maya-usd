#include "renderOverride.h"

#include <pxr/base/gf/matrix4d.h>

#include <pxr/base/tf/instantiateSingleton.h>

#include <pxr/imaging/glf/contextCaps.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/imaging/hdx/tokens.h>

#include <maya/M3dView.h>
#include <maya/MDrawContext.h>
#include <maya/MDagPath.h>
#include <maya/MEventMessage.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectionList.h>

#include <exception>

#include <hdmaya/delegates/delegateRegistry.h>

#include <hdmaya/utils.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdStreamRendererPlugin)
);

TF_INSTANTIATE_SINGLETON(HdMayaRenderOverride);

namespace {

constexpr auto HDMAYA_DEFAULT_RENDERER_PLUGIN_NAME = "HDMAYA_DEFAULT_RENDERER_PLUGIN";

TfToken _getDefaultRenderer() {
    const auto l = HdMayaRenderOverride::GetRendererPlugins();
    if (l.empty()) { return {}; }
    const auto* defaultRenderer = getenv(HDMAYA_DEFAULT_RENDERER_PLUGIN_NAME);
    if (defaultRenderer == nullptr) { return l[0]; }
    const TfToken defaultRendererToken(defaultRenderer);
    if (std::find(l.begin(), l.end(), defaultRendererToken) != l.end()) { return defaultRendererToken; }
    return l[0];
}

class HdMayaSceneRender : public MHWRender::MSceneRender
{
public:
    HdMayaSceneRender(const MString &name) :
        MHWRender::MSceneRender(name) {
    }

    MUint64 getObjectTypeExclusions() override {
        return ~(
            MHWRender::MFrameContext::kExcludeSelectHandles |
            // MHWRender::MFrameContext::kExcludeCameras |
            MHWRender::MFrameContext::kExcludeLights);
    }

    MHWRender::MClearOperation&
    clearOperation()
    {
        auto* renderer = MHWRender::MRenderer::theRenderer();
        const auto gradient = renderer->useGradient();
        const auto color1 = renderer->clearColor();
        const auto color2 = renderer->clearColor2();

        float c1[4] = { color1[0], color1[1], color1[2], 1.0f };
        float c2[4] = { color2[0], color2[1], color2[2], 1.0f };

        mClearOperation.setClearColor(c1);
        mClearOperation.setClearColor2(c2);
        mClearOperation.setClearGradient(gradient);
        return mClearOperation;
    }
};

class HdMayaManipulatorRender : public MHWRender::MSceneRender
{
public:
    HdMayaManipulatorRender(const MString &name) :
        MHWRender::MSceneRender(name) {

    }

    MUint64 getObjectTypeExclusions() override {
        return ~MHWRender::MFrameContext::kExcludeManipulators;
    }

    MHWRender::MClearOperation&
    clearOperation()
    {
        mClearOperation.setMask(MClearOperation::kClearNone);
        return mClearOperation;
    }
};

class HdMayaRender : public MHWRender::MUserRenderOperation {
public:
    HdMayaRender(const MString& name, HdMayaRenderOverride* override)
        : MHWRender::MUserRenderOperation(name), _override(override) { }

    MStatus
    execute(const MHWRender::MDrawContext& drawContext) override {
        return _override->Render(drawContext);
    }

    bool hasUIDrawables() const override {
        return false;
    }

    bool requiresLightData() const override {
        return false;
    }

private:
    HdMayaRenderOverride* _override;
};

}

HdMayaRenderOverride::HdMayaRenderOverride() :
    MHWRender::MRenderOverride("hydraViewportOverride"),
    _selectionTracker(new HdxSelectionTracker),
    _renderCollection(HdTokens->geometry, HdTokens->smoothHull,
            SdfPath::AbsoluteRootPath()),
    _selectionCollection(HdTokens->wire, HdTokens->wire),
    _colorSelectionHighlightColor(1.0f, 1.0f, 0.0f, 0.5f) {
    _ID = SdfPath("/HdMayaViewportRenderer").AppendChild(TfToken(TfStringPrintf("_HdMaya_%p", this)));
    _rendererName = _getDefaultRenderer();
    // This is a critical error, so we don't allow the construction
    // of the viewport renderer src if there is no renderer src
    // present.
    if (_rendererName.IsEmpty()) {
        // TODO: this should be checked when loading the plugin.
        throw std::runtime_error("No default renderer is available!");
    }

    MStatus status;
    auto id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew,
        ClearHydraCallback,
        nullptr,
        &status);
    if (status) { _callbacks.push_back(id); }
    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen,
        ClearHydraCallback,
        nullptr,
        &status);
    if (status) { _callbacks.push_back(id); }
    id = MEventMessage::addEventCallback(
        MString("SelectionChanged"),
        SelectionChangedCallback,
        nullptr,
        &status);
    if (status) { _callbacks.push_back(id); }
}

HdMayaRenderOverride::~HdMayaRenderOverride() {
    ClearHydraResources();

    for (auto operation : _operations) {
        delete operation;
    }

    for (auto callback: _callbacks) {
        MMessage::removeCallback(callback);
    }
}

TfTokenVector
HdMayaRenderOverride::GetRendererPlugins() {
    HfPluginDescVector pluginDescs;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

    TfTokenVector ret; ret.reserve(pluginDescs.size());
    for (const auto& desc: pluginDescs) {
        ret.emplace_back(desc.id);
    }
    return ret;
}

std::string
HdMayaRenderOverride::GetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdxRendererPluginRegistry::GetInstance().GetPluginDesc(id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

void
HdMayaRenderOverride::ChangeRendererPlugin(const TfToken& id) {
    auto& instance = GetInstance();
    if (instance._rendererName == id) { return; }
    const auto renderers = GetRendererPlugins();
    if (std::find(renderers.begin(), renderers.end(), id) == renderers.end()) { return; }
    instance._rendererName = id;
    if (instance._initializedViewport) {
        instance.ClearHydraResources();
    }
}

int
HdMayaRenderOverride::GetMaximumShadowMapResolution() {
    return GetInstance()._params.maximumShadowMapResolution;
}

void
HdMayaRenderOverride::SetMaximumShadowMapResolution(int resolution) {
    GetInstance()._params.maximumShadowMapResolution = resolution;
}

int
HdMayaRenderOverride::GetTextureMemoryPerTexture() {
    return static_cast<int>(GetInstance()._params.textureMemoryPerTexture);
}

void
HdMayaRenderOverride::SetTextureMemoryPerTexture(int memory) {
    GetInstance()._params.textureMemoryPerTexture = static_cast<size_t>(memory);
}

bool
HdMayaRenderOverride::GetWireframeSelectionHighlight() {
    return GetInstance()._wireframeSelectionHighlight;
}

void
HdMayaRenderOverride::SetWireframeSelectionHighlight(bool value) {
    GetInstance()._wireframeSelectionHighlight = value;
}

bool
HdMayaRenderOverride::GetColorSelectionHighlight() {
    return GetInstance()._colorSelectionHighlight;
}

void
HdMayaRenderOverride::SetColorSelectionHighlight(bool value) {
    GetInstance()._colorSelectionHighlight = value;
}

GfVec4d
HdMayaRenderOverride::GetColorSelectionHighlightColor() {
    return GetInstance()._colorSelectionHighlightColor;
}

void
HdMayaRenderOverride::SetColorSelectionHighlightColor(const GfVec4d& color) {
    GetInstance()._colorSelectionHighlightColor = GfVec4f(color);
}

MStatus
HdMayaRenderOverride::Render(const MHWRender::MDrawContext& drawContext) {
    auto renderFrame = [&] () {
        const auto originX = 0;
        const auto originY = 0;
        int width = 0; int height = 0;
        drawContext.getRenderTargetSize(width, height);

        GfVec4d viewport(originX, originY, width, height);
        _taskController->SetCameraMatrices(
            getGfMatrixFromMaya(drawContext.getMatrix(MFrameContext::kViewMtx)),
            getGfMatrixFromMaya(drawContext.getMatrix(MFrameContext::kProjectionMtx)));
        _taskController->SetCameraViewport(viewport);

        _engine.Execute(*_renderIndex, _taskController->GetTasks(HdxTaskSetTokens->colorRender));
    };

    if (!_initializedViewport) {
        GlfGlewInit();
        InitHydraResources();
#if LUMA_USD_BUILD
        if (_preferSimpleLight) {
            _taskController->SetEnableShadows(false);
            renderFrame();
            _taskController->SetEnableShadows(true);
        }
#endif
    }

    const auto displayStyle = drawContext.getDisplayStyle();
    _params.displaySmoothMeshes = !(displayStyle & MHWRender::MFrameContext::kFlatShaded);

    for (auto& it: _delegates) {
        it->SetParams(_params);
        it->PreFrame();
    }

    // TODO: Is there a way to improve this? Quite silly.
    auto enableShadows = true;
    auto* lightParam = drawContext.getLightParameterInformation(0, MHWRender::MDrawContext::kFilteredIgnoreLightLimit);
    if (lightParam != nullptr) {
        MIntArray intVals;
        if (lightParam->getParameter(MHWRender::MLightParameterInformation::kGlobalShadowOn, intVals) &&
            intVals.length() > 0) {
            enableShadows = intVals[0] != 0;
        }
    }

#ifdef LUMA_USD_BUILD
    _taskController->SetEnableShadows(enableShadows);
#endif

    HdxRenderTaskParams params;
    params.enableLighting = true;
    params.enableHardwareShading = true;

    if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
        params.complexity = HdComplexityBoundingBox;
    } else {
        params.complexity = HdComplexityVeryHigh;
    }

    if (displayStyle & MHWRender::MFrameContext::kWireFrame) {
        params.geomStyle = HdGeomStyleLines;
    } else {
        params.geomStyle = HdGeomStylePolygons;
    }

    // TODO: separate color for normal wireframe / selected
    MColor colour = M3dView::leadColor();
    params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);

    _taskController->SetRenderParams(params);
#ifdef LUMA_USD_BUILD
    HdxShadowTaskParams shadowParams;
    shadowParams.cullStyle = HdCullStyleNothing;

    _taskController->SetShadowParams(shadowParams);
#endif

    // Default color in usdview.
    _taskController->SetSelectionColor(_colorSelectionHighlightColor);
    _taskController->SetEnableSelection(_colorSelectionHighlight);

    renderFrame();

    // This causes issues with the embree delegate and potentially others.
    if (_wireframeSelectionHighlight &&
        _rendererName == _tokens->HdStreamRendererPlugin) {
        if (!_selectionCollection.GetRootPaths().empty()) {
            _taskController->SetCollection(_selectionCollection);
            renderFrame();
            _taskController->SetCollection(_renderCollection);
        }
    }

    for (auto& it: _delegates) {
        it->PostFrame();
    }

    if (!_taskController->IsConverged()) {
        MGlobal::executeCommandOnIdle("refresh -f");
    }

    return MStatus::kSuccess;
}

void
HdMayaRenderOverride::InitHydraResources() {
    _rendererPlugin = HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererName);
    auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
    _renderIndex = HdRenderIndex::New(renderDelegate);
    int delegateId = 0;
    for (const auto& creator: HdMayaDelegateRegistry::GetDelegateCreators()) {
        if (creator == nullptr) { continue; }
        auto newDelegate = creator(_renderIndex,
                _ID.AppendChild(TfToken(
                        TfStringPrintf("_Delegate_%i_%p",
                                delegateId++, this))));
        if (newDelegate) { _delegates.push_back(newDelegate); }
    }
    _taskController = new HdxTaskController(
        _renderIndex,
        _ID.AppendChild(TfToken(
            TfStringPrintf("_UsdImaging_%s_%p", TfMakeValidIdentifier(_rendererName.GetText()).c_str(), this))));

#ifdef LUMA_USD_BUILD
    _taskController->SetEnableShadows(true);
#endif
    VtValue selectionTrackerValue(_selectionTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selectionTrackerValue);
    _preferSimpleLight = _rendererName == _tokens->HdStreamRendererPlugin;
    for (auto& it: _delegates) {
        it->SetPreferSimpleLight(_preferSimpleLight);
        it->Populate();
    }

    _renderIndex->GetChangeTracker().AddCollection(
            _selectionCollection.GetName());
    SelectionChanged();

    _initializedViewport = true;
}

void
HdMayaRenderOverride::ClearHydraResources() {
    _delegates.clear();

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

void
HdMayaRenderOverride::ClearHydraCallback(void*) {
    GetInstance().ClearHydraResources();
}

void
HdMayaRenderOverride::SelectionChanged() {
    MSelectionList sel;
    if (!TF_VERIFY(MGlobal::getActiveSelectionList(sel))) { return; }
    SdfPathVector selectedPaths;
    for (auto& it: _delegates) {
        it->PopulateSelectedPaths(sel, selectedPaths);
    }
    _selectionCollection.SetRootPaths(selectedPaths);

    auto* selection = new HdSelection;
    for (auto& it: _delegates) {
        it->PopulateSelectedPaths(sel, selection);
    }
    _selectionTracker->SetSelection(HdSelectionSharedPtr(selection));
}

void
HdMayaRenderOverride::SelectionChangedCallback(void*) {
    GetInstance().SelectionChanged();
}

MHWRender::DrawAPI
HdMayaRenderOverride::supportedDrawAPIs() const {
    return MHWRender::kOpenGLCoreProfile | MHWRender::kOpenGL;
}

MStatus
HdMayaRenderOverride::setup(const MString& destination) {
    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer == nullptr) {
        return MStatus::kFailure;
    }

    if (_operations.empty()) {
        _operations.push_back(new HdMayaSceneRender("HydraRenderOverride_Scene"));
        _operations.push_back(new HdMayaRender("HydraRenderOverride_Hydra", this));
        _operations.push_back(new HdMayaManipulatorRender("HydraRenderOverride_Manipulator"));
        _operations.push_back(new MHWRender::MHUDRender());
        auto* presentTarget = new MHWRender::MPresentTarget("HydraRenderOverride_Present");
        presentTarget->setPresentDepth(true);
        presentTarget->setTargetBackBuffer(MHWRender::MPresentTarget::kCenterBuffer);
        _operations.push_back(presentTarget);
    }

    return MS::kSuccess;
}

MStatus
HdMayaRenderOverride::HdMayaRenderOverride::cleanup() {
    _currentOperation = -1;
    return MS::kSuccess;
}

bool
HdMayaRenderOverride::startOperationIterator() {
    _currentOperation = 0;
    return true;
}

MHWRender::MRenderOperation*
HdMayaRenderOverride::renderOperation() {
    if (_currentOperation >= 0 &&
        _currentOperation < static_cast<int>(_operations.size())) {
        return _operations[_currentOperation];
    }
    return nullptr;
}

bool
HdMayaRenderOverride::nextRenderOperation() {
    return ++_currentOperation < static_cast<int>(_operations.size());
}

PXR_NAMESPACE_CLOSE_SCOPE
