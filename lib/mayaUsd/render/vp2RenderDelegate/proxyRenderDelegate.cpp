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

#include "draw_item.h"
#include "mayaPrimCommon.h"
#include "render_delegate.h"
#include "tokens.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/utils/selectability.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/primGather.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/selectionTracker.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/pxr.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MColorPickerUtilities.h>
#include <maya/MEventMessage.h>
#include <maya/MFileIO.h>
#include <maya/MFnPluginData.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionContext.h>

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <ufe/globalSelection.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/namedSelection.h>
#endif
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/runTimeMgr.h>
#include <ufe/scene.h>
#include <ufe/sceneItem.h>
#include <ufe/sceneNotification.h>
#include <ufe/selectionNotification.h>
#endif

#if defined(BUILD_HDMAYA)
#include <mayaUsd/render/mayaToHydra/utils.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {
//! Representation selector for shaded and textured viewport mode
const HdReprSelector kSmoothHullReprSelector(HdReprTokens->smoothHull);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
//! Representation selector for default material viewport mode
const HdReprSelector kDefaultMaterialReprSelector(HdVP2ReprTokens->defaultMaterial);
#endif

//! Representation selector for wireframe viewport mode
const HdReprSelector kWireReprSelector(TfToken(), HdReprTokens->wire);

//! Representation selector for bounding box viewport mode
const HdReprSelector kBBoxReprSelector(TfToken(), HdVP2ReprTokens->bbox);

//! Representation selector for point snapping
const HdReprSelector kPointsReprSelector(TfToken(), TfToken(), HdReprTokens->points);

#if defined(WANT_UFE_BUILD)
//! \brief  Query the global selection list adjustment.
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

    if (shiftHeld && ctrlHeld) {
        listAdjustment = MGlobal::kAddToList;
    } else if (ctrlHeld) {
        listAdjustment = MGlobal::kRemoveFromList;
    } else if (shiftHeld) {
        listAdjustment = MGlobal::kXORWithList;
    }

    return listAdjustment;
}

//! \brief  Query the Kind to be selected from viewport.
//! \return A Kind token (https://graphics.pixar.com/usd/docs/api/kind_page_front.html). If the
//!         token is empty or non-existing in the hierarchy, the exact prim that gets picked
//!         in the viewport will be selected.
TfToken GetSelectionKind()
{
    static const MString kOptionVarName(MayaUsdOptionVars->SelectionKind.GetText());

    if (MGlobal::optionVarExists(kOptionVarName)) {
        MString optionVarValue = MGlobal::optionVarStringValue(kOptionVarName);
        return TfToken(optionVarValue.asChar());
    }
    return TfToken();
}

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _pointInstancesPickModeTokens,

    (PointInstancer)
    (Instances)
    (Prototypes)
);
// clang-format on

//! \brief  Query the pick mode to use when picking point instances in the viewport.
//! \return A UsdPointInstancesPickMode enum value indicating the pick mode behavior
//!         to employ when the picked object is a point instance.
//!
//! This function retrieves the value for the point instances pick mode optionVar
//! and converts it into a UsdPointInstancesPickMode enum value. If the optionVar
//! has not been set or otherwise has an invalid value, the default pick mode of
//! PointInstancer is returned.
UsdPointInstancesPickMode GetPointInstancesPickMode()
{
    static const MString kOptionVarName(MayaUsdOptionVars->PointInstancesPickMode.GetText());

    UsdPointInstancesPickMode pickMode = UsdPointInstancesPickMode::PointInstancer;

    if (MGlobal::optionVarExists(kOptionVarName)) {
        const MString optionVarValue = MGlobal::optionVarStringValue(kOptionVarName);
        const TfToken pickModeToken(UsdMayaUtil::convert(optionVarValue));

        if (pickModeToken == _pointInstancesPickModeTokens->Instances) {
            pickMode = UsdPointInstancesPickMode::Instances;
        } else if (pickModeToken == _pointInstancesPickModeTokens->Prototypes) {
            pickMode = UsdPointInstancesPickMode::Prototypes;
        }
    }

    return pickMode;
}

//! \brief  Returns the prim or an ancestor of it that is of the given kind.
//
// If neither the prim itself nor any of its ancestors above it in the
// namespace hierarchy have an authored kind that matches, an invalid null
// prim is returned.
UsdPrim GetPrimOrAncestorWithKind(const UsdPrim& prim, const TfToken& kind)
{
    UsdPrim iterPrim = prim;
    TfToken primKind;

    while (iterPrim) {
        if (UsdModelAPI(iterPrim).GetKind(&primKind) && KindRegistry::IsA(primKind, kind)) {
            break;
        }

        iterPrim = iterPrim.GetParent();
    }

    return iterPrim;
}

//! \brief  Populate Rprims into the Hydra selection from the UFE scene item.
void PopulateSelection(
    const Ufe::SceneItem::Ptr&  item,
    const Ufe::Path&            proxyPath,
    UsdImagingDelegate&         sceneDelegate,
    const HdSelectionSharedPtr& result)
{
    // Filter out items which are not under the current proxy shape.
    if (!item->path().startsWith(proxyPath)) {
        return;
    }

    // Filter out non-USD items.
    auto usdItem = std::dynamic_pointer_cast<MayaUsd::ufe::UsdSceneItem>(item);
    if (!usdItem) {
        return;
    }

    SdfPath   usdPath = usdItem->prim().GetPath();
    const int instanceIndex = usdItem->instanceIndex();

#if !defined(USD_IMAGING_API_VERSION) || USD_IMAGING_API_VERSION < 11
    usdPath = sceneDelegate.ConvertCachePathToIndexPath(usdPath);
#endif

    sceneDelegate.PopulateSelection(
        HdSelection::HighlightModeSelect, usdPath, instanceIndex, result);
}
#endif // defined(WANT_UFE_BUILD)

//! \brief  Append the selected prim paths to the result list.
void AppendSelectedPrimPaths(const HdSelectionSharedPtr& selection, SdfPathVector& result)
{
    if (!selection) {
        return;
    }

    SdfPathVector paths = selection->GetSelectedPrimPaths(HdSelection::HighlightModeSelect);
    if (paths.empty()) {
        return;
    }

    if (result.empty()) {
        result.swap(paths);
    } else {
        result.reserve(result.size() + paths.size());
        result.insert(result.end(), paths.begin(), paths.end());
    }
}

