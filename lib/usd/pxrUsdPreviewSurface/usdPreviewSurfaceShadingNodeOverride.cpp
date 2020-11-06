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
#include "usdPreviewSurfaceShadingNodeOverride.h"

#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MObject.h>
#include <maya/MPxSurfaceShadingNodeOverride.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurfacePlugin.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
const MString _transparencyParameter = "dummyTransparency"; //!< Transparency parameter
}

/* static */
MHWRender::MPxSurfaceShadingNodeOverride*
PxrMayaUsdPreviewSurfaceShadingNodeOverride::creator(const MObject& obj)
{
    // Shader fragments can only be registered after VP2 initialization, thus the function cannot
    // be called when loading plugin (which can happen before VP2 initialization in the case of
    // command-line rendering). The fragments will be deregistered when plugin is unloaded.
    PxrMayaUsdPreviewSurfacePlugin::registerFragments();

    return new PxrMayaUsdPreviewSurfaceShadingNodeOverride(obj);
}

PxrMayaUsdPreviewSurfaceShadingNodeOverride::PxrMayaUsdPreviewSurfaceShadingNodeOverride(
    const MObject& obj)
    : MPxSurfaceShadingNodeOverride(obj)
{
}

/* virtual */
PxrMayaUsdPreviewSurfaceShadingNodeOverride::~PxrMayaUsdPreviewSurfaceShadingNodeOverride() { }

/* virtual */
MString PxrMayaUsdPreviewSurfaceShadingNodeOverride::primaryColorParameter() const
{
    return "diffuseColor";
}

/* virtual */
MString PxrMayaUsdPreviewSurfaceShadingNodeOverride::transparencyParameter() const
{
    // See getCustomMappings() implementation for more details.
    return _transparencyParameter;
}

/* virtual */
MString PxrMayaUsdPreviewSurfaceShadingNodeOverride::bumpAttribute() const { return "normal"; }

/* virtual */
MHWRender::DrawAPI PxrMayaUsdPreviewSurfaceShadingNodeOverride::supportedDrawAPIs() const
{
    return MHWRender::kAllDevices;
}

/* virtual */
MString PxrMayaUsdPreviewSurfaceShadingNodeOverride::fragmentName() const
{
    return HdVP2ShaderFragmentsTokens->SurfaceFragmentGraphName.GetText();
}

/* virtual */
void PxrMayaUsdPreviewSurfaceShadingNodeOverride::getCustomMappings(
    MHWRender::MAttributeParameterMappingList& mappings)
{
    // The control on the Maya shader is 'opacity' (1.0 is opaque), but Maya
    // prefers to work in terms of transparency (0.0 is opaque). We want Maya
    // to manage enabling or disabling transparency of the shader instance for
    // us, so we map the "outTransparencyOn" attribute on the shader (which the
    // shader computes from "opacity") to the "dummyTransparency" parameter of
    // the fragment graph. transparencyParameter() above then instructs Maya to
    // execute transparency test on the value of the "dummyTransparency" parameter
    // (a positive value means to enable transparency whilst a non-positive value
    // means to disable transparency). Note the "opacity" parameter of the shader
    // fragment carries the alpha value that is actually used in shading.
    MHWRender::MAttributeParameterMapping transparencyMapping(
        _transparencyParameter, "outTransparencyOn", true, true);
    mappings.append(transparencyMapping);
}

PXR_NAMESPACE_CLOSE_SCOPE
