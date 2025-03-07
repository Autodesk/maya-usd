//
// Copyright 2024 Autodesk
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
#include "mtlxTranslationTableReader.h"
#include "shadingTokens.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <pxr/base/tf/staticTokens.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// Mapping from Maya to MaterialX:
using TokenHashMap = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
TokenHashMap mtlxToMaya {
    { TrMtlxOpenPBRTokens->base_weight, TrMayaOpenPBRTokens->baseWeight },
    { TrMtlxOpenPBRTokens->base_color, TrMayaOpenPBRTokens->baseColor },
    { TrMtlxOpenPBRTokens->base_diffuse_roughness, TrMayaOpenPBRTokens->baseDiffuseRoughness },
    { TrMtlxOpenPBRTokens->base_metalness, TrMayaOpenPBRTokens->baseMetalness },
    { TrMtlxOpenPBRTokens->specular_weight, TrMayaOpenPBRTokens->specularWeight },
    { TrMtlxOpenPBRTokens->specular_color, TrMayaOpenPBRTokens->specularColor },
    { TrMtlxOpenPBRTokens->specular_roughness, TrMayaOpenPBRTokens->specularRoughness },
    { TrMtlxOpenPBRTokens->specular_ior, TrMayaOpenPBRTokens->specularIOR },
    { TrMtlxOpenPBRTokens->specular_roughness_anisotropy,
      TrMayaOpenPBRTokens->specularRoughnessAnisotropy },
    { TrMtlxOpenPBRTokens->transmission_weight, TrMayaOpenPBRTokens->transmissionWeight },
    { TrMtlxOpenPBRTokens->transmission_color, TrMayaOpenPBRTokens->transmissionColor },
    { TrMtlxOpenPBRTokens->transmission_depth, TrMayaOpenPBRTokens->transmissionDepth },
    { TrMtlxOpenPBRTokens->transmission_scatter, TrMayaOpenPBRTokens->transmissionScatter },
    { TrMtlxOpenPBRTokens->transmission_scatter_anisotropy,
      TrMayaOpenPBRTokens->transmissionScatterAnisotropy },
    { TrMtlxOpenPBRTokens->transmission_dispersion_scale,
      TrMayaOpenPBRTokens->transmissionDispersionScale },
    { TrMtlxOpenPBRTokens->transmission_dispersion_abbe_number,
      TrMayaOpenPBRTokens->transmissionDispersionAbbeNumber },
    { TrMtlxOpenPBRTokens->subsurface_weight, TrMayaOpenPBRTokens->subsurfaceWeight },
    { TrMtlxOpenPBRTokens->subsurface_color, TrMayaOpenPBRTokens->subsurfaceColor },
    { TrMtlxOpenPBRTokens->subsurface_radius, TrMayaOpenPBRTokens->subsurfaceRadius },
    { TrMtlxOpenPBRTokens->subsurface_radius_scale, TrMayaOpenPBRTokens->subsurfaceRadiusScale },
    { TrMtlxOpenPBRTokens->subsurface_scatter_anisotropy,
      TrMayaOpenPBRTokens->subsurfaceScatterAnisotropy },
    { TrMtlxOpenPBRTokens->fuzz_weight, TrMayaOpenPBRTokens->fuzzWeight },
    { TrMtlxOpenPBRTokens->fuzz_color, TrMayaOpenPBRTokens->fuzzColor },
    { TrMtlxOpenPBRTokens->fuzz_roughness, TrMayaOpenPBRTokens->fuzzRoughness },
    { TrMtlxOpenPBRTokens->coat_weight, TrMayaOpenPBRTokens->coatWeight },
    { TrMtlxOpenPBRTokens->coat_color, TrMayaOpenPBRTokens->coatColor },
    { TrMtlxOpenPBRTokens->coat_roughness, TrMayaOpenPBRTokens->coatRoughness },
    { TrMtlxOpenPBRTokens->coat_roughness_anisotropy,
      TrMayaOpenPBRTokens->coatRoughnessAnisotropy },
    { TrMtlxOpenPBRTokens->coat_ior, TrMayaOpenPBRTokens->coatIOR },
    { TrMtlxOpenPBRTokens->coat_darkening, TrMayaOpenPBRTokens->coatDarkening },
    { TrMtlxOpenPBRTokens->thin_film_weight, TrMayaOpenPBRTokens->thinFilmWeight },
    { TrMtlxOpenPBRTokens->thin_film_thickness, TrMayaOpenPBRTokens->thinFilmThickness },
    { TrMtlxOpenPBRTokens->thin_film_ior, TrMayaOpenPBRTokens->thinFilmIOR },
    { TrMtlxOpenPBRTokens->emission_luminance, TrMayaOpenPBRTokens->emissionLuminance },
    { TrMtlxOpenPBRTokens->emission_color, TrMayaOpenPBRTokens->emissionColor },
    { TrMtlxOpenPBRTokens->geometry_opacity, TrMayaOpenPBRTokens->geometryOpacity },
    { TrMtlxOpenPBRTokens->geometry_thin_walled, TrMayaOpenPBRTokens->geometryThinWalled },
    { TrMtlxOpenPBRTokens->geometry_normal, TrMayaOpenPBRTokens->normalCamera },
    { TrMtlxOpenPBRTokens->geometry_coat_normal, TrMayaOpenPBRTokens->geometryCoatNormal },
    { TrMtlxOpenPBRTokens->geometry_tangent, TrMayaOpenPBRTokens->tangentUCamera },
    { TrMtlxOpenPBRTokens->geometry_coat_tangent, TrMayaOpenPBRTokens->geometryCoatTangent }
};

} // namespace

class MtlxUsd_OpenPBRSurfaceReader : public MtlxUsd_TranslationTableReader
{
public:
    MtlxUsd_OpenPBRSurfaceReader(const UsdMayaPrimReaderArgs& readArgs)
        : MtlxUsd_TranslationTableReader(readArgs)
    {
    }

    const TfToken& getMaterialName() const override { return TrMayaOpenPBRTokens->openPBRSurface; }
    const TfToken& getOutputName() const override { return TrMayaTokens->outColor; }
    const TranslationTable& getTranslationTable() const override { return mtlxToMaya; }
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_open_pbr_surface_surfaceshader, MtlxUsd_OpenPBRSurfaceReader);

PXR_NAMESPACE_CLOSE_SCOPE
