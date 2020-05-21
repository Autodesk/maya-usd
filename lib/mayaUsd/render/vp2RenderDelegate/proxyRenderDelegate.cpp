//
// Copyright 2020 Autodesk
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
#include "proxyRenderDelegate.h"

#include <maya/MFileIO.h>
#include <maya/MFnPluginData.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MEventMessage.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionContext.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/selectionTracker.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/primGather.h>

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/utils/util.h>

#include "render_delegate.h"
#include "tokens.h"

#if defined(WANT_UFE_BUILD)
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/runTimeMgr.h>
#include <ufe/sceneItem.h>
#include <ufe/selectionNotification.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
    //! Representation selector for shaded and textured viewport mode
    const HdReprSelector kSmoothHullReprSelector(HdReprTokens->smoothHull);

    //! Representation selector for wireframe viewport mode
    const HdReprSelector kWireReprSelector(TfToken(), HdReprTokens->wire);

    //! Representation selector for bounding box viewport mode
    const HdReprSelector kBBoxReprSelector(TfToken(), HdVP2ReprTokens->bbox);

    //! Representation selector for point snapping
    const HdReprSelector kPointsReprSelector(TfToken(), TfToken(), HdReprTokens->points);

    //! Representation selector for selection update
    const HdReprSelector kSelectionReprSelector(HdVP2ReprTokens->selection);

#if defined(WANT_UFE_BUILD)
    MGlobal::ListAdjustment GetListAdjustment()
    {
        // Keyboard modifiers can be queried from QApplication::keyboardModifiers()
        // in case running MEL command leads to performance hit. On the other hand
        // the advantage of using MEL command is the platform-agnostic state of the
        // CONTROL key that it provides for aligning to Maya's implementation.
        int modifiers = 0;
        MGlobal::executeCommand("getModifiers", modifiers);

        const bool shiftHeld = (modifiers % 2);
        const bool ctrlHeld = (modifiers / 4 % 2);

        MGlobal::ListAdjustment listAdjustment = MGlobal::kReplaceList;

        if (shiftHeld && ctrlHeld)
        {
            listAdjustment = MGlobal::kAddToList;
        }
        else if (ctrlHeld)
        {
            listAdjustment = MGlobal::kRemoveFromList;
        }
        else if (shiftHeld)
        {
            listAdjustment = MGlobal::kXORWithList;
        }

        return listAdjustment;
    }
#endif // defined(WANT_UFE_BUILD)

    //! \brief  Configure repr descriptions
    void _ConfigureReprs()
    {
        const HdMeshReprDesc reprDescHull(
            HdMeshGeomStyleHull,
            HdCullStyleDontCare,
            HdMeshReprDescTokens->surfaceShader,
            /*flatShadingEnabled=*/false,
            /*blendWireframeColor=*/false);

        const HdMeshReprDesc reprDescEdge(
            HdMeshGeomStyleHullEdgeOnly,
            HdCullStyleDontCare,
            HdMeshReprDescTokens->surfaceShader,
            /*flatShadingEnabled=*/false,
            /*blendWireframeColor=*/false
        );

        // Hull desc for shaded display, edge desc for selection highlight.
        HdMesh::ConfigureRepr(HdReprTokens->smoothHull, reprDescHull, reprDescEdge);

        // Edge desc for bbox display.
        HdMesh::ConfigureRepr(HdVP2ReprTokens->bbox, reprDescEdge);

        // Special token for selection update and no need to create repr. Adding
        // the empty desc to remove Hydra warning.
        HdMesh::ConfigureRepr(HdVP2ReprTokens->selection, HdMeshReprDesc());

        // Wireframe desc for bbox display.
        HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->bbox,
            HdBasisCurvesGeomStyleWire);

        // Special token for selection update and no need to create repr. Adding
        // the null desc to remove Hydra warning.
        HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->selection,
            HdBasisCurvesGeomStyleInvalid);
    }

#if defined(WANT_UFE_BUILD)
    class UfeSelectionObserver : public Ufe::Observer
    {
    public:
        UfeSelectionObserver(ProxyRenderDelegate& proxyRenderDelegate)
            : Ufe::Observer()
            , _proxyRenderDelegate(proxyRenderDelegate)
        {
        }

        void operator()(const Ufe::Notification& notification) override
        {
            // During Maya file read, each node will be selected in turn, so we get
            // notified for each node in the scene.  Prune this out.
            if (MFileIO::isOpeningFile()) {
                return;
            }

            auto selectionChanged =
                dynamic_cast<const Ufe::SelectionChanged*>(&notification);
            if (selectionChanged != nullptr) {
                _proxyRenderDelegate.SelectionChanged();
            }
        }

    private:
        ProxyRenderDelegate& _proxyRenderDelegate;
    };
