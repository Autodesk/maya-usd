#ifndef HD_VP2_RENDER_PARAM
#define HD_VP2_RENDER_PARAM

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/pxr.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MPxSubSceneOverride.h>

PXR_NAMESPACE_OPEN_SCOPE

class ProxyRenderDelegate;

/*! \brief  The HdRenderParam is an opaque(to core Hydra) handle, passed to each prim
            during Sync processing and providing access to VP2.
    \class  HdVP2RenderParam
*/
class HdVP2RenderParam final : public HdRenderParam {
public:
    /*! \brief  Constructor
    */
    HdVP2RenderParam(ProxyRenderDelegate& drawScene) 
        : _drawScene(drawScene) {}

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
    MSubSceneContainer*  GetContainer() const { return _container; }
    
    /*! \brief  Refreshed during each update, provides info about currently refreshed frame
    */
    UsdTimeCode          GetFrame() const { return _frame;  }

protected:
    ProxyRenderDelegate& _drawScene;                        //!< Subscene override used as integration interface for HdVP2RenderDelegate
    MSubSceneContainer*  _container{ nullptr };             //!< Container to all render items, only valid between begin and end update of subscene override.
    UsdTimeCode          _frame{ UsdTimeCode::Default() };  //!< Rendered frame (useful for caching of data)
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
