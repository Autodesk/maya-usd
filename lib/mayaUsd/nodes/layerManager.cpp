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
#include "layerManager.h"

#include <mayaUsd/commands/abstractLayerEditorWindow.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItemMuting.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <usdUfe/utils/layers.h>

#include <pxr/base/arch/env.h>
#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usdUtils/authoring.h>

#if PXR_VERSION < 2508
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#else
#include <pxr/usd/sdf/usdFileFormat.h>
#include <pxr/usd/sdf/usdaFileFormat.h>
#include <pxr/usd/sdf/usdcFileFormat.h>
#endif

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFileIO.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSceneMessage.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/selectionNotification.h>

#include <cstdio>
#include <iostream>
#include <set>

namespace {
static std::recursive_mutex findNodeMutex;
static MObjectHandle        layerManagerHandle;

// Utility func to disconnect an array plug, and all it's element plugs, and all
// their child plugs.
// Not in Utils, because it's not generic - ie, doesn't handle general case
// where compound/array plugs may be nested arbitrarily deep...
MStatus disconnectCompoundArrayPlug(MPlug arrayPlug)
{
    MStatus      status;
    MPlug        elemPlug;
    MPlug        srcPlug;
    MPlugArray   destPlugs;
    MDGModifier& dgmod = MayaUsd::MDGModifierUndoItem::create("Compound array plug disconnection");

    auto disconnectPlug = [&](MPlug plug) -> MStatus {
        MStatus status;
        srcPlug = plug.source(&status);
        if (!srcPlug.isNull()) {
            dgmod.disconnect(srcPlug, plug);
        }
        destPlugs.clear();
        plug.destinations(destPlugs, &status);
        for (size_t i = 0; i < destPlugs.length(); ++i) {
            dgmod.disconnect(plug, destPlugs[i]);
        }
        return status;
    };

    // Considered using numConnectedElements, but for arrays-of-compound attributes, not sure if
    // this will also detect connections to a child-of-an-element... so just iterating through all
    // plugs. Shouldn't be too many...
    const size_t numElements = arrayPlug.evaluateNumElements();
    // Iterate over all elements...
    for (size_t elemI = 0; elemI < numElements; ++elemI) {
        elemPlug = arrayPlug.elementByPhysicalIndex(elemI, &status);

        // Disconnect the element compound attribute
        disconnectPlug(elemPlug);

        // ...then disconnect any children
        if (elemPlug.numConnectedChildren() > 0) {
            for (size_t childI = 0; childI < elemPlug.numChildren(); ++childI) {
                disconnectPlug(elemPlug.child(childI));
            }
        }
    }
    return dgmod.doIt();
}

/// @brief Verify if the given node is from a reference.
static bool
isNodeFromDesiredOrigin(const MFnDependencyNode& node, MayaUsdProxyShapeBase* forProxyShape)
{
    const bool proxyIsFromReference
        = (forProxyShape && MFnDependencyNode(forProxyShape->thisMObject()).isFromReferencedFile());
    return node.isFromReferencedFile() == proxyIsFromReference;
}

MayaUsd::LayerManager* findNode(MayaUsdProxyShapeBase* forProxyShape)
{
    if (forProxyShape) {
        if (MayaUsd::LayerManager* layerManager = forProxyShape->getLayerManager()) {
            return layerManager;
        }
    }

    // Check for cached layer manager before searching
    MFnDependencyNode fn;
    if (layerManagerHandle.isValid() && layerManagerHandle.isAlive()) {
        MObject mobj { layerManagerHandle.object() };
        if (!mobj.isNull()) {
            fn.setObject(mobj);
            return static_cast<MayaUsd::LayerManager*>(fn.userNode());
        }
    }

    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (fn.typeId() == MayaUsd::LayerManager::typeId) {
            if (isNodeFromDesiredOrigin(fn, forProxyShape)) {
                layerManagerHandle = mobj;
                return static_cast<MayaUsd::LayerManager*>(fn.userNode());
            }
        }
    }
    return nullptr;
}

MayaUsd::LayerManager* findOrCreateNode(MayaUsdProxyShapeBase* forProxyShape)
{
    MayaUsd::LayerManager* lm = findNode(forProxyShape);
    if (!lm) {
        MDGModifier& modifier = MayaUsd::MDGModifierUndoItem::create("Node find or creation");
        MObject      manager = modifier.createNode(MayaUsd::LayerManager::typeId);
        modifier.doIt();

        lm = static_cast<MayaUsd::LayerManager*>(MFnDependencyNode(manager).userNode());
    }

    return lm;
}

void convertAnonymousLayersRecursive(
    SdfLayerRefPtr     layer,
    const std::string& basename,
    UsdStageRefPtr     stage)
{
    auto currentTarget = stage->GetEditTarget().GetLayer();

    std::vector<std::string> sublayers = layer->GetSubLayerPaths();
    for (size_t i = 0, n = sublayers.size(); i < n; ++i) {
        SdfLayerRefPtr subL = layer->Find(sublayers[i]);
        if (subL) {
            convertAnonymousLayersRecursive(subL, basename, stage);

            if (subL->IsAnonymous()) {
                MayaUsd::utils::LayerParent subLayerParent;
                subLayerParent._layerParent = layer;
                subLayerParent._proxyPath = basename;

                SdfLayerRefPtr newLayer
                    = MayaUsd::utils::saveAnonymousLayer(stage, subL, subLayerParent, basename);
                if (subL == currentTarget) {
                    stage->SetEditTarget(newLayer);
                }
            }
        }
    }
}

bool isCrashing()
{
#ifdef MAYA_HAS_CRASH_DETECTION
    return MGlobal::isInCrashHandler();
#else
    return false;
#endif
}

bool isCopyingSceneNodes()
{
    // When Maya is copy nodes, it exports them and sets this environment
    // variable during the export to let exporters know it is cutting or
    // copying nodes in a temporary Maya scene file.
    return PXR_NS::ArchHasEnv("MAYA_CUT_COPY_EXPORT");
}

constexpr auto kSaveOptionUICmd = "usdFileSaveOptions(true);";

} // namespace

namespace MAYAUSD_NS_DEF {

class LayerDatabase : public TfWeakBase
{
public:
    LayerDatabase();
    ~LayerDatabase();