#else
    void SelectionChangedCB(void* data)
    {
        ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(data);
        if (prd) {
            prd->SelectionChanged();
        }
    }
#endif

} // namespace

//! \brief  Draw classification used during plugin load to register in VP2
const MString ProxyRenderDelegate::drawDbClassification(
    TfStringPrintf("drawdb/subscene/vp2RenderDelegate/%s",
        MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText()).c_str());

//! \brief  Factory method registered at plugin load
MHWRender::MPxSubSceneOverride* ProxyRenderDelegate::Creator(const MObject& obj) {
    return new ProxyRenderDelegate(obj);
}

//! \brief  Constructor
ProxyRenderDelegate::ProxyRenderDelegate(const MObject& obj)
: MHWRender::MPxSubSceneOverride(obj)
{
    MDagPath proxyDagPath;
    MDagPath::getAPathTo(obj, proxyDagPath);

    const MFnDependencyNode fnDepNode(obj);
    _proxyShapeData.reset(new ProxyShapeData(static_cast<MayaUsdProxyShapeBase*>(fnDepNode.userNode()), proxyDagPath));
}

//! \brief  Destructor
ProxyRenderDelegate::~ProxyRenderDelegate() {
    _ClearRenderDelegate();

#if !defined(WANT_UFE_BUILD)
    if (_mayaSelectionCallbackId != 0) {
        MMessage::removeCallback(_mayaSelectionCallbackId);
    }
#endif
}

//! \brief  This drawing routine supports all devices (DirectX and OpenGL)
MHWRender::DrawAPI ProxyRenderDelegate::supportedDrawAPIs() const {
    return MHWRender::kAllDevices;
}

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
//! \brief  Enable subscene update in selection passes for deferred update of selection render items.
bool ProxyRenderDelegate::enableUpdateForSelection() const {
    return true;
}
#endif

//! \brief  Always requires update since changes are tracked by Hydraw change tracker and it will guarantee minimal update
bool ProxyRenderDelegate::requiresUpdate(const MSubSceneContainer& container, const MFrameContext& frameContext) const {
    return true;
}

void ProxyRenderDelegate::_ClearRenderDelegate()
{
    // The order of deletion matters. Some orders cause crashes.

    _sceneDelegate.reset();
    _taskController.reset();
    _renderIndex.reset();
    _renderDelegate.reset();
}

//! \brief  One time initialization of this drawing routine
void ProxyRenderDelegate::_InitRenderDelegate(MSubSceneContainer& container) {

    if (_proxyShapeData->ProxyShape() == nullptr)
        return;

    if (!_proxyShapeData->IsUsdStageUpToDate())
    {
        // delete everything so we stop drawing the old stage and draw the new one
        _ClearRenderDelegate();
        _dummyTasks.clear();
        container.clear();

        _proxyShapeData->UpdateUsdStage();
    }
    
    // No need to run all the checks if we got till the end
    if (_isInitialized())
        return;

    if (!_renderDelegate) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate VP2RenderDelegate");
        _renderDelegate.reset(new HdVP2RenderDelegate(*this));
    }

    if (!_renderIndex) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate RenderIndex");
#if USD_VERSION_NUM > 2002
        _renderIndex.reset(HdRenderIndex::New(_renderDelegate.get(), HdDriverVector()));
#else
        _renderIndex.reset(HdRenderIndex::New(_renderDelegate.get()));
