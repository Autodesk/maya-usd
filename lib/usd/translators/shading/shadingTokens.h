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

#define TR_USD_COMMON (varname)

// XXX: We duplicate these tokens here rather than create a dependency on
// usdImaging in case the plugin is being built with imaging disabled.
// If/when they move out of usdImaging to a place that is always available,
// they should be pulled from there instead.
#define TR_USD_TEXTURE                                                                     \
    (UsdUVTexture)(bias)(fallback)(file)(scale)(st)(wrapS)(wrapT)((RGBOutputName, "rgb"))( \
        (RedOutputName, "r"))((GreenOutputName, "g"))((BlueOutputName, "b"))(              \
        (AlphaOutputName, "a"))

#define TR_USD_PRIMVAR (UsdPrimvarReader_float2)(result)(black)(mirror)(repeat)

#define TR_USD_XFORM (UsdTransform2d)(in)(rotation)(translation)

TF_DECLARE_PUBLIC_TOKENS(TrUsdTokens, , TR_USD_COMMON TR_USD_TEXTURE TR_USD_PRIMVAR TR_USD_XFORM);

// clang-format off
#define TR_MAYA_MATERIALS \
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
    (thinWalled) \
    (normal) \
    (tangent)

#define TR_MAYA_FILE \
    (file) \
    (alphaGain) \
    (alphaOffset) \
    (colorGain) \
    (colorOffset) \
    (colorSpace) \
    (defaultColor) \
    (fileTextureName) \
    (filterType)

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
    (uvCoord) \
    (black) \
    (mirror) \
    (repeat) \
    ((UDIMTag, "<UDIM>")) \
    (uvTilingMode)

// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    TrMayaTokens,
    ,
    TR_MAYA_MATERIALS TR_MAYA_STANDARD_SURFACE TR_MAYA_FILE TR_MAYA_UV);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_TOKENS_H
