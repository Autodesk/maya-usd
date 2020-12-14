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

#include <mayaUsd/base/api.h>
#include <mayaUsd/render/pxrUsdMayaGL/usdProxyShapeAdapter.h>

#include <pxr/pxr.h>

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
    MAYAUSD_CORE_PUBLIC
    static const MString drawDbClassification;

    MAYAUSD_CORE_PUBLIC
    static MHWRender::MPxDrawOverride* Creator(const MObject& obj);

    MAYAUSD_CORE_PUBLIC
    ~UsdMayaProxyDrawOverride() override;

    MAYAUSD_CORE_PUBLIC
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    MAYAUSD_CORE_PUBLIC
    MMatrix transform(const MDagPath& objPath, const MDagPath& cameraPath) const override;

    MAYAUSD_CORE_PUBLIC
    MBoundingBox boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const override;

    MAYAUSD_CORE_PUBLIC
    bool isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const override;

    MAYAUSD_CORE_PUBLIC
    bool disableInternalBoundingBoxDraw() const override;

    MAYAUSD_CORE_PUBLIC
    MUserData* prepareForDraw(
        const MDagPath&                 objPath,
        const MDagPath&                 cameraPath,
        const MHWRender::MFrameContext& frameContext,
        MUserData*                      oldData) override;

    MAYAUSD_CORE_PUBLIC
    bool wantUserSelection() const override;

    MAYAUSD_CORE_PUBLIC
    bool userSelect(
        MHWRender::MSelectionInfo&     selectionInfo,
        const MHWRender::MDrawContext& context,
        MPoint&                        hitPoint,
        const MUserData*               data) override;

    MAYAUSD_CORE_PUBLIC
    static void draw(const MHWRender::MDrawContext& context, const MUserData* data);

private:
    UsdMayaProxyDrawOverride(const MObject& obj);

    UsdMayaProxyDrawOverride(const UsdMayaProxyDrawOverride&) = delete;
    UsdMayaProxyDrawOverride& operator=(const UsdMayaProxyDrawOverride&) = delete;

    PxrMayaHdUsdProxyShapeAdapter _shapeAdapter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
