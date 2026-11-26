//
// Copyright 2026 Autodesk
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
#include "gaussianSplatSH.h"

#if PXR_VERSION >= 2605

#include <pxr/base/gf/math.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/usdVol/tokens.h>

#include <algorithm>

namespace MAYAUSD_NS_DEF {

namespace {

//! Factor converting the first SH coefficient to a color: rgb = 0.5 + kSH_C0 * coeff.
constexpr float kSH_C0 = 0.28209479177387814f;

//! Highest SH degree the geometry stage evaluates.
constexpr size_t kMaxSHDegree = 3;

//! Number of coefficients (rgb triples) of the SH bands up to the given degree.
constexpr size_t _NumCoefficients(const size_t degree) { return (degree + 1) * (degree + 1); }

} // anonymous namespace

HdVP2GaussianSplatSHCoefficients HdVP2GaussianSplatReadSHCoefficients(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    const PXR_NS::SdfPath&   id,
    const size_t             numSplats)
{
    HdVP2GaussianSplatSHCoefficients sh;
    if (numSplats == 0) {
        return sh;
    }

    // Coefficients can be authored in float or half precision.
    const PXR_NS::VtValue cast = PXR_NS::VtValue::Cast<PXR_NS::VtVec3fArray>(
        sceneDelegate->Get(id, PXR_NS::UsdVolTokens->radianceSphericalHarmonicsCoefficients));
    if (!cast.IsEmpty()) {
        sh._values = cast.UncheckedGet<PXR_NS::VtVec3fArray>();
    }
    sh._stride = sh._values.size() / numSplats;
    sh._valid = sh._stride > 0 && sh._values.size() == sh._stride * numSplats;
    return sh;
}

/*! Highest SH degree, up to kMaxSHDegree, for which a splat has a complete set of coefficients.
    Missing coefficients are never read.
*/
size_t HdVP2GaussianSplatUsableSHDegree(const size_t coefficientsPerSplat)
{
    size_t degree = 0;
    while (degree < kMaxSHDegree && _NumCoefficients(degree + 1) <= coefficientsPerSplat) {
        ++degree;
    }
    return degree;
}

void HdVP2GaussianSplatFillSH0Colors(
    const HdVP2GaussianSplatSHCoefficients& sh,
    const size_t                            numSplats,
    const PXR_NS::GfVec3f&                  defaultColor,
    float*                                  out)
{
    unsigned int offset = 0;
    if (sh._valid) {
        for (size_t i = 0; i < numSplats; ++i) {
            // 3DGS band 0 (constant color) SH to RGB: rgb = 0.5 + C0 * coeff.
            const PXR_NS::GfVec3f color
                = PXR_NS::GfVec3f(0.5f) + kSH_C0 * sh._values[i * sh._stride];
            // SH can learn values that give out of range colors, clamp them.
            out[offset++] = PXR_NS::GfClamp(color[0], 0.0f, 1.0f);
            out[offset++] = PXR_NS::GfClamp(color[1], 0.0f, 1.0f);
            out[offset++] = PXR_NS::GfClamp(color[2], 0.0f, 1.0f);
            out[offset++] = 1.0f;
        }
    } else {
        for (size_t i = 0; i < numSplats; ++i) {
            out[offset++] = defaultColor[0];
            out[offset++] = defaultColor[1];
            out[offset++] = defaultColor[2];
            out[offset++] = 1.0f;
        }
    }
}

/*! Each splat takes ceil(3 * coefficients / 4) consecutive texels, in a row-major 2D texture.
    Only the coefficients of the given degree are stored.
*/
bool HdVP2GaussianSplatPackSHTexels(
    const HdVP2GaussianSplatSHCoefficients& sh,
    const size_t                            numSplats,
    const size_t                            degree,
    const unsigned int                      maxWidth,
    const unsigned int                      maxHeight,
    HdVP2GaussianSplatSHTexels&             result)
{
    const size_t numCoeffs = _NumCoefficients(degree);
    const size_t texelsPerSplat = (3 * numCoeffs + 3) / 4;
    const size_t totalTexels = numSplats * texelsPerSplat;

    const size_t width = std::min<size_t>(totalTexels, maxWidth);
    const size_t height = width > 0 ? (totalTexels + width - 1) / width : 0;
    if (width == 0 || height > maxHeight) {
        return false;
    }

    auto texels = std::make_shared<std::vector<float>>(width * height * 4, 0.0f);
    for (size_t i = 0; i < numSplats; ++i) {
        float* const           out = texels->data() + i * texelsPerSplat * 4;
        const PXR_NS::GfVec3f* in = sh._values.cdata() + i * sh._stride;
        for (size_t c = 0; c < numCoeffs; ++c) {
            out[3 * c + 0] = in[c][0];
            out[3 * c + 1] = in[c][1];
            out[3 * c + 2] = in[c][2];
        }
    }
    result._texels = std::move(texels);
    result._width = static_cast<unsigned int>(width);
    result._height = static_cast<unsigned int>(height);
    result._degree = degree;
    result._texelsPerSplat = texelsPerSplat;
    return true;
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
