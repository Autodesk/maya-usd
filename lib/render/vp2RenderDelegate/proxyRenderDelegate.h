#ifndef PROXY_RENDER_DELEGATE
#define PROXY_RENDER_DELEGATE

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================


#include "pxr/pxr.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include "../../base/api.h"

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPxSubSceneOverride.h>

// Maya 2020 adds new API to enable subscene update in selection passes for
// deferred update of selection render items.
#if MAYA_API_VERSION >= 20200000
#define MAYA_ENABLE_UPDATE_FOR_SELECTION
#endif

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;
class HdRenderIndex;
class HdRprimCollection;
class UsdImagingDelegate;
class MayaUsdProxyShapeBase;
class HdxTaskController;

/*! \brief  USD Proxy rendering routine via VP2 MPxSubSceneOverride

    This drawing routine leverages HdVP2RenderDelegate for synchronization
    of data between scene delegate and VP2. Final rendering is done by VP2
    as part of subscene override mechanism.

    USD Proxy can be rendered in a number of ways, to enable this drawing
    path set VP2_RENDER_DELEGATE_PROXY env variable before loading USD
    plugin.
*/
class ProxyRenderDelegate : public MHWRender::MPxSubSceneOverride
{
    ProxyRenderDelegate(const MObject& obj);

public:
    MAYAUSD_CORE_PUBLIC
    ~ProxyRenderDelegate() override;

    MAYAUSD_CORE_PUBLIC
    static const MString drawDbClassification;

    MAYAUSD_CORE_PUBLIC
    static MHWRender::MPxSubSceneOverride* Creator(const MObject& obj);

    MAYAUSD_CORE_PUBLIC
    MHWRender::DrawAPI supportedDrawAPIs() const override;

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    MAYAUSD_CORE_PUBLIC
    bool enableUpdateForSelection() const override;
#endif

    MAYAUSD_CORE_PUBLIC
    bool requiresUpdate(const MSubSceneContainer& container, const MFrameContext& frameContext) const override;

    MAYAUSD_CORE_PUBLIC
    void update(MSubSceneContainer& container, const MFrameContext& frameContext) override;

    MAYAUSD_CORE_PUBLIC
    void updateSelectionGranularity(
        const MDagPath& path,
        MSelectionContext& selectionContext) override;

    MAYAUSD_CORE_PUBLIC
    bool getInstancedSelectionPath(
        const MRenderItem& renderItem,
        const MIntersection& intersection,
        MDagPath& dagPath) const override;

    MAYAUSD_CORE_PUBLIC
    MayaUsdProxyShapeBase* getProxyShape() const;

private:
    ProxyRenderDelegate(const ProxyRenderDelegate&) = delete;
    ProxyRenderDelegate& operator=(const ProxyRenderDelegate&) = delete;

    void _InitRenderDelegate();
    bool _Populate();
    void _UpdateTime();
    void _Execute(const MHWRender::MFrameContext& frameContext);

    bool _isInitialized();

    MObject             _mObject;                   //!< Proxy shape MObject

    // USD & Hydra Objects
    HdEngine            _engine;                    //!< Hydra engine responsible for running synchronization between scene delegate and VP2RenderDelegate
    UsdStageRefPtr      _usdStage;                  //!< USD stage pointer
    HdRenderDelegate*   _renderDelegate{ nullptr }; //!< VP2RenderDelegate
    HdRenderIndex*      _renderIndex{ nullptr };    //!< Flattened representation of client scene graph
    HdRprimCollection*  _renderCollection{nullptr}; //!< Global switch of display representations
    HdxTaskController*  _taskController{ nullptr }; //!< Task controller necessary for execution with hydra engine (we don't really need it, but there doesn't seem to be a way to get synchronization running without it)
    UsdImagingDelegate* _sceneDelegate{ nullptr };  //!< USD scene delegate

    bool                _isPopulated{ false };      //!< If false, scene delegate wasn't populated yet within render index

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    MGlobal::ListAdjustment _globalListAdjustment;  //!< Adjustment mode for global selection list: ADD, REMOVE, REPLACE, XOR
#endif
};

/*! \brief  Is this object properly initialized and can start receiving updates. Once this is done, render index needs to be populated and then we rely on change tracker.
*/
inline bool ProxyRenderDelegate::_isInitialized() {
    return (_sceneDelegate != nullptr);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
