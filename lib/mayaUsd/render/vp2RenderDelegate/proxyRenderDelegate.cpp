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

#include "drawItem.h"
#include "material.h"
#include "mayaPrimCommon.h"
#include "renderDelegate.h"
#include "tokens.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/utils/diagnosticDelegate.h>
#include <mayaUsd/utils/selectability.h>

#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/points.h>
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
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MColorPickerUtilities.h>
#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MDGMessage.h>
#include <maya/MDisplayLayerMessage.h>
#endif
#include <maya/M3dView.h>
#include <maya/MEventMessage.h>
#include <maya/MFileIO.h>
#include <maya/MFnPluginData.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionContext.h>
#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#include <maya/MNodeMessage.h>
#endif

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/globalSelection.h>
#include <ufe/namedSelection.h>
#include <ufe/observableSelection.h>
#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <ufe/pathString.h>
#include <ufe/pathStringExcept.h>
#endif
#include <ufe/pathSegment.h>
#include <ufe/runTimeMgr.h>
#include <ufe/scene.h>
#include <ufe/sceneItem.h>
#include <ufe/sceneNotification.h>
#include <ufe/selectionNotification.h>

#if defined(BUILD_HDMAYA)
#include <mayaUsd/render/mayaToHydra/utils.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {

//! Representation selector for point snapping
const HdReprSelector kPointsReprSelector(TfToken(), TfToken(), HdReprTokens->points);

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
    auto usdItem = UsdUfe::downcast(item);
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

    const HdMeshReprDesc reprDescWire(
        HdMeshGeomStyleHullEdgeOnly,
        HdCullStyleDontCare,
        HdMeshReprDescTokens->surfaceShader,
        /*flatShadingEnabled=*/false,
        /*blendWireframeColor=*/true);

    // Hull desc for shaded display, edge desc for selection highlight.
    HdMesh::ConfigureRepr(HdReprTokens->smoothHull, reprDescHull, reprDescEdge);
    HdMesh::ConfigureRepr(HdVP2ReprTokens->smoothHullUntextured, reprDescHull, reprDescEdge);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    // Hull desc for default material display, edge desc for selection highlight.
    HdMesh::ConfigureRepr(
        HdVP2ReprTokens->defaultMaterial, reprDescHullDefaultMaterial, reprDescEdge);
#endif

    // Edge desc for bbox display.
    HdMesh::ConfigureRepr(HdVP2ReprTokens->bbox, reprDescEdge);

    // Forced representations are used for instanced geometry with display layer overrides
    HdMesh::ConfigureRepr(HdVP2ReprTokens->forcedBbox, reprDescEdge);
    HdMesh::ConfigureRepr(HdVP2ReprTokens->forcedWire, reprDescWire);
    // forcedUntextured repr doesn't use reprDescEdge descriptor because
    // its selection highlight will be drawn through a non-forced repr
    HdMesh::ConfigureRepr(HdVP2ReprTokens->forcedUntextured, reprDescHull);

    // smooth hull for untextured display
    HdBasisCurves::ConfigureRepr(
        HdVP2ReprTokens->smoothHullUntextured, HdBasisCurvesGeomStylePatch);

    // Wireframe desc for bbox display.
    HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->bbox, HdBasisCurvesGeomStyleWire);

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    // Wire for default material:
    HdBasisCurves::ConfigureRepr(HdVP2ReprTokens->defaultMaterial, HdBasisCurvesGeomStyleWire);
#endif

    HdPoints::ConfigureRepr(HdVP2ReprTokens->smoothHullUntextured, HdPointsGeomStylePoints);
}

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
        // Handle path change notifications here
#ifdef MAYA_HAS_DISPLAY_LAYER_API
#ifdef UFE_V4_FEATURES_AVAILABLE
        if (const auto sceneChanged = dynamic_cast<const Ufe::SceneChanged*>(&notification)) {
            if (Ufe::SceneChanged::SceneCompositeNotification == sceneChanged->opType()) {
                const auto& compNotification
                    = notification.staticCast<Ufe::SceneCompositeNotification>();
                for (const auto& op : compNotification) {
                    handleSceneOp(op);
                }
            } else {
                handleSceneOp(*sceneChanged);
            }
        }
#else
        if (auto objectRenamed = dynamic_cast<const Ufe::ObjectRename*>(&notification)) {
            _proxyRenderDelegate.DisplayLayerPathChanged(
                objectRenamed->previousPath(), objectRenamed->item()->path());
        } else if (
            auto objectReparented = dynamic_cast<const Ufe::ObjectReparent*>(&notification)) {
            _proxyRenderDelegate.DisplayLayerPathChanged(
                objectReparented->previousPath(), objectReparented->item()->path());
        } else if (
            auto compositeNotification
            = dynamic_cast<const Ufe::SceneCompositeNotification*>(&notification)) {
            for (const auto& op : compositeNotification->opsList()) {
                if (op.opType == Ufe::SceneCompositeNotification::OpType::ObjectRename
                    || op.opType == Ufe::SceneCompositeNotification::OpType::ObjectReparent) {
                    _proxyRenderDelegate.DisplayLayerPathChanged(op.path, op.item->path());
                }
            }
        }
