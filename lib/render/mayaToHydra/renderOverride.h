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
#ifndef __MTOH_VIEW_OVERRIDE_H__
#define __MTOH_VIEW_OVERRIDE_H__

#include <pxr/imaging/glf/glew.h>
#include <pxr/pxr.h>

#include <hdmaya/hdmaya.h>

#include <pxr/base/tf/singleton.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdSt/renderDelegate.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MMessage.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include "../../usd/hdmaya/delegates/delegate.h"
#include "../../usd/hdmaya/delegates/params.h"

#include "defaultLightDelegate.h"
#include "renderGlobals.h"
#include "utils.h"

#include <atomic>
#include <chrono>
#include <mutex>

#if HDMAYA_UFE_BUILD
#include <ufe/observer.h>
#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

class MtohRenderOverride : public MHWRender::MRenderOverride {
public:
    MtohRenderOverride(const MtohRendererDescription& desc);
    ~MtohRenderOverride() override;

    static void UpdateRenderGlobals();

    /// The names of all render delegates that are being used by at least
    /// one modelEditor panel.
    static std::vector<MString> AllActiveRendererNames();

    /// Returns a list of rprims in the render index for the given render
    /// delegate.
    ///
    /// Intended mostly for use in debugging and testing.
    static SdfPathVector RendererRprims(
        TfToken rendererName, bool visibleOnly = false);

    /// Returns the scene delegate id for the given render delegate and
    /// scene delegate names.
    ///
    /// Intended mostly for use in debugging and testing.
    static SdfPath RendererSceneDelegateId(
        TfToken rendererName, TfToken sceneDelegateName);

    MStatus Render(const MHWRender::MDrawContext& drawContext);

    void ClearHydraResources();
    void SelectionChanged();

    MString uiName() const override {
        return MString(_rendererDesc.displayName.GetText());
    }

    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MStatus setup(const MString& destination) override;
    MStatus cleanup() override;

    bool startOperationIterator() override;
    MHWRender::MRenderOperation* renderOperation() override;
    bool nextRenderOperation() override;

private:
    typedef std::pair<MString, MCallbackIdArray> PanelCallbacks;
    typedef std::vector<PanelCallbacks> PanelCallbacksList;

    static MtohRenderOverride* _GetByName(TfToken rendererName);

    void _InitHydraResources();
    void _RemovePanel(MString panelName);
    void _SelectionChanged();
    void _DetectMayaDefaultLighting(const MHWRender::MDrawContext& drawContext);
    void _UpdateRenderGlobals();
    void _UpdateRenderDelegateOptions();

    inline PanelCallbacksList::iterator _FindPanelCallbacks(MString panelName) {
        // There should never be that many render panels, so linear iteration
        // should be fine

        return std::find_if(
            _renderPanelCallbacks.begin(), _renderPanelCallbacks.end(),
            [&panelName](const PanelCallbacks& item) {
                return item.first == panelName;
            });
    }

    // Callbacks
    static void _ClearHydraCallback(void* data);
    static void _TimerCallback(float, float, void* data);
    static void _SelectionChangedCallback(void* data);
    static void _PanelDeletedCallback(const MString& panelName, void* data);
    static void _RendererChangedCallback(
        const MString& panelName, const MString& oldRenderer,
        const MString& newRenderer, void* data);
    static void _RenderOverrideChangedCallback(
        const MString& panelName, const MString& oldOverride,
        const MString& newOverride, void* data);

    MtohRendererDescription _rendererDesc;

    std::vector<MHWRender::MRenderOperation*> _operations;
    std::vector<MCallbackId> _callbacks;
    PanelCallbacksList _renderPanelCallbacks;
    MtohRenderGlobals _globals;

    std::mutex _convergenceMutex;
    std::chrono::system_clock::time_point _lastRenderTime;
    std::atomic<bool> _needsClear;

    HdEngine _engine;
    HdxRendererPlugin* _rendererPlugin = nullptr;
    HdxTaskController* _taskController = nullptr;
    HdRenderIndex* _renderIndex = nullptr;
    std::unique_ptr<MtohDefaultLightDelegate> _defaultLightDelegate = nullptr;
    HdxSelectionTrackerSharedPtr _selectionTracker;
    HdRprimCollection _renderCollection;
    HdRprimCollection _selectionCollection;
    GlfSimpleLight _defaultLight;

    std::vector<HdMayaDelegatePtr> _delegates;

    SdfPath _ID;

    int _currentOperation = -1;

    const bool _isUsingHdSt = false;
    bool _initializedViewport = false;
    bool _hasDefaultLighting = false;
    bool _renderGlobalsHaveChanged = false;
    bool _selectionChanged = true;
    bool _isConverged = false;

#if HDMAYA_UFE_BUILD
    UFE_NS::Observer::Ptr _ufeSelectionObserver;
#endif // HDMAYA_UFE_BUILD
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MTOH_VIEW_OVERRIDE_H__
