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

#ifndef PROXY_RENDER_DELEGATE
#define PROXY_RENDER_DELEGATE

#include "pxr/pxr.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include "../../base/api.h"

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MPxSubSceneOverride.h>

#include <memory>

#if defined(WANT_UFE_BUILD)
#include <ufe/observer.h>
#endif

// Conditional compilation due to Maya API gap.
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

/*! \brief  Enumerations for selection status
*/
enum HdVP2SelectionStatus {
    kUnselected        = 0, //!< The Rprim is not selected
    kPartiallySelected = 1, //!< The Rprim is partially selected (only applicable for instanced Rprims)
    kFullySelected     = 2  //!< The Rprim is selected (meaning fully selected for instanced Rprims)
};

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

    //! \brief  Return pointer to DG proxy shape node
    MAYAUSD_CORE_PUBLIC
    const MayaUsdProxyShapeBase* getProxyShape() const { return _proxyShape; }

    MAYAUSD_CORE_PUBLIC
    void SelectionChanged();

    MAYAUSD_CORE_PUBLIC
    const MColor& GetWireframeColor() const;

    MAYAUSD_CORE_PUBLIC
    const HdSelection::PrimSelectionState* GetPrimSelectionState(const SdfPath& path) const;

    MAYAUSD_CORE_PUBLIC
    HdVP2SelectionStatus GetPrimSelectionStatus(const SdfPath& path) const;

private:
    ProxyRenderDelegate(const ProxyRenderDelegate&) = delete;
    ProxyRenderDelegate& operator=(const ProxyRenderDelegate&) = delete;

    void _InitRenderDelegate();
    bool _Populate();
    void _UpdateSceneDelegate();
    void _Execute(const MHWRender::MFrameContext& frameContext);

    bool _isInitialized();

    void _FilterSelection();
    void _UpdateSelectionStates();

    const MayaUsdProxyShapeBase*  _proxyShape{ nullptr }; //!< DG proxy shape node
    MDagPath                      _proxyDagPath;          //!< DAG path of the proxy shape (assuming no DAG instancing)

    // USD & Hydra Objects
    HdEngine            _engine;                    //!< Hydra engine responsible for running synchronization between scene delegate and VP2RenderDelegate
    HdTaskSharedPtrVector _dummyTasks;              //!< Dummy task to bootstrap data preparation inside Hydra engine
    UsdStageRefPtr      _usdStage;                  //!< USD stage pointer
    HdRenderDelegate*   _renderDelegate{ nullptr }; //!< VP2RenderDelegate
    HdRenderIndex*      _renderIndex{ nullptr };    //!< Flattened representation of client scene graph
    HdxTaskController*  _taskController{ nullptr }; //!< Task controller necessary for execution with hydra engine (we don't really need it, but there doesn't seem to be a way to get synchronization running without it)
    UsdImagingDelegate* _sceneDelegate{ nullptr };  //!< USD scene delegate

    size_t              _excludePrimPathsVersion{ 0 }; //!< Last version of exluded prims used during render index populate

    bool                _isPopulated{ false };      //!< If false, scene delegate wasn't populated yet within render index
    bool                _selectionChanged{ false }; //!< Whether there is any selection change or not
    bool                _isProxySelected{ false };  //!< Whether the proxy shape is selected
    MColor              _wireframeColor;            //!< Wireframe color assigned to the proxy shape

    //! A collection of Rprims to prepare render data for specified reprs
    std::unique_ptr<HdRprimCollection> _defaultCollection;

    //! A collection of Rprims being selected
    HdSelectionSharedPtr               _selection;

#if defined(WANT_UFE_BUILD)
    //! Observer for UFE global selection change
    Ufe::Observer::Ptr  _ufeSelectionObserver;
#else
    //! Support proxy selection highlight when UFE is not available.
    MCallbackId         _mayaSelectionCallbackId{ 0 };
#endif

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    //! Adjustment mode for global selection list: ADD, REMOVE, REPLACE, XOR
    MGlobal::ListAdjustment _globalListAdjustment;
#endif
};

/*! \brief  Is this object properly initialized and can start receiving updates. Once this is done, render index needs to be populated and then we rely on change tracker.
*/
inline bool ProxyRenderDelegate::_isInitialized() {
    return (_sceneDelegate != nullptr);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
