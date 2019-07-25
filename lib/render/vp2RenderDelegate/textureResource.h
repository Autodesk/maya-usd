//
// Copyright 2016 Pixar
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
#ifndef HD_VP2_TEXTURE_RESOURCE_H
#define HD_VP2_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/textureResource.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/vec4f.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdint>
#include <memory>

#include <maya/MViewport2Renderer.h>
#include <maya/MTextureManager.h>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdVP2TextureResource> HdVP2TextureResourceSharedPtr;

/*! \brief  VP2 implementation of a Hydra texture resource.
    \class  HdVP2TextureResource
*/
class HdVP2TextureResource : public HdTextureResource, boost::noncopyable {
public:
    /*! \brief  A deleter for Maya textures for use with smart pointers.
    */
    struct VP2TextureDeleter
    {
        void operator () (MHWRender::MTexture*);
    };

    using VP2TextureUniquePtr = std::unique_ptr<
        MHWRender::MTexture,
        VP2TextureDeleter
    >;

    /*! \brief  A deleter for Maya sampler states for use with smart pointers.
    */
    struct VP2SamplerDeleter
    {
        void operator () (const MHWRender::MSamplerState*);
    };

    using VP2SamplerUniquePtr = std::unique_ptr<
        const MHWRender::MSamplerState,
        VP2SamplerDeleter
    >;

public:
    HdVP2TextureResource(VP2TextureUniquePtr&&, VP2SamplerUniquePtr&&);

    /*! \brief  Create an empty texture resource.
    */
    HdVP2TextureResource() {}

    ~HdVP2TextureResource() override;

    HdTextureType GetTextureType() const override;
    size_t GetMemoryUsed() override;

    /*! \brief  Get the VP2 texture
    */
    MHWRender::MTexture* GetTexture() const {
        return _vp2Texture.get();
    }

    /*! \brief  The VP2 sampler state
    */
    const MHWRender::MSamplerState* GetSampler() const {
        return _vp2Sampler.get();
    }

private:
    VP2TextureUniquePtr _vp2Texture;    //!< The VP2 texture
    VP2SamplerUniquePtr _vp2Sampler;    //!< The VP2 sampler state
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_VP2_TEXTURE_RESOURCE_H
