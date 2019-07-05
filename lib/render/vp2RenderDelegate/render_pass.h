#ifndef HD_VP2_RENDER_PASS
#define HD_VP2_RENDER_PASS

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdVP2RenderDelegate;

/*! \brief    Empty render pass class. Unused for rendering with VP2 but required by HdEngine & HdRenderDelegate.
    \class    HdVP2RenderPass
*/
class HdVP2RenderPass final : public HdRenderPass
{
public:
    //! \brief  Constructor
    HdVP2RenderPass(HdVP2RenderDelegate* delegate, HdRenderIndex *index, HdRprimCollection const &collection)
        : HdRenderPass(index, collection), _delegate(delegate)
    {}

    //! \brief  Destructor
    virtual ~HdVP2RenderPass() {}

    //! \brief  Empty execute
    void _Execute(HdRenderPassStateSharedPtr const &renderPassState, TfTokenVector const &renderTags) override {}

private:
    HdVP2RenderDelegate* _delegate{ nullptr };  // !< VP2 render delegate for which this render pass was created

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif