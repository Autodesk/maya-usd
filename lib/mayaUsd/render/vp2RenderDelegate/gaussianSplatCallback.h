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
#ifndef HDVP2_GAUSSIAN_SPLAT_CALLBACK_H
#define HDVP2_GAUSSIAN_SPLAT_CALLBACK_H

// Creation and release of the splat shader clone of a Rprim. The clone carries the pre-draw
// callback, which forwards each draw to the GaussianSplatViewCache registered for the clone.
// Main thread only.

#include <mayaUsd/mayaUsd.h>

#include <pxr/pxr.h>

#include <maya/MHWGeometry.h>
#include <maya/MShaderManager.h>

#include <string>

namespace MAYAUSD_NS_DEF {

/*! \brief  Create the splat shader instance of one Rprim: a clone of a file-static base shader
            acquired with the pre-draw callback. Main thread only. Returns null on failure.

    \param declaredColorSpace  The colorSpace metadata of the splat coefficients (empty when not
           authored, which means sRGB). When color management is active and the space differs
           from the rendering space, the base shader is the splat graph with an OCIO transform
           from that space to the rendering space (one cached base per space), else the plain
           one. The choice is made here, once per Rprim: a later change of the preferences or of
           the metadata is not applied to a live Rprim.
*/
MHWRender::MShaderInstance* CreateSplatShaderClone(const std::string& declaredColorSpace);

/*! \brief  Release a splat shader created by CreateSplatShaderClone. Main thread only, outside a
            draw, after its cache is unregistered and its render items are gone.
*/
void ReleaseSplatShader(MHWRender::MShaderInstance* shader);

} // namespace MAYAUSD_NS_DEF

#endif // HDVP2_GAUSSIAN_SPLAT_CALLBACK_H