    static LayerDatabase& instance();
    static void           setBatchSaveDelegate(BatchSaveDelegate delegate);
    static void           prepareForSaveCheck(bool*, void*);
    static void           cleanupForSave(void*);
    static void           prepareForExportCheck(bool*, void*);
    static void           prepareForWriteCheck(bool*, bool);
    static void           cleanupForWrite();
    static void           loadLayersPostRead(MayaUsdProxyShapeBase* forProxyShape);
    static void           cleanUpNewScene(void*);
    static void           clearManagerNode(MayaUsd::LayerManager* lm);
    static void           removeManagerNode(
                  MayaUsd::LayerManager* lm = nullptr,
                  MayaUsdProxyShapeBase* forProxyShape = nullptr);

    bool getProxiesToSave(bool isExport, bool* hasAnyProxy);
    bool saveInteractionRequired();
    bool supportedNodeType(MTypeId type);
    void addSupportForNodeType(MTypeId type);
    void removeSupportForNodeType(MTypeId type);
    bool remapSubLayerPaths(SdfLayerHandle parentLayer);
    bool addLayer(SdfLayerRefPtr layer, const std::string& identifier = std::string());
    bool removeLayer(SdfLayerRefPtr layer);
    void removeAllLayers();

    void        setSelectedStage(const std::string& stage);
    std::string getSelectedStage() const;

    bool saveLayerManagerSelectedStage();
    bool loadLayerManagerSelectedStage(MayaUsd::LayerManager& layerManager);

    SdfLayerHandle findLayer(std::string identifier) const;

    LayerManager::LayerNameMap getLayerNameMap() const;

    static bool isSaving() { return _isSavingMayaFile; }

private:
    void registerCallbacks();
    void unregisterCallbacks();

    void _addLayer(SdfLayerRefPtr layer, const std::string& identifier);
    void onStageSet(const MayaUsdProxyStageSetNotice& notice);

    bool            saveUsd(bool isExport);
    BatchSaveResult saveUsdToMayaFile();
    BatchSaveResult saveUsdToUsdFiles();
    void            convertAnonymousLayers(
                   MayaUsdProxyShapeBase* pShape,
                   const MObject&         proxyNode,
                   UsdStageRefPtr         stage);
    void saveUsdLayerToMayaFile(SdfLayerRefPtr layer, bool asAnonymous);
    void clearProxies();
    bool hasDirtyLayer() const;
    void refreshProxiesToSave();
    void updateLayerManagers();

    std::map<std::string, SdfLayerRefPtr> _idToLayer;
    TfNotice::Key                         _onStageSetKey;
    std::set<unsigned int>                _supportedTypes;
    std::vector<StageSavingInfo>          _proxiesToSave;
    std::vector<StageSavingInfo>          _internalProxiesToSave;
    std::string                           _selectedStage;
    static MCallbackId                    preSaveCallbackId;
    static MCallbackId                    postSaveCallbackId;
    static MCallbackId                    preExportCallbackId;
    static MCallbackId                    postExportCallbackId;
    static MCallbackId                    postNewCallbackId;
    static MCallbackId                    preOpenCallbackId;

    static MayaUsd::BatchSaveDelegate _batchSaveDelegate;

    static bool _isSavingMayaFile;
};

MCallbackId LayerDatabase::preSaveCallbackId = 0;
MCallbackId LayerDatabase::postSaveCallbackId = 0;
MCallbackId LayerDatabase::preExportCallbackId = 0;
MCallbackId LayerDatabase::postExportCallbackId = 0;
MCallbackId LayerDatabase::postNewCallbackId = 0;
MCallbackId LayerDatabase::preOpenCallbackId = 0;

MayaUsd::BatchSaveDelegate LayerDatabase::_batchSaveDelegate = nullptr;

bool LayerDatabase::_isSavingMayaFile = false;

/*static*/
LayerDatabase& LayerDatabase::instance()
{
    static LayerDatabase sLayerDB;
    sLayerDB.registerCallbacks();
    return sLayerDB;
}

LayerDatabase::LayerDatabase()
{
    TfWeakPtr<LayerDatabase> me(this);
    _onStageSetKey = TfNotice::Register(me, &LayerDatabase::onStageSet);
}

LayerDatabase::~LayerDatabase()
{
    if (_onStageSetKey.IsValid()) {
        TfNotice::Revoke(_onStageSetKey);
    }

    unregisterCallbacks();
}

void LayerDatabase::registerCallbacks()
{
    if (0 == preSaveCallbackId) {
        preSaveCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeSaveCheck, LayerDatabase::prepareForSaveCheck);
        postSaveCallbackId
            = MSceneMessage::addCallback(MSceneMessage::kAfterSave, LayerDatabase::cleanupForSave);
        preExportCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeExportCheck, LayerDatabase::prepareForExportCheck);
        postNewCallbackId
            = MSceneMessage::addCallback(MSceneMessage::kAfterNew, LayerDatabase::cleanUpNewScene);
        preOpenCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeOpen, LayerDatabase::cleanUpNewScene);
    }
}

void LayerDatabase::unregisterCallbacks()
{
    if (0 != preSaveCallbackId) {
        MSceneMessage::removeCallback(preSaveCallbackId);
        MSceneMessage::removeCallback(postSaveCallbackId);
        MSceneMessage::removeCallback(preExportCallbackId);
        MSceneMessage::removeCallback(postExportCallbackId);
        MSceneMessage::removeCallback(postNewCallbackId);
        MSceneMessage::removeCallback(preOpenCallbackId);

        preSaveCallbackId = 0;
        postSaveCallbackId = 0;
        preExportCallbackId = 0;
        postExportCallbackId = 0;
        postNewCallbackId = 0;
        preOpenCallbackId = 0;
    }
}

void LayerDatabase::addSupportForNodeType(MTypeId type)
{
    if (_supportedTypes.find(type.id()) == _supportedTypes.end()) {
        _supportedTypes.insert(type.id());
    }
}

void LayerDatabase::removeSupportForNodeType(MTypeId type) { _supportedTypes.erase(type.id()); }

bool LayerDatabase::supportedNodeType(MTypeId type)
{
    return (_supportedTypes.find(type.id()) != _supportedTypes.end());
}

void LayerDatabase::onStageSet(const MayaUsdProxyStageSetNotice& notice)
{
    const MayaUsdProxyShapeBase& psb = notice.GetProxyShape();
    UsdStageRefPtr               stage = psb.getUsdStage();
    if (stage) {
        removeLayer(stage->GetRootLayer());
        removeLayer(stage->GetSessionLayer());
    }
}

