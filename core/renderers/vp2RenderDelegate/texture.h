#ifndef HD_VP2_TEXTURE
#define HD_VP2_TEXTURE

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/pxr.h"
#include "pxr/imaging/hd/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2RenderDelegate;

/*! \brief  VP2 implementation of a Hydra texture.
    \class  HdVP2Texture
*/
class HdVP2Texture final : public HdTexture {
public:
    HdVP2Texture(HdVP2RenderDelegate*, const SdfPath&);

    ~HdVP2Texture() override = default;

private:
    HdTextureResourceSharedPtr _GetTextureResource(
        HdSceneDelegate*, const SdfPath&, HdTextureResource::ID
    ) const override;

    HdVP2RenderDelegate* const _renderDelegate;  //!< VP2 render delegate for which this texture was created
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
