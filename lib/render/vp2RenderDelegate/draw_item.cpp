// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/imaging/hd/mesh.h"

#include "draw_item.h"

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Constructor.

Data holder for its corresponding render item to facilitate parallelized evaluation.

Position buffer is held by HdVP2Mesh and shared among its render items.
*/
HdVP2DrawItem::HdVP2DrawItem(const HdRprimSharedData* sharedData, const HdMeshReprDesc& desc)
: HdDrawItem(sharedData)
{
    _renderItemId = GetRprimID().GetPrimPath().AppendChild(TfToken(TfStringPrintf("DrawItem_%p", this)));
    _renderItemName = GetRenderItemId().GetText();

    _mesh._indexBuffer.reset(new MHWRender::MIndexBuffer(MHWRender::MGeometry::kUnsignedInt32));

    if (desc.geomStyle != HdMeshGeomStylePoints)
    {
        const MHWRender::MVertexBufferDescriptor nbDesc("",
            MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3);
        _mesh._normalsBuffer.reset(new MHWRender::MVertexBuffer(nbDesc));

        const MHWRender::MVertexBufferDescriptor uvBufferDesc(
            "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 2);
        _mesh._uvBuffer.reset(new MHWRender::MVertexBuffer(uvBufferDesc));
    }
}

//! \brief  Destructor.
HdVP2DrawItem::~HdVP2DrawItem() {
}

//! \brief  Get access to render item data.
HdVP2DrawItem::RenderItemData& HdVP2DrawItem::GetRenderItemData() {
    return _mesh;
}

PXR_NAMESPACE_CLOSE_SCOPE
