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
#ifndef MAYAUSD_GAUSSIANSPLATPLY_H
#define MAYAUSD_GAUSSIANSPLATPLY_H

#include <mayaUsd/base/api.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace utils {

/// Number of SH coefficients per channel for a given degree: (degree+1)^2.
inline int splatShCoeffCount(int shDegree) { return (shDegree + 1) * (shDegree + 1); }

struct MAYAUSD_CORE_PUBLIC SplatCloud
{
    // (x, y, z)
    std::vector<float> _positions;
    // half-axis lenght (s0, s1, s2) activated using exp of PLY log-scale
    std::vector<float> _scales;
    // real-first convention (w, x, y, z)
    std::vector<float> _rotations;
    // post-sigmoid, [0,1]
    std::vector<float> _opacities;
    // layout [gaussian][coeff][channel]
    // coeff in [0;shCoeffCount()]
    // raw SH coefficients, NOT baked to display RGB
    std::vector<float> _shCoeffs;
    // [0;3]
    int _shDegree = 0;

    size_t count() const { return _opacities.size(); }
    int    shCoeffCount() const { return splatShCoeffCount(_shDegree); }

    /// All arrays sized consistently with count() and _shDegree.
    bool isValid() const;
};

/// Returns false and fills \p errorMsg if the file can't be opened, the PLY
/// header is malformed, a required property is missing, or the f_rest_*
/// count doesn't correspond to a supported SH degree.
MAYAUSD_CORE_PUBLIC
bool loadSplatPly(const std::string& plyPath, SplatCloud* out, std::string* errorMsg);

/// Write a SplatCloud as an INRIA-style 3D Gaussian Splat PLY
/// property order: x,y,z; nx,ny,nz; f_dc_0..2; f_rest_0..N; opacity;
/// scale_0..2; rot_0..3.
/// Normals are written as zero, for compatibility with external tools
///
/// Returns false and fills \p errorMsg if \p cloud.isValid() is false
/// or if \p path can't be opened or written.
MAYAUSD_CORE_PUBLIC
bool saveSplatPly(
    const SplatCloud&  cloud,
    const std::string& path,
    std::string*       errorMsg,
    bool               binary = true);

} // namespace utils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_GAUSSIANSPLATPLY_H