#endif

        // Add additional configurations after render index creation.
        static std::once_flag reprsOnce;
        std::call_once(reprsOnce, _ConfigureReprs);
    }

    if (!_sceneDelegate) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate SceneDelegate");

        // Make sure the delegate name is a valid identifier, since it may
        // include colons if the proxy node is in a Maya namespace.
        const std::string delegateName =
            TfMakeValidIdentifier(
                TfStringPrintf(
                    "Proxy_%s_%p",
                    _proxyShapeData->ProxyShape()->name().asChar(),
                    _proxyShapeData->ProxyShape()));
        const SdfPath delegateID =
            SdfPath::AbsoluteRootPath().AppendChild(TfToken(delegateName));

        _sceneDelegate.reset(new UsdImagingDelegate(_renderIndex.get(), delegateID));

        _taskController.reset(new HdxTaskController(_renderIndex.get(),
            delegateID.AppendChild(TfToken(TfStringPrintf("_UsdImaging_VP2_%p", this))) ));

        _defaultCollection.reset(new HdRprimCollection());
        _defaultCollection->SetName(HdTokens->geometry);

        _selection.reset(new HdSelection);

#if defined(WANT_UFE_BUILD)
        if (!_ufeSelectionObserver) {
            auto globalSelection = Ufe::GlobalSelection::get();
            if (globalSelection) {
                _ufeSelectionObserver = std::make_shared<UfeSelectionObserver>(*this);
                globalSelection->addObserver(_ufeSelectionObserver);
            }
        }
#else
        // Without UFE, support basic selection highlight at proxy shape level.
        if (!_mayaSelectionCallbackId)
        {
            _mayaSelectionCallbackId = MEventMessage::addEventCallback(
                "SelectionChanged", SelectionChangedCB, this);
        }
#endif

        // We don't really need any HdTask because VP2RenderDelegate uses Hydra
        // engine for data preparation only, but we have to add a dummy render
        // task to bootstrap data preparation.
        const HdTaskSharedPtrVector tasks = _taskController->GetRenderingTasks();
        for (const HdTaskSharedPtr& task : tasks) {
            if (dynamic_cast<const HdxRenderTask*>(task.get())) {
                _dummyTasks.push_back(task);
                break;
            }
        }
    }
}

//! \brief  Populate render index with prims coming from scene delegate.
//! \return True when delegate is ready to draw
bool ProxyRenderDelegate::_Populate() {
    if (!_isInitialized())
        return false;

    if (_proxyShapeData->UsdStage() && (!_isPopulated || !_proxyShapeData->IsUsdStageUpToDate() || !_proxyShapeData->IsExcludePrimsUpToDate()) ) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Populate");

        // It might have been already populated, clear it if so.
        SdfPathVector excludePrimPaths = _proxyShapeData->ProxyShape()->getExcludePrimPaths();
        for (auto& excludePrim : excludePrimPaths) {
            SdfPath indexPath = _sceneDelegate->ConvertCachePathToIndexPath(excludePrim);
            if (_renderIndex->HasRprim(indexPath)) {
                _renderIndex->RemoveRprim(indexPath);
            }
        }
        
        _sceneDelegate->Populate(_proxyShapeData->UsdStage()->GetPseudoRoot(),excludePrimPaths);
        
        _isPopulated = true;
        _proxyShapeData->UsdStageUpdated();
        _proxyShapeData->ExcludePrimsUpdated();
    }

    return _isPopulated;
}

//! \brief  Synchronize USD scene delegate with Maya's proxy shape.
void ProxyRenderDelegate::_UpdateSceneDelegate()
{
    if (!_proxyShapeData->ProxyShape() || !_sceneDelegate) {
        return;
    }

    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L1, "UpdateSceneDelegate");

    {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetTime");

        const UsdTimeCode timeCode = _proxyShapeData->ProxyShape()->getTime();
        _sceneDelegate->SetTime(timeCode);
    }

    const MMatrix inclusiveMatrix = _proxyShapeData->ProxyDagPath().inclusiveMatrix();
    const GfMatrix4d transform(inclusiveMatrix.matrix);
    constexpr double tolerance = 1e-9;
    if (!GfIsClose(transform, _sceneDelegate->GetRootTransform(), tolerance)) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetRootTransform");
        _sceneDelegate->SetRootTransform(transform);
    }

    const bool isVisible = _proxyShapeData->ProxyDagPath().isVisible();
    if (isVisible != _sceneDelegate->GetRootVisibility()) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetRootVisibility");
        _sceneDelegate->SetRootVisibility(isVisible);

        // Trigger selection update when a hidden proxy shape gets shown.
        if (isVisible) {
            SelectionChanged();
        }
    }

    const int refineLevel = _proxyShapeData->ProxyShape()->getComplexity();
    if (refineLevel != _sceneDelegate->GetRefineLevelFallback())
    {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetRefineLevelFallback");

        _sceneDelegate->SetRefineLevelFallback(refineLevel);
    }
}