//! \brief  Configure repr descriptions
void _ConfigureReprs()
{
    const HdMeshReprDesc reprDescHull(
        HdMeshGeomStyleHull,
        HdCullStyleDontCare,
        HdMeshReprDescTokens->surfaceShader,
        /*flatShadingEnabled=*/false,
        /*blendWireframeColor=*/false);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    const HdMeshReprDesc reprDescHullDefaultMaterial(
        HdMeshGeomStyleHull,
        HdCullStyleDontCare,
        HdMeshReprDescTokens->constantColor,
        /*flatShadingEnabled=*/false,
        /*blendWireframeColor=*/false);
#endif

    const HdMeshReprDesc reprDescEdge(
        HdMeshGeomStyleHullEdgeOnly,
        HdCullStyleDontCare,
        HdMeshReprDescTokens->surfaceShader,
        /*flatShadingEnabled=*/false,
        /*blendWireframeColor=*/false);

    // Hull desc for shaded display, edge desc for selection highlight.
    HdMesh::ConfigureRepr(HdReprTokens->smoothHull, reprDescHull, reprDescEdge);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    // Hull desc for default material display, edge desc for selection highlight.
    HdMesh::ConfigureRepr(
        HdVP2ReprTokens->defaultMaterial, reprDescHullDefaultMaterial, reprDescEdge);
#endif

    // Edge desc for bbox display.
    HdMesh::ConfigureRepr(HdVP2ReprTokens->bbox, reprDescEdge);

    // Wireframe desc for bbox display.
    HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->bbox, HdBasisCurvesGeomStyleWire);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    // Wire for default material:
    HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->defaultMaterial, HdBasisCurvesGeomStyleWire);
#endif
}

#if defined(WANT_UFE_BUILD)
class UfeObserver : public Ufe::Observer
{
public:
    UfeObserver(ProxyRenderDelegate& proxyRenderDelegate)
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

        if (dynamic_cast<const Ufe::SelectionChanged*>(&notification)
            || dynamic_cast<const Ufe::ObjectAdd*>(&notification)) {
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

// Copied from renderIndex.cpp, the code that does HdRenderIndex::GetDrawItems. But I just want the
// rprimIds, I don't want to go all the way to draw items.
#if defined(HD_API_VERSION) && HD_API_VERSION >= 42
struct _FilterParam
{
    const TfTokenVector& renderTags;
    const HdRenderIndex* renderIndex;
};

bool _DrawItemFilterPredicate(const SdfPath& rprimID, const void* predicateParam)
{
    const _FilterParam* filterParam = static_cast<const _FilterParam*>(predicateParam);

    const TfTokenVector& renderTags = filterParam->renderTags;
    const HdRenderIndex* renderIndex = filterParam->renderIndex;

    //
    // Render Tag Filter
    //
    if (renderTags.empty()) {
        // An empty render tag set means everything passes the filter
        // Primary user is tests, but some single task render delegates
        // that don't support render tags yet also use it.
        return true;
    } else {
        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        TfToken primRenderTag = renderIndex->GetRenderTag(rprimID);
        size_t  numRenderTags = renderTags.size();
        size_t  tagNum = 0;
        while (tagNum < numRenderTags) {
            if (renderTags[tagNum] == primRenderTag) {
                return true;
            }
            ++tagNum;
        }
    }
    return false;
}
#else
struct _FilterParam
{
    const HdRprimCollection& collection;
    const TfTokenVector&     renderTags;
    const HdRenderIndex*     renderIndex;
};

bool _DrawItemFilterPredicate(const SdfPath& rprimID, const void* predicateParam)
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
#endif

} // namespace

//! \brief  Draw classification used during plugin load to register in VP2
const MString ProxyRenderDelegate::drawDbClassification(
    TfStringPrintf(
        "drawdb/subscene/vp2RenderDelegate/%s",
        MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText())
        .c_str());

//! \brief  Factory method registered at plugin load
MHWRender::MPxSubSceneOverride* ProxyRenderDelegate::Creator(const MObject& obj)
{
    return new ProxyRenderDelegate(obj);
}

//! \brief  Constructor
ProxyRenderDelegate::ProxyRenderDelegate(const MObject& obj)
    : Autodesk::Maya::OPENMAYA_MPXSUBSCENEOVERRIDE_LATEST_NAMESPACE::MHWRender::MPxSubSceneOverride(
        obj)
{
    MDagPath proxyDagPath;
    MDagPath::getAPathTo(obj, proxyDagPath);

    const MFnDependencyNode fnDepNode(obj);
    _proxyShapeData.reset(new ProxyShapeData(
        static_cast<MayaUsdProxyShapeBase*>(fnDepNode.userNode()), proxyDagPath));
}

//! \brief  Destructor
ProxyRenderDelegate::~ProxyRenderDelegate()
{
    _ClearRenderDelegate();

#if !defined(WANT_UFE_BUILD)
    if (_mayaSelectionCallbackId != 0) {
        MMessage::removeCallback(_mayaSelectionCallbackId);
    }
#endif
}

//! \brief  This drawing routine supports all devices (DirectX and OpenGL)
MHWRender::DrawAPI ProxyRenderDelegate::supportedDrawAPIs() const { return MHWRender::kAllDevices; }

#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
//! \brief  Enable subscene update in selection passes for deferred update of selection render
//! items.
bool ProxyRenderDelegate::enableUpdateForSelection() const { return true; }
#endif

//! \brief  Always requires update since changes are tracked by Hydraw change tracker and it will
//! guarantee minimal update; only exception is if rendering through Maya-to-Hydra
bool ProxyRenderDelegate::requiresUpdate(
    const MSubSceneContainer& container,
    const MFrameContext&      frameContext) const
{
#if defined(BUILD_HDMAYA)
    // If the current viewport renderer is an mtoh one, skip this update, as
    // mtoh already has special handling for proxy shapes, and we don't want to
    // build out a render index we don't need
    if (IsMtohRenderOverride(frameContext)) {
        return false;
    }
#endif
    return true;
}

void ProxyRenderDelegate::_ClearRenderDelegate()
{
    // The order of deletion matters. Some orders cause crashes.

    _sceneDelegate.reset();
    _taskController.reset();
    _renderIndex.reset();
    _renderDelegate.reset();

    _dummyTasks.clear();

    // reset any version ids or dirty information that doesn't make sense if we clear
    // the render index.
    _changeVersions.reset();
    _taskRenderTagsValid = false;
    _isPopulated = false;
}

//! \brief  Clear data which is now stale because proxy shape attributes have changed
void ProxyRenderDelegate::_ClearInvalidData(MSubSceneContainer& container)
{
    TF_VERIFY(_proxyShapeData->ProxyShape());

    // We have to clear everything when the stage changes because the new stage doesn't necessarily
    // have anything in common with the old stage.
    // When excluded prims changes we don't have a way to know which (if any) prims were removed
    // from excluded prims & so must be re-added to the render index, so we take the easy way out
    // and clear everything. If this is a performance problem we can probably store the old value
    // of excluded prims, compare it to the new value and only add back the difference.
    if (!_proxyShapeData->IsUsdStageUpToDate() || !_proxyShapeData->IsExcludePrimsUpToDate()) {
        // Tell texture loading tasks to terminate (exit) if they have not finished yet
        if (_renderDelegate) {
            dynamic_cast<HdVP2RenderDelegate*>(_renderDelegate.get())->CleanupMaterials();
        }
        // delete everything so we can re-initialize with the new stage
        _ClearRenderDelegate();
        container.clear();
    }
}

//! \brief  Initialize the render delegate
void ProxyRenderDelegate::_InitRenderDelegate()
{
    TF_VERIFY(_proxyShapeData->ProxyShape());

    // No need to run all the checks if we got till the end
    if (_isInitialized())
        return;

    _proxyShapeData->UpdateUsdStage();
    _proxyShapeData->UsdStageUpdated();

    if (!_renderDelegate) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1,
            "Allocate VP2RenderDelegate");
        _renderDelegate.reset(new HdVP2RenderDelegate(*this));
    }

    if (!_renderIndex) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L1, "Allocate RenderIndex");
        _renderIndex.reset(HdRenderIndex::New(_renderDelegate.get(), HdDriverVector()));

        // Sync the _changeVersions so that we don't trigger a needlessly large update them on the
        // first frame.
        _changeVersions.sync(_renderIndex->GetChangeTracker());

        // Add additional configurations after render index creation.
        static std::once_flag reprsOnce;
        std::call_once(reprsOnce, _ConfigureReprs);
    }

    if (!_sceneDelegate) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1,
            "Allocate SceneDelegate");

        // Make sure the delegate name is a valid identifier, since it may
        // include colons if the proxy node is in a Maya namespace.
        const std::string delegateName = TfMakeValidIdentifier(TfStringPrintf(
            "Proxy_%s_%p",
            _proxyShapeData->ProxyShape()->name().asChar(),
            _proxyShapeData->ProxyShape()));
        const SdfPath delegateID = SdfPath::AbsoluteRootPath().AppendChild(TfToken(delegateName));

        _sceneDelegate.reset(new UsdImagingDelegate(_renderIndex.get(), delegateID));

        _taskController.reset(new HdxTaskController(
            _renderIndex.get(),
            delegateID.AppendChild(TfToken(TfStringPrintf("_UsdImaging_VP2_%p", this)))));

        _defaultCollection.reset(new HdRprimCollection());
        _defaultCollection->SetName(HdTokens->geometry);

