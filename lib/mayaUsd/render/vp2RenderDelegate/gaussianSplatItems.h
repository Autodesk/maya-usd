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
#ifndef HDVP2_GAUSSIAN_SPLAT_ITEMS_H
#define HDVP2_GAUSSIAN_SPLAT_ITEMS_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/token.h>

#include <maya/MHWGeometry.h>

namespace MAYAUSD_NS_DEF {

//! The render item a splat Rprim creates for a repr token.
struct HdVP2GaussianSplatItemDesc
{
    //! Maya draw mode of the item.
    MHWRender::MGeometry::DrawMode _drawMode = MHWRender::MGeometry::kShaded;
    //! True for the item that draws only in default material mode.
    bool _drawOnlyInDefaultMaterialMode = false;
    //! True for the wire item.
    bool _isWire = false;
};

/*! \brief  Describe the item of a repr token.

    | Token                  | Item                                                         |
    |------------------------|--------------------------------------------------------------|
    | smoothHull             | Shaded, kTextured, skipped in default material mode          |
    | smoothHullUntextured   | Shaded, kShaded, skipped in default material mode            |
    | defaultMaterial        | Shaded, kShaded and kTextured, only in default material mode |
    | wire                   | Wire, kWireframe                                             |
    | any other              | None (a bounding box layer hides splats)                     |

    Without the Maya 2025 default material exclusion API, defaultMaterial has no item.

    \return false when the token has no item.
*/
bool HdVP2GaussianSplatGetItemDesc(
    const PXR_NS::TfToken&      reprToken,
    HdVP2GaussianSplatItemDesc& desc);

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_ITEMS_H
