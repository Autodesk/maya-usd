#include "viewportRenderer.h"

#include <maya/MRenderingInfo.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/glf/contextCaps.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/imaging/hdx/tokens.h>

#include <memory>
#include <mutex>
#include <exception>

#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    std::unique_ptr<HdMayaViewportRenderer> _viewportRenderer = nullptr;
    std::mutex _viewportRendererLock;
    constexpr auto HDMAYA_DEFAULT_RENDERER_PLUGIN_NAME = "HDMAYA_DEFAULT_RENDERER_PLUGIN";

    TfToken _getDefaultRenderer() {
        const auto l = HdMayaViewportRenderer::GetRendererPlugins();
        if (l.empty()) { return {}; }
        const auto* defaultRenderer = getenv(HDMAYA_DEFAULT_RENDERER_PLUGIN_NAME);
        if (defaultRenderer == nullptr) { return l[0]; }
        const TfToken defaultRendererToken(defaultRenderer);
        if (std::find(l.begin(), l.end(), defaultRendererToken) != l.end()) { return defaultRendererToken; }
        return l[0];
    }
}

HdMayaViewportRenderer::HdMayaViewportRenderer() :
    MViewportRenderer("HdMayaViewportRenderer"),
    _selectionTracker(new HdxSelectionTracker) {
    fUIName.set("Hydra Viewport Renderer");
    fRenderingOverride = MViewportRenderer::kOverrideThenStandard;

    _ID = SdfPath("/HdMayaViewportRenderer").AppendChild(TfToken(TfStringPrintf("_HdMaya_%p", this)));
    _rendererName = _getDefaultRenderer();
    // This is a critical error, so we don't allow the construction
    // of the viewport renderer plugin if there is no renderer plugin
    // present.
    if (_rendererName.IsEmpty()) {
        throw std::runtime_error("No default renderer is available!");
    }
}

HdMayaViewportRenderer::~HdMayaViewportRenderer() {

}

HdMayaViewportRenderer* HdMayaViewportRenderer::GetInstance() {
    std::lock_guard<std::mutex> lg(_viewportRendererLock);
    if (_viewportRenderer == nullptr) {
        try {
            auto* vp = new HdMayaViewportRenderer();
            _viewportRenderer.reset(vp);
        } catch (std::runtime_error& e) {
            _viewportRenderer = nullptr;
            return nullptr;
        }
    }
    return _viewportRenderer.get();
}

void HdMayaViewportRenderer::Cleanup() {
    _viewportRenderer = nullptr;
}

MStatus HdMayaViewportRenderer::initialize() {
    _initializedViewport = true;
    GlfGlewInit();
    InitHydraResources();
    return MStatus::kSuccess;
}

MStatus HdMayaViewportRenderer::uninitialize() {
    ClearHydraResources();
    return MStatus::kSuccess;
}

void HdMayaViewportRenderer::InitHydraResources() {
    _rendererPlugin = HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererName);
    auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
    _renderIndex = HdRenderIndex::New(renderDelegate);
    int delegateId = 0;
    for (auto creator: HdMayaDelegateRegistry::GetDelegateCreators()) {
        if (creator == nullptr) { continue; }
        _delegates.push_back(creator(_renderIndex,
            _ID.AppendChild(TfToken(
                TfStringPrintf("_Delegate_%i_%p", delegateId++, this)))));
    }
    _taskController = new HdxTaskController(
        _renderIndex,
        _ID.AppendChild(TfToken(
            TfStringPrintf("_UsdImaging_%s_%p", TfMakeValidIdentifier(_rendererName.GetText()).c_str(), this))));

    HdxRenderTaskParams params;
    params.enableLighting = true;
    params.enableHardwareShading = true;
    _taskController->SetRenderParams(params);
    _taskController->SetEnableSelection(false);
#ifdef LUMA_USD_BUILD
    _taskController->SetEnableShadows(true);
#endif
    VtValue selectionTrackerValue(_selectionTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selectionTrackerValue);
}

void HdMayaViewportRenderer::ClearHydraResources() {
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


    _isPopulated = false;
}

TfTokenVector HdMayaViewportRenderer::GetRendererPlugins() {
    HfPluginDescVector pluginDescs;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

    TfTokenVector ret; ret.reserve(pluginDescs.size());
    for (const auto& desc: pluginDescs) {
        ret.emplace_back(desc.id);
    }
    return ret;
}

std::string HdMayaViewportRenderer::GetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdxRendererPluginRegistry::GetInstance().GetPluginDesc(id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

void HdMayaViewportRenderer::ChangeRendererPlugin(const TfToken& id) {
    auto* instance = GetInstance();
    if (instance->_rendererName == id) { return; }
    const auto renderers = GetRendererPlugins();
    if (std::find(renderers.begin(), renderers.end(), id) == renderers.end()) { return; }
    instance->_rendererName = id;
    if (instance->_initializedViewport) {
        instance->ClearHydraResources();
        instance->InitHydraResources();
    }
}

int HdMayaViewportRenderer::GetFallbackShadowMapResolution() {
    return GetInstance()->_fallbackShadowMapResolution;
}

void HdMayaViewportRenderer::SetFallbackShadowMapResolution(int resolution) {
    // TODO: dirty light adapters
    GetInstance()->_fallbackShadowMapResolution = resolution;
}

MStatus HdMayaViewportRenderer::render(const MRenderingInfo& renderInfo) {
    if (renderInfo.renderingAPI() != MViewportRenderer::kOpenGL) {
        return MS::kFailure;
    }

    // TODO: dynamically update everything. For now we are just
    // populating the delegate once.
    if (!_isPopulated) {
        for (auto& it: _delegates) {
            it->Populate();
        }
        _isPopulated = true;
    }

    for (auto& it: _delegates) {
        it->PreFrame();
    }

    renderInfo.renderTarget().makeTargetCurrent();

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // We need some empty vao to get the core profile running in some cases.
    const auto isCoreProfileContext = GlfContextCaps::GetInstance().coreProfile;
    GLuint vao;
    if (isCoreProfileContext) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    } else {
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const auto originX = renderInfo.originX();
    const auto originY = renderInfo.originY();
    const auto width = renderInfo.width();
    const auto height = renderInfo.height();

    GfVec4d viewport(originX, originY, width, height);
    _taskController->SetCameraMatrices(
        getGfMatrixFromMaya(renderInfo.viewMatrix()),
        getGfMatrixFromMaya(renderInfo.projectionMatrix()));
    _taskController->SetCameraViewport(viewport);

    _engine.Execute(*_renderIndex, _taskController->GetTasks(HdxTaskSetTokens->colorRender));

    if (isCoreProfileContext) {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
    } else {
        glPopAttrib(); // GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT
    }

    for (auto& it: _delegates) {
        it->PostFrame();
    }

    return MStatus::kSuccess;
}

bool HdMayaViewportRenderer::nativelySupports(MViewportRenderer::RenderingAPI api, float /*version*/) {
    return MViewportRenderer::kOpenGL == api;
}

bool HdMayaViewportRenderer::override(MViewportRenderer::RenderingOverride override) {
    return fRenderingOverride == override;
}

unsigned int
HdMayaViewportRenderer::overrideThenStandardExclusion() const {
    return ~static_cast<unsigned int>(kExcludeManipulators | kExcludeLights | kExcludeSelectHandles);
}

PXR_NAMESPACE_CLOSE_SCOPE