#if defined(WANT_UFE_BUILD)
        if (!_observer) {
            _observer = std::make_shared<UfeObserver>(*this);

            auto globalSelection = Ufe::GlobalSelection::get();
            if (TF_VERIFY(globalSelection)) {
                globalSelection->addObserver(_observer);
            }

#ifdef UFE_V2_FEATURES_AVAILABLE
            Ufe::Scene::instance().addObserver(_observer);
#else
            Ufe::Scene::instance().addObjectAddObserver(_observer);
#endif
        }
#else
        // Without UFE, support basic selection highlight at proxy shape level.
        if (!_mayaSelectionCallbackId) {
            _mayaSelectionCallbackId
                = MEventMessage::addEventCallback("SelectionChanged", SelectionChangedCB, this);
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
bool ProxyRenderDelegate::_Populate()
{
    TF_VERIFY(_proxyShapeData->ProxyShape());

    if (!_isInitialized())
        return false;

    if (_proxyShapeData->UsdStage() && !_isPopulated) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L1, "Populate");

        // Remove any excluded prims before populating
        SdfPathVector excludePrimPaths = _proxyShapeData->ProxyShape()->getExcludePrimPaths();
        for (auto& excludePrim : excludePrimPaths) {
            SdfPath indexPath = _sceneDelegate->ConvertCachePathToIndexPath(excludePrim);
            if (_renderIndex->HasRprim(indexPath)) {
                _renderIndex->RemoveRprim(indexPath);
            }
        }
        _proxyShapeData->ExcludePrimsUpdated();
        _sceneDelegate->Populate(_proxyShapeData->ProxyShape()->usdPrim(), excludePrimPaths);
        _isPopulated = true;
    }

    return _isPopulated;
}

//! \brief  Synchronize USD scene delegate with Maya's proxy shape.
void ProxyRenderDelegate::_UpdateSceneDelegate()
{
    TF_VERIFY(_proxyShapeData->ProxyShape());

    if (!_sceneDelegate)
        return;

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "UpdateSceneDelegate");

    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "SetTime");

        const UsdTimeCode timeCode = _proxyShapeData->ProxyShape()->getTime();
        _sceneDelegate->SetTime(timeCode);
    }

    // Update the root transform used to render by the delagate.
    // USD considers that the root prim transform is always the Identity matrix so that means
    // the root transform define the root prim transform. When the real stage root is used to
    // render this is not a issue because the root transform will be the maya transform.
    // The problem is when using a primPath as the root prim, we are losing
    // the prim path world transform. So we need to set the root transform as the world
    // transform of the prim used for rendering.
    const MMatrix inclusiveMatrix = _proxyShapeData->ProxyDagPath().inclusiveMatrix();
    GfMatrix4d    transform(inclusiveMatrix.matrix);

    if (_proxyShapeData->ProxyShape()->usdPrim().GetPath() != SdfPath::AbsoluteRootPath()) {
        const UsdTimeCode timeCode = _proxyShapeData->ProxyShape()->getTime();
        UsdGeomXformCache xformCache(timeCode);
        GfMatrix4d        m
            = xformCache.GetLocalToWorldTransform(_proxyShapeData->ProxyShape()->usdPrim());
        transform = m * transform;
    }

    constexpr double tolerance = 1e-9;
    if (!GfIsClose(transform, _sceneDelegate->GetRootTransform(), tolerance)) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "SetRootTransform");
        _sceneDelegate->SetRootTransform(transform);
    }

    const bool isVisible = _proxyShapeData->ProxyDagPath().isVisible();
    if (isVisible != _sceneDelegate->GetRootVisibility()) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "SetRootVisibility");
        _sceneDelegate->SetRootVisibility(isVisible);

        // Trigger selection update when a hidden proxy shape gets shown.
        if (isVisible) {
            SelectionChanged();
        }
    }

    const int refineLevel = _proxyShapeData->ProxyShape()->getComplexity();
    if (refineLevel != _sceneDelegate->GetRefineLevelFallback()) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L1,
            "SetRefineLevelFallback");

        _sceneDelegate->SetRefineLevelFallback(refineLevel);
    }
}

//! \brief  Execute Hydra engine to perform minimal VP2 draw data update based on change tracker.
void ProxyRenderDelegate::_Execute(const MHWRender::MFrameContext& frameContext)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "Execute");

    ++_frameCounter;
    _UpdateRenderTags();

    // If update for selection is enabled, the draw data for the "points" repr
    // won't be prepared until point snapping is activated; otherwise the draw
    // data have to be prepared early for possible activation of point snapping.
#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    HdReprSelector reprSelector;

    const bool inSelectionPass = (frameContext.getSelectionInfo() != nullptr);
#if !defined(MAYA_NEW_POINT_SNAPPING_SUPPORT) || defined(WANT_UFE_BUILD)
    const bool inPointSnapping = pointSnappingActive();
#endif

#if defined(WANT_UFE_BUILD)
    // Query selection adjustment and kind only if the update is triggered in a selection pass.
    if (inSelectionPass && !inPointSnapping) {
        _globalListAdjustment = GetListAdjustment();
        _selectionKind = GetSelectionKind();
        _pointInstancesPickMode = GetPointInstancesPickMode();
    } else {
        _globalListAdjustment = MGlobal::kReplaceList;
        _selectionKind = TfToken();
        _pointInstancesPickMode = UsdPointInstancesPickMode::PointInstancer;
    }
