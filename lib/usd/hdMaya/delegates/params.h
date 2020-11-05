//
// Copyright 2019 Luma Pictures
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
#ifndef HDMAYA_PARAMS_H
#define HDMAYA_PARAMS_H

#include <pxr/pxr.h>

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaParams
{
    int  textureMemoryPerTexture = 4 * 1024 * 1024;
    int  maximumShadowMapResolution = 2048;
    bool displaySmoothMeshes = true;
    bool enableMotionSamples = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_PARAMS_H
