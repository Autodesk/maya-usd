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
#ifndef HDMAYA_DELEGATE_H
#define HDMAYA_DELEGATE_H

#include <memory>

#include <maya/MDrawContext.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionContext.h>
#include <maya/MSelectionList.h>

#include <pxr/pxr.h>
#include <pxr/imaging/glf/glew.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/usd/sdf/path.h>

#include <hdMaya/api.h>
#include <hdMaya/delegates/params.h>

#include <pxr/imaging/hd/rendererPlugin.h>

#if WANT_UFE_BUILD
#include <ufe/selection.h>
#endif // WANT_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate {
public:
    struct InitData {
        inline InitData(
            TfToken nameIn, HdEngine& engineIn, HdRenderIndex* renderIndexIn,
            HdRendererPlugin* rendererPluginIn,
            HdxTaskController* taskControllerIn, const SdfPath& delegateIDIn,
            bool isHdStIn)
            : name(nameIn),
              engine(engineIn),
              renderIndex(renderIndexIn),
              rendererPlugin(rendererPluginIn),
              taskController(taskControllerIn),
              delegateID(delegateIDIn),
              isHdSt(isHdStIn) {}

        TfToken name;
        HdEngine& engine;
        HdRenderIndex* renderIndex;
        HdRendererPlugin* rendererPlugin;
        HdxTaskController* taskController;
        SdfPath delegateID;
        bool isHdSt;
    };

    HDMAYA_API
    HdMayaDelegate(const InitData& initData);
    HDMAYA_API
    virtual ~HdMayaDelegate() = default;

    virtual void Populate() = 0;
    virtual void PreFrame(const MHWRender::MDrawContext& context) {}
    virtual void PostFrame() {}

    HDMAYA_API
    virtual void SetParams(const HdMayaParams& params);
    const HdMayaParams& GetParams() { return _params; }

    const SdfPath& GetMayaDelegateID() { return _mayaDelegateID; }
    TfToken GetName() { return _name; }
    bool IsHdSt() { return _isHdSt; }

    virtual void PopulateSelectedPaths(
        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection) {}

#if WANT_UFE_BUILD
    virtual void PopulateSelectedPaths(
        const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection) {}

    virtual bool SupportsUfeSelection() { return false; }
#endif // WANT_UFE_BUILD

#if MAYA_API_VERSION >= 20210000
    virtual void PopulateSelectionList(
        const HdxPickHitVector& hits,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList& mayaSelection,
        MPointArray& worldSpaceHitPts)
    {
    }
#endif

    void SetLightsEnabled(const bool enabled) { _lightsEnabled = enabled; }
    bool GetLightsEnabled() { return _lightsEnabled; }

    inline HdEngine& GetEngine() { return _engine; }
    inline HdxTaskController* GetTaskController() { return _taskController; }

private:
    HdMayaParams _params;

    // Note that because there may not be a 1-to-1 relationship between
    // a HdMayaDelegate and a HdSceneDelegate, this may be different than
    // "the" scene delegate id.  In the case of HdMayaSceneDelegate,
    // which inherits from HdSceneDelegate, they are the same; but for, ie,
    // HdMayaALProxyDelegate, for which there are multiple HdSceneDelegates
    // for each HdMayaDelegate, the _mayaDelegateID is different from each
    // HdSceneDelegate's id.
    const SdfPath _mayaDelegateID;
    TfToken _name;
    HdEngine& _engine;
    HdxTaskController* _taskController;
    bool _isHdSt = false;
    bool _lightsEnabled = true;
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_DELEGATE_H
