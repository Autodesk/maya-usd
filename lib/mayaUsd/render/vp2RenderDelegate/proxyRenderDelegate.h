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

#include <mayaUsd/base/api.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hd/task.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/version.h>

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPxSubSceneOverride.h>

#include <memory>

#if defined(WANT_UFE_BUILD)
#include <ufe/observer.h>
#endif

// Conditional compilation due to Maya API gap.
#if MAYA_API_VERSION >= 20200000
#define MAYA_ENABLE_UPDATE_FOR_SELECTION
#endif

#if PXR_VERSION < 2008
#define ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
/*  In USD v20.05 and earlier when the purpose of an rprim changes the visibility gets dirtied,
    and that doesn't update the render tag version.

    Pixar is in the process of fixing this one as noted in:
    https://groups.google.com/forum/#!topic/usd-interest/9pzFbtCEY-Y

    Logged as:
    https://github.com/PixarAnimationStudios/USD/issues/1243
*/
#endif

// Use the latest MPxSubSceneOverride API
#ifndef OPENMAYA_MPXSUBSCENEOVERRIDE_LATEST_NAMESPACE
#define OPENMAYA_MPXSUBSCENEOVERRIDE_LATEST_NAMESPACE OPENMAYA_MAJOR_NAMESPACE
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
enum HdVP2SelectionStatus
{
    kUnselected,        //!< A Rprim is not selected
    kPartiallySelected, //!< A Rprim is partially selected (only applicable for instanced Rprims)
    kFullyActive,       //!< A Rprim is active (meaning fully active for instanced Rprims)
    kFullyLead          //!< A Rprim is lead (meaning fully lead for instanced Rprims)
};

//! Pick resolution behavior to use when the picked object is a point instance.
enum class UsdPointInstancesPickMode
{
    //! The PointInstancer prim that generated the point instance is picked. If
    //! multiple nested PointInstancers are involved, the top-level
    //! PointInstancer is the one picked. If a selection kind is specified, the
    //! traversal up the hierarchy looking for a kind match will begin at that
    //! PointInstancer.
    PointInstancer,
    //! The specific point instance is picked. These are represented as
    //! UsdSceneItems with UFE paths to a PointInstancer prim and a non-negative
    //! instanceIndex for the specific point instance. In this mode, any setting
    //! for selection kind is ignored.
    Instances,
    //! The prototype being instanced by the point instance is picked. If a
    //! selection kind is specified, the traversal up the hierarchy looking for
    //! a kind match will begin at the prototype prim.
    Prototypes
};

/*! \brief  USD Proxy rendering routine via VP2 MPxSubSceneOverride

    This drawing routine leverages HdVP2RenderDelegate for synchronization
    of data between scene delegate and VP2. Final rendering is done by VP2
    as part of subscene override mechanism.

    USD Proxy can be rendered in a number of ways and by default we use VP2RenderDelegate.
    Use MAYAUSD_DISABLE_VP2_RENDER_DELEGATE  env variable before loading USD
    plugin to switch to the legacy rendering with draw override approach.
*/
class ProxyRenderDelegate
    : public Autodesk::Maya::OPENMAYA_MPXSUBSCENEOVERRIDE_LATEST_NAMESPACE::MHWRender::
          MPxSubSceneOverride
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
    bool requiresUpdate(const MSubSceneContainer& container, const MFrameContext& frameContext)
        const override;

    MAYAUSD_CORE_PUBLIC
    void update(MSubSceneContainer& container, const MFrameContext& frameContext) override;

    MAYAUSD_CORE_PUBLIC
    void
    updateSelectionGranularity(const MDagPath& path, MSelectionContext& selectionContext) override;

    MAYAUSD_CORE_PUBLIC
    bool getInstancedSelectionPath(
        const MRenderItem&   renderItem,
        const MIntersection& intersection,
        MDagPath&            dagPath) const override;

#ifdef MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT
    MAYAUSD_CORE_PUBLIC
    bool
    updateUfeIdentifiers(MHWRender::MRenderItem& renderItem, MStringArray& ufeIdentifiers) override;
#endif

#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
    MAYAUSD_CORE_PUBLIC
    SdfPath GetScenePrimPath(
        const SdfPath&      rprimId,
        int                 instanceIndex,
        HdInstancerContext* instancerContext = nullptr) const;
#else
    MAYAUSD_CORE_PUBLIC
    SdfPath GetScenePrimPath(const SdfPath& rprimId, int instanceIndex) const;
#endif

    MAYAUSD_CORE_PUBLIC
    void SelectionChanged();

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    MAYAUSD_CORE_PUBLIC
    void DisplayLayerMembershipChanged();

    MAYAUSD_CORE_PUBLIC
    void DisplayLayerDirty(MFnDisplayLayer& displayLayer);
