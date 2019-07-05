//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

