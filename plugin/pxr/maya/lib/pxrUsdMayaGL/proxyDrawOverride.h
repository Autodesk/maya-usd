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
#ifndef PXRUSDMAYAGL_PROXY_DRAW_OVERRIDE_H
#define PXRUSDMAYAGL_PROXY_DRAW_OVERRIDE_H

/// \file pxrUsdMayaGL/proxyDrawOverride.h

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/usdProxyShapeAdapter.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MSelectionContext.h>
#include <maya/MString.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


class UsdMayaProxyDrawOverride : public MHWRender::MPxDrawOverride
{
    public:
        PXRUSDMAYAGL_API
        static const MString drawDbClassification;

        PXRUSDMAYAGL_API
        static MHWRender::MPxDrawOverride* Creator(const MObject& obj);

        PXRUSDMAYAGL_API
        ~UsdMayaProxyDrawOverride() override;

        PXRUSDMAYAGL_API
        MHWRender::DrawAPI supportedDrawAPIs() const override;

        PXRUSDMAYAGL_API
        MMatrix transform(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        MBoundingBox boundingBox(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        bool isBounded(
                const MDagPath& objPath,
                const MDagPath& cameraPath) const override;

        PXRUSDMAYAGL_API
        bool disableInternalBoundingBoxDraw() const override;

        PXRUSDMAYAGL_API
        MUserData* prepareForDraw(
                const MDagPath& objPath,
                const MDagPath& cameraPath,
                const MHWRender::MFrameContext& frameContext,
                MUserData* oldData) override;

#if MAYA_API_VERSION >= 20180000
        PXRUSDMAYAGL_API
        bool wantUserSelection() const override;

        PXRUSDMAYAGL_API
        bool userSelect(
                MHWRender::MSelectionInfo& selectionInfo,
                const MHWRender::MDrawContext& context,
                MPoint& hitPoint,
                const MUserData* data) override;
#endif

        PXRUSDMAYAGL_API
        static void draw(
                const MHWRender::MDrawContext& context,
                const MUserData* data);

    private:
        UsdMayaProxyDrawOverride(const MObject& obj);

        UsdMayaProxyDrawOverride(const UsdMayaProxyDrawOverride&) = delete;
        UsdMayaProxyDrawOverride& operator=(const UsdMayaProxyDrawOverride&) = delete;

        PxrMayaHdUsdProxyShapeAdapter _shapeAdapter;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
