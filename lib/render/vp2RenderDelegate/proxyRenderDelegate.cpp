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
#include "render_delegate.h"
#include "tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "../../nodes/proxyShapeBase.h"
#include "../../nodes/stageData.h"
#include "../../utils/util.h"

#include <maya/MFileIO.h>
#include <maya/MFnPluginData.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MEventMessage.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionContext.h>

#if defined(WANT_UFE_BUILD)
#include <ufe/sceneItem.h>
#include <ufe/runTimeMgr.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
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
    MDagPath::getAPathTo(obj, _proxyDagPath);

    const MFnDependencyNode fnDepNode(obj);
    _proxyShape = static_cast<MayaUsdProxyShapeBase*>(fnDepNode.userNode());
}

//! \brief  Destructor
ProxyRenderDelegate::~ProxyRenderDelegate() {
    delete _sceneDelegate;
    delete _taskController;
    delete _renderIndex;
    delete _renderDelegate;

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

//! \brief  One time initialization of this drawing routine
void ProxyRenderDelegate::_InitRenderDelegate() {
    // No need to run all the checks if we got till the end
    if (_isInitialized())
        return;

    if (_proxyShape == nullptr)
        return;

    if (!_usdStage) {
        _usdStage = _proxyShape->getUsdStage();
    }

    if (!_renderDelegate) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate VP2RenderDelegate");
        _renderDelegate = new HdVP2RenderDelegate(*this);
    }

    if (!_renderIndex) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate RenderIndex");
#if USD_VERSION_NUM > 2002
        _renderIndex = HdRenderIndex::New(_renderDelegate, HdDriverVector());
#else
        _renderIndex = HdRenderIndex::New(_renderDelegate);
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
                    _proxyShape->name().asChar(),
                    _proxyShape));
        const SdfPath delegateID =
            SdfPath::AbsoluteRootPath().AppendChild(TfToken(delegateName));

        _sceneDelegate = new UsdImagingDelegate(_renderIndex, delegateID);

        _taskController = new HdxTaskController(_renderIndex,
            delegateID.AppendChild(TfToken(TfStringPrintf("_UsdImaging_VP2_%p", this))) );

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
        _mayaSelectionCallbackId = MEventMessage::addEventCallback(
            "SelectionChanged", SelectionChangedCB, this);
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

    if (_usdStage && (!_isPopulated || _proxyShape->getExcludePrimPathsVersion() != _excludePrimPathsVersion) ) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Populate");

        // It might have been already populated, clear it if so.
        SdfPathVector excludePrimPaths = _proxyShape->getExcludePrimPaths();
        for (auto& excludePrim : excludePrimPaths) {
            SdfPath indexPath = _sceneDelegate->ConvertCachePathToIndexPath(excludePrim);
            if (_renderIndex->HasRprim(indexPath)) {
                _renderIndex->RemoveRprim(indexPath);
            }
        }
        
        _sceneDelegate->Populate(_usdStage->GetPseudoRoot(),excludePrimPaths);
        
        _isPopulated = true;
        _excludePrimPathsVersion = _proxyShape->getExcludePrimPathsVersion();
    }

    return _isPopulated;
}

//! \brief  Synchronize USD scene delegate with Maya's proxy shape.
void ProxyRenderDelegate::_UpdateSceneDelegate()
{
    if (!_proxyShape || !_sceneDelegate) {
        return;
    }

    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L1, "UpdateSceneDelegate");

    {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetTime");

        const UsdTimeCode timeCode = _proxyShape->getTime();
        _sceneDelegate->SetTime(timeCode);
    }

    const MMatrix inclusiveMatrix = _proxyDagPath.inclusiveMatrix();
    const GfMatrix4d transform(inclusiveMatrix.matrix);
    constexpr double tolerance = 1e-9;
    if (!GfIsClose(transform, _sceneDelegate->GetRootTransform(), tolerance)) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetRootTransform");
        _sceneDelegate->SetRootTransform(transform);
    }

    const bool isVisible = _proxyDagPath.isVisible();
    if (isVisible != _sceneDelegate->GetRootVisibility()) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1, "SetRootVisibility");
        _sceneDelegate->SetRootVisibility(isVisible);

        // Trigger selection update when a hidden proxy shape gets shown.
        if (isVisible) {
            SelectionChanged();
        }
    }

    const int refineLevel = _proxyShape->getComplexity();
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
            _wireframeColor = MHWRender::MGeometryUtilities::wireframeColor(_proxyDagPath);
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

    _engine.Execute(_renderIndex, &_dummyTasks);
}

//! \brief  Main update entry from subscene override.
void ProxyRenderDelegate::update(MSubSceneContainer& container, const MFrameContext& frameContext) {
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L1, "ProxyRenderDelegate::update");

    _InitRenderDelegate();

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
    if (_proxyShape == nullptr) {
        return false;
    }

    if (!_proxyShape->isUfeSelectionEnabled()) {
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
    if (drawInstID > 0) {
        const int usdInstID = drawInstID - 1;
        rprimId = _sceneDelegate->GetPathForInstanceIndex(rprimId, usdInstID, nullptr);
    }

    const SdfPath usdPath(_sceneDelegate->ConvertIndexPathToCachePath(rprimId));

    const Ufe::PathSegment pathSegment(usdPath.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);
    const Ufe::SceneItem::Ptr& si = handler->createItem(_proxyShape->ufePath() + pathSegment);
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
    if (_proxyShape == nullptr) {
        return;
    }

    _selection.reset(new HdSelection);

    const auto proxyPath = _proxyShape->ufePath();
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
    auto status = MHWRender::MGeometryUtilities::displayStatus(_proxyDagPath);

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
        _engine.Execute(_renderIndex, &_dummyTasks);
        _taskController->SetCollection(*_defaultCollection);
    }
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

PXR_NAMESPACE_CLOSE_SCOPE