void LayerDatabase::setBatchSaveDelegate(BatchSaveDelegate delegate)
{
    _batchSaveDelegate = delegate;
}

void LayerDatabase::prepareForSaveCheck(bool* retCode, void*)
{
    // This is called during a Maya notification callback, so no undo supported.
    OpUndoItemMuting muting;
    prepareForWriteCheck(retCode, false);
}

void LayerDatabase::cleanupForSave(void*)
{
    // This is call by Maya when the Maya save has finished.
    cleanupForWrite();
}

namespace {

MString formatProxyShapeWarning(const char* message, const StageSavingInfo& info)
{
    MString text;
    text.format(message, info.dagPath.partialPathName());
    return text;
}

// Handle a dirty stage during export as USD.
void handleDirtyStageDuringExport(const StageSavingInfo& info)
{
    if (!info.stage)
        return;

    const UsdUfe::StageDirtyState dirty = UsdUfe::isStageDirty(*info.stage);
    if (dirty == UsdUfe::StageDirtyState::kClean)
        return;

    if (info.stage->GetRootLayer()->IsAnonymous()) {
        MGlobal::displayWarning(formatProxyShapeWarning(
            "A reference to ^1s could not be exported because the root layer is anonymous."
            " To include this stage, you will need to save the anonymous root layer to disk"
            " and re-export the scene.",
            info));
        return;
    }

    if (dirty == UsdUfe::StageDirtyState::kDirtyRootLayers) {
        MGlobal::displayWarning(formatProxyShapeWarning(
            "^1s may not appear in the exported scene exactly as it appears in the scene"
            " because there are layers that have not been saved to disk. Saving those"
            " layers in the layer editor may be needed.",
            info));
        return;
    }

    if (dirty == UsdUfe::StageDirtyState::kDirtySessionLayers) {
        MGlobal::displayWarning(formatProxyShapeWarning(
            "^1s may not appear in the exported scene exactly as it appears in the scene"
            " because there are opinions in the session layer which are not propagated"
            " into the USD files.",
            info));
        return;
    }
}

} // namespace

void LayerDatabase::prepareForExportCheck(bool* retCode, void*)
{
    *retCode = true;

    // This is called during a Maya notification callback, so no undo supported.
    OpUndoItemMuting muting;

    auto& layerDB = LayerDatabase::instance();

    bool       hasAnyProxy = false;
    const bool isExport = true;
    if (!layerDB.getProxiesToSave(isExport, &hasAnyProxy))
        return;

    for (const StageSavingInfo& info : layerDB._proxiesToSave)
        handleDirtyStageDuringExport(info);

    for (const auto& info : layerDB._internalProxiesToSave)
        handleDirtyStageDuringExport(info);

    layerDB.clearProxies();
}

void LayerDatabase::prepareForWriteCheck(bool* retCode, bool isExport)
{
    _isSavingMayaFile = true;
    cleanUpNewScene(nullptr);

    LayerDatabase::instance().saveLayerManagerSelectedStage();

    bool hasAnyProxy = false;
    if (LayerDatabase::instance().getProxiesToSave(isExport, &hasAnyProxy)) {

        int dialogResult = true;

        if (!isCopyingSceneNodes()) {
            if (MGlobal::kInteractive == MGlobal::mayaState() && !isCrashing()
                && LayerDatabase::instance().saveInteractionRequired()) {
                MGlobal::executeCommand(kSaveOptionUICmd, dialogResult);
            }
        }

        if (dialogResult) {
            dialogResult = LayerDatabase::instance().saveUsd(isExport);
        }

        *retCode = dialogResult;
    } else {
        *retCode = true;
    }

    // Note: for now we only save USD change made in stage in the main
    //       Maya scene. We don't save changes made to stages in Maya
    //       references.
    if (!hasAnyProxy)
        removeManagerNode(nullptr, nullptr);
}

void LayerDatabase::cleanupForWrite()
{
    // Reset the flag that records a Maya scene save is in progress.
    // Used to avoid deleting the layer manager node mid-save if some
    // other code happens to access the layers.
    _isSavingMayaFile = false;
}

void LayerDatabase::clearProxies()
{
    _proxiesToSave.clear();
    _internalProxiesToSave.clear();
}

void LayerDatabase::updateLayerManagers()
{
    auto creator = MayaUsd::AbstractLayerEditorCreator::instance();
    if (!creator)
        return;

    for (const std::string& panelName : creator->getAllPanelNames()) {
        AbstractLayerEditorWindow* window = creator->getWindow(panelName.c_str());
        if (!window)
            continue;

        window->updateLayerModel();
    }
}

bool LayerDatabase::hasDirtyLayer() const
{
    for (const auto& info : _proxiesToSave) {
        const UsdUfe::StageDirtyState dirty = UsdUfe::isStageDirty(*info.stage);
        if (dirty != UsdUfe::StageDirtyState::kClean) {
            return true;
        }
    }
    for (const auto& info : _internalProxiesToSave) {
        const UsdUfe::StageDirtyState dirty = UsdUfe::isStageDirty(*info.stage);
        if (dirty != UsdUfe::StageDirtyState::kClean) {
            return true;
        }
    }
    return false;
}

