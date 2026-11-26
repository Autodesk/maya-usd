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
#include "gaussianSplatResources.h"

#if PXR_VERSION >= 2605

#include <pxr/base/tf/diagnostic.h>

#include <maya/MColor.h>
#include <maya/MStateManager.h>
#include <maya/MTextureManager.h>
#include <maya/MViewport2Renderer.h>

#include <memory>
#include <utility>

// The Tf diagnostic macros name Tf types without a namespace.
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

//! Side of the quad of a wire item, in pixels, whatever the scale of the splat.
constexpr float kWireQuadPixelSize = 8.0f;

} // anonymous namespace

HdVP2GaussianSplatResources::HdVP2GaussianSplatResources(PXR_NS::HdVP2RenderDelegate* delegate)
    : _delegate(delegate)
{
}

void HdVP2GaussianSplatResources::CreateShaders(const bool instanced)
{
    // Only the pair for the draw the Rprim uses. Its instanced state is fixed for its life.
    PXR_NS::HdVP2ShaderUniquePtr& shaded = instanced ? _shadedInstancedShader : _shadedShader;
    PXR_NS::HdVP2ShaderUniquePtr& wire = instanced ? _wireInstancedShader : _wireShader;

    //! Clone the template into an empty pointer. Returns true when a clone was created.
    const auto cloneTemplate = [this, instanced](PXR_NS::HdVP2ShaderUniquePtr& clone) {
        if (clone) {
            return false;
        }
        MHWRender::MShaderInstance* const shaderTemplate = instanced
            ? _delegate->GetSplatInstancedFallbackShader(MColor())
            : _delegate->GetSplatFallbackShader(MColor());
        if (TF_VERIFY(shaderTemplate)) {
            clone.reset(shaderTemplate->clone());
        }
        return static_cast<bool>(clone);
    };

    cloneTemplate(shaded);
    if (cloneTemplate(wire)) {
        // The wire clones draw opaque quads of a fixed pixel size. Parameters are set on the main
        // thread, before the item gets the shader.
        MHWRender::MShaderInstance* const wireShader = wire.get();
        _delegate->GetVP2ResourceRegistry().EnqueueCommit([wireShader]() {
            wireShader->setParameter("fixedQuadSize", kWireQuadPixelSize);
            wireShader->setParameter("noDiscard", 1.0f);
        });
    }
}

MHWRender::MShaderInstance*
HdVP2GaussianSplatResources::GetShader(const bool isWire, const bool instanced) const
{
    return isWire ? (instanced ? _wireInstancedShader.get() : _wireShader.get())
                  : (instanced ? _shadedInstancedShader.get() : _shadedShader.get());
}

void HdVP2GaussianSplatResources::ApplySHParameters(MHWRender::MShaderInstance* shader) const
{
    if (_shTexture && _shSampler) {
        MHWRender::MTextureAssignment assignment;
        assignment.texture = _shTexture.get();
        shader->setParameter("shTexture", assignment);
        shader->setParameter("shTextureSampler", *_shSampler);
        shader->setParameter("shTexelsPerSplat", _shTexelsPerSplat);
        shader->setParameter("shDegree", _shDegree);
    } else {
        shader->setParameter("shDegree", 0.0f);
    }
}

void HdVP2GaussianSplatResources::UpdateSHTexture(
    const PXR_NS::SdfPath&                  id,
    const HdVP2GaussianSplatSHCoefficients& sh,
    const size_t                            numSplats)
{
    auto update = std::make_shared<HdVP2GaussianSplatSHTexels>();

    const size_t degree
        = (numSplats > 0 && sh._valid) ? HdVP2GaussianSplatUsableSHDegree(sh._stride) : 0;
    bool doesNotFit = false;
    if (degree > 0) {
        // The output target size is the proxy of the device texture limit.
        unsigned int                maxWidth = 0;
        unsigned int                maxHeight = 0;
        MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
        if (renderer) {
            renderer->GPUmaximumOutputTargetSize(maxWidth, maxHeight);
        }

        doesNotFit
            = !HdVP2GaussianSplatPackSHTexels(sh, numSplats, degree, maxWidth, maxHeight, *update);
        if (doesNotFit && !_shErrorReported) {
            TF_RUNTIME_ERROR(
                "Gaussian splat %s: the SH data (%zu splats, degree %zu) does not fit the GPU "
                "texture limit (%u x %u), using band 0 color.",
                id.GetText(),
                numSplats,
                degree,
                maxWidth,
                maxHeight);
        }
    }
    // Reported once while the data does not fit, cleared when it does again.
    _shErrorReported = doesNotFit;

    MHWRender::MShaderInstance* const shaders[2]
        = { _shadedShader.get(), _shadedInstancedShader.get() };
    if (!shaders[0] && !shaders[1]) {
        return;
    }

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([this, id, shaders, update]() {
        MHWRender::MTextureManager* const textureMgr = [] {
            MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
            return renderer ? renderer->getTextureManager() : nullptr;
        }();

        PXR_NS::HdVP2TextureUniquePtr newTexture;
        if (update->_texels && textureMgr) {
            MHWRender::MTextureDescription desc;
            desc.setToDefault2DTexture();
            desc.fWidth = update->_width;
            desc.fHeight = update->_height;
            desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
            desc.fBytesPerRow = update->_width * 4 * sizeof(float);
            desc.fBytesPerSlice = desc.fBytesPerRow * update->_height;

            // An unnamed texture is never served from the texture cache, so a change in data
            // cannot get a stale texture. Mipmaps are off, neighbouring splats must not blend.
            newTexture.reset(textureMgr->acquireTexture("", desc, update->_texels->data(), false));
            if (!newTexture && !_shErrorReported) {
                _shErrorReported = true;
                TF_RUNTIME_ERROR(
                    "Gaussian splat %s: could not create the SH texture, using band 0 color.",
                    id.GetText());
            }
        }

        const MHWRender::MSamplerState* sampler = nullptr;
        if (newTexture) {
            MHWRender::MSamplerStateDesc samplerDesc;
            samplerDesc.filter = MHWRender::MSamplerState::kMinMagMipPoint;
            samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW
                = MHWRender::MSamplerState::kTexClamp;
            sampler = _delegate->GetSamplerState(samplerDesc);
        }

        // The SH0 color stream, which is always bound, is the fallback.
        PXR_NS::HdVP2TextureUniquePtr oldTexture = std::move(_shTexture);
        if (newTexture && sampler) {
            _shTexture = std::move(newTexture);
            _shSampler = sampler;
            _shTexelsPerSplat = static_cast<float>(update->_texelsPerSplat);
            _shDegree = static_cast<float>(update->_degree);
        } else {
            _shTexture.reset();
            _shSampler = nullptr;
            _shDegree = 0.0f;
        }

        // The shaders have their own reference to the bound texture. The old one is released after
        // the new one is bound.
        for (MHWRender::MShaderInstance* const shader : shaders) {
            if (shader) {
                ApplySHParameters(shader);
            }
        }
    });
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