#endif // defined(WANT_UFE_BUILD)

#else // !defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    HdReprSelector reprSelector = kPointsReprSelector;

    constexpr bool     inSelectionPass = false;
#if !defined(MAYA_NEW_POINT_SNAPPING_SUPPORT)
    constexpr bool     inPointSnapping = false;
#endif
#endif // defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)

    const unsigned int currentDisplayStyle = frameContext.getDisplayStyle();

    // Query the wireframe color assigned to proxy shape.
    if (currentDisplayStyle
        & (MHWRender::MFrameContext::kBoundingBox | MHWRender::MFrameContext::kWireFrame)) {
        _wireframeColor
            = MHWRender::MGeometryUtilities::wireframeColor(_proxyShapeData->ProxyDagPath());
    }
#ifdef MAYA_HAS_DISPLAY_STYLE_ALL_VIEWPORTS
    const unsigned int displayStyle = frameContext.getDisplayStyleOfAllViewports();
#else
    const unsigned int displayStyle = currentDisplayStyle;
#endif

    // Work around USD issue #1516. There is a significant performance overhead caused by populating
    // selection, so only force the populate selection to occur when we detect a change which
    // impacts the instance indexing.
    HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();
    bool             forcePopulateSelection = !_changeVersions.instanceIndexValid(changeTracker);
    _changeVersions.sync(changeTracker);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    if (_selectionModeChanged || (_selectionChanged && !inSelectionPass)
        || forcePopulateSelection) {
        _UpdateSelectionStates();
        _selectionChanged = false;
        _selectionModeChanged = false;
    }
#else
    if ((_selectionChanged && !inSelectionPass) || forcePopulateSelection) {
        _UpdateSelectionStates();
        _selectionChanged = false;
    }
#endif

    if (inSelectionPass) {
        // The new Maya point snapping support doesn't require point snapping items any more.
#if !defined(MAYA_NEW_POINT_SNAPPING_SUPPORT)
        if (inPointSnapping && !reprSelector.Contains(HdReprTokens->points)) {
            reprSelector = reprSelector.CompositeOver(kPointsReprSelector);
        }
#endif
    } else {
        // Update repr selector based on display style of the current viewport
        if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
            if (!reprSelector.Contains(HdVP2ReprTokens->bbox)) {
                reprSelector = reprSelector.CompositeOver(kBBoxReprSelector);
            }
        } else {
            // To support Wireframe on Shaded mode, the two displayStyle checks
            // should not be mutually excluded.
            if (displayStyle & MHWRender::MFrameContext::kGouraudShaded) {
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
                if (displayStyle & MHWRender::MFrameContext::kDefaultMaterial) {
                    if (!reprSelector.Contains(HdVP2ReprTokens->defaultMaterial)) {
                        reprSelector = reprSelector.CompositeOver(kDefaultMaterialReprSelector);
                    }
                } else
#endif
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

    // if there are no repr's to update then don't even call sync.
    if (reprSelector != HdReprSelector()) {
        if (_defaultCollection->GetReprSelector() != reprSelector) {
            _defaultCollection->SetReprSelector(reprSelector);
            _taskController->SetCollection(*_defaultCollection);

            // Mark everything "dirty" so that sync is called on everything
            // If there are multiple views up with different viewport modes then
            // this is slow.
            auto& rprims = _renderIndex->GetRprimIds();
            for (auto path : rprims) {
                changeTracker.MarkRprimDirty(path, MayaUsdRPrim::DirtyDisplayMode);
            }
        }

        _engine.Execute(_renderIndex.get(), &_dummyTasks);
    }
}

//! \brief  Main update entry from subscene override.
void ProxyRenderDelegate::update(MSubSceneContainer& container, const MFrameContext& frameContext)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L1,
        "ProxyRenderDelegate::update");

    // Without a proxy shape we can't do anything
    if (_proxyShapeData->ProxyShape() == nullptr)
        return;

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    const MSelectionInfo* selectionInfo = frameContext.getSelectionInfo();
    if (selectionInfo) {
        bool oldSnapToPoints = _snapToPoints;
#if MAYA_API_VERSION >= 20220000
        _snapToPoints = selectionInfo->pointSnapping();
#else
        _snapToPoints = pointSnappingActive();
#endif
        if (_snapToPoints != oldSnapToPoints) {
            _selectionModeChanged = true;
        }
    }

    MStatus status;
    if (selectionInfo) {
        bool oldSnapToSelectedObjects = _snapToSelectedObjects;
        _snapToSelectedObjects = selectionInfo->snapToActive(&status);
        TF_VERIFY(status == MStatus::kSuccess);
        if (_snapToSelectedObjects != oldSnapToSelectedObjects) {
            _selectionModeChanged = true;
        }
    }
#endif

    _ClearInvalidData(container);

    _InitRenderDelegate();

    // Give access to current time and subscene container to the rest of render delegate world via
    // render param's.
    auto* param = reinterpret_cast<HdVP2RenderParam*>(_renderDelegate->GetRenderParam());
    param->BeginUpdate(container, _sceneDelegate->GetTime());

    if (_Populate()) {
        _UpdateSceneDelegate();
        _Execute(frameContext);
    }
    param->EndUpdate();
}

//! \brief  Update selection granularity for point snapping.
void ProxyRenderDelegate::updateSelectionGranularity(
    const MDagPath&               path,
    MHWRender::MSelectionContext& selectionContext)
{
    Selectability::prepareForSelection();

    // The component level is coarse-grain, causing Maya to produce undesired face/edge selection
    // hits, as well as vertex selection hits that are required for point snapping. Switch to the
    // new vertex selection level if available in order to produce vertex selection hits only.
    if (pointSnappingActive()) {
#if MAYA_API_VERSION >= 20220100
        selectionContext.setSelectionLevel(MHWRender::MSelectionContext::kVertex);
#else
        selectionContext.setSelectionLevel(MHWRender::MSelectionContext::kComponent);
#endif
    }
}

// Resolves an rprimId and instanceIndex back to the original USD gprim and instance index.
// see UsdImagingDelegate::GetScenePrimPath.
// This version works against all the older versions of USD we care about. Once those old
// versions go away, and we only support USD_IMAGING_API_VERSION >= 14 then we can remove
// this function.
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
SdfPath ProxyRenderDelegate::GetScenePrimPath(
    const SdfPath&      rprimId,
    int                 instanceIndex,
    HdInstancerContext* instancerContext) const
