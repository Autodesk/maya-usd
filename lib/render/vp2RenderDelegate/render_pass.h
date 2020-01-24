//
// Copyright 2019 Autodesk
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

#ifndef HD_VP2_RENDER_PASS
#define HD_VP2_RENDER_PASS

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
        : HdRenderPass(index, collection)
    {}

    //! \brief  Destructor
    virtual ~HdVP2RenderPass() {}

    //! \brief  Empty execute
    void _Execute(HdRenderPassStateSharedPtr const &renderPassState, TfTokenVector const &renderTags) override {}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
