//
// Copyright 2019 Autodesk, Inc. All rights reserved.
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

#ifndef HD_VP2_MATERIAL
#define HD_VP2_MATERIAL

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"

#include <maya/MShaderManager.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2RenderDelegate;

using HdVP2TextureResourceSharedPtr = boost::shared_ptr<class HdVP2TextureResource>;
using HdVP2TextureResourceSharedPtrVector = std::vector<HdVP2TextureResourceSharedPtr>;

/*! \brief  A VP2-specific implementation for a Hydra material prim.
    \class  HdVP2Material

    Provides a basic implementation of a Hydra material.

    A Blinn fragment is used to prototype the surface preview node, but it
    should eventually be replaced with pxrUsdPreviewSurface (see MAYA-98711).

    Only one texture named diffuseColor is currently supported. More work is
    required to support multiple textures (to work around Maya API limitations
    around sharable fragment parameters).

    Transparency is not currently supported.

    Mapping different color channels of the same texture to different surface
    shader inputs (e.g. RGB to color, A to opacity) without duplication
    requires new Maya APIs.
*/
class HdVP2Material final : public HdMaterial {
public:
    HdVP2Material(HdVP2RenderDelegate*, const SdfPath&);

    ~HdVP2Material() override;

    void Sync(HdSceneDelegate*, HdRenderParam*, HdDirtyBits*) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;
    void Reload() override;

    MHWRender::MShaderInstance* GetSurfaceShader() const;

    /*! \brief  Get primvar requirements required by this material.
    */
    const TfTokenVector& GetRequiredPrimvars() const {
        return _requiredPrimvars;
    }

    /*! \brief  A deleter for Maya shaders, for use with smart pointers.
    */
    struct VP2ShaderDeleter
    {
        void operator () (MHWRender::MShaderInstance*);
    };

    using VP2ShaderUniquePtr = std::unique_ptr<
        MHWRender::MShaderInstance,
        VP2ShaderDeleter
    >;

private:
    MHWRender::MShaderInstance* _CreateShaderInstance(const HdMaterialNetwork& mat);
    void _UpdateShaderInstance(const HdMaterialNetwork& mat);

    HdVP2RenderDelegate* const _renderDelegate; //!< VP2 render delegate for which this material was created

    VP2ShaderUniquePtr  _surfaceShader;         //!< VP2 surface shader instance
    SdfPath _surfaceShaderId;

    TfTokenVector _requiredPrimvars;            //!< primvars required by this material
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