#else
SdfPath ProxyRenderDelegate::GetScenePrimPath(const SdfPath& rprimId, int instanceIndex) const
#endif
{
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
    SdfPath usdPath = _sceneDelegate->GetScenePrimPath(rprimId, instanceIndex, instancerContext);
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 13
    SdfPath usdPath = _sceneDelegate->GetScenePrimPath(rprimId, instanceIndex);
#else
    SdfPath indexPath;
    if (drawInstID > 0) {
        indexPath = _sceneDelegate->GetPathForInstanceIndex(rprimId, instanceIndex, nullptr);
    } else {
        indexPath = rprimId;
    }

    SdfPath usdPath = _sceneDelegate->ConvertIndexPathToCachePath(indexPath);

    // Examine the USD path. If it is not a valid prim path, the selection hit is from a single
    // instance Rprim and indexPath is actually its instancer Rprim id. In this case we should
    // call GetPathForInstanceIndex() using 0 as the instance index.
    if (!usdPath.IsPrimPath()) {
        indexPath = _sceneDelegate->GetPathForInstanceIndex(rprimId, 0, nullptr);
        usdPath = _sceneDelegate->ConvertIndexPathToCachePath(indexPath);
    }

    // The "Instances" point instances pick mode is not supported for
    // USD_IMAGING_API_VERSION < 14 (core USD versions earlier than 20.08), so
    // no using instancerContext here.
#endif

    return usdPath;
}

//! \brief  Selection for both instanced and non-instanced cases.
bool ProxyRenderDelegate::getInstancedSelectionPath(
    const MHWRender::MRenderItem&   renderItem,
    const MHWRender::MIntersection& intersection,
    MDagPath&                       dagPath) const
{
#if defined(WANT_UFE_BUILD)
    // When point snapping, only the point position matters, so return the DAG path and avoid the
    // UFE global selection list to be updated.
    if (pointSnappingActive()) {
        dagPath = _proxyShapeData->ProxyDagPath();
        return true;
    }

    if (!_proxyShapeData->ProxyShape() || !_proxyShapeData->ProxyShape()->isUfeSelectionEnabled()) {
        return false;
    }

    auto handler = Ufe::RunTimeMgr::instance().hierarchyHandler(USD_UFE_RUNTIME_ID);
    if (handler == nullptr)
        return false;

    const SdfPath rprimId = HdVP2DrawItem::RenderItemToPrimPath(renderItem);

    // If drawInstID is positive, it means the selection hit comes from one instanced render item,
    // in this case its instance transform matrices have been sorted w.r.t. USD instance index, thus
    // instanceIndex is drawInstID minus 1 because VP2 instance IDs start from 1.  Otherwise the
    // selection hit is from one regular render item, but the Rprim can be either plain or single
    // instance, because we don't use instanced draw for single instance render items in order to
    // improve draw performance in Maya 2020 and before.
    const int drawInstID = intersection.instanceID();
    int       instanceIndex = (drawInstID > 0) ? drawInstID - 1 : UsdImagingDelegate::ALL_INSTANCES;

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    // Get the custom data from the MRenderItem and map the instance index to the USD instance index
    auto mayaToUsd = MayaUsdCustomData::Get(renderItem);
    if (instanceIndex != UsdImagingDelegate::ALL_INSTANCES
        && ((int)mayaToUsd.size()) > instanceIndex) {
        instanceIndex = mayaToUsd[instanceIndex];
    }
#endif

    SdfPath topLevelPath;
    int     topLevelInstanceIndex = UsdImagingDelegate::ALL_INSTANCES;

#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
    HdInstancerContext instancerContext;
    SdfPath            usdPath = GetScenePrimPath(rprimId, instanceIndex, &instancerContext);

    if (!instancerContext.empty()) {
        // Store the top-level instancer and instance index if the Rprim is the
        // result of point instancing. These will be used for the "Instances"
        // point instances pick mode.
        topLevelPath = instancerContext.front().first;
        topLevelInstanceIndex = instancerContext.front().second;
    }
#else
    SdfPath usdPath = GetScenePrimPath(rprimId, instanceIndex);
#endif

    // If update for selection is enabled, we can query the Maya selection list
    // adjustment, USD selection kind, and USD point instances pick mode once
    // per selection update to avoid the cost of executing MEL commands or
    // searching optionVars for each intersection.
#if defined(MAYA_ENABLE_UPDATE_FOR_SELECTION)
    const TfToken&                   selectionKind = _selectionKind;
    const UsdPointInstancesPickMode& pointInstancesPickMode = _pointInstancesPickMode;
    const MGlobal::ListAdjustment&   listAdjustment = _globalListAdjustment;
#else
    const TfToken selectionKind = GetSelectionKind();
    const UsdPointInstancesPickMode pointInstancesPickMode = GetPointInstancesPickMode();
    const MGlobal::ListAdjustment listAdjustment = GetListAdjustment();
#endif

    UsdPrim       prim = _proxyShapeData->UsdStage()->GetPrimAtPath(usdPath);
    const UsdPrim topLevelPrim = _proxyShapeData->UsdStage()->GetPrimAtPath(topLevelPath);

    // Enforce selectability metadata.
    if (!Selectability::isSelectable(prim)) {
        dagPath = MDagPath();
        return true;
    }

    // Resolve the selection based on the point instances pick mode.
    // Note that in all cases except for "Instances" when the picked
    // PointInstancer is *not* an instance proxy, we reset the instanceIndex to
    // ALL_INSTANCES. As a result, the behavior of Viewport 2.0 may differ
    // slightly for "Prototypes" from that of usdview. Though the pick should
    // resolve to the same prim as it would in usdview, the selection
    // highlighting in Viewport 2.0 will highlight all instances rather than
    // only a single point instancer prototype as it does in usdview. We do
    // this to ensure that only when in "Instances" point instances pick mode
    // will we ever construct UFE scene items that represent point instances
    // and have an instance index component at the end of their Ufe::Path.
    switch (pointInstancesPickMode) {
    case UsdPointInstancesPickMode::Instances: {
        if (topLevelPrim) {
            prim = topLevelPrim;
            instanceIndex = topLevelInstanceIndex;
        }
        if (prim.IsInstanceProxy()) {
            while (prim.IsInstanceProxy()) {
                prim = prim.GetParent();
            }
            instanceIndex = UsdImagingDelegate::ALL_INSTANCES;
        }
        usdPath = prim.GetPath();
        break;
    }
    case UsdPointInstancesPickMode::Prototypes: {
        // The scene prim path returned by Hydra *is* the prototype prim's
        // path. We still reset instanceIndex since we're not picking a point
        // instance.
        instanceIndex = UsdImagingDelegate::ALL_INSTANCES;
        break;
    }
    case UsdPointInstancesPickMode::PointInstancer:
    default: {
        if (topLevelPrim) {
            prim = topLevelPrim;
        }
        while (prim.IsInstanceProxy()) {
            prim = prim.GetParent();
        }
        usdPath = prim.GetPath();
        instanceIndex = UsdImagingDelegate::ALL_INSTANCES;
        break;
    }
    }

    // If we didn't pick a point instance above, then search up from the picked
    // prim to satisfy the requested USD selection kind, if specified. If no
    // selection kind is specified, the exact prim that was picked from the
    // viewport should be selected, so there is no need to walk the scene
    // hierarchy.
    if (instanceIndex == UsdImagingDelegate::ALL_INSTANCES && !selectionKind.IsEmpty()) {
        prim = GetPrimOrAncestorWithKind(prim, selectionKind);
        if (prim) {
            usdPath = prim.GetPath();
        }
    }

    const Ufe::PathSegment pathSegment
        = MayaUsd::ufe::usdPathToUfePathSegment(usdPath, instanceIndex);
    const Ufe::SceneItem::Ptr& si
        = handler->createItem(_proxyShapeData->ProxyShape()->ufePath() + pathSegment);
    if (!si) {
        TF_WARN("Failed to create UFE scene item for Rprim '%s'", rprimId.GetText());
        return false;
    }

#ifdef UFE_V2_FEATURES_AVAILABLE
    TF_UNUSED(listAdjustment);

    auto ufeSel = Ufe::NamedSelection::get("MayaSelectTool");
    ufeSel->append(si);
#else
    auto globalSelection = Ufe::GlobalSelection::get();

    switch (listAdjustment) {
    case MGlobal::kReplaceList:
        // The list has been cleared before viewport selection runs, so we
        // can add the new hits directly. UFE selection list is a superset
        // of Maya selection list, calling clear()/replaceWith() on UFE
        // selection list would clear Maya selection list.
        globalSelection->append(si);
        break;
    case MGlobal::kAddToList: globalSelection->append(si); break;
    case MGlobal::kRemoveFromList: globalSelection->remove(si); break;
    case MGlobal::kXORWithList:
        if (!globalSelection->remove(si)) {
            globalSelection->append(si);
        }
        break;
    default: TF_WARN("Unexpected MGlobal::ListAdjustment enum for selection."); break;
    }
#endif
#else
    dagPath = _proxyShapeData->ProxyDagPath();
#endif

    return true;
}

