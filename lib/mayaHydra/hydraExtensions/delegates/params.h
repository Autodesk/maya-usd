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
#ifndef MAYAHYDRALIB_PARAMS_H
#define MAYAHYDRALIB_PARAMS_H

#include <pxr/pxr.h>

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraParams are the global parameters for this plug-in.
 * Please note that we add to these parameters the parameters read from the chosen hydra render
 * delegate and expose them in our UI with the MEL function mtohRenderOverride_AddAttribute see
 * renderGlobals.cpp.
 */
struct MayaHydraParams
{
    bool  proxyPurpose = false;
    bool  renderPurpose = false;
    bool  guidePurpose = false;
    int   textureMemoryPerTexture = 4 * 1024 * 1024;
    int   maximumShadowMapResolution = 2048;
    float motionSampleStart = 0;
    float motionSampleEnd = 0;
    bool  displaySmoothMeshes = true;

    bool motionSamplesEnabled() const { return motionSampleStart != 0 || motionSampleEnd != 0; }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_PARAMS_H
