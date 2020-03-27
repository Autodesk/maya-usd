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

#include "pxr/pxr.h"
#include "../../base/api.h"
#include "./usdProxyShapeAdapter.h"

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
                const MDrawInfo& drawInfo,
                bool objectAndActiveOnly,
                MDrawRequestQueue& requests) override;

        MAYAUSD_CORE_PUBLIC
        void draw(
                const MDrawRequest& request,
                M3dView& view) const override;

        MAYAUSD_CORE_PUBLIC
        bool select(
                MSelectInfo& selectInfo,
                MSelectionList& selectionList,
                MPointArray& worldSpaceSelectPts) const override;

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
