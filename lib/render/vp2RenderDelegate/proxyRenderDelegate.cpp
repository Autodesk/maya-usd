// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "proxyRenderDelegate.h"
#include "render_delegate.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/taskController.h"
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
#include <maya/MProfiler.h>
#include <maya/MSelectionContext.h>

#if defined(WANT_UFE_BUILD)
#include "ufe/sceneItem.h"
#include "ufe/runTimeMgr.h"
#include "ufe/globalSelection.h"
#include "ufe/observableSelection.h"
#include "ufe/selectionNotification.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
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
 , _mObject(obj) {
}

//! \brief  Destructor
ProxyRenderDelegate::~ProxyRenderDelegate() {
    delete _sceneDelegate;
    delete _taskController;
    delete _renderIndex;
    delete _renderDelegate;
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

//! \brief  Return pointer to DG proxy shape node
MayaUsdProxyShapeBase* ProxyRenderDelegate::getProxyShape() const {
    const MFnDependencyNode depNodeFn(_mObject);
    MayaUsdProxyShapeBase* usdSubSceneShape = static_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    return usdSubSceneShape;
}

//! \brief  One time initialization of this drawing routine
void ProxyRenderDelegate::_InitRenderDelegate() {
    // No need to run all the checks if we got till the end
    if (_isInitialized())
        return;

    MStatus status;
    MayaUsdProxyShapeBase* usdSubSceneShape = getProxyShape();
    if (!usdSubSceneShape)
        return;

    if (!_usdStage) {
        _usdStage = usdSubSceneShape->getUsdStage();
    }

    if (!_renderDelegate) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate VP2RenderDelegate");
        _renderDelegate = new HdVP2RenderDelegate(*this);
    }

    if (!_renderIndex) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate RenderIndex");
        _renderIndex = HdRenderIndex::New(_renderDelegate);
    }

    if (!_sceneDelegate) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Allocate SceneDelegate");

        SdfPath delegateID = SdfPath::AbsoluteRootPath().AppendChild(TfToken(TfStringPrintf(
            "Proxy_%s_%p", usdSubSceneShape->name().asChar(), usdSubSceneShape)));

        _sceneDelegate = new UsdImagingDelegate(_renderIndex, delegateID);

        _taskController = new HdxTaskController(_renderIndex,
            delegateID.AppendChild(TfToken(TfStringPrintf("_UsdImaging_VP2_%p", this))) );

        // Assign a collection of active representations, then each Rprim will
        // initialize and update draw data for these active repr(s). If update
        // for selection is enabled, the draw data for the "points" repr won't
        // be prepared until point snapping is activated; otherwise they have
        // to be early prepared for potential activation of point snapping.
        _defaultCollection.reset(new HdRprimCollection(HdTokens->geometry,
#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
            HdReprSelector(HdReprTokens->smoothHull)
#else
            HdReprSelector(HdReprTokens->smoothHull, TfToken(), HdReprTokens->points)
#endif
        ));

        _taskController->SetCollection(*_defaultCollection);

        _selectionHighlightCollection.reset(new HdRprimCollection(HdTokens->geometry,
            HdReprSelector(HdReprTokens->wire)
        ));

        _selection.reset(new HdSelection);

#if defined(WANT_UFE_BUILD)
        if (!_ufeSelectionObserver) {
            auto globalSelection = Ufe::GlobalSelection::get();
            if (globalSelection) {
                _ufeSelectionObserver = std::make_shared<UfeSelectionObserver>(*this);
                globalSelection->addObserver(_ufeSelectionObserver);
            }
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

    if (!_isPopulated && _usdStage) {
        MProfilingScope subProfilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1, "Populate");
        _sceneDelegate->Populate(_usdStage->GetPseudoRoot());

        _isPopulated = true;
    }

    return _isPopulated;
}

//! \brief  Synchronize USD scene delegate time with Maya's scene time.
void ProxyRenderDelegate::_UpdateTime() {
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L1, "Update Time");

    MayaUsdProxyShapeBase* usdSubSceneShape = getProxyShape();
    if(usdSubSceneShape && _sceneDelegate) {
        UsdTimeCode timeCode = usdSubSceneShape->getTime();
        _sceneDelegate->SetTime(timeCode);
    }
}

//! \brief  Execute Hydra engine which will performe minimal update VP2 state update based on change tracker.
void ProxyRenderDelegate::_Execute(const MHWRender::MFrameContext& frameContext) {
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L1, "Execute");

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    // Since Maya 2020, subscene update can be invoked in a selection pass.
    const bool inSelectionPass = (frameContext.getSelectionInfo() != nullptr);
    const bool inPointSnapping = pointSnappingActive();

    // Query selection adjustment mode only if the update is triggered in a selection pass.
    _globalListAdjustment = (inSelectionPass && !inPointSnapping) ? GetListAdjustment() : MGlobal::kReplaceList;
#else
    // Before Maya 2020, subscene update would never be invoked in a selection pass.
    constexpr bool inSelectionPass = false;
    constexpr bool inPointSnapping = false;
