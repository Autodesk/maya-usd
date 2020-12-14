//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/usdmaya/Api.h"

#include <maya/MPxSurfaceShapeUI.h>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The UI component of the proxy shape node
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeUI : public MPxSurfaceShapeUI
{
public:
    /// \brief  ctor
    ProxyShapeUI();

    /// \brief  dtor
    ~ProxyShapeUI();

    /// \brief  returns a new instance of this UI component
    AL_USDMAYA_PUBLIC
    static void* creator();

    /// \brief  legacy VP1 rendering interface
    /// \param  drawInfo  Drawing state information.
    /// \param  isObjectAndActiveOnly Used to determine if draw requests for components need to be
    /// supplied. If false, some
    ///         or all components are active and draw requests must be built for all components.
    /// \param  requests Queue on which to place the draw request.
    void getDrawRequests(
        const MDrawInfo&   drawInfo,
        bool               isObjectAndActiveOnly,
        MDrawRequestQueue& requests) override;

    /// \param  request the drawing request
    /// \param  view  the interactive 3d view in which to draw
    void draw(const MDrawRequest& request, M3dView& view) const override;

    /// \brief  used to select the proxy shape
    /// \param  selectInfo  the Selection state information.
    /// \param  selectionList List of items selected by this method.
    /// \param  worldSpaceSelectPts List of points used to sort corresponding selections in
    /// single-select mode. \return true if something was selected, false otherwise
    bool select(
        MSelectInfo&    selectInfo,
        MSelectionList& selectionList,
        MPointArray&    worldSpaceSelectPts) const override;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