#endif

    MAYAUSD_CORE_PUBLIC
    const MColor& GetWireframeColor() const;

    MAYAUSD_CORE_PUBLIC
    GfVec3f GetDefaultColor(const TfToken& className);

    // Returns the selection highlight color for a given HdPrim type.
    // If className is empty, returns the lead highlight color.
    MAYAUSD_CORE_PUBLIC
    MColor GetSelectionHighlightColor(const TfToken& className = TfToken());

    MAYAUSD_CORE_PUBLIC
    const HdSelection::PrimSelectionState* GetLeadSelectionState(const SdfPath& path) const;

    MAYAUSD_CORE_PUBLIC
    const HdSelection::PrimSelectionState* GetActiveSelectionState(const SdfPath& path) const;

    MAYAUSD_CORE_PUBLIC
    HdVP2SelectionStatus GetSelectionStatus(const SdfPath& path) const;

    MAYAUSD_CORE_PUBLIC
    bool DrawRenderTag(const TfToken& renderTag) const;

    MAYAUSD_CORE_PUBLIC
    UsdImagingDelegate* GetUsdImagingDelegate() const;

    MAYAUSD_CORE_PUBLIC
    MDagPath GetProxyShapeDagPath() const;

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MAYAUSD_CORE_PUBLIC
    bool SnapToSelectedObjects() const;

    MAYAUSD_CORE_PUBLIC
    bool SnapToPoints() const;
#endif

private:
    ProxyRenderDelegate(const ProxyRenderDelegate&) = delete;
    ProxyRenderDelegate& operator=(const ProxyRenderDelegate&) = delete;

    //! \brief The main stages of update
    void _ClearInvalidData(MSubSceneContainer& container);
    void _InitRenderDelegate();
    bool _Populate();
    void _UpdateSceneDelegate();
    void _Execute(const MHWRender::MFrameContext& frameContext);

    bool _isInitialized();
    void _PopulateSelection();
    void _UpdateSelectionStates();
    void _UpdateRenderTags();
    void _ClearRenderDelegate();
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    void _DirtyUfeSubtree(const MString& root);
    void _DirtyUsdSubtree(const UsdPrim& prim);
    void _UpdateDisplayLayers();
    void _RequestRefresh();
#endif
    SdfPathVector
    _GetFilteredRprims(HdRprimCollection const& collection, TfTokenVector const& renderTags);

    /*! \brief  Hold all data related to the proxy shape.

        In addition to holding data read from the proxy shape, ProxyShapeData tracks when data read
        from the proxy shape changes. For simple numeric types cache the last value read from
        _proxyShape & compare to the current value. For complicated types we keep a version number
        of the last value we read to make fast comparisons.
    */
    class ProxyShapeData
    {
        const MayaUsdProxyShapeBase* const _proxyShape { nullptr }; //!< DG proxy shape node
        const MDagPath _proxyDagPath; //!< DAG path of the proxy shape (assuming no DAG instancing)
        UsdStageRefPtr _usdStage;     //!< USD stage pointer
        size_t         _excludePrimsVersion {
            0
        }; //!< Last version of exluded prims used during render index populate
        size_t _usdStageVersion { 0 }; //!< Last version of stage used during render index populate
        bool   _drawRenderPurpose {
            false
        }; //!< Should the render delegate draw rprims with the "render" purpose
        bool _drawProxyPurpose {
            false
        }; //!< Should the render delegate draw rprims with the "proxy" purpose
        bool _drawGuidePurpose {
            false
        }; //!< Should the render delegate draw rprims with the "guide" purpose
    public:
        ProxyShapeData(const MayaUsdProxyShapeBase* proxyShape, const MDagPath& proxyDagPath);
        const MayaUsdProxyShapeBase* ProxyShape() const;
        const MDagPath&              ProxyDagPath() const;
        UsdStageRefPtr               UsdStage() const;
        void                         UpdateUsdStage();
        bool                         IsUsdStageUpToDate() const;
        void                         UsdStageUpdated();
        bool                         IsExcludePrimsUpToDate() const;
        void                         ExcludePrimsUpdated();
        void                         UpdatePurpose(
                                    bool* drawRenderPurposeChanged,
                                    bool* drawProxyPurposeChanged,
                                    bool* drawGuidePurposeChanged);
        bool DrawRenderPurpose() const;
        bool DrawProxyPurpose() const;
        bool DrawGuidePurpose() const;
    };
    std::unique_ptr<ProxyShapeData> _proxyShapeData;

    // USD & Hydra Objects
    HdEngine _engine; //!< Hydra engine responsible for running synchronization between scene
                      //!< delegate and VP2RenderDelegate
    HdTaskSharedPtrVector
        _dummyTasks; //!< Dummy task to bootstrap data preparation inside Hydra engine

    std::unique_ptr<HdRenderDelegate> _renderDelegate; //!< VP2RenderDelegate
    std::unique_ptr<HdRenderIndex> _renderIndex; //!< Flattened representation of client scene graph
    std::unique_ptr<HdxTaskController>
        _taskController; //!< Task controller necessary for execution with hydra engine (we don't
                         //!< really need it, but there doesn't seem to be a way to get
                         //!< synchronization running without it)
    std::unique_ptr<UsdImagingDelegate> _sceneDelegate; //!< USD scene delegate

    bool _isPopulated {
        false
    }; //!< If false, scene delegate wasn't populated yet within render index
    bool _selectionChanged { true }; //!< Whether there is any selection change or not

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    bool _refreshRequested {
        false
    }; //!< True if the render delegate has already requested a refresh.
    bool _displayLayerMembershipChanged {
        false
    }; //~< Whether or not there is any display layer membership changes.
    MCallbackId              _mayaDisplayLayerMembersCallbackId { 0 };
    MObjectArray             _mayaDisplayLayers;
    std::vector<MCallbackId> _mayaDisplayLayerCallbacks;
