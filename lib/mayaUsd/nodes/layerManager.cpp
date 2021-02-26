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

#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#include <pxr/usd/usdUtils/authoring.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDagPath.h>
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

// Utility func to disconnect an array plug, and all it's element plugs, and all
// their child plugs.
// Not in Utils, because it's not generic - ie, doesn't handle general case
// where compound/array plugs may be nested arbitrarily deep...
MStatus disconnectCompoundArrayPlug(MPlug arrayPlug)
{
    MStatus     status;
    MPlug       elemPlug;
    MPlug       srcPlug;
    MPlugArray  destPlugs;
    MDGModifier dgmod;

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

SdfFileFormatConstPtr
getFileFormatForLayer(const std::string& identifierVal, const std::string& serializedVal)
{
    // If there is serialized data and it does not start with "#usda" then the format is Sdf.
    // Else we look at the file extension to determine what it should be, which could be Sdf, Usd,
    // Usdc, or Usda.

    SdfFileFormatConstPtr fileFormat;

    if (!serializedVal.empty() && !TfStringStartsWith(serializedVal, "#usda ")) {
        fileFormat = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
    } else {
        // In order to make the layer reloadable by SdfLayer::Reload(), we need the
        // correct file format from identifier.
        if (TfStringEndsWith(identifierVal, ".usd")) {
            fileFormat = SdfFileFormat::FindById(UsdUsdFileFormatTokens->Id);
        } else if (TfStringEndsWith(identifierVal, ".usdc")) {
            fileFormat = SdfFileFormat::FindById(UsdUsdcFileFormatTokens->Id);
        } else if (TfStringEndsWith(identifierVal, ".sdf")) {
            fileFormat = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
        } else {
            fileFormat = SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id);
        }
    }

    return fileFormat;
}

MayaUsd::LayerManager* findNode()
{
    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (fn.typeId() == MayaUsd::LayerManager::typeId && !fn.isFromReferencedFile()) {
            return static_cast<MayaUsd::LayerManager*>(fn.userNode());
        }
    }
    return nullptr;
}

void setNewProxyPath(const MString& proxyNodeName, const MString& newValue)
{
    MString script;
    script.format("setAttr -type \"string\" ^1s.filePath \"^2s\"", proxyNodeName, newValue);
    MGlobal::executeCommand(
        script,
        /*display*/ true,
        /*undo*/ false);
}

void convertAnonymousLayersRecursive(
    SdfLayerRefPtr     layer,
    const std::string& basename,
    UsdStageRefPtr     stage)
{
    auto currentTarget = stage->GetEditTarget().GetLayer();

    std::vector<std::string> sublayers = layer->GetSubLayerPaths();
    for (size_t i = 0, n = sublayers.size(); i < n; ++i) {
        auto subL = layer->Find(sublayers[i]);
        if (subL) {
            convertAnonymousLayersRecursive(subL, basename, stage);

            if (subL->IsAnonymous()) {
                auto newLayer = UsdMayaSerialization::saveAnonymousLayer(subL, layer, basename);
                if (subL == currentTarget) {
                    stage->SetEditTarget(newLayer);
                }
            }
        }
    }
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
    static void           prepareForWriteCheck(bool*, void*);
    static void           loadLayersPostRead(void*);
    static void           cleanUpNewScene(void*);
    static void           removeManagerNode(MayaUsd::LayerManager* lm = nullptr);

    bool getStagesToSave();
    bool supportedNodeType(MTypeId type);
    void addSupportForNodeType(MTypeId type);
    void removeSupportForNodeType(MTypeId type);
    bool remapSubLayerPaths(SdfLayerHandle parentLayer);
    bool addLayer(SdfLayerRefPtr layer, const std::string& identifier = std::string());
    bool removeLayer(SdfLayerRefPtr layer);
    void removeAllLayers();

    SdfLayerHandle findLayer(std::string identifier) const;

private:
    void registerCallbacks();
    void unregisterCallbacks();

    void _addLayer(SdfLayerRefPtr layer, const std::string& identifier);
    void onStageSet(const MayaUsdProxyStageSetNotice& notice);

    bool saveUsd();
    bool saveUsdToMayaFile();
    bool saveUsdToUsdFiles();
    void convertAnonymousLayers(MayaUsdProxyShapeBase* pShape, UsdStageRefPtr stage);
    void saveSessionLayer(MayaUsdProxyShapeBase* pShape, UsdStageRefPtr stage);

    std::map<std::string, SdfLayerRefPtr> _idToLayer;
    TfNotice::Key                         _onStageSetKey;
    std::set<unsigned int>                _supportedTypes;
    std::vector<UsdStageRefPtr>           _stagesToSave;
    static MCallbackId                    preSaveCallbackId;
    static MCallbackId                    preExportCallbackId;
    static MCallbackId                    postNewCallbackId;
    static MCallbackId                    preOpenCallbackId;

    static MayaUsd::BatchSaveDelegate _batchSaveDelegate;
};

