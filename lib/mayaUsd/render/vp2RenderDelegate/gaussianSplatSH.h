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
#ifndef HDVP2_GAUSSIAN_SPLAT_SH_H
#define HDVP2_GAUSSIAN_SPLAT_SH_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/types.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/usd/sdf/path.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace MAYAUSD_NS_DEF {

/*! \brief  Spherical harmonics (SH) coefficients of a splat prim, read from the scene delegate.

    The coefficients are stored splat by splat, _stride coefficients (rgb triples) per splat.
*/
struct HdVP2GaussianSplatSHCoefficients
{
    PXR_NS::VtVec3fArray _values;
    size_t               _stride = 0;
    //! True when the array has a whole number of coefficients (at least one) per splat.
    bool _valid = false;
};

/*! \brief  SH coefficients packed into the texels of an RGBA32F texture.

    Each splat takes _texelsPerSplat consecutive texels of a row-major 2D texture, holding the rgb
    triples of the coefficients of the usable degree. Built on a worker, uploaded on the main
    thread.
*/
struct HdVP2GaussianSplatSHTexels
{
    //! Packed texels, RGBA32F, _width * _height of them. Empty when no texture is wanted.
    std::shared_ptr<std::vector<float>> _texels;
    unsigned int                        _width = 0;
    unsigned int                        _height = 0;
    //! Usable SH degree. 0 when the SH0 color stream is used.
    size_t _degree = 0;
    //! Texels per splat in the texture.
    size_t _texelsPerSplat = 0;
};

//! \brief  Read the SH coefficients of a prim with numSplats splats.
HdVP2GaussianSplatSHCoefficients HdVP2GaussianSplatReadSHCoefficients(
    PXR_NS::HdSceneDelegate* sceneDelegate,
    const PXR_NS::SdfPath&   id,
    size_t                   numSplats);

//! \brief  Highest SH degree the geometry stage evaluates for this data, from 0 to 3.
size_t HdVP2GaussianSplatUsableSHDegree(size_t coefficientsPerSplat);

/*! \brief  Fill numSplats float4 colors (rgb and 1) from the first SH coefficient of each splat.

    The color is the 3DGS band 0 color, clamped to 0..1. Without valid coefficients every splat
    gets defaultColor.
*/
void HdVP2GaussianSplatFillSH0Colors(
    const HdVP2GaussianSplatSHCoefficients& sh,
    size_t                                  numSplats,
    const PXR_NS::GfVec3f&                  defaultColor,
    float*                                  out);

/*! \brief  Pack the coefficients of the given SH degree (above 0) into texels.

    The texture is at most maxWidth texels wide and maxHeight texels high.

    \return false when the texels do not fit, in which case result is left unchanged.
*/
bool HdVP2GaussianSplatPackSHTexels(
    const HdVP2GaussianSplatSHCoefficients& sh,
    size_t                                  numSplats,
    size_t                                  degree,
    unsigned int                            maxWidth,
    unsigned int                            maxHeight,
    HdVP2GaussianSplatSHTexels&             result);

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_SH_H