#endif
#endif
        // Handle selection change notifications here
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

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#ifdef UFE_V4_FEATURES_AVAILABLE
    void handleSceneOp(const Ufe::SceneCompositeNotification::Op& op)
    {
        if (op.opType == Ufe::SceneChanged::ObjectPathChange) {
            if (op.subOpType == Ufe::ObjectPathChange::ObjectReparent
                || op.subOpType == Ufe::ObjectPathChange::ObjectRename) {
                _proxyRenderDelegate.DisplayLayerPathChanged(op.path, op.item->path());
            }
        }
    }
#endif
#endif

private:
    ProxyRenderDelegate& _proxyRenderDelegate;
};

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#ifdef MAYA_HAS_NEW_DISPLAY_LAYER_MESSAGING_API
void displayLayerMembershipChangedCB(void* data, const MString& memberPath)
{
    ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(data);
    if (prd) {
        prd->DisplayLayerMembershipChanged(memberPath);
    }
}
#else
void displayLayerMembershipChangedCB(void* data)
{
    ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(data);
    if (prd) {
        for (const auto& stage : MayaUsd::ufe::getAllStages()) {
            auto stagePath = Ufe::PathString::string(MayaUsd::ufe::stagePath(stage));
            prd->DisplayLayerMembershipChanged(MString(stagePath.c_str()));
        }
    }
}
#endif

void displayLayerDirtyCB(MObject& node, void* clientData)
{
    ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(clientData);
    if (prd && node.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(node);
        prd->DisplayLayerDirty(displayLayer);
    }
}
#endif

void colorPrefsChangedCB(void* clientData)
{
    ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(clientData);
    if (prd) {
        prd->ColorPrefsChanged();
    }
}

void colorManagementRefreshCB(void* clientData)
{
    ProxyRenderDelegate* prd = static_cast<ProxyRenderDelegate*>(clientData);
    if (prd) {
        prd->ColorManagementRefresh();
    }
}

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

bool _longDurationRendering = false;

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

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    if (_mayaDisplayLayerAddedCallbackId != 0)
        MMessage::removeCallback(_mayaDisplayLayerAddedCallbackId);
    if (_mayaDisplayLayerRemovedCallbackId != 0)
        MMessage::removeCallback(_mayaDisplayLayerRemovedCallbackId);
    if (_mayaDisplayLayerMembersCallbackId != 0)
        MMessage::removeCallback(_mayaDisplayLayerMembersCallbackId);
    for (auto cb : _mayaDisplayLayerDirtyCallbackIds) {
        MMessage::removeCallback(cb.second);
    }
#endif
    for (auto id : _mayaColorPrefsCallbackIds) {
        MMessage::removeCallback(id);
    }
    for (auto id : _mayaColorManagementCallbackIds) {
        MMessage::removeCallback(id);
    }
}

//! \brief  This drawing routine supports all devices (DirectX and OpenGL)
MHWRender::DrawAPI ProxyRenderDelegate::supportedDrawAPIs() const { return MHWRender::kAllDevices; }

//! \brief  Enable subscene update in selection passes for deferred update of selection render
//! items.
bool ProxyRenderDelegate::enableUpdateForSelection() const { return true; }

