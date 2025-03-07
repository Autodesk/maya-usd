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
    { TrMtlxTokens->base, TrMayaTokens->base },
    { TrMtlxTokens->base_color, TrMayaTokens->baseColor },
    { TrMtlxTokens->diffuse_roughness, TrMayaTokens->diffuseRoughness },
    { TrMtlxTokens->metalness, TrMayaTokens->metalness },
    { TrMtlxTokens->specular, TrMayaTokens->specular },
    { TrMtlxTokens->specular_color, TrMayaTokens->specularColor },
    { TrMtlxTokens->specular_roughness, TrMayaTokens->specularRoughness },
    { TrMtlxTokens->specular_IOR, TrMayaTokens->specularIOR },
    { TrMtlxTokens->specular_anisotropy, TrMayaTokens->specularAnisotropy },
    { TrMtlxTokens->specular_rotation, TrMayaTokens->specularRotation },
    { TrMtlxTokens->transmission, TrMayaTokens->transmission },
    { TrMtlxTokens->transmission_color, TrMayaTokens->transmissionColor },
    { TrMtlxTokens->transmission_depth, TrMayaTokens->transmissionDepth },
    { TrMtlxTokens->transmission_scatter, TrMayaTokens->transmissionScatter },
    { TrMtlxTokens->transmission_scatter_anisotropy, TrMayaTokens->transmissionScatterAnisotropy },
    { TrMtlxTokens->transmission_dispersion, TrMayaTokens->transmissionDispersion },
    { TrMtlxTokens->transmission_extra_roughness, TrMayaTokens->transmissionExtraRoughness },
    { TrMtlxTokens->subsurface, TrMayaTokens->subsurface },
    { TrMtlxTokens->subsurface_color, TrMayaTokens->subsurfaceColor },
    { TrMtlxTokens->subsurface_radius, TrMayaTokens->subsurfaceRadius },
    { TrMtlxTokens->subsurface_scale, TrMayaTokens->subsurfaceScale },
    { TrMtlxTokens->subsurface_anisotropy, TrMayaTokens->subsurfaceAnisotropy },
    { TrMtlxTokens->sheen, TrMayaTokens->sheen },
    { TrMtlxTokens->sheen_color, TrMayaTokens->sheenColor },
    { TrMtlxTokens->sheen_roughness, TrMayaTokens->sheenRoughness },
    { TrMtlxTokens->coat, TrMayaTokens->coat },
    { TrMtlxTokens->coat_color, TrMayaTokens->coatColor },
    { TrMtlxTokens->coat_roughness, TrMayaTokens->coatRoughness },
    { TrMtlxTokens->coat_anisotropy, TrMayaTokens->coatAnisotropy },
    { TrMtlxTokens->coat_rotation, TrMayaTokens->coatRotation },
    { TrMtlxTokens->coat_IOR, TrMayaTokens->coatIOR },
    { TrMtlxTokens->coat_normal, TrMayaTokens->coatNormal },
    { TrMtlxTokens->coat_affect_color, TrMayaTokens->coatAffectColor },
    { TrMtlxTokens->coat_affect_roughness, TrMayaTokens->coatAffectRoughness },
    { TrMtlxTokens->thin_film_thickness, TrMayaTokens->thinFilmThickness },
    { TrMtlxTokens->thin_film_IOR, TrMayaTokens->thinFilmIOR },
    { TrMtlxTokens->emission, TrMayaTokens->emission },
    { TrMtlxTokens->emission_color, TrMayaTokens->emissionColor },
    { TrMtlxTokens->opacity, TrMayaTokens->opacity },
    { TrMtlxTokens->thin_walled, TrMayaTokens->thinWalled },
    { TrMtlxTokens->normal, TrMayaTokens->normalCamera },
    { TrMtlxTokens->tangent, TrMayaTokens->tangentUCamera }
};

} // namespace

class MtlxUsd_StandardSurfaceReader : public MtlxUsd_TranslationTableReader
{
public:
    MtlxUsd_StandardSurfaceReader(const UsdMayaPrimReaderArgs& readArgs)
        : MtlxUsd_TranslationTableReader(readArgs)
    {
    }

    const TfToken& getMaterialName() const override { return TrMayaTokens->standardSurface; }
    const TfToken& getOutputName() const override { return TrMayaTokens->outColor; }
    const TranslationTable& getTranslationTable() const override { return mtlxToMaya; }
};

PXRUSDMAYA_REGISTER_SHADER_READER(ND_standard_surface_surfaceshader, MtlxUsd_StandardSurfaceReader);

PXR_NAMESPACE_CLOSE_SCOPE