#endif

    // HdC_TODO: we need to figure out how to use Hydra repr more effectively
    // when more reprs are coming.
    if (inSelectionPass) {
        if (inPointSnapping) {
            // Update display reprs at the first frame where point snapping is activated.
            if (!_defaultCollection->GetReprSelector().Contains(HdReprTokens->points)) {
                const HdReprSelector under(TfToken(), TfToken(), HdReprTokens->points);
                const HdReprSelector reprSelector =
                    _defaultCollection->GetReprSelector().CompositeOver(under);
                _defaultCollection->SetReprSelector(reprSelector);
                _taskController->SetCollection(*_defaultCollection);
            }
        }
    }
    else if (_selectionChanged) {
        _selectionChanged = false;

        if (_UpdateSelectionHighlight()) {
            // Update display reprs at the first frame where selection highlight is activated.
            if (!_defaultCollection->GetReprSelector().Contains(HdReprTokens->wire)) {
                const HdReprSelector under(TfToken(), HdReprTokens->wire, TfToken());
                const HdReprSelector reprSelector =
                    _defaultCollection->GetReprSelector().CompositeOver(under);
                _defaultCollection->SetReprSelector(reprSelector);
                _taskController->SetCollection(*_defaultCollection);
            }
        }
    }

    _engine.Execute(_renderIndex, &_dummyTasks);
}

//! \brief  Main update entry from subscene override.
void ProxyRenderDelegate::update(MSubSceneContainer& container, const MFrameContext& frameContext) {
    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L1, "ProxyRenderDelegate::update");

    _InitRenderDelegate();
    if (_Populate()) {
        // Give access to current time and subscene container to the rest of render delegate world via render param's.
        auto* param = reinterpret_cast<HdVP2RenderParam*>(_renderDelegate->GetRenderParam());
        param->BeginUpdate(container, _sceneDelegate->GetTime());

        _UpdateTime();
        _Execute(frameContext);

        param->EndUpdate();
    }
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
    // When point snapping, only the point position matters, so return false
    // to use the DAG path from the default implementation and avoid the UFE
    // global selection list to be updated.
    if (pointSnappingActive())
        return false;

    auto handler = Ufe::RunTimeMgr::instance().hierarchyHandler(USD_UFE_RUNTIME_ID);
    if (handler == nullptr)
        return false;

    MayaUsdProxyShapeBase* proxyShape = getProxyShape();
    if (proxyShape == nullptr)
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
    const Ufe::SceneItem::Ptr& si = handler->createItem(proxyShape->ufePath() + pathSegment);
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
    MayaUsdProxyShapeBase* proxyShape = getProxyShape();
    if (proxyShape == nullptr) {
        return;
    }

    _selection.reset(new HdSelection);

    const auto proxyPath = proxyShape->ufePath();
    const auto globalSelection = Ufe::GlobalSelection::get();

    for (const Ufe::SceneItem::Ptr& item : *globalSelection) {
        if (item->runTimeId() != USD_UFE_RUNTIME_ID) {
            continue;
        }

        const Ufe::Path::Segments& segments = item->path().getSegments();
        if ((segments.size() != 2) || (proxyPath != segments[0])) {
            continue;
        }

        const SdfPath usdPath(segments[1].string());
        const SdfPath idxPath(_sceneDelegate->ConvertCachePathToIndexPath(usdPath));

        _sceneDelegate->PopulateSelection(HdSelection::HighlightModeSelect,
            idxPath, UsdImagingDelegate::ALL_INSTANCES, _selection);
    }
#endif
}

/*! \brief  Update for selection highlight

    \return True if selection highlight needs to be shown.
*/
bool ProxyRenderDelegate::_UpdateSelectionHighlight()
{
    bool retVal = false;

    const bool wasProxySelected = _isProxySelected;

    MDagPath proxyDagPath;
    MDagPath::getAPathTo(_mObject, proxyDagPath);
    auto status = MHWRender::MGeometryUtilities::displayStatus(proxyDagPath);
    _isProxySelected = ((status == MHWRender::kHilite) || (status == MHWRender::kLead));

    constexpr HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;

    SdfPathVector rootPaths;

    if (_isProxySelected) {
        rootPaths.push_back(SdfPath::AbsoluteRootPath());
        retVal = true;
    }
    else if (wasProxySelected) {
        rootPaths.push_back(SdfPath::AbsoluteRootPath());
        _FilterSelection();
        retVal = !_selection->GetSelectedPrimPaths(mode).empty();
    }
    else {
        SdfPathVector oldPaths = _selection->GetSelectedPrimPaths(mode);
        _FilterSelection();
        SdfPathVector newPaths = _selection->GetSelectedPrimPaths(mode);

        if (!oldPaths.empty() || !newPaths.empty()) {
            rootPaths = std::move(oldPaths);
            rootPaths.reserve(rootPaths.size() + newPaths.size());
            rootPaths.insert(rootPaths.end(), newPaths.begin(), newPaths.end());
        }

        retVal = !newPaths.empty();
    }

    if (!rootPaths.empty()) {
        _inSelectionHighlightUpdate = true;

        _selectionHighlightCollection->SetRootPaths(rootPaths);
        _taskController->SetCollection(*_selectionHighlightCollection);
        _engine.Execute(_renderIndex, &_dummyTasks);
        _taskController->SetCollection(*_defaultCollection);
        
        _inSelectionHighlightUpdate = false;
    }

    return retVal;
}

//! \brief  Query whether the proxy is selected.
bool ProxyRenderDelegate::IsProxySelected() const
{
    return _isProxySelected;
}

//! \brief  Query the selection state of a given prim.
const HdSelection::PrimSelectionState*
ProxyRenderDelegate::GetPrimSelectionState(const SdfPath& path) const
{
    return (_selection == nullptr) ? nullptr :
        _selection->GetPrimSelectionState(HdSelection::HighlightModeSelect, path);
}

PXR_NAMESPACE_CLOSE_SCOPE
