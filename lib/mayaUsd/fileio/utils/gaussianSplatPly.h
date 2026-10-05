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
#ifndef PXRUSDMAYA_GAUSSIAN_SPLAT_PLY_H
#define PXRUSDMAYA_GAUSSIAN_SPLAT_PLY_H

#include <string>
#include <vector>

/// Shared INRIA/3DGS DC normalization constant: radiance = 0.5 + SH_C0 * dc.
/// Used to bake/un-bake the DC spherical harmonic band to/from display RGB.
constexpr float SH_C0 = 0.28209479177387814f;

/// Number of SH coefficients per channel for a given degree: (degree+1)^2.
inline int SplatShCoeffCount(int shDegree) { return (shDegree + 1) * (shDegree + 1); }

struct SplatCloud
{
    // (x, y, z)
    std::vector<float> positions;
    // half-axis lenght (s0, s1, s2) activated using exp of PLY log-scale
    std::vector<float> scales;
    // real-first convention (w, x, y, z)
    std::vector<float> rotations;
    // post-sigmoid, [0,1]
    std::vector<float> opacities;
    // layout [gaussian][coeff][channel]
    // coeff in [0;shCoeffCount()]
    //  NOT baked to display RGB, use SplatDisplayColors()
    std::vector<float> shCoeffs;
    // [0;3]
    int shDegree = 0;

    size_t count() const { return opacities.size(); }
    int    shCoeffCount() const { return SplatShCoeffCount(shDegree); }

    /// All arrays sized consistently with count() and shDegree.
    bool isValid() const;
};

/// Bake the DC spherical harmonic band to display RGB (0.5 + SH_C0 * dc)
std::vector<float> SplatDisplayColors(const SplatCloud& cloud);

/// Returns false and fills \p errorMsg if the file can't be opened, the PLY
/// header is malformed, a required property is missing, or the f_rest_*
/// count doesn't correspond to a supported SH degree.
/// \p out is left untouched on failure.
bool loadSplatPly(const std::string& plyPath, SplatCloud* out, std::string* errorMsg);

/// Write a SplatCloud as an INRIA-style 3D Gaussian Splat PLY
/// property order: x,y,z; nx,ny,nz; f_dc_0..2; f_rest_0..N; opacity;
/// scale_0..2; rot_0..3.
/// Normals are written as zero, for compatibility with external tools
///
/// Returns false and fills \p errorMsg if \p cloud.isValid() is false
/// or if \p path can't be opened or written.
bool saveSplatPly(
    const SplatCloud&  cloud,
    const std::string& path,
    std::string*       errorMsg,
    bool               binary = true);

#endif