MCallbackId LayerDatabase::preSaveCallbackId = 0;
MCallbackId LayerDatabase::preExportCallbackId = 0;
MCallbackId LayerDatabase::postNewCallbackId = 0;
MCallbackId LayerDatabase::preOpenCallbackId = 0;

MayaUsd::BatchSaveDelegate LayerDatabase::_batchSaveDelegate = nullptr;

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
            MSceneMessage::kBeforeSaveCheck, LayerDatabase::prepareForWriteCheck);
        preExportCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeExportCheck, LayerDatabase::prepareForWriteCheck);
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
        MSceneMessage::removeCallback(preExportCallbackId);
        MSceneMessage::removeCallback(preOpenCallbackId);

        preSaveCallbackId = 0;
        preExportCallbackId = 0;
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

void LayerDatabase::prepareForWriteCheck(bool* retCode, void*)
{
    cleanUpNewScene(nullptr);

    if (LayerDatabase::instance().getStagesToSave()) {

        int dialogResult = true;

        if (MGlobal::kInteractive == MGlobal::mayaState()) {
            MGlobal::executeCommand(kSaveOptionUICmd, dialogResult);
        }

        if (dialogResult) {
            dialogResult = LayerDatabase::instance().saveUsd();
        }

        *retCode = dialogResult;
    } else {
        *retCode = true;
    }
}

bool LayerDatabase::getStagesToSave()
{
    auto allStages = MayaUsd::ufe::getAllStages();
    bool checkSelection = (MFileIO::kExportTypeSelected == MFileIO::exportType());
    const UFE_NS::GlobalSelection::Ptr& ufeSelection = UFE_NS::GlobalSelection::get();

    _stagesToSave.clear();
    for (const auto& stage : allStages) {
        auto stagePath = MayaUsd::ufe::stagePath(stage);
        if (!checkSelection
            || (ufeSelection->contains(stagePath) || ufeSelection->containsAncestor(stagePath))) {
            // Remove the leading "|world| component.
            std::string       strStagePath = stagePath.popHead().string();
            MDagPath          proxyDagPath = UsdMayaUtil::nameToDagPath(strStagePath);
            MFnDependencyNode fn(proxyDagPath.node());
            if (!fn.isFromReferencedFile() && supportedNodeType(fn.typeId())) {
                SdfLayerHandleVector allLayers = stage->GetLayerStack(true);
                for (auto layer : allLayers) {
                    if (layer->IsDirty()) {
                        _stagesToSave.push_back(stage);
                        break;
                    }
                }
            }
        }
    }

    return !_stagesToSave.empty();
}

bool LayerDatabase::saveUsd()
{
    bool result = true;

    auto opt = UsdMayaSerialization::serializeUsdEditsLocationOption();

    if (UsdMayaSerialization::kIgnoreUSDEdits != opt) {
        if (_batchSaveDelegate) {
            result = _batchSaveDelegate(_stagesToSave);
        }

        if (result) {
            if (UsdMayaSerialization::kSaveToUSDFiles == opt) {
                result = saveUsdToUsdFiles();
            } else {
                result = saveUsdToMayaFile();
            }
        }
    }

    _stagesToSave.clear();
    return result;
}

