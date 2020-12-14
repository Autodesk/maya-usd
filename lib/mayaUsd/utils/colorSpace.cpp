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
#include "colorSpace.h"

#include <pxr/base/tf/envSetting.h>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(
    PIXMAYA_LINEAR_COLORS,
    false,
    "If colors from maya should be treated as linear.  "
    "When false, colors are assumed to be gamma-corrected.");

bool UsdMayaColorSpace::IsColorManaged()
{
    // in theory this could vary per scene, but we think mixing that within any
    // given pipeline is likely confusing.  Also, we want to avoid this function
    // calling out to mel.
    return TfGetEnvSetting(PIXMAYA_LINEAR_COLORS);
}
