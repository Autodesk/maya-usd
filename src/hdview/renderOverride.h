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
#ifndef __HDMAYA_VIEW_OVERRIDE_H__
#define __HDMAYA_VIEW_OVERRIDE_H__

#include <pxr/imaging/glf/glew.h>
#include <pxr/pxr.h>

#include <pxr/base/tf/singleton.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdSt/renderDelegate.h>

#include <pxr/usdImaging/usdImagingGL/hdEngine.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>

#include <maya/MMessage.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include <hdmaya/delegates/delegate.h>
#include <hdmaya/delegates/params.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaRenderOverride : public MHWRender::MRenderOverride,
                             TfSingleton<HdMayaRenderOverride> {
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
    static bool GetWireframeSelectionHighlight();
    static void SetWireframeSelectionHighlight(bool value);
    static bool GetColorSelectionHighlight();
    static void SetColorSelectionHighlight(bool value);
    static GfVec4d GetColorSelectionHighlightColor();
    static void SetColorSelectionHighlightColor(const GfVec4d& color);

    MStatus Render(const MHWRender::MDrawContext& drawContext);

    void ClearHydraResources();

    MString uiName() const override {
        return MString("Hydra Viewport Override");
    }

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString& destination) override;
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

    GfVec4f _colorSelectionHighlightColor;

    SdfPath _ID;
    TfToken _rendererName;

    int _currentOperation = -1;

    bool _initializedViewport = false;
    bool _preferSimpleLight = false;
    bool _wireframeSelectionHighlight = true;
    bool _colorSelectionHighlight = true;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_VIEW_OVERRIDE_H__