//! \brief  Always requires update since changes are tracked by Hydraw change tracker and it will
//! guarantee minimal update; only exception is if rendering through Maya-to-Hydra
bool ProxyRenderDelegate::requiresUpdate(
    const MSubSceneContainer& container,
    const MFrameContext&      frameContext) const
{
    // Hydra-based render overrides already take care of USD data,
    // so avoid duplicating the effort.
    if (px_vp20Utils::HasHydraRenderOverride(frameContext)) {
        return false;
    }

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

    // Initialize the optionVar ShowDisplayColorTextureOff, which will decide if display color will
    // be used when untextured mode is selected
    const MString optionVarName(MayaUsdOptionVars->ShowDisplayColorTextureOff.GetText());
    if (!MGlobal::optionVarExists(optionVarName)) {
        MGlobal::setOptionVarValue(optionVarName, 0);
    }

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

        if (!_observer) {
            _observer = std::make_shared<UfeObserver>(*this);

            auto globalSelection = Ufe::GlobalSelection::get();
            if (TF_VERIFY(globalSelection)) {
                globalSelection->addObserver(_observer);
            }

            Ufe::Scene::instance().addObserver(_observer);
        }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
        // Display layers maybe loaded before us, so make sure to track/cache them
        _usdStageDisplayLayersDirty = true;
        MFnDisplayLayerManager displayLayerManager(
            MFnDisplayLayerManager::currentDisplayLayerManager());
        auto layers = displayLayerManager.getAllDisplayLayers();
        for (unsigned int j = 0; j < layers.length(); ++j) {
            DisplayLayerAdded(layers[j], this);
            AddDisplayLayerToCache(layers[j]);
        }

        // Monitor display layers
        if (!_mayaDisplayLayerAddedCallbackId) {
            _mayaDisplayLayerAddedCallbackId
                = MDGMessage::addNodeAddedCallback(DisplayLayerAdded, "displayLayer", this);
        }
        if (!_mayaDisplayLayerRemovedCallbackId) {
            _mayaDisplayLayerRemovedCallbackId
                = MDGMessage::addNodeRemovedCallback(DisplayLayerRemoved, "displayLayer", this);
        }
        if (!_mayaDisplayLayerMembersCallbackId) {
            _mayaDisplayLayerMembersCallbackId
#ifdef MAYA_HAS_NEW_DISPLAY_LAYER_MESSAGING_API
                = MDisplayLayerMessage::addDisplayLayerMemberChangedCallback(
#else
                = MDisplayLayerMessage::addDisplayLayerMembersChangedCallback(
#endif
                    displayLayerMembershipChangedCB, this);
        }
#endif
        // Monitor color prefs.
        _mayaColorPrefsCallbackIds.push_back(
            MEventMessage::addEventCallback("ColorIndexChanged", colorPrefsChangedCB, this));
        _mayaColorPrefsCallbackIds.push_back(
            MEventMessage::addEventCallback("DisplayColorChanged", colorPrefsChangedCB, this));
        _mayaColorPrefsCallbackIds.push_back(
            MEventMessage::addEventCallback("DisplayRGBColorChanged", colorPrefsChangedCB, this));

        // Monitor color management prefs.
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtEnabledChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtWorkingSpaceChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtConfigChanged", colorManagementRefreshCB, this));
        _mayaColorManagementCallbackIds.push_back(MEventMessage::addEventCallback(
            "colorMgtConfigFilePathChanged", colorManagementRefreshCB, this));

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

void ProxyRenderDelegate::_PopulateCleanup()
{
    // Get rid of shaders no longer in use.
    HdVP2ShaderUniquePtr::cleanupDeadShaders();
}

InstancePrototypePath ProxyRenderDelegate::GetPathInPrototype(const SdfPath& id)
{
    HdInstancerContext instancerContext;
    auto               usdInstancePath = GetScenePrimPath(id, 0, &instancerContext);

    // In case of point instancer, we already have the path in prototype, return it.
    if (!instancerContext.empty()) {
        return InstancePrototypePath(usdInstancePath, kPointInstancing);
    }

    // In case of a native instance, obtain the path in prototype and return it.
    auto usdInstancePrim = _proxyShapeData->UsdStage()->GetPrimAtPath(usdInstancePath);
    auto usdPrototypePath = usdInstancePrim.GetPrimInPrototype().GetPath();
    return InstancePrototypePath(usdPrototypePath, kNativeInstancing);
}

void ProxyRenderDelegate::UpdateInstancingMapEntry(
    const InstancePrototypePath& oldPathInPrototype,
    const InstancePrototypePath& newPathInPrototype,
    const SdfPath&               rprimId)
{
    if (oldPathInPrototype != newPathInPrototype) {
        // remove the old entry from the map
        if (!oldPathInPrototype.first.IsEmpty()) {
            auto range = _instancingMap.equal_range(oldPathInPrototype);
            auto it = std::find(
                range.first,
                range.second,
                std::pair<const InstancePrototypePath, SdfPath>(oldPathInPrototype, rprimId));
            if (it != range.second) {
                _instancingMap.erase(it);
            }
        }

        // add new entry to the map
        if (!newPathInPrototype.first.IsEmpty()) {
            _instancingMap.insert(std::make_pair(newPathInPrototype, rprimId));
        }
    }
}

#ifdef MAYA_HAS_DISPLAY_LAYER_API
void ProxyRenderDelegate::_DirtyUsdSubtree(const UsdPrim& prim)
{
    if (!prim.IsValid())
        return;

    HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();

    auto markRprimDirty = [this, &changeTracker](const UsdPrim& prim) {
        constexpr HdDirtyBits dirtyBits = HdChangeTracker::DirtyVisibility
            | HdChangeTracker::DirtyRepr | HdChangeTracker::DirtyDisplayStyle
            | MayaUsdRPrim::DirtySelectionHighlight | MayaUsdRPrim::DirtyDisplayLayers
            | HdChangeTracker::DirtyMaterialId;

        if (prim.IsA<UsdGeomGprim>()) {
            auto range = _instancingMap.equal_range(
                InstancePrototypePath(prim.GetPath(), kPointInstancing));
            if (range.first != range.second) {
                // Point instancing prim
                for (auto it = range.first; it != range.second; ++it) {
                    if (_renderIndex->HasRprim(it->second)) {
                        changeTracker.MarkRprimDirty(it->second, dirtyBits);
                    }
                }
            } else if (prim.IsInstanceProxy()) {
                // Native instancing prim
                range = _instancingMap.equal_range(
                    InstancePrototypePath(prim.GetPrimInPrototype().GetPath(), kNativeInstancing));
                for (auto it = range.first; it != range.second; ++it) {
                    if (_renderIndex->HasRprim(it->second)) {
                        changeTracker.MarkRprimDirty(it->second, dirtyBits);
                    }
                }
            } else {
                // Non-instanced prim
                auto indexPath = _sceneDelegate->ConvertCachePathToIndexPath(prim.GetPath());
                if (_renderIndex->HasRprim(indexPath)) {
                    changeTracker.MarkRprimDirty(indexPath, dirtyBits);
                }
            }
        }
    };

    markRprimDirty(prim);
    auto range = prim.GetFilteredDescendants(UsdTraverseInstanceProxies());
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        markRprimDirty(iter->GetPrim());
    }
}

bool ProxyRenderDelegate::_DirtyUfeSubtree(const Ufe::Path& rootPath)
{
    Ufe::Path proxyShapePath = MayaUsd::ufe::stagePath(_proxyShapeData->UsdStage());
    if (rootPath.runTimeId() == MayaUsd::ufe::getUsdRunTimeId()) {
        if (rootPath.startsWith(proxyShapePath)) {
            _DirtyUsdSubtree(MayaUsd::ufe::ufePathToPrim(rootPath));
            return true;
        }
    } else if (rootPath.runTimeId() == MayaUsd::ufe::getMayaRunTimeId()) {
        if (proxyShapePath.startsWith(rootPath)) {
            _DirtyUsdSubtree(_proxyShapeData->UsdStage()->GetPseudoRoot());
            return true;
        }
    }

    return false;
}

bool _StringToUfePath(const MString& str, Ufe::Path& path)
{
    try {
        path = Ufe::PathString::path(str.asChar());
    } catch (const Ufe::InvalidPath&) {
        return false;
    } catch (const Ufe::InvalidPathComponentSeparator&) {
        return false;
    } catch (const Ufe::EmptyPathSegment&) {
        return false;
    }

    return true;
}

bool ProxyRenderDelegate::_DirtyUfeSubtree(const MString& rootStr)
{
    Ufe::Path rootPath;
    if (_StringToUfePath(rootStr, rootPath)) {
        return _DirtyUfeSubtree(rootPath);
    }

    return false;
}
#endif

void ProxyRenderDelegate::ComputeCombinedDisplayStyles(const unsigned int newDisplayStyle)
{
    // Add new display styles to the map
    if (newDisplayStyle & MHWRender::MFrameContext::kBoundingBox) {
        _combinedDisplayStyles[HdVP2ReprTokens->bbox] = _frameCounter;
    } else {
        if (newDisplayStyle & MHWRender::MFrameContext::kWireFrame) {
            _combinedDisplayStyles[HdReprTokens->wire] = _frameCounter;
        }

        if (newDisplayStyle & MHWRender::MFrameContext::kGouraudShaded) {
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
            if (newDisplayStyle & MHWRender::MFrameContext::kDefaultMaterial) {
                _combinedDisplayStyles[HdVP2ReprTokens->defaultMaterial] = _frameCounter;
            } else
#endif
                if (newDisplayStyle & MHWRender::MFrameContext::kTextured) {
                _combinedDisplayStyles[HdReprTokens->smoothHull] = _frameCounter;
            } else {
                _combinedDisplayStyles[HdVP2ReprTokens->smoothHullUntextured] = _frameCounter;
            }
        }
    }

    // Erase aged styles
    for (auto it = _combinedDisplayStyles.begin(); it != _combinedDisplayStyles.end();) {
        auto          curIt = it++;
        constexpr int numFramesToAge = 8;
        if (curIt->second + numFramesToAge < _frameCounter) {
            _combinedDisplayStyles.erase(curIt);
        }
    }

    // Erase excessive styles
    while (_combinedDisplayStyles.size() > HdReprSelector::MAX_TOPOLOGY_REPRS) {
        auto oldestIt = _combinedDisplayStyles.begin();
        auto curIt = _combinedDisplayStyles.begin();
        for (++curIt; curIt != _combinedDisplayStyles.end(); ++curIt) {
            if (oldestIt->second > curIt->second) {
                oldestIt = curIt;
            }
        }

        _combinedDisplayStyles.erase(oldestIt);
    }
}

//! \brief  Execute Hydra engine to perform minimal VP2 draw data update based on change tracker.
void ProxyRenderDelegate::_Execute(const MHWRender::MFrameContext& frameContext)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorC_L1, "Execute");

    ++_frameCounter;

    _refreshRequested = false;

    _UpdateRenderTags();

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    UpdateProxyShapeDisplayLayers();
#endif

    // If update for selection is enabled, the draw data for the "points" repr
    // won't be prepared until point snapping is activated; otherwise the draw
    // data have to be prepared early for possible activation of point snapping.
    const bool inSelectionPass = (frameContext.getSelectionInfo() != nullptr);
    const bool inPointSnapping = pointSnappingActive();

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

    HdReprSelector reprSelector;
    if (inSelectionPass) {
        // The new Maya point snapping support doesn't require point snapping items any more.
#if !defined(MAYA_NEW_POINT_SNAPPING_SUPPORT)
        if (inPointSnapping && !reprSelector.Contains(HdReprTokens->points)) {
            reprSelector = reprSelector.CompositeOver(kPointsReprSelector);
        }
#endif
    } else {
        ComputeCombinedDisplayStyles(frameContext.getDisplayStyle());

        // Update repr selector based on combined display styles
        TfToken reprNames[HdReprSelector::MAX_TOPOLOGY_REPRS];
        auto    it = _combinedDisplayStyles.begin();
        for (size_t j = 0;
             (it != _combinedDisplayStyles.end()) && (j < HdReprSelector::MAX_TOPOLOGY_REPRS);
             ++it, ++j) {
            reprNames[j] = it->first;
        }

        reprSelector = HdReprSelector(reprNames[0], reprNames[1], reprNames[2]);
    }

    // if there are no repr's to update then don't even call sync.
    if (reprSelector != HdReprSelector()) {
        HdDirtyBits dirtyBits = HdChangeTracker::Clean;

        // check to see if representation mode changed
        if (_defaultCollection->GetReprSelector() != reprSelector) {
            _defaultCollection->SetReprSelector(reprSelector);
            _taskController->SetCollection(*_defaultCollection);
            dirtyBits |= MayaUsdRPrim::DirtyDisplayMode;
        }

        if (_colorPrefsChanged) {
            _colorPrefsChanged = false;
            dirtyBits |= MayaUsdRPrim::DirtySelectionHighlight;
        }

#if MAYA_API_VERSION >= 20230200
        // check to see if the color space changed
        MString colorTransformId;
        frameContext.viewTransformName(colorTransformId);
        if (colorTransformId != _colorTransformId) {
            _colorTransformId = colorTransformId;
            dirtyBits |= MayaUsdRPrim::DirtySelectionHighlight;
        }
#endif

        // if switching to textured mode, we need to update materials
        const bool neededTexturedMaterials = _needTexturedMaterials;
        _needTexturedMaterials
            = _combinedDisplayStyles.find(HdReprTokens->smoothHull) != _combinedDisplayStyles.end();
        if (_needTexturedMaterials && !neededTexturedMaterials) {
            auto materials = _renderIndex->GetSprimSubtree(
                HdPrimTypeTokens->material, SdfPath::AbsoluteRootPath());
            for (auto material : materials) {
                changeTracker.MarkSprimDirty(material, HdMaterial::DirtyParams);
                // Tell all the Rprims associated with this material to recompute primvars
                HdVP2Material* vp2material = static_cast<HdVP2Material*>(
                    _renderIndex->GetSprim(HdPrimTypeTokens->material, material));
                vp2material->MaterialChanged(_sceneDelegate.get());
            }
        }

        if (dirtyBits != HdChangeTracker::Clean) {
            // Mark everything "dirty" so that sync is called on everything
            // If there are multiple views up with different viewport modes then
            // this is slow.
            auto& rprims = _renderIndex->GetRprimIds();
            for (auto path : rprims) {
                changeTracker.MarkRprimDirty(path, dirtyBits);
            }
        }

        _engine.Execute(_renderIndex.get(), &_dummyTasks);
    }
}

