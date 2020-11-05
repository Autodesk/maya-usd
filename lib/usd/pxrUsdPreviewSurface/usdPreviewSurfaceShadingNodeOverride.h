//
// Copyright 2018 Pixar
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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_H

/// \file usdPreviewSurfaceShadingNodeOverride.h
#include "api.h"

#include <pxr/pxr.h>

#include <maya/MObject.h>
#include <maya/MPxSurfaceShadingNodeOverride.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPreviewSurfaceShadingNodeOverride : public MHWRender::MPxSurfaceShadingNodeOverride
{
public:
    PXRUSDPREVIEWSURFACE_API
    static MHWRender::MPxSurfaceShadingNodeOverride* creator(const MObject& obj);

    PXRUSDPREVIEWSURFACE_API
    PxrMayaUsdPreviewSurfaceShadingNodeOverride(const MObject& obj);
    PXRUSDPREVIEWSURFACE_API
    ~PxrMayaUsdPreviewSurfaceShadingNodeOverride() override;

    // MPxSurfaceShadingNodeOverride Overrides.

    PXRUSDPREVIEWSURFACE_API
    MString primaryColorParameter() const override;

    PXRUSDPREVIEWSURFACE_API
    MString transparencyParameter() const override;

    PXRUSDPREVIEWSURFACE_API
    MString bumpAttribute() const override;

    // MPxShadingNodeOverride Overrides.

    PXRUSDPREVIEWSURFACE_API
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    PXRUSDPREVIEWSURFACE_API
    MString fragmentName() const override;

    PXRUSDPREVIEWSURFACE_API
    void getCustomMappings(MHWRender::MAttributeParameterMappingList& mappings) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