//! \brief  Execute Hydra engine to perform minimal VP2 draw data update based on change tracker.
void ProxyRenderDelegate::_Execute(const MHWRender::MFrameContext& frameContext)
{
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L1, "Execute");

    _UpdateRenderTags();

    // If update for selection is enabled, the draw data for the "points" repr
    // won't be prepared until point snapping is activated; otherwise the draw
    // data have to be prepared early for possible activation of point snapping.
#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    HdReprSelector reprSelector;

    const bool inSelectionPass = (frameContext.getSelectionInfo() != nullptr);
    const bool inPointSnapping = pointSnappingActive();

#if defined(WANT_UFE_BUILD)
    // Query selection adjustment mode only if the update is triggered in a selection pass.
    _globalListAdjustment = (inSelectionPass && !inPointSnapping) ? GetListAdjustment() : MGlobal::kReplaceList;
#endif // defined(WANT_UFE_BUILD)

#else // !defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    HdReprSelector reprSelector = kPointsReprSelector;

    constexpr bool inSelectionPass = false;
    constexpr bool inPointSnapping = false;
#endif // defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)

    if (inSelectionPass) {
        if (inPointSnapping && !reprSelector.Contains(HdReprTokens->points)) {
            reprSelector = reprSelector.CompositeOver(kPointsReprSelector);
        }
    }
    else {
        if (_selectionChanged) {
            _UpdateSelectionStates();
            _selectionChanged = false;
        }

        const unsigned int displayStyle = frameContext.getDisplayStyle();

        // Query the wireframe color assigned to proxy shape.
        if (displayStyle & (
            MHWRender::MFrameContext::kBoundingBox |
            MHWRender::MFrameContext::kWireFrame))
        {
            _wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(_proxyShapeData->ProxyDagPath());
        }

        // Update repr selector based on display style of the current viewport
        if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
            if (!reprSelector.Contains(HdVP2ReprTokens->bbox)) {
                reprSelector = reprSelector.CompositeOver(kBBoxReprSelector);
            }
        }
        else {
            // To support Wireframe on Shaded mode, the two displayStyle checks
            // should not be mutually excluded.
            if (displayStyle & MHWRender::MFrameContext::kGouraudShaded) {
                if (!reprSelector.Contains(HdReprTokens->smoothHull)) {
                    reprSelector = reprSelector.CompositeOver(kSmoothHullReprSelector);
                }
            }

            if (displayStyle & MHWRender::MFrameContext::kWireFrame) {
                if (!reprSelector.Contains(HdReprTokens->wire)) {
                    reprSelector = reprSelector.CompositeOver(kWireReprSelector);
                }
            }
        }
    }

    if (_defaultCollection->GetReprSelector() != reprSelector) {
        _defaultCollection->SetReprSelector(reprSelector);
        _taskController->SetCollection(*_defaultCollection);
    }

    _engine.Execute(_renderIndex.get(), &_dummyTasks);
}

//! \brief  Main update entry from subscene override.
void ProxyRenderDelegate::update(MSubSceneContainer& container, const MFrameContext& frameContext) {
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L1, "ProxyRenderDelegate::update");

    _InitRenderDelegate(container);

    // Give access to current time and subscene container to the rest of render delegate world via render param's.
    auto* param = reinterpret_cast<HdVP2RenderParam*>(_renderDelegate->GetRenderParam());
    param->BeginUpdate(container, _sceneDelegate->GetTime());

    if (_Populate()) {
        _UpdateSceneDelegate();
        _Execute(frameContext);
    }
    param->EndUpdate();
}

//! \brief  Switch to component-level selection for point snapping.
void ProxyRenderDelegate::updateSelectionGranularity(
    const MDagPath& path,
    MHWRender::MSelectionContext& selectionContext)
{
    if (pointSnappingActive())
    {
        selectionContext.setSelectionLevel(MHWRender::MSelectionContext::kComponent);
    }
}

