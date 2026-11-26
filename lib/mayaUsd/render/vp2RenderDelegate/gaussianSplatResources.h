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
#ifndef HDVP2_GAUSSIAN_SPLAT_RESOURCES_H
#define HDVP2_GAUSSIAN_SPLAT_RESOURCES_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include "gaussianSplatSH.h"
#include "material.h"
#include "renderDelegate.h"
#include "shader.h"

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MShaderManager.h>

namespace MAYAUSD_NS_DEF {

/*! \brief  Shader clones and SH texture of one splat Rprim.
    \class  HdVP2GaussianSplatResources

    The shader templates are compiled once per variant by the render delegate (MShaderCache), the
    Rprim only owns its clones. The shaded items share one clone per variant, the wire item has
    its own. An Rprim only has the pair for its draw: plain, or instanced when it has an instancer
    at its first Sync. That state is fixed for the life of the Rprim.

    The clones are created on a worker. Their parameters, the SH texture and its sampler are set,
    acquired and released only in commit lambdas, on the main thread. The texture is released by
    its deleter when the Rprim is destroyed. Maya refcounts a bound texture, so a shader keeps its
    own reference.
*/
class HdVP2GaussianSplatResources
{
public:
    explicit HdVP2GaussianSplatResources(PXR_NS::HdVP2RenderDelegate* delegate);

    /*! \brief  Clone the shader templates that are missing.

        Creates the plain pair (shaded and wire), or the instanced pair when instanced is true,
        never both. Worker thread. The parameters of the wire clones are enqueued for the main
       thread.
    */
    void CreateShaders(bool instanced);

    //! \brief  The Rprim's clone for an item, or null when it does not exist yet.
    MHWRender::MShaderInstance* GetShader(bool isWire, bool instanced) const;

    /*! \brief  Bind the SH texture and the SH parameters to a shaded clone.

        Main thread only. Without a texture the clone uses the SH0 color stream.
    */
    void ApplySHParameters(MHWRender::MShaderInstance* shader) const;

    /*! \brief  Pack the SH coefficients into texels and enqueue the update of the SH texture.

        Worker thread. Only the texels are packed and the device limit is checked here. The packed
        copy lives in the commit lambda, which acquires the texture, binds it to the shaded clones
        and releases the old one. When the data does not fit the device, or the texture cannot be
        created, an error is reported once and the SH0 color stream is used.
    */
    void UpdateSHTexture(
        const PXR_NS::SdfPath&                  id,
        const HdVP2GaussianSplatSHCoefficients& sh,
        size_t                                  numSplats);

private:
    PXR_NS::HdVP2RenderDelegate* _delegate { nullptr };

    //! Rprim-owned clone of the plain splat shader template, created at the first Sync of a plain
    //! Rprim. All the shaded items of the Rprim use it, they only differ by item flags.
    PXR_NS::HdVP2ShaderUniquePtr _shadedShader;

    //! Rprim-owned clone of the same template for the wire item. It has fixed-size quads and no
    //! stochastic discard, and never gets the SH texture (the wire color is the SH0 stream).
    PXR_NS::HdVP2ShaderUniquePtr _wireShader;

    //! Rprim-owned clones of the instanced shader template, for the shaded items and for the wire
    //! item. They are created instead of the plain clones at the first Sync of an instanced Rprim.
    PXR_NS::HdVP2ShaderUniquePtr _shadedInstancedShader;
    PXR_NS::HdVP2ShaderUniquePtr _wireInstancedShader;

    //! SH texture of the Rprim (RGBA32F, point-read in the geometry stage). It only exists while
    //! the usable SH degree is above 0 and the data fits the device.
    PXR_NS::HdVP2TextureUniquePtr _shTexture;

    //! What the shaded clones are bound to, main thread only. Applied to a clone when the SH data
    //! changes.
    const MHWRender::MSamplerState* _shSampler = nullptr;
    float                           _shTexelsPerSplat = 1.0f;
    float                           _shDegree = 0.0f;

    //! True once an SH texture error was reported, so that it is reported once. Cleared when
    //! the data uses the texture again.
    bool _shErrorReported = false;
};

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_RESOURCES_H
