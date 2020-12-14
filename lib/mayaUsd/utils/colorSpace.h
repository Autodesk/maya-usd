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
#ifndef PXRUSDMAYA_COLORSPACE_H
#define PXRUSDMAYA_COLORSPACE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Helper functions for dealing with colors stored in Maya.
///
/// Technically, this doesn't need to be tied to Usd.
namespace UsdMayaColorSpace {

/// Returns true if we treat colors from Maya as linear colors.
///
/// Before color management (viewport 1.0), all Maya colors were stored with
/// gamma correction.  When we have a mix of shapes we need to draw, some shaded
/// via native Maya and others with our custom shapes, we need to know if the
/// Maya colors are considered linear or not.  If things are color correct, our
/// shape needs to write linear colors to the framebuffer and we leave the final
/// correction up to Maya.  Otherwise, we want to draw things as if they were
/// modeled in Maya.  While this may not be "correct" in all situations, at
/// least it is consistent with native Maya shading.
///
/// Currently, this value is controlled via an environment variable:
///
/// PIXMAYA_LINEAR_COLORS
///
/// You should only be setting that if you've more or less fully switched to
/// Viewport 2.0 (as proper color management is only supported there).
///
MAYAUSD_CORE_PUBLIC
bool IsColorManaged();

/// Converts a linear color into the appropriate Maya color space as determined by
/// the above \c IsColorManaged.
template <typename T> T ConvertLinearToMaya(const T& linearColor)
{
    return IsColorManaged() ? linearColor : GfConvertLinearToDisplay(linearColor);
}

/// Converts a Maya color space into a linear color.
template <typename T> T ConvertMayaToLinear(const T& mayaColor)
{
    return IsColorManaged() ? mayaColor : GfConvertDisplayToLinear(mayaColor);
}

}; // namespace UsdMayaColorSpace

PXR_NAMESPACE_CLOSE_SCOPE

#endif