//! \brief  UFE-based selection for both instanced and non-instanced cases.
bool ProxyRenderDelegate::getInstancedSelectionPath(
    const MHWRender::MRenderItem& renderItem,
    const MHWRender::MIntersection& intersection,
    MDagPath& dagPath) const
{
#if defined(WANT_UFE_BUILD)
    if (_proxyShapeData->ProxyShape() == nullptr) {
        return false;
    }

    if (!_proxyShapeData->ProxyShape()->isUfeSelectionEnabled()) {
        return false;
    }

    // When point snapping, only the point position matters, so return false
    // to use the DAG path from the default implementation and avoid the UFE
    // global selection list to be updated.
    if (pointSnappingActive())
        return false;

    auto handler = Ufe::RunTimeMgr::instance().hierarchyHandler(USD_UFE_RUNTIME_ID);
    if (handler == nullptr)
        return false;

    // Extract id of the owner Rprim. A SdfPath directly created from the render
    // item name could be ill-formed if the render item represents instancing:
    // "/TreePatch/Tree_1.proto_leaves_id0/DrawItem_xxxxxxxx". Thus std::string
    // is used instead to extract Rprim id.
    const std::string renderItemName = renderItem.name().asChar();
    const auto pos = renderItemName.find_last_of(USD_UFE_SEPARATOR);
    SdfPath rprimId(renderItemName.substr(0, pos));

    // If the selection hit comes from an instanced render item, its instance
    // transform matrices should have been sorted according to USD instance ID,
    // therefore drawInstID is usdInstID plus 1 considering VP2 defines the
    // instance ID of the first instance as 1.
    const int drawInstID = intersection.instanceID();
    const int usdInstID = drawInstID - 1;
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 13
    rprimId = _sceneDelegate->GetScenePrimPath(rprimId, usdInstID);
#else
    if (drawInstID > 0) {
        rprimId = _sceneDelegate->GetPathForInstanceIndex(rprimId, usdInstID, nullptr);
    }
#endif

    const SdfPath usdPath(_sceneDelegate->ConvertIndexPathToCachePath(rprimId));

    const Ufe::PathSegment pathSegment(usdPath.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);
    const Ufe::SceneItem::Ptr& si = handler->createItem(_proxyShapeData->ProxyShape()->ufePath() + pathSegment);
    if (!si) {
        TF_WARN("UFE runtime is not updated for the USD stage. Please save scene and reopen.");
        return false;
    }

    auto globalSelection = Ufe::GlobalSelection::get();

    // If update for selection is enabled, we can query selection list adjustment
    // mode once per selection update to avoid any potential performance hit due
    // to MEL command execution.
#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    switch (_globalListAdjustment)
#else
    switch (GetListAdjustment())
#endif
    {
    case MGlobal::kReplaceList:
        // The list has been cleared before viewport selection runs, so we
        // can add the new hits directly. UFE selection list is a superset
        // of Maya selection list, calling clear()/replaceWith() on UFE
        // selection list would clear Maya selection list.
        globalSelection->append(si);
        break;
    case MGlobal::kAddToList:
        globalSelection->append(si);
        break;
    case MGlobal::kRemoveFromList:
        globalSelection->remove(si);
        break;
    case MGlobal::kXORWithList:
        if (!globalSelection->remove(si))
        {
            globalSelection->append(si);
        }
        break;
    default:
        TF_WARN("Unexpected MGlobal::ListAdjustment enum for selection.");
        break;
    }

    return true;
#else
    return false;
#endif
}

//! \brief  Notify of selection change.
void ProxyRenderDelegate::SelectionChanged()
{
    _selectionChanged = true;
}

//! \brief  Filter selection for Rprims under the proxy shape.
void ProxyRenderDelegate::_FilterSelection()
{
#if defined(WANT_UFE_BUILD)
    if (_proxyShapeData->ProxyShape() == nullptr) {
        return;
    }

    _selection.reset(new HdSelection);

    const auto proxyPath = _proxyShapeData->ProxyShape()->ufePath();
    const auto globalSelection = Ufe::GlobalSelection::get();

    for (const Ufe::SceneItem::Ptr& item : *globalSelection) {
        if (item->runTimeId() != USD_UFE_RUNTIME_ID) {
            continue;
        }

        const Ufe::Path::Segments& segments = item->path().getSegments();
        if ((segments.size() != 2) || (proxyPath != segments[0])) {
            continue;
        }

        SdfPath usdPath(segments[1].string());
#if !defined(USD_IMAGING_API_VERSION) || USD_IMAGING_API_VERSION < 11
        usdPath = _sceneDelegate->ConvertCachePathToIndexPath(usdPath);
#endif

        _sceneDelegate->PopulateSelection(HdSelection::HighlightModeSelect,
            usdPath, UsdImagingDelegate::ALL_INSTANCES, _selection);
    }
#endif
}