bool LayerDatabase::getProxiesToSave(bool isExport, bool* hasAnyProxy)
{
    if (hasAnyProxy)
        *hasAnyProxy = false;

    bool checkSelection = isExport && (MFileIO::kExportTypeSelected == MFileIO::exportType());
    const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();

    clearProxies();

    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (!fn.isFromReferencedFile() && supportedNodeType(fn.typeId())) {
            if (hasAnyProxy)
                *hasAnyProxy = true;

            MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
            UsdStageRefPtr         stage = pShape ? pShape->getUsdStage() : nullptr;
            if (!stage) {
                continue;
            }

            auto stagePath = MayaUsd::ufe::stagePath(stage);
            if (!checkSelection
                || (ufeSelection->contains(stagePath)
                    || ufeSelection->containsAncestor(stagePath))) {
                // Should we save the stage?
                // 1) Shareable Stage : In this case we case we only care about saving if the
                // input is not an incoming connection, since in that case the node that "owns" the
                // stage (upstream node) is responsible for saving. For example if you have multiple
                // proxy shapes daisy chained one's out_stage feeding the other's in_stage. In that
                // case only one proxy is responsible for saving (the first one).
                // 2) Unshareable Stage: Similarly but in this case if the stage is unshared, it
                // means we are responsible for saving the root layer (and stubs for the sublayers
                // so we can put them back in the same spot). So doesn't matter if its incoming or
                // not, we need to save.
                if (!pShape->isShareableStage() || !pShape->isStageIncoming()) {
                    SdfLayerHandleVector allLayers = stage->GetUsedLayers(true);
                    for (auto layer : allLayers) {
                        if (TF_VERIFY(layer) && layer->IsDirty()) {
                            StageSavingInfo info;
                            MDagPath::getAPathTo(mobj, info.dagPath);
                            info.stage = stage;
                            info.shareable = pShape->isShareableStage();
                            info.isIncoming = pShape->isStageIncoming();

                            // Where should we save the stage?
                            // We handle unshared composition internally in Maya USD file
                            // The reason we have this distinction now is that some Layers are
                            // special case layers that should be saved to the maya file only.
                            // There are two examples currently of this, the session layer and
                            // unshared root layer. So since we have batchSave and a delegate which
                            // handles saving externally, we need to manage some proxies ourselves
                            // and control where they save
                            if (pShape->isShareableStage()) {
                                _proxiesToSave.emplace_back(std::move(info));
                            } else {
                                _internalProxiesToSave.emplace_back(std::move(info));
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    return ((_proxiesToSave.size() + _internalProxiesToSave.size()) > 0);
}

bool LayerDatabase::saveInteractionRequired() { return _proxiesToSave.size() > 0; }

static void refreshSavingInfo(StageSavingInfo& info)
{
    MFnDependencyNode fn;
    MObject           mobj = info.dagPath.node();
    fn.setObject(mobj);
    if (!fn.isFromReferencedFile() && LayerDatabase::instance().supportedNodeType(fn.typeId())) {
        MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
        info.stage = pShape ? pShape->getUsdStage() : nullptr;
    }
}

void LayerDatabase::refreshProxiesToSave()
{
    for (StageSavingInfo& info : _proxiesToSave) {
        refreshSavingInfo(info);
    }
    for (StageSavingInfo& info : _internalProxiesToSave) {
        refreshSavingInfo(info);
    }
}

void LayerDatabase::setSelectedStage(const std::string& stage)
{
    if (_selectedStage == stage)
        return;

    _selectedStage = stage;
    // Mark the scene as modified.
    MGlobal::executeCommand("file -modified 1");
}

std::string LayerDatabase::getSelectedStage() const { return _selectedStage; }

bool LayerDatabase::saveLayerManagerSelectedStage()
{
    // Note: for now we only save USD change made in stage in the main
    //       Maya scene. We don't save changes made to stages in Maya
    //       references.
    MayaUsd::LayerManager* lm = findOrCreateNode(nullptr);
    if (!lm)
        return false;

    MStatus     status;
    MDataBlock  dataBlock = lm->_forceCache();
    MDataHandle selectedStageHandle = dataBlock.outputValue(lm->selectedStage, &status);
    if (!status)
        return false;

    // Note: when empty, we clear the the selected stage attribute so that the
    //       attribute does not get written to the scene, which improve backward
    //       compatibility.
    const std::string stageName = getSelectedStage();
    if (stageName.size() > 0)
        selectedStageHandle.setString(stageName.c_str());
    else
        selectedStageHandle.setMObject(MObject::kNullObj);

    selectedStageHandle.setClean();
    dataBlock.setClean(lm->selectedStage);

    return true;
}

bool LayerDatabase::loadLayerManagerSelectedStage(MayaUsd::LayerManager& layerManager)
{
    MStatus status;

    MPlug selectedStagePlug(layerManager.thisMObject(), layerManager.selectedStage);
    setSelectedStage(selectedStagePlug.asString(MDGContext::fsNormal, &status).asChar());

    return status;
}

bool LayerDatabase::saveUsd(bool isExport)
{
    BatchSaveResult result = MayaUsd::kNotHandled;

    auto opt = MayaUsd::utils::serializeUsdEditsLocationOption();

    if (MayaUsd::utils::kIgnoreUSDEdits != opt) {
        // When Maya is crashing or copying/cutting scene nodes, we don't want to
        // save the the USD file to avoid overwriting them with possibly unwanted
        // data. Instead, we will save the USD data inside the temporary crash recovery Maya file.
        if (isCrashing() || isCopyingSceneNodes()) {
            result = kPartiallyCompleted;
            opt = MayaUsd::utils::kSaveToMayaSceneFile;
        } else if (_batchSaveDelegate && _proxiesToSave.size() > 0) {
            result = _batchSaveDelegate(_proxiesToSave, isExport);
        }

        // kAbort: we should abort and return false, which Maya will take as
        // an indication to abort the file operation.
        //
        // kCompleted: the delegate has completely handled the save operation,
        // we should return true and do nothing else here.
        //
        // kPartiallyCompleted: the delegate has partialy handled the saving of
        // files.  In this case we will have to iterate over the scene again
        // in order to find any unsaved stages that are still dirty.

        if (result == kAbort) {
            return false;
        } else if (result == kCompleted && _internalProxiesToSave.size() == 0) {
            return true;
        } else if (result == kPartiallyCompleted && !hasDirtyLayer()) {
            return true;
        }

        // After the potentially partial save, we need to refresh the stages
        // to be saved because the saving might have modified the proxy shape
        // attributes and we need to re-evaluate these nodes so that the stages
        // are re-created with the new attribute values if needed.
        refreshProxiesToSave();

        if (MayaUsd::utils::kSaveToUSDFiles == opt) {
            result = saveUsdToUsdFiles();
        } else {
            result = saveUsdToMayaFile();
        }
    } else {
        result = MayaUsd::kCompleted;
    }

    clearProxies();
    updateLayerManagers();
    return (MayaUsd::kCompleted == result);
}

MStatus addLayerToBuilder(
    MayaUsd::LayerManager* lm,
    MArrayDataBuilder&     builder,
    SdfLayerHandle         layer,
    bool                   isAnon,
    bool                   stubOnly = false,
    bool                   exportOnlyIfDirty = false)
{
    if (!lm)
        return MS::kFailure;

    MStatus     status = MS::kSuccess;
    MDataHandle layersElemHandle = builder.addLast(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MDataHandle idHandle = layersElemHandle.child(lm->identifier);
    MDataHandle fileFormatIdHandle = layersElemHandle.child(lm->fileFormatId);
    MDataHandle serializedHandle = layersElemHandle.child(lm->serialized);
    MDataHandle anonHandle = layersElemHandle.child(lm->anonymous);

    idHandle.setString(UsdMayaUtil::convert(layer->GetIdentifier()));
    anonHandle.setBool(isAnon);

    auto fileFormatIdToken = layer->GetFileFormat()->GetFormatId();
    fileFormatIdHandle.setString(UsdMayaUtil::convert(fileFormatIdToken.GetString()));

    std::string temp;
    if (!stubOnly && ((exportOnlyIfDirty && layer->IsDirty()) || !exportOnlyIfDirty)) {
        if (!layer->ExportToString(&temp)) {
            status = MS::kFailure;
        }
    }

    serializedHandle.setString(UsdMayaUtil::convert(temp));

    return status;
}

MStatus setValueForAttr(const MObject& node, const MObject& attribute, const std::string& value)
{
    MString val = UsdMayaUtil::convert(value);
    MPlug   attrPlug(node, attribute);
    return attrPlug.setValue(val);
}

struct SaveStageToMayaResult
{
    bool _saveSuceeded { false };
    bool _stageHasDirtyLayers { false };
};

template <typename T, typename IgnoreLayerFn>
void saveLayersToMayaFile(
    const T&&              allLayers,
    const IgnoreLayerFn&&  ignoreLayerFn,
    MayaUsd::LayerManager* lm,
    MArrayDataBuilder&     builder,
    MayaUsdProxyShapeBase& proxyShape,
    SaveStageToMayaResult& result)
{
    for (auto layer : allLayers) {
        if (ignoreLayerFn(layer)) {
            continue;
        }
        addLayerToBuilder(
            lm,
            builder,
            layer,
            layer->IsAnonymous(),
            proxyShape.isIncomingLayer(layer->GetIdentifier()),
            true);
        if (layer->IsDirty()) {
            result._stageHasDirtyLayers = true;
        }
    }
}

SaveStageToMayaResult saveStageToMayaFile(
    MayaUsd::LayerManager* lm,
    MArrayDataBuilder&     builder,
    const MObject&         proxyNode,
    UsdStageRefPtr         stage)
{
    SaveStageToMayaResult result;

    if (!lm) {
        return result;
    }

    const MFnDependencyNode depNodeFn(proxyNode);
    MayaUsdProxyShapeBase*  pShape = static_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    if (!pShape)
        return result;

    pShape->setLayerManager(nullptr);

    std::unordered_set<std::string> localLayerIds;

    // Save session layer and its sublayers
    saveLayersToMayaFile(
        UsdUfe::getAllSublayerRefs(stage->GetSessionLayer(), true),
        [&localLayerIds](const auto& layer) {
            localLayerIds.emplace(layer->GetIdentifier());
            return false;
        },
        lm,
        builder,
        *pShape,
        result);

    // Save root layer and its sublayers
    saveLayersToMayaFile(
        UsdUfe::getAllSublayerRefs(stage->GetRootLayer(), true),
        [&localLayerIds](const auto& layer) {
            localLayerIds.emplace(layer->GetIdentifier());
            return false;
        },
        lm,
        builder,
        *pShape,
        result);

    // Save non local layers (reference layers and sub layers in reference layers),
    // skip those have been saved previously from local stack
    saveLayersToMayaFile(
        stage->GetUsedLayers(true),
        [&localLayerIds](const auto& layer) {
            return TF_VERIFY(layer)
                && localLayerIds.find(layer->GetIdentifier()) != localLayerIds.cend();
        },
        lm,
        builder,
        *pShape,
        result);

    if (result._stageHasDirtyLayers) {
        setValueForAttr(
            proxyNode,
            MayaUsdProxyShapeBase::sessionLayerNameAttr,
            stage->GetSessionLayer()->GetIdentifier());

        setValueForAttr(
            proxyNode,
            MayaUsdProxyShapeBase::rootLayerNameAttr,
            stage->GetRootLayer()->GetIdentifier());
    }

    pShape->setLayerManager(lm);

    result._saveSuceeded = true;
    return result;
}

SaveStageToMayaResult saveStageToMayaFile(const MObject& proxyNode, UsdStageRefPtr stage)
{
    // Note: for now we only save USD change made in stage in the main
    //       Maya scene. We don't save changes made to stages in Maya
    //       references.
    SaveStageToMayaResult  result;
    MayaUsd::LayerManager* lm = findOrCreateNode(nullptr);
    if (!lm)
        return result;

    MStatus           status;
    MDataBlock        dataBlock = lm->_forceCache();
    MArrayDataHandle  layersHandle = dataBlock.outputArrayValue(lm->layers, &status);
    MArrayDataBuilder builder(&dataBlock, lm->layers, 1 /*maybe nb stages?*/, &status);

    result = saveStageToMayaFile(lm, builder, proxyNode, stage);

    layersHandle.set(builder);

    layersHandle.setAllClean();
    dataBlock.setClean(lm->layers);

    return result;
}

BatchSaveResult LayerDatabase::saveUsdToMayaFile()
{
    // Note: for now we only save USD change made in stage in the main
    //       Maya scene. We don't save changes made to stages in Maya
    //       references.
    MayaUsd::LayerManager* lm = findOrCreateNode(nullptr);
    if (!lm) {
        return MayaUsd::kNotHandled;
    }

    MStatus           status;
    MDataBlock        dataBlock = lm->_forceCache();
    MArrayDataHandle  layersHandle = dataBlock.outputArrayValue(lm->layers, &status);
    MArrayDataBuilder builder(&dataBlock, lm->layers, 1 /*maybe nb stages?*/, &status);

    bool atLeastOneDirty = false;

    MFnDependencyNode fn;
    for (size_t i = 0; i < _proxiesToSave.size() + _internalProxiesToSave.size(); i++) {
        const StageSavingInfo& info = i < _proxiesToSave.size()
            ? _proxiesToSave[i]
            : _internalProxiesToSave[i - _proxiesToSave.size()];
        MObject mobj = info.dagPath.node();
        fn.setObject(mobj);
        if (!fn.isFromReferencedFile()
            && LayerDatabase::instance().supportedNodeType(fn.typeId())) {

            // Here if its unshared or not an incoming connection we save otherwise skip
            if (!info.shareable || !info.isIncoming) {
                auto result = saveStageToMayaFile(lm, builder, mobj, info.stage);
                if (result._stageHasDirtyLayers) {
                    atLeastOneDirty = true;
                }
                layersHandle.set(builder);
            }
        }
    }

    clearProxies();
    layersHandle.setAllClean();
    dataBlock.setClean(lm->layers);

    if (!atLeastOneDirty) {
        MDGModifier& modifier = MDGModifierUndoItem::create("Save USD to Maya node deletion");
        modifier.deleteNode(lm->thisMObject());
        modifier.doIt();
    }

    return MayaUsd::kCompleted;
}

BatchSaveResult LayerDatabase::saveUsdToUsdFiles()
{
    MFnDependencyNode fn;
    for (size_t i = 0; i < _proxiesToSave.size() + _internalProxiesToSave.size(); i++) {
        const StageSavingInfo& info = i < _proxiesToSave.size()
            ? _proxiesToSave[i]
            : _internalProxiesToSave[i - _proxiesToSave.size()];

        MObject mobj = info.dagPath.node();
        fn.setObject(mobj);
        if (!fn.isFromReferencedFile()
            && LayerDatabase::instance().supportedNodeType(fn.typeId())) {
            MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());

            // Unshared Composition Saves to MayaFile Always
            if (!info.shareable) {
                saveStageToMayaFile(mobj, info.stage);
            } else {
                // No need to save stages from external sources
                if (info.isIncoming) {
                    continue;
                }
                convertAnonymousLayers(pShape, mobj, info.stage);
                const auto& sessionLayer = info.stage->GetSessionLayer();
                const auto& allLayers = info.stage->GetUsedLayers(true);
                for (auto layer : allLayers) {
                    if (TF_VERIFY(layer)) {
                        if (layer != sessionLayer && layer->PermissionToSave()
                            && layer->IsDirty()) {
                            if (!MayaUsd::utils::saveLayerWithFormat(layer)) {
                                MString errMsg;
                                MString layerName(layer->GetDisplayName().c_str());
                                errMsg.format("Could not save layer ^1s.", layerName);
                                MGlobal::displayError(errMsg);
                            }
                        }
                    }
                }
            }
        }
    }

    clearProxies();

    return MayaUsd::kCompleted;
}

void LayerDatabase::convertAnonymousLayers(
    MayaUsdProxyShapeBase* pShape,
    const MObject&         proxyNode,
    UsdStageRefPtr         stage)
{
    SdfLayerHandle root = stage->GetRootLayer();
    std::string    proxyName(pShape->name().asChar());

    convertAnonymousLayersRecursive(root, proxyName, stage);

    // Note: retrieve root again since it may have been changed by the call
    //       to convertAnonymousLayersRecursive
    root = stage->GetRootLayer();
    if (root->IsAnonymous()) {
        // Only set up-axis and units metadata on the root layer
        // and only if it is anonymous before being saved.
        MayaUsd::utils::setLayerUpAxisAndUnits(root);

        const bool wasTargetLayer = (stage->GetEditTarget().GetLayer() == root);
        PXR_NS::SdfFileFormat::FileFormatArguments args;
        std::string newFileName = MayaUsd::utils::generateUniqueFileName(proxyName);
        const bool  isRelative = UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
        if (isRelative) {
            newFileName = UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(newFileName);
        }
        if (!MayaUsd::utils::saveLayerWithFormat(root, newFileName)) {
            MString errMsg;
            MString layerName(root->GetDisplayName().c_str());
            errMsg.format("Could not save layer ^1s.", layerName);
            MGlobal::displayError(errMsg);
        }

        SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(newFileName);
        MayaUsd::utils::setNewProxyPath(
            pShape->name(),
            UsdMayaUtil::convert(newFileName),
            isRelative ? MayaUsd::utils::kProxyPathRelative : MayaUsd::utils::kProxyPathAbsolute,
            newLayer,
            wasTargetLayer);
    }

    SdfLayerHandle session = stage->GetSessionLayer();
    if (!session->IsEmpty()) {
        convertAnonymousLayersRecursive(session, proxyName, stage);

        saveUsdLayerToMayaFile(session, true);

        // TODO: should update the target layer of the proxy shape if the session was the target.
        setValueForAttr(
            proxyNode,
            MayaUsdProxyShapeBase::sessionLayerNameAttr,
            stage->GetSessionLayer()->GetIdentifier());
    }
}

void LayerDatabase::saveUsdLayerToMayaFile(SdfLayerRefPtr layer, bool asAnonymous)
{
    // Note: for now we only save USD change made in stage in the main
    //       Maya scene. We don't save changes made to stages in Maya
    //       references.
    MayaUsd::LayerManager* lm = findOrCreateNode(nullptr);
    if (!lm)
        return;

    MStatus           status;
    MDataBlock        dataBlock = lm->_forceCache();
    MArrayDataHandle  layersHandle = dataBlock.outputArrayValue(lm->layers, &status);
    MArrayDataBuilder builder(&dataBlock, lm->layers, 1 /*maybe nb stages?*/, &status);

    addLayerToBuilder(lm, builder, layer, asAnonymous);

    layersHandle.set(builder);

    layersHandle.setAllClean();
    dataBlock.setClean(lm->layers);
}

void LayerDatabase::loadLayersPostRead(MayaUsdProxyShapeBase* forProxyShape)
{
    MayaUsd::LayerManager* lm = findNode(forProxyShape);
    if (!lm)
        return;

    const char*                 identifierTempSuffix = "_tmp";
    MStatus                     status;
    MPlug                       allLayersPlug(lm->thisMObject(), lm->layers);
    MPlug                       singleLayerPlug;
    MPlug                       idPlug;
    MPlug                       fileFormatIdPlug;
    MPlug                       anonymousPlug;
    MPlug                       serializedPlug;
    std::string                 identifierVal;
    std::string                 fileFormatIdVal;
    std::string                 serializedVal;
    SdfLayerRefPtr              layer;
    std::vector<SdfLayerRefPtr> createdLayers;

    const unsigned int numElements = allLayersPlug.numElements();
    for (unsigned int i = 0; i < numElements; ++i) {
        layer = nullptr;

        singleLayerPlug = allLayersPlug.elementByPhysicalIndex(i, &status);
        idPlug = singleLayerPlug.child(lm->identifier, &status);
        fileFormatIdPlug = singleLayerPlug.child(lm->fileFormatId, &status);
        anonymousPlug = singleLayerPlug.child(lm->anonymous, &status);
        serializedPlug = singleLayerPlug.child(lm->serialized, &status);

        identifierVal = idPlug.asString(MDGContext::fsNormal, &status).asChar();
        if (identifierVal.empty()) {
            MGlobal::displayError(
                MString("Error - plug ") + idPlug.partialName(true) + " had empty identifier");
            continue;
        }

        fileFormatIdVal = fileFormatIdPlug.asString(MDGContext::fsNormal, &status).asChar();
        if (fileFormatIdVal.empty()) {
            MGlobal::displayInfo(
                MString("No file format in ") + fileFormatIdPlug.partialName(true)
                + " plug. Will use identifier to work it out.");
        }

        bool layerContainsEdits = true;
        serializedVal = serializedPlug.asString(MDGContext::fsNormal, &status).asChar();
        if (serializedVal.empty()) {
            layerContainsEdits = false;
        }

        bool isAnon = anonymousPlug.asBool(MDGContext::fsNormal, &status);
        if (isAnon) {
            // Note that the new identifier will not match the old identifier - only the "tag"
            // will be retained
            layer
                = SdfLayer::CreateAnonymous(SdfLayer::GetDisplayNameFromIdentifier(identifierVal));
        } else {
            SdfLayerHandle layerHandle = SdfLayer::Find(identifierVal);
            if (layerHandle) {
                layer = layerHandle;
            } else {
                // TODO: currently, there is a small window here, after the find, and before the
                // New, where another process might sneak in and create a layer with the same
                // identifier, which could cause an error. This seems unlikely, but we have a
                // discussion with Pixar to find a way to avoid this.

                SdfFileFormatConstPtr fileFormat;
                if (!fileFormatIdVal.empty()) {
                    fileFormat = SdfFileFormat::FindById(TfToken(fileFormatIdVal));
                } else {
                    fileFormat = SdfFileFormat::FindByExtension(
                        ArGetResolver().GetExtension(identifierVal));
                    if (!fileFormat) {
                        MGlobal::displayError(
                            MString("Cannot determine file format for identifier '")
                            + identifierVal.c_str() + "' for plug " + idPlug.partialName(true));
                        continue;
                    }
                }

                if (layerContainsEdits) {
                    // In order to make the layer reloadable by SdfLayer::Reload(), we hack the
                    // identifier with temp one on creation and call layer->SetIdentifier()
                    // again to set the timestamp:
                    layer = SdfLayer::New(fileFormat, identifierVal + identifierTempSuffix);
                    if (!layer) {
                        MGlobal::displayError(
                            MString("Error - failed to create new layer for identifier '")
                            + identifierVal.c_str() + "' for plug " + idPlug.partialName(true));
                        continue;
                    }
                    layer->SetIdentifier(
                        identifierVal); // Make it reloadable by SdfLayer::Reload(true)
                    layer->Clear();     // Mark it dirty to make it reloadable by SdfLayer::Reload()
                                        // without force=true
                } else {
                    layer = SdfLayer::FindOrOpen(identifierVal);
                }
            }
        }

        if (layer) {
            if (layerContainsEdits) {
                if (!layer->ImportFromString(serializedVal)) {
                    MGlobal::displayError(
                        MString("Failed to import serialized layer: ") + serializedVal.c_str());
                    continue;
                }
            }

            LayerDatabase::instance().addLayer(layer, identifierVal);
            createdLayers.push_back(layer);
        }
    }

    LayerDatabase::instance().loadLayerManagerSelectedStage(*lm);

    if (!_isSavingMayaFile)
        removeManagerNode(lm, forProxyShape);

    for (auto it = createdLayers.begin(); it != createdLayers.end(); ++it) {
        SdfLayerHandle lh = (*it);
        LayerDatabase::instance().remapSubLayerPaths(lh);
    }
}

void LayerDatabase::cleanUpNewScene(void*)
{
    LayerDatabase::instance().removeAllLayers();
    LayerDatabase::removeManagerNode();
}

LayerManager::LayerNameMap LayerDatabase::getLayerNameMap() const
{
    LayerManager::LayerNameMap nameMap;
    for (const auto& idAndLayer : _idToLayer) {
        const std::string& layerName = idAndLayer.first;
        const std::string& currentName = idAndLayer.second->GetIdentifier();
        if (currentName != layerName) {
            nameMap[layerName] = currentName;
        }
    }
    return nameMap;
}

bool LayerDatabase::remapSubLayerPaths(SdfLayerHandle parentLayer)
{
    bool                     modifiedPaths = false;
    std::vector<std::string> paths = parentLayer->GetSubLayerPaths();
    for (size_t i = 0, n = paths.size(); i < n; ++i) {
        SdfLayerRefPtr subLayer = findLayer(paths[i]);
        if (subLayer) {
            if (subLayer->GetIdentifier() != paths[i]) {
                paths[i] = subLayer->GetIdentifier();
                modifiedPaths = true;
            }
        }
    }

    if (modifiedPaths) {
        parentLayer->SetSubLayerPaths(paths);
    }

    return modifiedPaths;
}

bool LayerDatabase::addLayer(SdfLayerRefPtr layer, const std::string& identifier)
{
    _addLayer(layer, layer->GetIdentifier());
    if (identifier != layer->GetIdentifier() && !identifier.empty()) {
        _addLayer(layer, identifier);
    }

    return true;
}

void LayerDatabase::_addLayer(SdfLayerRefPtr layer, const std::string& identifier)
{
    auto insertIdResult = _idToLayer.emplace(identifier, layer);
    if (!insertIdResult.second) {
        insertIdResult.first->second = layer;
    }
}

bool LayerDatabase::removeLayer(SdfLayerRefPtr layer)
{
    std::vector<std::string> paths = layer->GetSubLayerPaths();
    for (auto pathName : paths) {
        SdfLayerRefPtr childLayer = findLayer(pathName);
        if (childLayer) {
            removeLayer(childLayer);
        }
    }

    auto iter = _idToLayer.begin();
    while (iter != _idToLayer.end()) {
        if ((*iter).second == layer)
            iter = _idToLayer.erase(iter);
        else
            iter++;
    }

    return true;
}

void LayerDatabase::removeAllLayers() { _idToLayer.clear(); }

SdfLayerHandle LayerDatabase::findLayer(std::string identifier) const
{
    auto foundIdAndLayer = _idToLayer.find(identifier);
    if (foundIdAndLayer != _idToLayer.end()) {
        return foundIdAndLayer->second;
    }

    return SdfLayerHandle();
}

void LayerDatabase::clearManagerNode(MayaUsd::LayerManager* lm)
{
    if (!lm)
        return;

    MStatus status;
    MPlug   arrayPlug(lm->thisMObject(), lm->layers);

    // First, disconnect any connected attributes
    disconnectCompoundArrayPlug(arrayPlug);

    // Then wipe the array attribute
    MDataBlock       dataBlock = lm->_forceCache();
    MArrayDataHandle layersArrayHandle = dataBlock.outputArrayValue(lm->layers, &status);

    MArrayDataBuilder builder(&dataBlock, lm->layers, 0, &status);
    layersArrayHandle.set(builder);
    layersArrayHandle.setAllClean();
    dataBlock.setClean(lm->layers);
}

void LayerDatabase::removeManagerNode(
    MayaUsd::LayerManager* lm,
    MayaUsdProxyShapeBase* forProxyShape)
{
    if (!lm) {
        lm = findNode(forProxyShape);
    }
    if (!lm) {
        return;
    }

    // This is called during a Maya notification callback, so no undo supported.
    OpUndoItemMuting muting;

    clearManagerNode(lm);

    MDGModifier& modifier = MDGModifierUndoItem::create("Manager node removal");
    modifier.deleteNode(lm->thisMObject());
    modifier.doIt();
}

//----------------------------------------------------------------------------------------------------------------------

const MString LayerManager::typeName("mayaUsdLayerManager");
const MTypeId LayerManager::typeId(0x58000097);

MObject LayerManager::layers = MObject::kNullObj;
MObject LayerManager::identifier = MObject::kNullObj;
MObject LayerManager::fileFormatId = MObject::kNullObj;
MObject LayerManager::serialized = MObject::kNullObj;
MObject LayerManager::anonymous = MObject::kNullObj;
MObject LayerManager::selectedStage = MObject::kNullObj;

struct _OnSceneResetListener : public TfWeakBase
{
    _OnSceneResetListener()
    {
        TfWeakPtr<_OnSceneResetListener> me(this);
        TfNotice::Register(me, &_OnSceneResetListener::OnSceneReset);
    }

    void OnSceneReset(const UsdMayaSceneResetNotice& notice)
    {
        layerManagerHandle = MObject::kNullObj;
    }
};

/* static */
void LayerManager::SetBatchSaveDelegate(BatchSaveDelegate delegate)
{
    LayerDatabase::setBatchSaveDelegate(delegate);
}

/* static */
void* LayerManager::creator() { return new LayerManager(); }

/* static */
MStatus LayerManager::initialize()
{
    try {
        MStatus           stat;
        MFnTypedAttribute fn_str;
        MFnStringData     stringData;

        selectedStage
            = fn_str.create("selectedStage", "sst", MFnData::kString, MObject::kNullObj, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_str.setCached(true);
        fn_str.setReadable(true);
        fn_str.setStorable(true);
        fn_str.setHidden(true);
        stat = addAttribute(selectedStage);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        identifier = fn_str.create("identifier", "id", MFnData::kString, MObject::kNullObj, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_str.setCached(true);
        fn_str.setReadable(true);
        fn_str.setStorable(true);
        fn_str.setHidden(true);
        stat = addAttribute(identifier);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        fileFormatId
            = fn_str.create("fileFormatId", "fid", MFnData::kString, MObject::kNullObj, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_str.setCached(true);
        fn_str.setReadable(true);
        fn_str.setStorable(true);
        fn_str.setHidden(true);
        stat = addAttribute(fileFormatId);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        serialized = fn_str.create("serialized", "szd", MFnData::kString, MObject::kNullObj, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_str.setCached(true);
        fn_str.setReadable(true);
        fn_str.setStorable(true);
        fn_str.setHidden(true);
        stat = addAttribute(serialized);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        MFnNumericAttribute fn_bool;
        anonymous = fn_bool.create("anonymous", "ann", MFnNumericData::kBoolean, false, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_bool.setCached(true);
        fn_bool.setReadable(true);
        fn_bool.setStorable(true);
        fn_bool.setHidden(true);
        stat = addAttribute(anonymous);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        MFnCompoundAttribute fn_cmp;
        layers = fn_cmp.create("layers", "lyr", &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        stat = fn_cmp.addChild(identifier);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        stat = fn_cmp.addChild(fileFormatId);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        stat = fn_cmp.addChild(serialized);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        stat = fn_cmp.addChild(anonymous);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        fn_cmp.setCached(true);
        fn_cmp.setReadable(true);
        fn_cmp.setWritable(true);
        fn_cmp.setStorable(true);
        fn_cmp.setConnectable(true);
        fn_cmp.setHidden(true);
        fn_cmp.setArray(true);
        fn_cmp.setUsesArrayDataBuilder(true);
        stat = addAttribute(layers);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

    } catch (const MStatus& status) {
        return status;
    }

    static _OnSceneResetListener onSceneResetListener;
    return MS::kSuccess;
}

LayerManager::LayerManager()
    : MPxNode()
{
}

LayerManager::~LayerManager() { }

/* static */
SdfLayerHandle LayerManager::findLayer(std::string identifier, MayaUsdProxyShapeBase* forProxyShape)
{
    std::lock_guard<std::recursive_mutex> lock(findNodeMutex);

    LayerDatabase::loadLayersPostRead(forProxyShape);

    return LayerDatabase::instance().findLayer(identifier);
}

/* static */
LayerManager::LayerNameMap LayerManager::getLayerNameMap(MayaUsdProxyShapeBase* forProxyShape)
{
    std::lock_guard<std::recursive_mutex> lock(findNodeMutex);

    LayerDatabase::loadLayersPostRead(forProxyShape);

    return LayerDatabase::instance().getLayerNameMap();
}

/* static */
void LayerManager::addSupportForNodeType(MTypeId type)
{
    return LayerDatabase::instance().addSupportForNodeType(type);
}

/* static */
void LayerManager::removeSupportForNodeType(MTypeId type)
{
    return LayerDatabase::instance().removeSupportForNodeType(type);
}

bool LayerManager::supportedNodeType(MTypeId nodeId)
{
    return LayerDatabase::instance().supportedNodeType(nodeId);
}

/* static */
void LayerManager::setSelectedStage(const std::string& stage)
{
    return LayerDatabase::instance().setSelectedStage(stage);
}

/* static */
std::string LayerManager::getSelectedStage(MayaUsdProxyShapeBase* forProxyShape)
{
    LayerDatabase::loadLayersPostRead(forProxyShape);
    return LayerDatabase::instance().getSelectedStage();
}

/* static */
bool LayerManager::isSaving() { return LayerDatabase::isSaving(); }

} // namespace MAYAUSD_NS_DEF
