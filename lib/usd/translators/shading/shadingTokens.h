//
// Copyright 2021 Autodesk
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
#ifndef MTLXUSDTRANSLATORS_SHADING_TOKENS_H
#define MTLXUSDTRANSLATORS_SHADING_TOKENS_H

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define TR_USD_COMMON \
    (varname)

// XXX: We duplicate these tokens here rather than create a dependency on
// usdImaging in case the plugin is being built with imaging disabled.
// If/when they move out of usdImaging to a place that is always available,
// they should be pulled from there instead.
#define TR_USD_TEXTURE \
    (UsdUVTexture) \
    (bias) \
    (fallback) \
    (file) \
    (scale) \
    (st) \
    (wrapS) \
    (wrapT) \
    (black) \
    (mirror) \
    (repeat) \
    (sourceColorSpace) \
    (raw) \
    (sRGB) \
    ((RGBOutputName, "rgb")) \
    ((RedOutputName, "r")) \
    ((GreenOutputName, "g")) \
    ((BlueOutputName, "b")) \
    ((AlphaOutputName, "a"))

#define TR_USD_PRIMVAR \
    (UsdPrimvarReader_float2) \
    (result)

#define TR_USD_XFORM \
    (UsdTransform2d) \
    (in) \
    (rotation) \
    (translation)

// clang-format on

TF_DECLARE_PUBLIC_TOKENS(TrUsdTokens, , TR_USD_COMMON TR_USD_TEXTURE TR_USD_PRIMVAR TR_USD_XFORM);

// clang-format off
#define TR_MAYA_MATERIALS \
    (usdPreviewSurface) \
    (eccentricity) \
    (specularRollOff) \
    (cosinePower) \
    (roughness) \
    (color) \
    (transparency) \
    (diffuse) \
    (incandescence) \
    (normalCamera) \
    (displacement) \
    (outColor) \
    (outColorR) \
    (outColorG) \
    (outColorB) \
    (outAlpha) \
    (outTransparency) \
    (outTransparencyR) \
    (outTransparencyG) \
    (outTransparencyB)

#define TR_MAYA_STANDARD_SURFACE \
    (standardSurface) \
    (base) \
    (baseColor) \
    (diffuseRoughness) \
    (metalness) \
    (specular) \
    (specularColor) \
    (specularRoughness) \
    (specularIOR) \
    (specularAnisotropy) \
    (specularRotation) \
    (transmission) \
    (transmissionColor) \
    (transmissionDepth) \
    (transmissionScatter) \
    (transmissionScatterAnisotropy) \
    (transmissionDispersion) \
    (transmissionExtraRoughness) \
    (subsurface) \
    (subsurfaceColor) \
    (subsurfaceRadius) \
    (subsurfaceScale) \
    (subsurfaceAnisotropy) \
    (sheen) \
    (sheenColor) \
    (sheenRoughness) \
    (coat) \
    (coatColor) \
    (coatRoughness) \
    (coatAnisotropy) \
    (coatRotation) \
    (coatIOR) \
    (coatNormal) \
    (coatAffectColor) \
    (coatAffectRoughness) \
    (thinFilmThickness) \
    (thinFilmIOR) \
    (emission) \
    (emissionColor) \
    (opacity) \
    (opacityR) \
    (opacityG) \
    (opacityB) \
    (thinWalled) \
    (tangentUCamera)

#define TR_MAYA_FILE \
    (file) \
    (alphaGain) \
    (alphaOffset) \
    (alphaIsLuminance) \
    (colorGain) \
    (colorOffset) \
    (colorSpace) \
    (defaultColor) \
    (fileTextureName) \
    (filterType) \
    ((UDIMTag, "<UDIM>")) \
    (uvTilingMode) \
    (Raw) \
    (sRGB)

#define TR_MAYA_UV \
    (place2dTexture) \
    (coverage) \
    (translateFrame) \
    (rotateFrame) \
    (mirrorU) \
    (mirrorV) \
    (stagger) \
    (wrapU) \
    (wrapV) \
    (repeatUV) \
    (offset) \
    (rotateUV) \
    (noiseUV) \
    (vertexUvOne) \
    (vertexUvTwo) \
    (vertexUvThree) \
    (vertexCameraOne) \
    (outUvFilterSize) \
    (uvFilterSize) \
    (outUV) \
    (uvCoord)

// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    TrMayaTokens,
    ,
    TR_MAYA_MATERIALS TR_MAYA_STANDARD_SURFACE TR_MAYA_FILE TR_MAYA_UV);

#ifdef WANT_MATERIALX_TRANSLATORS

// clang-format off
#define TR_MTLX_COMMON \
    ((conversionName, "MaterialX")) \
    ((contextName, "mtlx")) \
    ((niceName, "MaterialX shading")) \
    ((exportDescription, "Exports bound shaders as a MaterialX UsdShade network.")) \
    ((importDescription, "Search for a MaterialX UsdShade network to import."))

#define TR_MTLX_NODE_DEFS \
    (ND_standard_surface_surfaceshader) \
    (ND_UsdPreviewSurface_surfaceshader) \
    (ND_image_float) \
    (ND_image_vector2) \
    (ND_image_color3) \
    (ND_image_color4) \
    (ND_geompropvalue_vector2) \
    (ND_luminance_color3) \
    (ND_luminance_color4) \
    (ND_convert_color3_vector3) \
    (ND_normalmap)

#define TR_MTLX_STANDARD_SURFACE \
    (base) \
    (base_color) \
    (diffuse_roughness) \
    (metalness) \
    (specular) \
    (specular_color) \
    (specular_roughness) \
    (specular_IOR) \
    (specular_anisotropy) \
    (specular_rotation) \
    (transmission) \
    (transmission_color) \
    (transmission_depth) \
    (transmission_scatter) \
    (transmission_scatter_anisotropy) \
    (transmission_dispersion) \
    (transmission_extra_roughness) \
    (subsurface) \
    (subsurface_color) \
    (subsurface_radius) \
    (subsurface_scale) \
    (subsurface_anisotropy) \
    (sheen) \
    (sheen_color) \
    (sheen_roughness) \
    (coat) \
    (coat_color) \
    (coat_roughness) \
    (coat_anisotropy) \
    (coat_rotation) \
    (coat_IOR) \
    (coat_normal) \
    (coat_affect_color) \
    (coat_affect_roughness) \
    (thin_film_thickness) \
    (thin_film_IOR) \
    (emission) \
    (emission_color) \
    (opacity) \
    (thin_walled) \
    (normal) \
    (tangent)

#define TR_MTLX_IMAGE \
    (file) \
    ((paramDefault, "default")) \
    (texcoord) \
    (uaddressmode) \
    (vaddressmode) \
    (filtertype) \
    (clamp) \
    (periodic) \
    (mirror) \
    (closest) \
    (linear) \
    (cubic)

#define TR_MTLX_ATTRIBUTES \
    (geomprop) \
    (channels) \
    (in) \
    (in1) \
    (in2) \
    (out)

// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    TrMtlxTokens,
    ,
    TR_MTLX_COMMON TR_MTLX_NODE_DEFS TR_MTLX_STANDARD_SURFACE TR_MTLX_IMAGE TR_MTLX_ATTRIBUTES);

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_TOKENS_H
