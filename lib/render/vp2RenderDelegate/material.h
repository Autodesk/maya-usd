#ifndef HD_VP2_MATERIAL
#define HD_VP2_MATERIAL

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

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
    MHWRender::MShaderInstance* GetDisplacementShader() const;

    /*! \brief  Gets primvar names used as UVs for textures in this material.
    */
    const std::set<TfToken>& GetUVPrimvarNames() const {
        return _uvPrimvarNames;
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
    HdVP2TextureResourceSharedPtr _GetTextureResource(
        HdSceneDelegate *sceneDelegate,
        HdMaterialParam const& param
    );

    HdVP2RenderDelegate* const _renderDelegate;		//!< VP2 render delegate for which this material was created

	VP2ShaderUniquePtr  _surfaceShader;         //!< VP2 surface shader instance
	VP2ShaderUniquePtr  _displacementShader;    //!< VP2 displacement shader instance

    std::set<TfToken>   _uvPrimvarNames;    //!< primvar names used as UVs for textures in this material
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