void ProxyRenderDelegate::setLongDurationRendering() { _longDurationRendering = true; }

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

    // If the rendering was flagged as possibly taking a long time,
    // show the wait cursor.
    //
    // Note: using the wait cursor sets the long duration flag,
    //       so reset the flag after setting up the cursor, otherwise
    //       once one rendering would be long-duration, all of them
    //       would be flagged afterward.
    UsdUfe::WaitCursor waitCursor(_longDurationRendering);
    _longDurationRendering = false;

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    const MSelectionInfo* selectionInfo = frameContext.getSelectionInfo();
    if (selectionInfo) {
        bool oldSnapToPoints = _snapToPoints;
        _snapToPoints = selectionInfo->pointSnapping();
        if (_snapToPoints != oldSnapToPoints) {
            _selectionModeChanged = true;
        }
    }

    MStatus status;
    if (selectionInfo) {
        bool oldSnapToSelectedObjects = _snapToSelectedObjects;
        _snapToSelectedObjects = selectionInfo->snapToActive(&status);
        if (status != MStatus::kSuccess) {
            TF_WARN("Could not snap selected object.");
        }
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
    _currentFrameContext = &frameContext;

    if (_Populate()) {
        _UpdateSceneDelegate();
        _Execute(frameContext);
        _PopulateCleanup();
    }

    _currentFrameContext = nullptr;
    param->EndUpdate();
}

