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
#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <hdmaya/hdmaya.h>

#include <pxr/imaging/glf/glew.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MSelectionList.h>

#include <memory>

#include <hdmaya/api.h>
#include <hdmaya/delegates/params.h>

#if HDMAYA_UFE_BUILD
#include <ufe/selection.h>
#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate {
public:
    struct InitData {
        inline InitData(
            HdEngine& engineIn, HdRenderIndex* renderIndexIn,
            HdxRendererPlugin* rendererPluginIn,
            HdxTaskController* taskControllerIn, const SdfPath& delegateIDIn,
            bool isHdStIn)
            : engine(engineIn),
              renderIndex(renderIndexIn),
              rendererPlugin(rendererPluginIn),
              taskController(taskControllerIn),
              delegateID(delegateIDIn),
              isHdSt(isHdStIn) {}

        HdEngine& engine;
        HdRenderIndex* renderIndex;
        HdxRendererPlugin* rendererPlugin;
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

    bool IsHdSt() { return _isHdSt; }

    virtual void PopulateSelectedPaths(
        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
        HdSelection* selection) {}

#if HDMAYA_UFE_BUILD
    virtual void PopulateSelectedPaths(
        const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
        HdSelection* selection) {}

    virtual bool SupportsUfeSelection() { return false; }
#endif // HDMAYA_UFE_BUILD

    void SetLightsEnabled(const bool enabled) { _lightsEnabled = enabled; }
    bool GetLightsEnabled() { return _lightsEnabled; }

private:
    HdMayaParams _params;
    bool _isHdSt = false;
    bool _lightsEnabled = true;
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