bool LayerDatabase::saveUsdToMayaFile()
{
    MayaUsd::LayerManager* lm = findNode();
    if (!lm) {
        MDGModifier modifier;
        MObject     manager = modifier.createNode(MayaUsd::LayerManager::typeId);
        modifier.doIt();

        lm = static_cast<MayaUsd::LayerManager*>(MFnDependencyNode(manager).userNode());
    }

    if (!lm) {
        return false;
    }

    MStatus           status;
    MDataBlock        dataBlock = lm->_forceCache();
    MArrayDataHandle  layersHandle = dataBlock.outputArrayValue(lm->layers, &status);
    MArrayDataBuilder builder(&dataBlock, lm->layers, 1 /*maybe nb stages?*/, &status);

    bool atLeastOneDirty = false;

    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (!fn.isFromReferencedFile()
            && LayerDatabase::instance().supportedNodeType(fn.typeId())) {
            MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
            UsdStageRefPtr         stage = pShape ? pShape->getUsdStage() : nullptr;
            if (!stage) {
                continue;
            }

            bool thisStageHasDirtyLayer = false;

            std::string          temp;
            SdfLayerHandleVector allLayers = stage->GetLayerStack(true);
            for (auto layer : allLayers) {
                MDataHandle layersElemHandle = builder.addLast(&status);
                MDataHandle idHandle = layersElemHandle.child(lm->identifier);
                MDataHandle serializedHandle = layersElemHandle.child(lm->serialized);
                MDataHandle anonHandle = layersElemHandle.child(lm->anonymous);

                idHandle.setString(UsdMayaUtil::convert(layer->GetIdentifier()));
                anonHandle.setBool(layer->IsAnonymous());

                if (layer->IsDirty()) {
                    atLeastOneDirty = true;
                    thisStageHasDirtyLayer = true;
                    layer->ExportToString(&temp);
                } else {
                    temp.clear();
                }
                serializedHandle.setString(UsdMayaUtil::convert(temp));
            }
            layersHandle.set(builder);

            if (thisStageHasDirtyLayer) {
                auto    sessionLayer = stage->GetSessionLayer();
                auto    usdIdentifier = sessionLayer->GetIdentifier();
                MString idString = UsdMayaUtil::convert(usdIdentifier);

                MPlug sessionLayerNamePlug(mobj, MayaUsdProxyShapeBase::sessionLayerNameAttr);
                sessionLayerNamePlug.setValue(idString);

                auto rootLayer = stage->GetRootLayer();
                usdIdentifier = rootLayer->GetIdentifier();
                idString = UsdMayaUtil::convert(usdIdentifier);

                MPlug rootLayerNamePlug(mobj, MayaUsdProxyShapeBase::rootLayerNameAttr);
                rootLayerNamePlug.setValue(idString);
            }
        }
    }
    layersHandle.setAllClean();
    dataBlock.setClean(lm->layers);

    if (!atLeastOneDirty) {
        MDGModifier modifier;
        modifier.deleteNode(lm->thisMObject());
        modifier.doIt();
    }

    return true;
}

bool LayerDatabase::saveUsdToUsdFiles()
{
    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (!fn.isFromReferencedFile()
            && LayerDatabase::instance().supportedNodeType(fn.typeId())) {
            MayaUsdProxyShapeBase* pShape = static_cast<MayaUsdProxyShapeBase*>(fn.userNode());
            UsdStageRefPtr         stage = pShape ? pShape->getUsdStage() : nullptr;
            if (!stage) {
                continue;
            }

            convertAnonymousLayers(pShape, stage);

            SdfLayerHandleVector allLayers = stage->GetLayerStack(false);
            for (auto layer : allLayers) {
                if (layer->PermissionToSave()) {
                    layer->Save();
                }
            }
        }
    }

    return true;
}

void LayerDatabase::convertAnonymousLayers(MayaUsdProxyShapeBase* pShape, UsdStageRefPtr stage)
{
    SdfLayerHandle root = stage->GetRootLayer();
    std::string    proxyName(pShape->name().asChar());

    convertAnonymousLayersRecursive(root, proxyName, stage);

    if (root->IsAnonymous()) {
        PXR_NS::SdfFileFormat::FileFormatArguments args;
        args["format"] = UsdMayaSerialization::usdFormatArgOption();
        std::string newFileName = UsdMayaSerialization::generateUniqueFileName(proxyName);
        root->Export(newFileName, "", args);

        setNewProxyPath(pShape->name(), UsdMayaUtil::convert(newFileName));
    }

    SdfLayerHandle session = stage->GetSessionLayer();
    if (!session->IsEmpty()) {
        convertAnonymousLayersRecursive(session, proxyName, stage);

        saveSessionLayer(pShape, stage);
    }
}

