// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "texture.h"

#include "pxr/imaging/hd/sceneDelegate.h"

#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    int _profilerCategory = MProfiler::addCategory("HdVP2RenderDelegate", "HdVP2RenderDelegate");   //!< Profiler category
} //namespace

/*! \brief  Constructor
*/
HdVP2Texture::HdVP2Texture(
    HdVP2RenderDelegate* _renderDelegate, const SdfPath& id
)   : HdTexture(id)
    , _renderDelegate(_renderDelegate)
{
}

/*! \brief  Return the texture resource
*/
HdTextureResourceSharedPtr
HdVP2Texture::_GetTextureResource(
    HdSceneDelegate* sceneDelegate,
    const SdfPath& sceneID,
    HdTextureResource::ID texID
) const {
    TF_UNUSED(texID);
    return sceneDelegate->GetTextureResource(sceneID);
}

PXR_NAMESPACE_CLOSE_SCOPE

