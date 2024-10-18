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
#ifndef HD_VP2_RENDER_PARAM
#define HD_VP2_RENDER_PARAM

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MPxSubSceneOverride.h>

PXR_NAMESPACE_OPEN_SCOPE

class ProxyRenderDelegate;

/*! \brief  The HdRenderParam is an opaque(to core Hydra) handle, passed to each prim
            during Sync processing and providing access to VP2.
    \class  HdVP2RenderParam
*/
class HdVP2RenderParam final : public HdRenderParam
{
public:
    /*! \brief  Constructor
     */
    HdVP2RenderParam(ProxyRenderDelegate& drawScene)
        : _drawScene(drawScene)
    {
    }

    /*! \brief  Destructor
     */
    ~HdVP2RenderParam() override = default;

    void BeginUpdate(MSubSceneContainer& container, UsdTimeCode frame);
    void EndUpdate();

    /*! \brief  Get access to subscene override used to draw the scene
     */
    ProxyRenderDelegate& GetDrawScene() const { return _drawScene; }

    /*! \brief  Get access to render item container - only valid during draw update
     */
    MSubSceneContainer* GetContainer() const { return _container; }

    /*! \brief  Refreshed during each update, provides info about currently refreshed frame
     */
    UsdTimeCode GetFrame() const { return _frame; }

protected:
    ProxyRenderDelegate&
                        _drawScene; //!< Subscene override used as integration interface for HdVP2RenderDelegate
    MSubSceneContainer* _container {
        nullptr
    }; //!< Container to all render items, only valid between begin and end update of subscene
       //!< override.
    UsdTimeCode _frame { UsdTimeCode::Default() }; //!< Rendered frame (useful for caching of data)
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