/*! \brief  Notify selection change to rprims.
*/
void ProxyRenderDelegate::_UpdateSelectionStates()
{
    auto status = MHWRender::MGeometryUtilities::displayStatus(_proxyShapeData->ProxyDagPath());

    const bool wasProxySelected = _isProxySelected;
    _isProxySelected =
        (status == MHWRender::kHilite) ||
        (status == MHWRender::kLead) ||
        (status == MHWRender::kActive);

    SdfPathVector rootPaths;

    if (_isProxySelected) {
        rootPaths.push_back(SdfPath::AbsoluteRootPath());
    }
    else if (wasProxySelected) {
        rootPaths.push_back(SdfPath::AbsoluteRootPath());
        _FilterSelection();
    }
    else {
        constexpr HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;

        SdfPathVector oldPaths = _selection->GetSelectedPrimPaths(mode);
        _FilterSelection();
        SdfPathVector newPaths = _selection->GetSelectedPrimPaths(mode);

        if (!oldPaths.empty() || !newPaths.empty()) {
            rootPaths = std::move(oldPaths);
            rootPaths.reserve(rootPaths.size() + newPaths.size());
            rootPaths.insert(rootPaths.end(), newPaths.begin(), newPaths.end());
        }
    }

    if (!rootPaths.empty()) {
        HdRprimCollection collection(HdTokens->geometry, kSelectionReprSelector);
        collection.SetRootPaths(rootPaths);
        _taskController->SetCollection(collection);
        _engine.Execute(_renderIndex.get(), &_dummyTasks);
        _taskController->SetCollection(*_defaultCollection);
    }
}

void ProxyRenderDelegate::_UpdateRenderTags()
{
    // USD pulls the required render tags from the task list passed into execute.
    // Only rprims which are dirty & which match the current set of render tags
    // will get a Sync call.
    // Render tags are harder for us to handle than HdSt because we have our own
    // cached version of the scene in MPxSubSceneOverride. HdSt draws using
    // HdRenderIndex::GetDrawItems(), and that returns only items that pass the
    // render tag filter. There is no need for HdSt to do any update on items that
    // are being hidden, because the render pass level filtering will prevent
    // them from drawing.
    // The Vp2RenderDelegate implements render tags using MRenderItem::Enable(),
    // which means we do need to update individual MRenderItems when the render
    // tags change.
    // When we change the desired render tags on the proxyShape we'll be adding
    // and/or removing some tags, so we can have existing MRenderItems that need
    // to be hidden, or hidden items that need to be shown.
    // This function needs to do three things:
    //  1: Make sure every rprim with a render tag whose visibility changed gets
    //     marked dirty. This will ensure the upcoming execute call will update
    //     the visibility of the MRenderItems in MPxSubSceneOverride.
    //  2: Make a special call to Execute() with only the render tags which have
    //     been removed. We need this extra call to hide the previously shown
    //     items.
    //  3: Update the render tags with the new final render tags so that the next
    //     real call to execute will update all the now visible items.
    bool renderPurposeChanged, proxyPurposeChanged, guidePurposeChanged;
    _proxyShapeData->UpdatePurpose(
        &renderPurposeChanged, &proxyPurposeChanged, &guidePurposeChanged);
    if (renderPurposeChanged || proxyPurposeChanged || guidePurposeChanged)
    {
        // Build the list of render tags which were added or removed (changed)
        // and the list of render tags which were removed.
        TfTokenVector changedRenderTags;
        TfTokenVector removedRenderTags;
        if (renderPurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->render);
            if (!_proxyShapeData->DrawRenderPurpose())
                removedRenderTags.push_back(HdRenderTagTokens->render);
        }
        if (proxyPurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->proxy);
            if (!_proxyShapeData->DrawProxyPurpose())
                removedRenderTags.push_back(HdRenderTagTokens->proxy);
        }
        if (guidePurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->guide);
            if (!_proxyShapeData->DrawGuidePurpose())
                removedRenderTags.push_back(HdRenderTagTokens->guide);
        }

        // Mark all the rprims which have a render tag which changed dirty
        SdfPathVector    rprimsToDirty = _GetFilteredRprims(*(_defaultCollection.get()), changedRenderTags);
        HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();
        for (auto id : rprimsToDirty) {
            changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyRenderTag);
        }

        // Make a special pass over the removed render tags so that the objects
        // can hide themselves.
        _taskController->SetRenderTags(removedRenderTags);
        _engine.Execute(_renderIndex.get(), &_dummyTasks);

        // Set the new render tags correctly.The regular execute will cause
        // any newly visible rprims will update and mark themselves visible.
        TfTokenVector renderTags
            = { HdRenderTagTokens->geometry }; // always draw geometry render tag purpose.
        if (_proxyShapeData->DrawRenderPurpose()) {
            renderTags.push_back(HdRenderTagTokens->render);
        }
        if (_proxyShapeData->DrawProxyPurpose()) {
            renderTags.push_back(HdRenderTagTokens->proxy);
        }
        if (_proxyShapeData->DrawGuidePurpose()) {
            renderTags.push_back(HdRenderTagTokens->guide);
        }
        _taskController->SetRenderTags(renderTags);
    }
}


