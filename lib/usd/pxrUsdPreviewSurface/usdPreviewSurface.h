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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_H

/// \file usdPreviewSurface.h

#include "api.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS                                          \
    ((ClearcoatAttrName, "clearcoat"))                                                           \
    ((ClearcoatRoughnessAttrName, "clearcoatRoughness"))                                         \
    ((DiffuseColorAttrName, "diffuseColor"))                                                     \
    ((DisplacementAttrName, "displacement"))                                                     \
    ((EmissiveColorAttrName, "emissiveColor"))                                                   \
    ((IorAttrName, "ior"))                                                                       \
    ((MetallicAttrName, "metallic"))                                                             \
    ((NormalAttrName, "normal"))                                                                 \
    ((OcclusionAttrName, "occlusion"))                                                           \
    ((OpacityAttrName, "opacity"))                                                               \
    ((OpacityThresholdAttrName, "opacityThreshold"))                                             \
    ((RoughnessAttrName, "roughness"))                                                           \
    ((SpecularColorAttrName, "specularColor"))                                                   \
    ((DisplayCPVAttrName, "displayCPV"))                                                         \
    ((UseSpecularWorkflowAttrName, "useSpecularWorkflow"))                                       \
    ((OutColorAttrName, "outColor"))                                                             \
    ((OutTransparencyAttrName, "outTransparency"))                                               \
    ((OutTransparencyOnAttrName, "outTransparencyOn"))                                           \
    ((niceName, "USD Preview Surface"))                                                          \
    ((exportDescription, "Exports the bound shader as a USD preview surface UsdShade network.")) \
    ((importDescription, "Search for a USD preview surface UsdShade network to import."))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceTokens,
    PXRUSDPREVIEWSURFACE_API,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS);

class PxrMayaUsdPreviewSurface : public MPxNode
{
public:
    PXRUSDPREVIEWSURFACE_API
    static void* creator();

    PXRUSDPREVIEWSURFACE_API
    static MStatus initialize();

    PXRUSDPREVIEWSURFACE_API
    void postConstructor() override;

    PXRUSDPREVIEWSURFACE_API
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

    PXRUSDPREVIEWSURFACE_API
    MStatus connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;

private:
    PxrMayaUsdPreviewSurface();
    ~PxrMayaUsdPreviewSurface() override;

    PxrMayaUsdPreviewSurface(const PxrMayaUsdPreviewSurface&) = delete;
    PxrMayaUsdPreviewSurface& operator=(const PxrMayaUsdPreviewSurface&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
