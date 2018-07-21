#ifndef __HDMAYA_VIEW_OVERRIDE_H__
#define __HDMAYA_VIEW_OVERRIDE_H__

#include <pxr/pxr.h>
#include <pxr/imaging/glf/glew.h>

#include <pxr/base/tf/singleton.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdSt/renderDelegate.h>

#include <pxr/usdImaging/usdImagingGL/hdEngine.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>

#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MMessage.h>

#include <hdmaya/delegates/delegate.h>
#include <hdmaya/delegates/params.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaRenderOverride : public MHWRender::MRenderOverride, TfSingleton<HdMayaRenderOverride> {
    friend class TfSingleton<HdMayaRenderOverride>;
    HdMayaRenderOverride();
public:
    ~HdMayaRenderOverride() override;
    static HdMayaRenderOverride& GetInstance() {
        return TfSingleton<HdMayaRenderOverride>::GetInstance();
    }

    static void DeleteInstance() {
        return TfSingleton<HdMayaRenderOverride>::DeleteInstance();
    }

    static bool CurrentlyExists() {
        return TfSingleton<HdMayaRenderOverride>::CurrentlyExists();
    }

    static TfTokenVector GetRendererPlugins();
    static std::string GetRendererPluginDisplayName(const TfToken& id);
    static void ChangeRendererPlugin(const TfToken& id);
    static int GetMaximumShadowMapResolution();
    static void SetMaximumShadowMapResolution(int resolution);
    static int GetTextureMemoryPerTexture();
    static void SetTextureMemoryPerTexture(int memory);

    MStatus Render(const MHWRender::MDrawContext& drawContext);

    void ClearHydraResources();

    MString uiName() const override {
        return MString("Hydra Viewport Override");
    }

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString & destination) override;
    MStatus cleanup() override;

    bool startOperationIterator() override;
    MHWRender::MRenderOperation* renderOperation() override;
    bool nextRenderOperation() override;
private:
    void InitHydraResources();
    static void ClearHydraCallback(void*);
    void SelectionChanged();
    static void SelectionChangedCallback(void*);

    std::vector<MHWRender::MRenderOperation*> _operations;
    std::vector<MCallbackId> _callbacks;
    HdMayaParams _params;

    HdEngine _engine;
    HdxRendererPlugin* _rendererPlugin = nullptr;
    HdxTaskController* _taskController = nullptr;
    HdRenderIndex* _renderIndex = nullptr;
    HdxSelectionTrackerSharedPtr _selectionTracker;
    HdRprimCollection _renderCollection;
    HdRprimCollection _selectionCollection;

    std::vector<HdMayaDelegatePtr> _delegates;

    SdfPath _ID;
    TfToken _rendererName;

    int _currentOperation = -1;

    bool _initializedViewport = false;
    bool _preferSimpleLight = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_VIEW_OVERRIDE_H__