// Copied from renderIndex.cpp, the code that does HdRenderIndex::GetDrawItems. But I just want the rprimIds, I don't want to go all the way to draw items.
struct _FilterParam {
    const HdRprimCollection& collection;
    const TfTokenVector&     renderTags;
    const HdRenderIndex*     renderIndex;
};

static bool _DrawItemFilterPredicate(const SdfPath& rprimID, const void* predicateParam)
{
    const _FilterParam* filterParam = static_cast<const _FilterParam*>(predicateParam);

    const HdRprimCollection& collection = filterParam->collection;
    const TfTokenVector&     renderTags = filterParam->renderTags;
    const HdRenderIndex*     renderIndex = filterParam->renderIndex;

    //
    // Render Tag Filter
    //
    bool passedRenderTagFilter = false;
    if (renderTags.empty()) {
        // An empty render tag set means everything passes the filter
        // Primary user is tests, but some single task render delegates
        // that don't support render tags yet also use it.
        passedRenderTagFilter = true;
    } else {
        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        TfToken primRenderTag = renderIndex->GetRenderTag(rprimID);
        size_t  numRenderTags = renderTags.size();
        size_t  tagNum = 0;
        while (!passedRenderTagFilter && tagNum < numRenderTags) {
            if (renderTags[tagNum] == primRenderTag) {
                passedRenderTagFilter = true;
            }
            ++tagNum;
        }
    }

    //
    // Material Tag Filter
    //
    bool passedMaterialTagFilter = false;

    // Filter out rprims that do not match the collection's materialTag.
    // E.g. We may want to gather only opaque or translucent prims.
    // An empty materialTag on collection means: ignore material-tags.
    // This is important for tasks such as the selection-task which wants
    // to ignore materialTags and receive all prims in its collection.
    TfToken const& collectionMatTag = collection.GetMaterialTag();
    if (collectionMatTag.IsEmpty() || renderIndex->GetMaterialTag(rprimID) == collectionMatTag) {
        passedMaterialTagFilter = true;
    }

    return (passedRenderTagFilter && passedMaterialTagFilter);
}

SdfPathVector ProxyRenderDelegate::_GetFilteredRprims(
    HdRprimCollection const& collection,
    TfTokenVector const&     renderTags)
{
    SdfPathVector rprimIds;
    const SdfPathVector& paths = _renderIndex->GetRprimIds();
    const SdfPathVector& includePaths = collection.GetRootPaths();
    const SdfPathVector& excludePaths = collection.GetExcludePaths();
    _FilterParam         filterParam = { collection, renderTags, _renderIndex.get() };
    HdPrimGather         gather;
    gather.PredicatedFilter(
        paths, includePaths, excludePaths, _DrawItemFilterPredicate, &filterParam, &rprimIds);

    return rprimIds;
}

//! \brief  Query the selection state of a given prim.
const HdSelection::PrimSelectionState*
ProxyRenderDelegate::GetPrimSelectionState(const SdfPath& path) const
{
    return (_selection == nullptr) ? nullptr :
        _selection->GetPrimSelectionState(HdSelection::HighlightModeSelect, path);
}