#endif

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    bool _selectionModeChanged { true }; //!< Whether the global selection mode has changed
    bool _snapToPoints { false };        //!< Whether point snapping is enabled or not
    bool _snapToSelectedObjects {
        false
    }; //!< Whether point snapping should snap to selected objects
#endif
    MColor _wireframeColor; //!< Wireframe color assigned to the proxy shape

    std::mutex _mayaCommandEngineMutex;
    uint64_t   _frameCounter { 0 };

    typedef std::pair<MColor, std::atomic<uint64_t>>  MColorCache;
    typedef std::pair<GfVec3f, std::atomic<uint64_t>> GfVec3fCache;

    MColorCache  _activeMeshColorCache { MColor(), 0 };
    MColorCache  _activeCurveColorCache { MColor(), 0 };
    MColorCache  _activePointsColorCache { MColor(), 0 };
    MColorCache  _leadColorCache { MColor(), 0 };
    GfVec3fCache _dormantCurveColorCache { GfVec3f(), 0 };
    GfVec3fCache _dormantPointsColorCache { GfVec3f(), 0 };

    //! A collection of Rprims to prepare render data for specified reprs
    std::unique_ptr<HdRprimCollection> _defaultCollection;

    // Version Id values saved from the last time we queried the HdChangeTracker.
    // These values are initialized to 1 in HdChangeTracker
    struct HdChangeTrackerVersions
    {
        //! The render tag version the last time _Execute was called
        unsigned int _renderTag { 0 };
        //! The visibility change count the last time _Execute was called
        unsigned int _visibility { 0 };
        //! The combined instancer index version and instance index change count the last time
        //! _Execute was called
        unsigned int _instanceIndex { 0 };

        void sync(const HdChangeTracker& tracker)
        {
            _renderTag = tracker.GetRenderTagVersion();
            _visibility = tracker.GetVisibilityChangeCount();
            _instanceIndex = tracker.GetInstancerIndexVersion();
        }

        bool renderTagValid(const HdChangeTracker& tracker)
        {
            return _renderTag == tracker.GetRenderTagVersion();
        }

        bool visibilityValid(const HdChangeTracker& tracker)
        {
            return _visibility == tracker.GetVisibilityChangeCount();
        }

        bool instanceIndexValid(const HdChangeTracker& tracker)
        {
            return _instanceIndex == tracker.GetInstancerIndexVersion();
        }

        void reset()
        {
            _renderTag = 0;
            _visibility = 0;
            _instanceIndex = 0;
        }
    };
    HdChangeTrackerVersions _changeVersions;
    bool                    _taskRenderTagsValid {
        false
    }; //!< If false the render tags on the dummy render task are not the minimum set of tags.

    MHWRender::DisplayStatus _displayStatus {
        MHWRender::kNoStatus
    };                                     //!< The display status of the proxy shape
    HdSelectionSharedPtr _leadSelection;   //!< A collection of Rprims being lead selection
    HdSelectionSharedPtr _activeSelection; //!< A collection of Rprims being active selection

#if defined(WANT_UFE_BUILD)
    //! Observer to listen to UFE changes
    Ufe::Observer::Ptr _observer;
#else
    //! Minimum support for proxy selection when UFE is not available.
    MCallbackId _mayaSelectionCallbackId { 0 };
#endif

#if defined(WANT_UFE_BUILD) && defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    //! Adjustment mode for global selection list: ADD, REMOVE, REPLACE, XOR
    MGlobal::ListAdjustment _globalListAdjustment;

    //! Token of the Kind to be selected from viewport. If it empty, select the exact prims.
    TfToken _selectionKind;

    //! Pick resolution behavior to use when the picked object is a point instance.
    UsdPointInstancesPickMode _pointInstancesPickMode;
#endif
};

/*! \brief  Is this object properly initialized and can start receiving updates. Once this is done,
 * render index needs to be populated and then we rely on change tracker.
 */
inline bool ProxyRenderDelegate::_isInitialized() { return (_sceneDelegate != nullptr); }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