#ifdef MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT
bool ProxyRenderDelegate::updateUfeIdentifiers(
    MHWRender::MRenderItem& renderItem,
    MStringArray&           ufeIdentifiers)
{

    if (MayaUsdCustomData::ItemDataDirty(renderItem)) {
        // Set the custom data clean right away, incase we get a re-entrant call into
        // updateUfeIdentifiers.
        MayaUsdCustomData::ItemDataDirty(renderItem, false);
        const SdfPath rprimId = HdVP2DrawItem::RenderItemToPrimPath(renderItem);

        InstancePrimPaths& instancePrimPaths = MayaUsdCustomData::GetInstancePrimPaths(rprimId);

        auto   mayaToUsd = MayaUsdCustomData::Get(renderItem);
        size_t instanceCount = mayaToUsd.size();
        if (instanceCount > 0) {
            for (size_t mayaInstanceId = 0; mayaInstanceId < instanceCount; mayaInstanceId++) {
                int usdInstanceId = mayaToUsd[mayaInstanceId];
                if (usdInstanceId == UsdImagingDelegate::ALL_INSTANCES)
                    continue;

                // try making a cache of the USD ID to the ufeIdentifier.
                if (instancePrimPaths[usdInstanceId] == SdfPath()) {
                    instancePrimPaths[usdInstanceId] = GetScenePrimPath(rprimId, usdInstanceId);
                }
#ifdef DEBUG
                else {
                    // verify the entry is still correct
                    TF_VERIFY(
                        instancePrimPaths[usdInstanceId]
                        == GetScenePrimPath(rprimId, usdInstanceId));
                }
#endif

                ufeIdentifiers.append(instancePrimPaths[usdInstanceId].GetString().c_str());
            }
        }
        return true;
    }
    return false;
}
#endif

//! \brief  Notify of selection change.
void ProxyRenderDelegate::SelectionChanged() { _selectionChanged = true; }

//! \brief  Populate lead and active selection for Rprims under the proxy shape.
void ProxyRenderDelegate::_PopulateSelection()
{
#if defined(WANT_UFE_BUILD)
    if (_proxyShapeData->ProxyShape() == nullptr) {
        return;
    }

    _leadSelection.reset(new HdSelection);
    _activeSelection.reset(new HdSelection);

    const auto proxyPath = _proxyShapeData->ProxyShape()->ufePath();
    const auto globalSelection = Ufe::GlobalSelection::get();

    // Populate lead selection from the last item in UFE global selection.
    auto it = globalSelection->crbegin();
    if (it != globalSelection->crend()) {
        PopulateSelection(*it, proxyPath, *_sceneDelegate, _leadSelection);

        // Start reverse iteration from the second last item in UFE global
        // selection and populate active selection.
        for (it++; it != globalSelection->crend(); it++) {
            PopulateSelection(*it, proxyPath, *_sceneDelegate, _activeSelection);
        }
    }
#endif
}

/*! \brief  Notify selection change to rprims.
 */
void ProxyRenderDelegate::_UpdateSelectionStates()
{
    const MHWRender::DisplayStatus previousStatus = _displayStatus;
    _displayStatus = MHWRender::MGeometryUtilities::displayStatus(_proxyShapeData->ProxyDagPath());

    SdfPathVector        rootPaths;
    const SdfPathVector* dirtyPaths = nullptr;

    if (_displayStatus == MHWRender::kLead || _displayStatus == MHWRender::kActive) {
        if (_displayStatus != previousStatus) {
            rootPaths.push_back(SdfPath::AbsoluteRootPath());
            dirtyPaths = &_renderIndex->GetRprimIds();
        }
    } else if (previousStatus == MHWRender::kLead || previousStatus == MHWRender::kActive) {
        rootPaths.push_back(SdfPath::AbsoluteRootPath());
        dirtyPaths = &_renderIndex->GetRprimIds();
        _PopulateSelection();
    } else {
        // Append pre-update lead and active selection.
        AppendSelectedPrimPaths(_leadSelection, rootPaths);
        AppendSelectedPrimPaths(_activeSelection, rootPaths);

        // Update lead and active selection.
        _PopulateSelection();

        // Append post-update lead and active selection.
        AppendSelectedPrimPaths(_leadSelection, rootPaths);
        AppendSelectedPrimPaths(_activeSelection, rootPaths);

        dirtyPaths = &rootPaths;
    }

    if (!rootPaths.empty()) {
        // When the selection changes then we have to update all the selected render
        // items. Set a dirty flag on each of the rprims so they know what to update.
        // Avoid trying to set dirty the absolute root as it is not a Rprim.
        HdDirtyBits dirtySelectionBits = MayaUsdRPrim::DirtySelectionHighlight;
#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
        // If the selection mode changes, for example into or out of point snapping,
        // then we need to do a little extra work.
        if (_selectionModeChanged)
            dirtySelectionBits |= MayaUsdRPrim::DirtySelectionMode;
#endif
        HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();
        for (auto path : *dirtyPaths) {
            if (_renderIndex->HasRprim(path))
                changeTracker.MarkRprimDirty(path, dirtySelectionBits);
        }

        // now that the appropriate prims have been marked dirty trigger
        // a sync so that they all update.
        HdRprimCollection collection(HdTokens->geometry, _defaultCollection->GetReprSelector());
        collection.SetRootPaths(rootPaths);
        _taskController->SetCollection(collection);
        _engine.Execute(_renderIndex.get(), &_dummyTasks);
        _taskController->SetCollection(*_defaultCollection);
    }
}

