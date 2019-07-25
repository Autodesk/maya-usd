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

#include "textureResource.h"
#include "pxr/imaging/hd/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE

/* !\brief Releases the reference to the VP2 texture owned by a smart pointer.
*/
void
HdVP2TextureResource::VP2TextureDeleter::operator () (
    MHWRender::MTexture* texture
) {
    if (!texture)
        return;

    MHWRender::MRenderer* const renderer = MRenderer::theRenderer();
    if (!TF_VERIFY(renderer))
        return;

    MHWRender::MTextureManager* const
        textureMgr = renderer->getTextureManager();

    if (!TF_VERIFY(textureMgr))
        return;

    textureMgr->releaseTexture(texture);

};

/* !\brief Releases the reference to the VP2 sampler owned by a smart pointer.
*/
void
HdVP2TextureResource::VP2SamplerDeleter::operator () (
    const MHWRender::MSamplerState* sampler
) {
    if (sampler)
        MHWRender::MStateManager::releaseSamplerState(sampler);
};

/*! \brief  Constructor
    
    Takes ownership of the provided texture and sampler.
*/
HdVP2TextureResource::HdVP2TextureResource(
    HdVP2TextureResource::VP2TextureUniquePtr&& vp2Texture,
    HdVP2TextureResource::VP2SamplerUniquePtr&& vp2Sampler
)
    : _vp2Texture(std::move(vp2Texture))
    , _vp2Sampler(std::move(vp2Sampler))
{
}

/*! \brief  Destructor

    Releases the texture and the sampler.
*/
HdVP2TextureResource::~HdVP2TextureResource() 
{
}

/*! \brief  Get the texture type.

    Only UV textures are supported currently.
*/
HdTextureType HdVP2TextureResource::GetTextureType() const
{ 
    return HdTextureType::Uv;
}

/*! \brief  Get the amount of memory used by the texture.

    TODO: implement.
*/
size_t HdVP2TextureResource::GetMemoryUsed()
{
    /*if (_texture) {
        return _texture->GetMemoryUsed();
    } else  */{
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