void LayerDatabase::saveSessionLayer(MayaUsdProxyShapeBase* pShape, UsdStageRefPtr stage)
{
    MayaUsd::LayerManager* lm = findNode();
    if (!lm) {
        MDGModifier modifier;
        MObject     manager = modifier.createNode(MayaUsd::LayerManager::typeId);
        modifier.doIt();

        lm = static_cast<MayaUsd::LayerManager*>(MFnDependencyNode(manager).userNode());
    }

    if (!lm)
        return;

    MStatus           status;
    MDataBlock        dataBlock = lm->_forceCache();
    MArrayDataHandle  layersHandle = dataBlock.outputArrayValue(lm->layers, &status);
    MArrayDataBuilder builder(&dataBlock, lm->layers, 1 /*maybe nb stages?*/, &status);

    std::string    temp;
    SdfLayerHandle session = stage->GetSessionLayer();

    MDataHandle layersElemHandle = builder.addLast(&status);
    MDataHandle idHandle = layersElemHandle.child(lm->identifier);
    MDataHandle serializedHandle = layersElemHandle.child(lm->serialized);
    MDataHandle anonHandle = layersElemHandle.child(lm->anonymous);

    idHandle.setString(UsdMayaUtil::convert(session->GetIdentifier()));
    anonHandle.setBool(true);

    session->ExportToString(&temp);

    serializedHandle.setString(UsdMayaUtil::convert(temp));

    layersHandle.set(builder);

    layersHandle.setAllClean();
    dataBlock.setClean(lm->layers);
}

void LayerDatabase::loadLayersPostRead(void*)
{
    MayaUsd::LayerManager* lm = findNode();
    if (!lm)
        return;

    const char*                 identifierTempSuffix = "_tmp";
    MStatus                     status;
    MPlug                       allLayersPlug(lm->thisMObject(), lm->layers);
    MPlug                       singleLayerPlug;
    MPlug                       idPlug;
    MPlug                       anonymousPlug;
    MPlug                       serializedPlug;
    std::string                 identifierVal;
    std::string                 serializedVal;
    SdfLayerRefPtr              layer;
    std::vector<SdfLayerRefPtr> createdLayers;

    const unsigned int numElements = allLayersPlug.numElements();
    for (unsigned int i = 0; i < numElements; ++i) {
        layer = nullptr;

        singleLayerPlug = allLayersPlug.elementByPhysicalIndex(i, &status);
        idPlug = singleLayerPlug.child(lm->identifier, &status);
        anonymousPlug = singleLayerPlug.child(lm->anonymous, &status);
        serializedPlug = singleLayerPlug.child(lm->serialized, &status);

        identifierVal = idPlug.asString(MDGContext::fsNormal, &status).asChar();
        if (identifierVal.empty()) {
            MGlobal::displayError(
                MString("Error - plug ") + idPlug.partialName(true) + "had empty identifier");
            continue;
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

                SdfFileFormatConstPtr fileFormat
                    = getFileFormatForLayer(identifierVal, serializedVal);

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

    removeManagerNode(lm);

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

void LayerDatabase::removeManagerNode(MayaUsd::LayerManager* lm)
{
    if (!lm) {
        lm = findNode();
    }
    if (!lm) {
        return;
    }

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

    MDGModifier modifier;
    modifier.deleteNode(lm->thisMObject());
    modifier.doIt();
}

//----------------------------------------------------------------------------------------------------------------------

const MString LayerManager::typeName("mayaUsdLayerManager");
const MTypeId LayerManager::typeId(0x58000097);

MObject LayerManager::layers = MObject::kNullObj;
MObject LayerManager::identifier = MObject::kNullObj;
MObject LayerManager::serialized = MObject::kNullObj;
MObject LayerManager::anonymous = MObject::kNullObj;

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

        identifier = fn_str.create("identifier", "id", MFnData::kString, MObject::kNullObj, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        fn_str.setCached(true);
        fn_str.setReadable(true);
        fn_str.setStorable(true);
        fn_str.setHidden(true);
        stat = addAttribute(identifier);
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

    return MS::kSuccess;
}

LayerManager::LayerManager()
    : MPxNode()
{
}

LayerManager::~LayerManager() { }

/* static */
SdfLayerHandle LayerManager::findLayer(std::string identifier)
{
    std::lock_guard<std::recursive_mutex> lock(findNodeMutex);

    LayerDatabase::loadLayersPostRead(nullptr);

    return LayerDatabase::instance().findLayer(identifier);
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

} // namespace MAYAUSD_NS_DEF
