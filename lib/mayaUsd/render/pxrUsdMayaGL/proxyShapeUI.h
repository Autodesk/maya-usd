//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYAGL_PROXY_SHAPE_UI_H
#define PXRUSDMAYAGL_PROXY_SHAPE_UI_H

/// \file pxrUsdMayaGL/proxyShapeUI.h

// XXX: With Maya versions up through 2019 on Linux, M3dView.h ends up
// indirectly including an X11 header that #define's "Bool" as int:
//   - <maya/M3dView.h> includes <maya/MNativeWindowHdl.h>
//   - <maya/MNativeWindowHdl.h> includes <X11/Intrinsic.h>
//   - <X11/Intrinsic.h> includes <X11/Xlib.h>
//   - <X11/Xlib.h> does: "#define Bool int"
// This can cause compilation issues if <pxr/usd/sdf/types.h> is included
// afterwards, so to fix this, we ensure that it gets included first.
//
// The X11 include appears to have been removed in Maya 2020+, so this should
// no longer be an issue with later versions.
#include <mayaUsd/base/api.h>
#include <mayaUsd/render/pxrUsdMayaGL/usdProxyShapeAdapter.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/types.h>

#include <maya/M3dView.h>
#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionList.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaProxyShapeUI : public MPxSurfaceShapeUI
{
public:
    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    void getDrawRequests(
        const MDrawInfo&   drawInfo,
        bool               objectAndActiveOnly,
        MDrawRequestQueue& requests) override;

    MAYAUSD_CORE_PUBLIC
    void draw(const MDrawRequest& request, M3dView& view) const override;

    MAYAUSD_CORE_PUBLIC
    bool select(
        MSelectInfo&    selectInfo,
        MSelectionList& selectionList,
        MPointArray&    worldSpaceSelectPts) const override;

private:
    UsdMayaProxyShapeUI();
    ~UsdMayaProxyShapeUI() override;

    UsdMayaProxyShapeUI(const UsdMayaProxyShapeUI&) = delete;
    UsdMayaProxyShapeUI& operator=(const UsdMayaProxyShapeUI&) = delete;

    // Note that MPxSurfaceShapeUI::select() is declared as const, so we
    // must declare _shapeAdapter as mutable so that we're able to modify
    // it.
    mutable PxrMayaHdUsdProxyShapeAdapter _shapeAdapter;

    // In Viewport 2.0, the MPxDrawOverride destructor is called when its
    // shape is deleted, in which case the shape's adapter is removed from
    // the batch renderer. In the legacy viewport though, that's not the
    // case. The MPxSurfaceShapeUI destructor may not get called until the
    // scene is closed or Maya exits. As a result, MPxSurfaceShapeUI
    // objects must listen for node removal messages from Maya and remove
    // their shape adapter from the batch renderer if their node is the one
    // being removed. Otherwise, deleted shapes may still be drawn.
    static void _OnNodeRemoved(MObject& node, void* clientData);

    MCallbackId _onNodeRemovedCallbackId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
