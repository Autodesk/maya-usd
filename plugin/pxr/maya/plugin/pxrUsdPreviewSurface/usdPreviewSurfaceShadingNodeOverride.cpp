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
#include <pxrUsdPreviewSurface/usdPreviewSurfaceShadingNodeOverride.h>

#include <pxr/pxr.h>

#include <pxr/base/tf/staticTokens.h>

#include <maya/MObject.h>
#include <maya/MPxSurfaceShadingNodeOverride.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceShadingNodeTokens,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADING_NODE_OVERRIDE_TOKENS);


/* static */
MHWRender::MPxSurfaceShadingNodeOverride*
PxrMayaUsdPreviewSurfaceShadingNodeOverride::creator(const MObject& obj)
{
    return new PxrMayaUsdPreviewSurfaceShadingNodeOverride(obj);
}

PxrMayaUsdPreviewSurfaceShadingNodeOverride::PxrMayaUsdPreviewSurfaceShadingNodeOverride(
        const MObject& obj) :
    MPxSurfaceShadingNodeOverride(obj)
{
}

/* virtual */
PxrMayaUsdPreviewSurfaceShadingNodeOverride::~PxrMayaUsdPreviewSurfaceShadingNodeOverride()
{
}

/* virtual */
MString
PxrMayaUsdPreviewSurfaceShadingNodeOverride::primaryColorParameter() const
{
    return "diffuseColor";
}

/* virtual */
MString
PxrMayaUsdPreviewSurfaceShadingNodeOverride::transparencyParameter() const
{
    return "transparency";
}

/* virtual */
MString
PxrMayaUsdPreviewSurfaceShadingNodeOverride::bumpAttribute() const
{
    return "normal";
}

/* virtual */
MHWRender::DrawAPI
PxrMayaUsdPreviewSurfaceShadingNodeOverride::supportedDrawAPIs() const
{
    return MHWRender::kAllDevices;
}

/* virtual */
MString
PxrMayaUsdPreviewSurfaceShadingNodeOverride::fragmentName() const
{
    return PxrMayaUsdPreviewSurfaceShadingNodeTokens->SurfaceFragmentGraphName.GetText();
}

/* virtual */
void
PxrMayaUsdPreviewSurfaceShadingNodeOverride::getCustomMappings(
        MHWRender::MAttributeParameterMappingList& mappings)
{
    // The control on the Maya shader is 'opacity' (1.0 is opaque), but Maya
    // prefers to work in terms of transparency (0.0 is opaque). We want Maya
    // to manage enabling or disabling transparency of the shader instance for
    // us, so we map the "outTransparency" attribute on the shader (which the
    // shader computes from "opacity") to the "transparency" parameter of the
    // fragment graph. transparencyParameter() above then instructs Maya to
    // watch for changes in value for that parameter.
    MHWRender::MAttributeParameterMapping transparencyMapping(
        "transparency", "outTransparency", true, true);
    mappings.append(transparencyMapping);
}


PXR_NAMESPACE_CLOSE_SCOPE