//! \brief  Query the selection status of a given prim.
HdVP2SelectionStatus
ProxyRenderDelegate::GetPrimSelectionStatus(const SdfPath& path) const
{
    if (_isProxySelected) {
        return kFullySelected;
    }

    const HdSelection::PrimSelectionState* state = GetPrimSelectionState(path);
    if (state && state->fullySelected) {
        return kFullySelected;
    }

    return state ? kPartiallySelected : kUnselected;
}

//! \brief  Query the wireframe color assigned to the proxy shape.
const MColor& ProxyRenderDelegate::GetWireframeColor() const
{
    return _wireframeColor;
}

bool ProxyRenderDelegate::DrawRenderTag(const TfToken& renderTag) const
{
    if (renderTag == HdRenderTagTokens->geometry) {
        return true;
    } else if (renderTag == HdRenderTagTokens->render) {
        return _proxyShapeData->DrawRenderPurpose();
    } else if (renderTag == HdRenderTagTokens->guide) {
        return _proxyShapeData->DrawGuidePurpose();
    } else if (renderTag == HdRenderTagTokens->proxy) {
        return _proxyShapeData->DrawProxyPurpose();
    } else if (renderTag == HdRenderTagTokens->hidden) {
        return false;
    } else {
        TF_WARN("Unknown render tag");
        return true;
    }
}

// ProxyShapeData
ProxyRenderDelegate::ProxyShapeData::ProxyShapeData(const MayaUsdProxyShapeBase* proxyShape, const MDagPath& proxyDagPath)
    : _proxyShape(proxyShape)
    , _proxyDagPath(proxyDagPath)
{
    assert(_proxyShape);
}
inline const MayaUsdProxyShapeBase* ProxyRenderDelegate::ProxyShapeData::ProxyShape() const { return _proxyShape; }
inline const MDagPath& ProxyRenderDelegate::ProxyShapeData::ProxyDagPath() const { return _proxyDagPath; }
inline UsdStageRefPtr ProxyRenderDelegate::ProxyShapeData::UsdStage() const { return _usdStage; }
inline void ProxyRenderDelegate::ProxyShapeData::UpdateUsdStage() { _usdStage = _proxyShape->getUsdStage(); }
inline bool ProxyRenderDelegate::ProxyShapeData::IsUsdStageUpToDate() const {
    return _proxyShape->getUsdStageVersion() == _usdStageVersion;
}
inline void ProxyRenderDelegate::ProxyShapeData::UsdStageUpdated() {
    _usdStageVersion = _proxyShape->getUsdStageVersion();
}
inline bool ProxyRenderDelegate::ProxyShapeData::IsExcludePrimsUpToDate() const {
    return _proxyShape->getExcludePrimPathsVersion() == _excludePrimsVersion;
}
inline void ProxyRenderDelegate::ProxyShapeData::ExcludePrimsUpdated() {
    _excludePrimsVersion = _proxyShape->getExcludePrimPathsVersion();
}
inline void ProxyRenderDelegate::ProxyShapeData::UpdatePurpose(
    bool* drawRenderPurposeChanged,
    bool* drawProxyPurposeChanged,
    bool* drawGuidePurposeChanged)
{
    bool drawRenderPurpose, drawProxyPurpose, drawGuidePurpose;

    ProxyShape()->getDrawPurposeToggles(&drawRenderPurpose, &drawProxyPurpose, &drawGuidePurpose);
    if (drawRenderPurposeChanged)
        *drawRenderPurposeChanged = (drawRenderPurpose != _drawRenderPurpose);
    if (drawProxyPurposeChanged)
        *drawProxyPurposeChanged = (drawProxyPurpose != _drawProxyPurpose);
    if (drawGuidePurposeChanged)
        *drawGuidePurposeChanged = (drawGuidePurpose != _drawGuidePurpose);

    _drawRenderPurpose = drawRenderPurpose;
    _drawProxyPurpose = drawProxyPurpose;
    _drawGuidePurpose = drawGuidePurpose;
}

inline bool ProxyRenderDelegate::ProxyShapeData::DrawRenderPurpose() const
{
    return _drawRenderPurpose;
}
inline bool ProxyRenderDelegate::ProxyShapeData::DrawProxyPurpose() const
{
    return _drawProxyPurpose;
}
inline bool ProxyRenderDelegate::ProxyShapeData::DrawGuidePurpose() const
{
    return _drawGuidePurpose;
}

    PXR_NAMESPACE_CLOSE_SCOPE