MDagPath ProxyRenderDelegate::GetProxyShapeDagPath() const
{
    return _proxyShapeData->ProxyDagPath();
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
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 16
    // Can no longer pass ALL_INSTANCES as the instanceIndex
    SdfPath usdPath = (instanceIndex == UsdImagingDelegate::ALL_INSTANCES)
        ? rprimId.ReplacePrefix(_sceneDelegate->GetDelegateID(), SdfPath::AbsoluteRootPath())
        : _sceneDelegate->GetScenePrimPath(rprimId, instanceIndex, instancerContext);
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
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

static std::vector<int> fillInstanceIds(unsigned int instanceCount)
{
    std::vector<int> usdInstanceIds;
    usdInstanceIds.reserve(instanceCount);
    for (unsigned int usdInstanceId = 0; usdInstanceId < instanceCount; usdInstanceId++)
        usdInstanceIds.emplace_back(usdInstanceId);
    return usdInstanceIds;
}

SdfPathVector
ProxyRenderDelegate::GetScenePrimPaths(const SdfPath& rprimId, unsigned int instanceCount) const
{
    return GetScenePrimPaths(rprimId, fillInstanceIds(instanceCount));
}

SdfPathVector ProxyRenderDelegate::GetScenePrimPaths(
    const SdfPath&   rprimId,
    std::vector<int> instanceIndexes) const
{
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 17
    return _sceneDelegate->GetScenePrimPaths(rprimId, instanceIndexes);
#else
    SdfPathVector usdPaths;
    usdPaths.reserve(instanceIndexes.size());
    for (int instanceIndex : instanceIndexes) {
        usdPaths.emplace_back(GetScenePrimPath(rprimId, instanceIndex));
    }
    return usdPaths;
#endif
}

//! \brief  Selection for both instanced and non-instanced cases.
bool ProxyRenderDelegate::getInstancedSelectionPath(
    const MHWRender::MRenderItem&   renderItem,
    const MHWRender::MIntersection& intersection,
    MDagPath&                       dagPath) const
{
    // When point snapping, only the point position matters, so return the DAG path and avoid the
    // UFE global selection list to be updated.
    if (pointSnappingActive()) {
        dagPath = _proxyShapeData->ProxyDagPath();
        return true;
    }

    if (!_proxyShapeData->ProxyShape() || !_proxyShapeData->ProxyShape()->isUfeSelectionEnabled()) {
        return false;
    }

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
    SdfPath      usdPath = GetScenePrimPath(rprimId, instanceIndex);
#endif

    // If update for selection is enabled, we can query the Maya selection list
    // adjustment, USD selection kind, and USD point instances pick mode once
    // per selection update to avoid the cost of executing MEL commands or
    // searching optionVars for each intersection.
    const TfToken&                   selectionKind = _selectionKind;
    const UsdPointInstancesPickMode& pointInstancesPickMode = _pointInstancesPickMode;

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

    const Ufe::PathSegment pathSegment = UsdUfe::usdPathToUfePathSegment(usdPath, instanceIndex);
    auto si = Ufe::Hierarchy::createItem(_proxyShapeData->ProxyShape()->ufePath() + pathSegment);
    if (!si) {
        TF_WARN("Failed to create UFE scene item for Rprim '%s'", rprimId.GetText());
        return false;
    }

    auto ufeSel = Ufe::NamedSelection::get("MayaSelectTool");
    ufeSel->append(si);

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

#ifdef MAYA_HAS_DISPLAY_LAYER_API
void ProxyRenderDelegate::DisplayLayerAdded(MObject& node, void* clientData)
{
    ProxyRenderDelegate* me = static_cast<ProxyRenderDelegate*>(clientData);
    MObjectHandle        handle(node);
    if (me->_mayaDisplayLayerDirtyCallbackIds.count(handle) == 0) {
        me->_mayaDisplayLayerDirtyCallbackIds[handle]
            = MNodeMessage::addNodeDirtyCallback(node, displayLayerDirtyCB, me, nullptr);
    }
}

void ProxyRenderDelegate::DisplayLayerRemoved(MObject& node, void* clientData)
{
    ProxyRenderDelegate* me = static_cast<ProxyRenderDelegate*>(clientData);
    MObjectHandle        handle(node);
    auto                 iter = me->_mayaDisplayLayerDirtyCallbackIds.find(handle);
    if (iter != me->_mayaDisplayLayerDirtyCallbackIds.end()) {
        MMessage::removeCallback(iter->second);
        me->_mayaDisplayLayerDirtyCallbackIds.erase(iter);
    }
}

//! \brief  Notify of display layer membership change.
void ProxyRenderDelegate::DisplayLayerMembershipChanged(const MString& memberPath)
{
    Ufe::Path path;
    if (!_StringToUfePath(memberPath, path)) {
        return;
    }

    // First, update the caches
    Ufe::Path proxyShapePath = MayaUsd::ufe::stagePath(_proxyShapeData->UsdStage());
    if (path.runTimeId() == MayaUsd::ufe::getUsdRunTimeId()) {
        if (path.startsWith(proxyShapePath) && path.nbSegments() > 1) {
            MFnDisplayLayerManager displayLayerManager(
                MFnDisplayLayerManager::currentDisplayLayerManager());

            SdfPath usdPath(path.getSegments()[1].string());
            MObject displayLayerObj = displayLayerManager.getLayer(memberPath);
            if (displayLayerObj.hasFn(MFn::kDisplayLayer)
                && MFnDisplayLayer(displayLayerObj).name() != "defaultLayer") {
                _usdPathToDisplayLayerMap[usdPath] = displayLayerObj;
            } else {
                _usdPathToDisplayLayerMap.erase(usdPath);
            }
        }
    } else if (path.runTimeId() == MayaUsd::ufe::getMayaRunTimeId()) {
        if (proxyShapePath.startsWith(path)) {
            _usdStageDisplayLayersDirty = true;
        }
    }

    // Then, dirty the subtree
    if (_DirtyUfeSubtree(path)) {
        _RequestRefresh();
    }
}

void ProxyRenderDelegate::DisplayLayerDirty(MFnDisplayLayer& displayLayer)
{
    MSelectionList members;
    displayLayer.getMembers(members);

    bool subtreeDirtied = false;
    auto membersCount = members.length();
    for (unsigned int j = 0; j < membersCount; ++j) {
        MDagPath dagPath;
        if (members.getDagPath(j, dagPath) == MS::kSuccess) {
            subtreeDirtied |= _DirtyUfeSubtree(MayaUsd::ufe::dagPathToUfe(dagPath));
        } else {
            MStringArray selectionStrings;
            members.getSelectionStrings(j, selectionStrings);
            for (auto it = selectionStrings.begin(); it != selectionStrings.end(); ++it) {
                subtreeDirtied |= _DirtyUfeSubtree(*it);
            }
        }
    }

    if (subtreeDirtied) {
        _RequestRefresh();
    }
}

void ProxyRenderDelegate::DisplayLayerPathChanged(
    const Ufe::Path& oldPath,
    const Ufe::Path& newPath)
{
    Ufe::Path proxyShapePath = MayaUsd::ufe::stagePath(_proxyShapeData->UsdStage());
    if (oldPath.runTimeId() == MayaUsd::ufe::getUsdRunTimeId()) {
        if (oldPath.startsWith(proxyShapePath) && oldPath.nbSegments() > 1
            && newPath.nbSegments() > 1) {
            std::vector<std::pair<SdfPath, MObject>> pathsToUpdate;
            SdfPath oldUsdPrefix(oldPath.getSegments()[1].string());
            auto    it = _usdPathToDisplayLayerMap.lower_bound(oldUsdPrefix);
            while (it != _usdPathToDisplayLayerMap.end() && it->first.HasPrefix(oldUsdPrefix)) {
                pathsToUpdate.push_back(*it);
                _usdPathToDisplayLayerMap.erase(it++);
            }

            SdfPath newUsdPrefix(newPath.getSegments()[1].string());
            for (const auto& item : pathsToUpdate) {
                SdfPath newItemPath = item.first.ReplacePrefix(oldUsdPrefix, newUsdPrefix);
                _usdPathToDisplayLayerMap[newItemPath] = item.second;
            }
        }
    } else if (oldPath.runTimeId() == MayaUsd::ufe::getMayaRunTimeId()) {
        // Check both paths since we don't know if the proxy shape path has already been updated
        if (proxyShapePath.startsWith(oldPath) || proxyShapePath.startsWith(newPath)) {
            _usdStageDisplayLayersDirty = true;
        }
    }

    // Try to dirty both paths since we don't know if the proxy shape path has already been updated
    if (_DirtyUfeSubtree(oldPath) || _DirtyUfeSubtree(newPath)) {
        _RequestRefresh();
    }
}

void ProxyRenderDelegate::AddDisplayLayerToCache(MObject& displayLayerObj)
{
    if (!displayLayerObj.hasFn(MFn::kDisplayLayer)) {
        return;
    }

    MFnDisplayLayer displayLayer(displayLayerObj);
    if (displayLayer.name() == "defaultLayer") {
        return;
    }

    MSelectionList members;
    displayLayer.getMembers(members);
    auto      membersCount = members.length();
    Ufe::Path proxyShapePath = MayaUsd::ufe::stagePath(_proxyShapeData->UsdStage());
    for (unsigned int j = 0; j < membersCount; ++j) {
        // skip maya paths, as they will be updated from UpdateProxyShapeDisplayLayers
        MDagPath dagPath;
        if (members.getDagPath(j, dagPath) == MS::kSuccess) {
            continue;
        }

        // add usd paths
        MStringArray selectionStrings;
        members.getSelectionStrings(j, selectionStrings);
        for (auto it = selectionStrings.begin(); it != selectionStrings.end(); ++it) {
            Ufe::Path path;
            if (!_StringToUfePath(*it, path)) {
                continue;
            }

            if (!path.startsWith(proxyShapePath) || (path.nbSegments() < 2)) {
                continue;
            }

            SdfPath usdPath(path.getSegments()[1].string());
            _usdPathToDisplayLayerMap[usdPath] = displayLayerObj;
        }
    }
}

void ProxyRenderDelegate::UpdateProxyShapeDisplayLayers()
{
    if (!_usdStageDisplayLayersDirty) {
        return;
    }

    _usdStageDisplayLayersDirty = false;
    MFnDisplayLayerManager displayLayerManager(
        MFnDisplayLayerManager::currentDisplayLayerManager());

    _usdStageDisplayLayers
        = displayLayerManager.getAncestorLayersInclusive(GetProxyShapeDagPath().fullPathName());
}

MObject ProxyRenderDelegate::GetDisplayLayer(const SdfPath& path)
{
    auto it = _usdPathToDisplayLayerMap.find(path);
    return it != _usdPathToDisplayLayerMap.end() ? it->second : MObject();
}
#endif

void ProxyRenderDelegate::_RequestRefresh()
{
    if (!_refreshRequested)
        M3dView::scheduleRefreshAllViews();
    _refreshRequested = true;
}

void ProxyRenderDelegate::ColorPrefsChanged()
{
    _colorPrefsChanged = true;
    _RequestRefresh();
}

void ProxyRenderDelegate::ColorManagementRefresh()
{
    // Need to resync all color management aware materials
    HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();
    auto             materials
        = _renderIndex->GetSprimSubtree(HdPrimTypeTokens->material, SdfPath::AbsoluteRootPath());
    for (auto material : materials) {
        changeTracker.MarkSprimDirty(material, HdMaterial::DirtyParams);
    }

    _RequestRefresh();
}

//! \brief  Populate lead and active selection for Rprims under the proxy shape.
void ProxyRenderDelegate::_PopulateSelection()
{
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
    if (rprimRenderTagChanged) {
        for (auto path : _renderIndex->GetRprimIds()) {
            if (changeTracker.GetRprimDirtyBits(path) & HdChangeTracker::DirtyRenderTag) {
                // Since USD 23.02, DirtyRenderTag is not enough to provoke a sync,
                // so we add an extra dirty flag - DirtyVisibility
                changeTracker.MarkRprimDirty(path, HdChangeTracker::DirtyVisibility);
            }
        }
    }

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
            // Since USD 23.02, DirtyRenderTag is not enough to provoke a sync,
            // so we add an extra dirty flag - DirtyVisibility
            changeTracker.MarkRprimDirty(
                id, HdChangeTracker::DirtyRenderTag | HdChangeTracker::DirtyVisibility);
        }
    }

    // When the render tag on an rprim changes we do a pass over all rprims to update
    // their visibility. The frame after we do the pass over all the tags, set the tags back to
    // the minimum set of tags.
    if (anyPurposeChanged || rprimRenderTagChanged || !_taskRenderTagsValid) {
        TfTokenVector renderTags
            = { HdRenderTagTokens->geometry }; // always draw geometry render tag purpose.
        if (_proxyShapeData->DrawRenderPurpose() || renderPurposeChanged || rprimRenderTagChanged) {
            renderTags.push_back(HdRenderTagTokens->render);
        }
        if (_proxyShapeData->DrawProxyPurpose() || proxyPurposeChanged || rprimRenderTagChanged) {
            renderTags.push_back(HdRenderTagTokens->proxy);
        }
        if (_proxyShapeData->DrawGuidePurpose() || guidePurposeChanged || rprimRenderTagChanged) {
            renderTags.push_back(HdRenderTagTokens->guide);
        }
        _taskController->SetRenderTags(renderTags);
        // if the changedRenderTags is not empty then we could have some tags
        // in the _taskController just so that we get one sync to hide the render
        // items. In that case we need to leave _taskRenderTagsValid false, so that
        // we get a chance to remove that tag next frame.
        _taskRenderTagsValid = !(anyPurposeChanged || rprimRenderTagChanged);
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
MColor ProxyRenderDelegate::GetWireframeColor()
{
    static const MColor defaultColor(0.f, 0.f, 0.f);
#if MAYA_API_VERSION < 20230000
    return defaultColor;
#else
    return _GetDisplayColor(_wireframeColorCache, "polymeshDormant", false, defaultColor);
#endif
}

GfVec3f ProxyRenderDelegate::GetDefaultColor(const TfToken& className)
{
    static const GfVec3f kDefaultColor(0.000f, 0.016f, 0.376f);

    // Prepare to construct the query command.
    const char*   queryName = "unsupported";
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

MColor ProxyRenderDelegate::GetTemplateColor(bool active)
{
    MColorCache& colorCache = active ? _activeTemplateColorCache : _dormantTemplateColorCache;
    const char*  colorName = active ? "templateActive" : "templateDormant";
    static const MColor defaultColor(0.5f, 0.5f, 0.5f);

    return _GetDisplayColor(colorCache, colorName, active, defaultColor);
}

MColor ProxyRenderDelegate::GetReferenceColor()
{
    static const MColor defaultColor(0.f, 0.f, 0.f);
    return _GetDisplayColor(_referenceColorCache, "referenceLayer", true, defaultColor);
}

MColor ProxyRenderDelegate::_GetDisplayColor(
    MColorCache&  colorCache,
    const char*   colorName,
    bool          colorCorrection,
    const MColor& defaultColor)
{
    // Check the cache. It is safe since colorCache.second is atomic
    if (colorCache.second == _frameCounter) {
        return colorCache.first;
    }

    // Enter the mutex and check the cache again
    std::lock_guard<std::mutex> mutexGuard(_mayaCommandEngineMutex);
    if (colorCache.second == _frameCounter) {
        return colorCache.first;
    }

    // Construct the query command string.
    MString queryCommand;
    queryCommand = "displayRGBColor -q \"";
    queryCommand += colorName;
    queryCommand += "\"";

    // Query and return the display color.
    MDoubleArray colorResult;
    MGlobal::executeCommand(queryCommand, colorResult);

    if (colorResult.length() == 3) {
        colorCache.first = MColor(colorResult[0], colorResult[1], colorResult[2]);
#if MAYA_API_VERSION >= 20230200
        if (colorCorrection && _currentFrameContext) {
            colorCache.first = _currentFrameContext->applyViewTransform(
                colorCache.first, MFrameContext::kInverse);
        }
#endif
    } else {
        TF_WARN("Failed to obtain display color %s.", colorName);

        // In case of any failure, return the default color
        colorCache.first = defaultColor;
    }

    // Update the cache and return
    colorCache.second = _frameCounter;
    return colorCache.first;
}

//! \brief
MColor ProxyRenderDelegate::GetSelectionHighlightColor(const TfToken& className)
{
    static const MColor kDefaultLeadColor(0.056f, 1.0f, 0.366f, 1.0f);
    static const MColor kDefaultActiveColor(1.0f, 1.0f, 1.0f, 1.0f);
#if MAYA_API_VERSION < 20230000
    // Taken from v0.14.0 of mayaUSD plugin
    // https://github.com/Autodesk/maya-usd/blob/69e465032c423f4559bfef75c77bca0836366950/lib/mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.cpp#L1433
    return className.IsEmpty() ? kDefaultLeadColor : kDefaultActiveColor;
#else
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
#if MAYA_API_VERSION >= 20230000
        fromPalette = false;
        queryName = "polymeshActive";
#else
        queryName = "polymesh";
#endif
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
#endif // MAYA_API_VERSION < 20230000
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