/*! \brief  Trigger rprim update for rprims whose visibility changed because of render tags change
 */
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
    // which means we do need to update individual MRenderItems when the displayed
    // render tags change, or when the render tag on an rprim changes.
    //
    // To handle an rprim's render tag value changing we need to be sure that
    // the dummy render task we use to draw includes all render tags. If we
    // leave any tags out then when an rprim changes from a visible tag to
    // a hidden one that rprim will get marked dirty, but Sync will not be
    // called because the rprim doesn't match the current render tags.
    //
    // When we change the desired render tags on the proxyShape we'll be adding
    // and/or removing some tags, so we can have existing MRenderItems that need
    // to be hidden, or hidden items that need to be shown. To do that we need
    // to make sure every rprim with a render tag whose visibility changed gets
    // marked dirty. This will ensure the upcoming execute call will update
    // the visibility of the MRenderItems in MPxSubSceneOverride.
    HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();

    // The renderTagsVersion increments when the render tags on an rprim are marked dirty,
    // or when the global render tags are set. Check to see if the render tags version has
    // changed since the last time we set the render tags so we know if there is a change
    // to an individual rprim or not.
    bool rprimRenderTagChanged = !_changeVersions.renderTagValid(changeTracker);
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
    rprimRenderTagChanged
        = rprimRenderTagChanged || !_changeVersions.visibilityValid(changeTracker);
#endif

    bool renderPurposeChanged = false;
    bool proxyPurposeChanged = false;
    bool guidePurposeChanged = false;
    _proxyShapeData->UpdatePurpose(
        &renderPurposeChanged, &proxyPurposeChanged, &guidePurposeChanged);
    bool anyPurposeChanged = renderPurposeChanged || proxyPurposeChanged || guidePurposeChanged;
    if (anyPurposeChanged) {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L1, "Update Purpose");

        TfTokenVector changedRenderTags;

        // Build the list of render tags which were added or removed (changed)
        // and the list of render tags which were removed.
        if (renderPurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->render);
        }
        if (proxyPurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->proxy);
        }
        if (guidePurposeChanged) {
            changedRenderTags.push_back(HdRenderTagTokens->guide);
        }

        // Mark all the rprims which have a render tag which changed dirty
        SdfPathVector rprimsToDirty = _GetFilteredRprims(*_defaultCollection, changedRenderTags);

        for (auto& id : rprimsToDirty) {
            // this call to MarkRprimDirty will increment the change tracker render
            // tag version. We don't want this to cause rprimRenderTagChanged to be
            // true when a tag hasn't actually changed.
            changeTracker.MarkRprimDirty(id, HdChangeTracker::DirtyRenderTag);
        }
    }

    // Vp2RenderDelegate implements render tags as a per-render item setting.
    // To handle cases when an rprim changes from a displayed tag to a hidden tag
    // we need to visit all the rprims and set the enable flag correctly on
    // all their render items. Do visit all the rprims we need to set the
    // render tags to be all the tags.
    // When an rprim has it's renderTag changed the global render tag version
    // id will change.
    if (rprimRenderTagChanged) {
        // Sync every rprim, no matter what it's tag is. We don't know the tag(s) of
        // the rprim that changed, so the only way we can be sure to sync it is by
        // syncing everything.
        TfTokenVector renderTags = { HdRenderTagTokens->geometry,
                                     HdRenderTagTokens->render,
                                     HdRenderTagTokens->proxy,
                                     HdRenderTagTokens->guide };
        _taskController->SetRenderTags(renderTags);
        _taskRenderTagsValid = false;
    }
    // When the render tag on an rprim changes we do a pass over all rprims to update
    // their visibility. The frame after we do the pass over all the tags, set the tags back to
    // the minimum set of tags.
    else if (anyPurposeChanged || !_taskRenderTagsValid) {
        TfTokenVector renderTags
            = { HdRenderTagTokens->geometry }; // always draw geometry render tag purpose.
        if (_proxyShapeData->DrawRenderPurpose() || renderPurposeChanged) {
            renderTags.push_back(HdRenderTagTokens->render);
        }
        if (_proxyShapeData->DrawProxyPurpose() || proxyPurposeChanged) {
            renderTags.push_back(HdRenderTagTokens->proxy);
        }
        if (_proxyShapeData->DrawGuidePurpose() || guidePurposeChanged) {
            renderTags.push_back(HdRenderTagTokens->guide);
        }
        _taskController->SetRenderTags(renderTags);
        // if the changedRenderTags is not empty then we could have some tags
        // in the _taskController just so that we get one sync to hide the render
        // items. In that case we need to leave _taskRenderTagsValid false, so that
        // we get a chance to remove that tag next frame.
        _taskRenderTagsValid = !anyPurposeChanged;
    }

    // TODO: UsdImagingDelegate is purpose-aware. There are methods
    // SetDisplayRender, SetDisplayProxy and SetDisplayGuides which inform the
    // scene delegate of what is displayed, and changes the behavior of
    // UsdImagingDelegate::GetRenderTag(). So far I don't see an advantage of
    // using this feature for MayaUSD, but it may be useful at some point in
    // the future.
}

//! \brief  List the rprims in collection that match renderTags
SdfPathVector ProxyRenderDelegate::_GetFilteredRprims(
    HdRprimCollection const& collection,
    TfTokenVector const&     renderTags)
{
    SdfPathVector        rprimIds;
    const SdfPathVector& paths = _renderIndex->GetRprimIds();
    const SdfPathVector& includePaths = collection.GetRootPaths();
    const SdfPathVector& excludePaths = collection.GetExcludePaths();
#if defined(HD_API_VERSION) && HD_API_VERSION >= 42
    _FilterParam filterParam = { renderTags, _renderIndex.get() };
#else
    _FilterParam filterParam = { collection, renderTags, _renderIndex.get() };
#endif
    HdPrimGather gather;
    gather.PredicatedFilter(
        paths, includePaths, excludePaths, _DrawItemFilterPredicate, &filterParam, &rprimIds);

    return rprimIds;
}

//! \brief  Query the selection state of a given prim from the lead selection.
const HdSelection::PrimSelectionState*
ProxyRenderDelegate::GetLeadSelectionState(const SdfPath& path) const
{
    return (_leadSelection == nullptr)
        ? nullptr
        : _leadSelection->GetPrimSelectionState(HdSelection::HighlightModeSelect, path);
}

//! \brief  Qeury the selection state of a given prim from the active selection.
const HdSelection::PrimSelectionState*
ProxyRenderDelegate::GetActiveSelectionState(const SdfPath& path) const
{
    return (_activeSelection == nullptr)
        ? nullptr
        : _activeSelection->GetPrimSelectionState(HdSelection::HighlightModeSelect, path);
}

//! \brief  Query the selection status of a given prim.
HdVP2SelectionStatus ProxyRenderDelegate::GetSelectionStatus(const SdfPath& path) const
{
    if (_displayStatus == MHWRender::kLead) {
        return kFullyLead;
    }

    if (_displayStatus == MHWRender::kActive) {
        return kFullyActive;
    }

    const HdSelection::PrimSelectionState* state = GetLeadSelectionState(path);
    if (state) {
        return state->fullySelected ? kFullyLead : kPartiallySelected;
    }

    state = GetActiveSelectionState(path);
    if (state) {
        return state->fullySelected ? kFullyActive : kPartiallySelected;
    }

    return kUnselected;
}

//! \brief  Query the wireframe color assigned to the proxy shape.
const MColor& ProxyRenderDelegate::GetWireframeColor() const { return _wireframeColor; }

GfVec3f ProxyRenderDelegate::GetDefaultColor(const TfToken& className)
{
    static const GfVec3f kDefaultColor(0.000f, 0.016f, 0.376f);

    // Prepare to construct the query command.
    const char*  queryName = "unsupported";
    GfVec3fCache* colorCache = nullptr;
    if (className == HdPrimTypeTokens->basisCurves) {
        colorCache = &_dormantCurveColorCache;
        queryName = "curve";
    } else if (className == HdPrimTypeTokens->points) {
        colorCache = &_dormantPointsColorCache;
        queryName = "particle";
    } else {
        TF_WARN(
            "ProxyRenderDelegate::GetDefaultColor - unsupported class: '%s'",
            className.IsEmpty() ? "empty" : className.GetString().c_str());
        return kDefaultColor;
    }

    // Check the cache. It is safe since colorCache->second is atomic
    if (colorCache->second == _frameCounter) {
        return colorCache->first;
    }

    // Enter the mutex and check the cache again
    std::lock_guard<std::mutex> mutexGuard(_mayaCommandEngineMutex);
    if (colorCache->second == _frameCounter) {
        return colorCache->first;
    }

    MString queryCommand = "int $index = `displayColor -q -dormant \"";
    queryCommand += queryName;
    queryCommand += "\"`; colorIndex -q $index;";

    // Execute Maya command engine to fetch the color
    MDoubleArray colorResult;
    MGlobal::executeCommand(queryCommand, colorResult);

    if (colorResult.length() == 3) {
        colorCache->first = GfVec3f(colorResult[0], colorResult[1], colorResult[2]);
    } else {
        TF_WARN("Failed to obtain default color");
        colorCache->first = kDefaultColor;
    }

    // Update the cache and return
    colorCache->second = _frameCounter;
    return colorCache->first;
}

//! \brief
MColor ProxyRenderDelegate::GetSelectionHighlightColor(const TfToken& className)
{
    static const MColor kDefaultLeadColor(0.056f, 1.0f, 0.366f, 1.0f);
    static const MColor kDefaultActiveColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Prepare to construct the query command.
    bool         fromPalette = true;
    const char*  queryName = "unsupported";
    MColorCache* colorCache = nullptr;
    if (className.IsEmpty()) {
        colorCache = &_leadColorCache;
        fromPalette = false;
        queryName = "lead";
    } else if (className == HdPrimTypeTokens->mesh) {
        colorCache = &_activeMeshColorCache;
        fromPalette = false;
        queryName = "polymeshActive";
    } else if (className == HdPrimTypeTokens->basisCurves) {
        colorCache = &_activeCurveColorCache;
        queryName = "curve";
    } else if (className == HdPrimTypeTokens->points) {
        colorCache = &_activePointsColorCache;
        queryName = "particle";
    } else {
        TF_WARN(
            "ProxyRenderDelegate::GetSelectionHighlightColor - unsupported class: '%s'",
            className.GetString().c_str());
        return kDefaultActiveColor;
    }

    // Check the cache. It is safe since colorCache->second is atomic
    if (colorCache->second == _frameCounter) {
        return colorCache->first;
    }

    // Enter the mutex and check the cache again
    std::lock_guard<std::mutex> mutexGuard(_mayaCommandEngineMutex);
    if (colorCache->second == _frameCounter) {
        return colorCache->first;
    }

    // Construct the query command string.
    MString queryCommand;
    if (fromPalette) {
        queryCommand = "int $index = `displayColor -q -active \"";
        queryCommand += queryName;
        queryCommand += "\"`; colorIndex -q $index;";
    } else {
        queryCommand = "displayRGBColor -q \"";
        queryCommand += queryName;
        queryCommand += "\"";
    }

    // Query and return the selection color.
    MDoubleArray colorResult;
    MGlobal::executeCommand(queryCommand, colorResult);

    if (colorResult.length() == 3) {
        MColor color(colorResult[0], colorResult[1], colorResult[2]);

        if (className.IsEmpty()) {
            // The 'lead' color is returned in display space, so we need to convert it to
            // rendering space. However, function MColorPickerUtilities::applyViewTransform
            // is supported only starting from Maya 2023, so in opposite case we just return
            // the default lead color.
#if MAYA_API_VERSION >= 20230000
            colorCache->first
                = MColorPickerUtilities::applyViewTransform(color, MColorPickerUtilities::kInverse);
#else
            colorCache->first = kDefaultLeadColor;
#endif
        } else {
            colorCache->first = color;
        }
    } else {
        TF_WARN(
            "Failed to obtain selection highlight color for '%s' objects",
            className.IsEmpty() ? "lead" : className.GetString().c_str());

        // In case of any failure, return the default color
        colorCache->first = className.IsEmpty() ? kDefaultLeadColor : kDefaultActiveColor;
    }

    // Update the cache and return
    colorCache->second = _frameCounter;
    return colorCache->first;
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

UsdImagingDelegate* ProxyRenderDelegate::GetUsdImagingDelegate() const
{
    return _sceneDelegate.get();
}

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
bool ProxyRenderDelegate::SnapToSelectedObjects() const { return _snapToSelectedObjects; }
bool ProxyRenderDelegate::SnapToPoints() const { return _snapToPoints; }
#endif

// ProxyShapeData
ProxyRenderDelegate::ProxyShapeData::ProxyShapeData(
    const MayaUsdProxyShapeBase* proxyShape,
    const MDagPath&              proxyDagPath)
    : _proxyShape(proxyShape)
    , _proxyDagPath(proxyDagPath)
{
    assert(_proxyShape);
}
inline const MayaUsdProxyShapeBase* ProxyRenderDelegate::ProxyShapeData::ProxyShape() const
{
    return _proxyShape;
}
inline const MDagPath& ProxyRenderDelegate::ProxyShapeData::ProxyDagPath() const
{
    return _proxyDagPath;
}
inline UsdStageRefPtr ProxyRenderDelegate::ProxyShapeData::UsdStage() const { return _usdStage; }
inline void           ProxyRenderDelegate::ProxyShapeData::UpdateUsdStage()
{
    _usdStage = _proxyShape->getUsdStage();
}
inline bool ProxyRenderDelegate::ProxyShapeData::IsUsdStageUpToDate() const
{
    return _proxyShape->getUsdStageVersion() == _usdStageVersion;
}
inline void ProxyRenderDelegate::ProxyShapeData::UsdStageUpdated()
{
    _usdStageVersion = _proxyShape->getUsdStageVersion();
}
inline bool ProxyRenderDelegate::ProxyShapeData::IsExcludePrimsUpToDate() const
{
    return _proxyShape->getExcludePrimPathsVersion() == _excludePrimsVersion;
}
inline void ProxyRenderDelegate::ProxyShapeData::ExcludePrimsUpdated()
{
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
