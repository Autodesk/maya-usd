////
//// Copyright 2021 Autodesk
////
//// Licensed under the Apache License, Version 2.0 (the "License");
//// you may not use this file except in compliance with the License.
//// You may obtain a copy of the License at
////
////     http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing, software
//// distributed under the License is distributed on an "AS IS" BASIS,
//// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//// See the License for the specific language governing permissions and
//// limitations under the License.
////

#include "utilSerialization.h"

#include "layerEditorDCCFunctions.h"
#include "layerLocking.h"
#include "layerMuting.h"
#include "tokens.h"
#include "utilFileSystem.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/stageCache.h>
#include <pxr/usd/usd/stageCacheContext.h>
#if PXR_VERSION >= 2511
#include <pxr/usd/sdf/usdFileFormat.h>
#include <pxr/usd/sdf/usdaFileFormat.h>
#include <pxr/usd/sdf/usdcFileFormat.h>
#else
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#endif
#include <pxr/usd/usdUtils/stageCache.h>

#include <ufe/pathString.h>

#include <filesystem>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    std::function<void(std::string, std::string, const PXR_NS::SdfLayerRefPtr&, bool)>
        updateDCCObjectRootLayerFunction;
    std::function<std::vector<PXR_NS::UsdStageCache*>()> getStageCachesFunction;
    std::function<void(const PXR_NS::SdfLayerRefPtr&)>    layerUpAxisAndUnitsFn;
}

namespace UsdLayerEditor {
namespace Serialization {

    enum LayerEditorSerializationErrors
    {
        NoAnonLayerProvided,
        CannotSaveNonAnonLayer,
        CannotSaveAnonLayerWhenSysLocked,
        FailedAnonLayerSave,
        FailedAnonLayerReload
    };

    TF_REGISTRY_FUNCTION(TfEnum)
    {
        TF_ADD_ENUM_NAME(
            NoAnonLayerProvided, "No anonymous layer provided.");
        TF_ADD_ENUM_NAME(
            CannotSaveNonAnonLayer, "Cannot save non anonymous layer.");
        TF_ADD_ENUM_NAME(
            CannotSaveAnonLayerWhenSysLocked, "Cannot save non anonymous layer that is sys locked.");
        TF_ADD_ENUM_NAME(
            FailedAnonLayerSave, "Anonymous layer has failed.");
        TF_ADD_ENUM_NAME(
            FailedAnonLayerReload, "Anonymous layer reload has failed.");
    };


void setUpdateDCCObjectRootLayerFunction(
    std::function<void(std::string, std::string, const PXR_NS::SdfLayerRefPtr&, bool)> updateFunction)
{
    updateDCCObjectRootLayerFunction = updateFunction;
}

void setGetStageCachesFunction(std::function<std::vector<PXR_NS::UsdStageCache*>()> getCachesFunction)
{
    getStageCachesFunction = getCachesFunction;
}

void setLayerUpAxisAndUnitsFn(std::function<void(const PXR_NS::SdfLayerRefPtr&)> fn)
{
    layerUpAxisAndUnitsFn = fn;
}

class RecursionDetector
{
public:
    RecursionDetector() { }
    void push(const std::string& path) { _paths.push_back(path); }

    void pop() { _paths.pop_back(); }
    bool contains(const std::string& in_path) const
    {
        return !in_path.empty()
            && std::find(_paths.cbegin(), _paths.cend(), in_path) != _paths.cend();
    }

    std::vector<std::string> _paths;
};

void populateChildren(
    const std::string&           objectPath,
    const UsdStageRefPtr&        stage,
    SdfLayerRefPtr               layer,
    RecursionDetector*           recursionDetector,
    LayerInfos&                  anonLayersToSave,
    std::vector<SdfLayerRefPtr>& dirtyLayersToSave)
{
    auto subPaths = layer->GetSubLayerPaths();

    RecursionDetector defaultDetector;
    if (!recursionDetector) {
        recursionDetector = &defaultDetector;
    }
    recursionDetector->push(layer->GetRealPath());

    for (auto iter = subPaths.rbegin(); iter != subPaths.rend(); ++iter) {
        std::string path = *iter;
        std::string actualPath = PXR_NS::SdfComputeAssetPathRelativeToLayer(layer, path);
        auto        subLayer = PXR_NS::SdfLayer::FindOrOpen(actualPath);
        if (subLayer && !recursionDetector->contains(subLayer->GetRealPath())) {
            populateChildren(
                objectPath,
                stage,
                subLayer,
                recursionDetector,
                anonLayersToSave,
                dirtyLayersToSave);

            if (subLayer->IsAnonymous()) {
                LayerInfo info;
                info.stage = stage;
                info.layer = subLayer;
                info.parent._objectPath = objectPath;
                info.parent._layerParent = layer;
                anonLayersToSave.push_back(info);
            } else if (subLayer->IsDirty()) {
                dirtyLayersToSave.push_back(subLayer);
            }
        }
    }

    recursionDetector->pop();
}

void updateMutedLayers(
    const UsdStageRefPtr& stage,
    const SdfLayerRefPtr& oldLayer,
    const SdfLayerRefPtr& newLayer)
{
    if (!stage)
        return;
    if (!oldLayer)
        return;
    if (!newLayer)
        return;

    if (stage->IsLayerMuted(oldLayer->GetIdentifier())) {
        addMutedLayer(newLayer);
        stage->MuteLayer(newLayer->GetIdentifier());
    }
}

void updateLockedLayers(
    const std::string&    proxyPath,
    const SdfLayerRefPtr& oldLayer,
    const SdfLayerRefPtr& newLayer)
{
    if (!oldLayer)
        return;
    if (!newLayer)
        return;

    if (isLayerSystemLocked(oldLayer)) {
        lockLayer(proxyPath, newLayer, LayerLock_SystemLocked);
    } else if (isLayerLocked(oldLayer)) {
        lockLayer(proxyPath, newLayer, LayerLock_Locked);
    }
}

std::vector<PXR_NS::UsdStageCache*> getStageCaches()
{
    if (getStageCachesFunction)
        return getStageCachesFunction();
    return { &PXR_NS::UsdUtilsStageCache::Get() };
}

void updateAllCachedStageWithLayer(SdfLayerRefPtr originalLayer, const std::string& newFilePath)
{
    // Update all known stage caches managed by the Maya USD plugin that contained
    // stages using the original anonymous layer so that new stagesusing new saved
    // layer are created with the load rules and the muted layers of the original
    // stage.
    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(newFilePath);
    if (!newLayer) {
        TF_WARN("The filename %s is an invalid file name for a layer.", newFilePath.c_str());
        return;
    }

    for (PXR_NS::UsdStageCache* cache : getStageCaches()) {
        std::vector<UsdStageRefPtr> stages = cache->FindAllMatching(originalLayer);
        std::vector<UsdStageRefPtr> updatedStages;
        for (auto& stage : stages) {
            auto sessionLayer = stage->GetSessionLayer();
            updatedStages.emplace_back(
                UsdStage::Open(newLayer, sessionLayer, UsdStage::InitialLoadSet::LoadNone));
            cache->Erase(stage);
        }
        for (auto& updatedStage : updatedStages) {
            cache->Insert(updatedStage);
        }
    }
}

std::string suggestedStartFolder(PXR_NS::UsdStageRefPtr stage)
{
    PXR_NS::SdfLayerRefPtr root = stage ? stage->GetRootLayer() : nullptr;
    if (!root && !root->IsAnonymous()) {
        return root->GetRealPath();
    }

    return FileSystem::getDCCSceneFileDir();
}

std::string generateUniqueFileName(const std::string& basename)
{
    std::string newFileName = FileSystem::getUniqueFileName(
        FileSystem::getDCCSceneFileDir(),
        !basename.empty() ? basename : "anonymous",
#if PXR_VERSION >= 2511
        PXR_NS::SdfUsdFileFormatTokens->Id.GetText());
#else
        PXR_NS::UsdUsdFileFormatTokens->Id.GetText());
#endif
    return newFileName;
}

std::string generateUniqueLayerFileName(const std::string& basename, const SdfLayerRefPtr& layer)
{
    std::string layerNumber("1");
    if (layer)
        layerNumber = FileSystem::getNumberSuffix(layer->GetDisplayName());
#if PXR_VERSION >= 2511
    const std::string ext = PXR_NS::SdfUsdFileFormatTokens->Id.GetText();
#else
    const std::string ext = PXR_NS::UsdUsdFileFormatTokens->Id.GetText();
#endif
    const std::string layerFilename = basename + "-layer" + layerNumber + "." + ext;
    const std::string dir = getSceneFolder();

    return FileSystem::ensureUniqueFileName(FileSystem::appendPaths(dir, layerFilename));
}

std::string usdFormatArgOption()
{
    const bool binary = getSaveLayerFormatBinary();
#if PXR_VERSION >= 2511
    return binary ? PXR_NS::SdfUsdcFileFormatTokens->Id.GetText()
                  : PXR_NS::SdfUsdaFileFormatTokens->Id.GetText();
#else
    return binary ? PXR_NS::UsdUsdcFileFormatTokens->Id.GetText()
                  : PXR_NS::UsdUsdaFileFormatTokens->Id.GetText();
#endif
}

/* static */
USDUnsavedEditsOption serializeUsdEditsLocationOption()
{
    int saveOption = getSerializedUsdEditsLocation();
    if (saveOption < kSaveToUSDFiles || saveOption > kIgnoreUSDEdits) {
        saveOption = kSaveToUSDFiles;
        setSerializedUsdEditsLocation(saveOption);
    }

    if (saveOption == kSaveToSceneFile) {
        return kSaveToSceneFile;
    } else if (saveOption == kIgnoreUSDEdits) {
        return kIgnoreUSDEdits;
    } else {
        return kSaveToUSDFiles;
    }
} // namespace MAYAUSD_NS_DEF

 static bool isCompatibleWithSave(
     SdfLayerRefPtr     layer,
     const std::string& filePath,
     const std::string& formatArg)
{
     if (!layer)
         return false;

     // Save cannot specify the filename, so the file name must match to use save.
     if (layer->GetRealPath() != filePath)
         return false;

#if PXR_VERSION >= 2511
     const TfToken underlyingFormat = SdfUsdFileFormat::GetUnderlyingFormatForLayer(*layer);
#else
     const TfToken underlyingFormat = UsdUsdFileFormat::GetUnderlyingFormatForLayer(*layer);
#endif
     if (underlyingFormat.size()) {
         return underlyingFormat == formatArg;
     } else {
         const SdfFileFormat::FileFormatArguments currentFormatArgs
             = layer->GetFileFormatArguments();

         // If we cannot find the format argument then we cannot validate that the file format
         // match so we err to the side of safety and claim they don't match.
         const auto keyAndValue = currentFormatArgs.find("format");
         if (keyAndValue == currentFormatArgs.end())
             return false;

         return keyAndValue->second == formatArg;
     }
 }


 void setLayerUpAxisAndUnits(const SdfLayerRefPtr& layer)
 {
     if (!layer || !layer->PermissionToEdit())
         return;
     if (layerUpAxisAndUnitsFn)
         layerUpAxisAndUnitsFn(layer);
 }

bool saveLayerWithFormat(
    SdfLayerRefPtr     layer,
    const std::string& requestedFilePath,
    const std::string& requestedFormatArg)
{
     const std::string& filePath
         = requestedFilePath.empty() ? layer->GetRealPath() : requestedFilePath;

     const std::string& formatArg
         = requestedFormatArg.empty() ? usdFormatArgOption() : requestedFormatArg;

     FileSystem::updatePostponedRelativePaths(layer, filePath);

     if (isCompatibleWithSave(layer, filePath, formatArg)) {
         if (!layer->Save()) {
             return false;
         }
     } else {
         PXR_NS::SdfFileFormat::FileFormatArguments args;
#if PXR_VERSION >= 2511
         args[SdfUsdFileFormatTokens->FormatArg] = formatArg;
#else
         args[UsdUsdFileFormatTokens->FormatArg] = formatArg;
#endif
         if (!layer->Export(filePath, "", args)) {
             return false;
         }
     }

     updateAllCachedStageWithLayer(layer, filePath);

    return true;
}

std::string getSceneFolder()
{
    std::string fileDir = FileSystem::getDCCSceneFileDir();
    if (fileDir.empty()) {
        fileDir = FileSystem::getDCCWorkspaceScenesDir();
    }

    return fileDir;
}


void updateTargetLayer(const std::string& proxyNodeName, const SdfLayerRefPtr& layer)
{
    auto stage = UsdUfe::getStage(Ufe::PathString::path(proxyNodeName));
    if (!stage) {
        return;
    }
    stage->SetEditTarget(layer);
}

void updateRootLayer(
    const std::string& proxy,
    const std::string& layerPath,
    UsdStageRefPtr  stage,
    const SdfLayerRefPtr& layer,
    bool                          isTargetLayer)
{
    if (updateDCCObjectRootLayerFunction)
        updateDCCObjectRootLayerFunction(proxy, layerPath, layer, isTargetLayer);
}

 SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr     stage,
    SdfLayerRefPtr     anonLayer,
    LayerParent        parent,
    const std::string& basename,
    std::string        formatArg,
    std::string*       errorMsg)
{
    PathInfo pathInfo;
    pathInfo.absolutePath = generateUniqueLayerFileName(basename, anonLayer);
    return saveAnonymousLayer(stage, anonLayer, pathInfo, parent, formatArg, errorMsg);
}

 SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr  stage,
    SdfLayerRefPtr  anonLayer,
    const PathInfo& pathInfo,
    LayerParent     parent,
    std::string     formatArg,
    std::string*    errorMsg)
{
    FileSystem::FileBackup backup(pathInfo.absolutePath);
    std::string                       filePath(pathInfo.absolutePath);

    if (!anonLayer) {
        TF_ERROR(NoAnonLayerProvided, "No layer provided to save to '%s'", filePath.c_str());
        return nullptr;
    }

    if (!anonLayer->IsAnonymous()) {
        TF_ERROR(CannotSaveNonAnonLayer, "Cannot save non-anonymous layer '%s' under a different file name", anonLayer->GetDisplayName().c_str());
        return nullptr;
    }

    if (isLayerSystemLocked(anonLayer)) {
        TF_ERROR(CannotSaveAnonLayerWhenSysLocked, "Cannot save layer '%s' when system-locked", anonLayer->GetDisplayName().c_str());
        return nullptr;
    }

    // Only set up-axis and units metadata on the root layer
    // and only if it is anonymous before being saved.
    if (stage->GetRootLayer() == anonLayer) {
        setLayerUpAxisAndUnits(anonLayer);
    }

    ensureUSDFileExtension(filePath);

    const bool wasTargetLayer = (stage->GetEditTarget().GetLayer() == anonLayer);

    if (!saveLayerWithFormat(anonLayer, filePath, formatArg)) {
        TF_ERROR(FailedAnonLayerSave, "Failed to save layer '%s' to '%s'", anonLayer->GetDisplayName().c_str(), filePath.c_str());
        return nullptr;
    }

    auto       parentLayer = parent._layerParent;
    const bool isSubLayer = (parentLayer != nullptr);

    if (pathInfo.savePathAsRelative) {
        if (!pathInfo.customRelativeAnchor.empty()) {
            std::string relativePathAnchor = pathInfo.customRelativeAnchor;
            filePath
                = FileSystem::makePathRelativeTo(filePath, relativePathAnchor).first;
        } else if (isSubLayer) {
            filePath = FileSystem::getPathRelativeToLayerFile(filePath, parentLayer);
            if (std::filesystem::path(filePath).is_absolute()) {
                FileSystem::markPathAsPostponedRelative(parentLayer, filePath);
            }
        } else {
            filePath = FileSystem::getPathRelativeToDCCSceneFile(filePath);
        }
    } else {
        if (isSubLayer) {
            FileSystem::unmarkPathAsPostponedRelative(parentLayer, filePath);
        }
    }

    // Note: we need to open the layer with the absolute path. The relative path is only
    //       used by the parent layer to refer to the sub-layer relative to itself. When
    //       opening the layer in isolation, we need to use the absolute path. Failure to
    //       do so will make finding the layer by its own identifier fail! A symptom of
    //       this failure is that drag-and-drop in the Layer Manager UI fails immediately
    //       after saving a layer with a relative path.
    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(pathInfo.absolutePath);

    if (!newLayer) {
        TF_ERROR(FailedAnonLayerReload, "Failed to reload layer '%s' from '%s'", anonLayer->GetDisplayName().c_str(), filePath.c_str());
        return nullptr;
    }

    // Now replace the layer in the parent, using a relative path if requested.
    if (isSubLayer) {
        updateSubLayer(parentLayer, anonLayer, filePath);
    } else if (!parent._objectPath.empty()) {
        // if ever we support relative paths in the DCC, can return the relative path
        // i.e. "filePath" variable
        if (updateDCCObjectRootLayerFunction)
            updateDCCObjectRootLayerFunction(
                parent._objectPath, pathInfo.absolutePath, newLayer, wasTargetLayer);
    }

    updateTargetLayer(parent._objectPath, newLayer);
    updateMutedLayers(stage, anonLayer, newLayer);
    updateLockedLayers(parent._objectPath, anonLayer, newLayer);

    backup.commit();

    return newLayer;
}

 void updateSubLayer(
    const SdfLayerRefPtr& parentLayer,
    const SdfLayerRefPtr& oldSubLayer,
    const std::string&    newSubLayerPath)
{
    if (!parentLayer)
        return;

    if (!oldSubLayer)
        return;

    // Note: we don't know if the old sub-layer was referenced with an absolute
    //       or relative path, so we try replacing both and its identifier.
    SdfSubLayerProxy subLayers = parentLayer->GetSubLayerPaths();

    subLayers.Replace(oldSubLayer->GetIdentifier(), newSubLayerPath);

    const std::string oldAbsPath = oldSubLayer->GetRealPath();
    if (oldAbsPath.length() > 0) {
        subLayers.Replace(oldAbsPath, newSubLayerPath);

        const std::string oldRelPath
            = FileSystem::getPathRelativeToLayerFile(oldAbsPath, parentLayer);
        subLayers.Replace(oldRelPath, newSubLayerPath);
    }
}

 void ensureUSDFileExtension(std::string& filePath)
{
    const std::string& extension = SdfFileFormat::GetFileExtension(filePath);
    const std::string  defaultExt("usd");
    const std::string  usdCrateExt("usdc");
    const std::string  usdASCIIExt("usda");
    const std::string  usdPackageExt("usdz");
    if (extension != defaultExt && extension != usdCrateExt && extension != usdASCIIExt
        && extension != usdPackageExt) {
        filePath.append(".");
        filePath.append(defaultExt.c_str());
    }
}

void getLayersToSaveFromStage(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            objectPath,
    StageLayersToSave&            layersInfo)
{
    if (!stage) {
        return;
    }

    auto root = stage->GetRootLayer();
    populateChildren(
        objectPath,
        stage,
        root,
        nullptr,
        layersInfo._anonLayers,
        layersInfo._dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        LayerInfo info;
        info.stage = stage;
        info.layer = root;
        info.parent._objectPath = objectPath;
        info.parent._layerParent = nullptr;
        layersInfo._anonLayers.push_back(info);
    } else if (root->IsDirty()) {
        layersInfo._dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(
        objectPath,
        stage,
        session,
        nullptr,
        layersInfo._anonLayers,
        layersInfo._dirtyFileBackedLayers);
}

void getLayersToSaveFromDCCObject(const std::string& objectPath, StageLayersToSave& layersInfo)
{
    auto stage = UsdUfe::getStage(Ufe::PathString::path(objectPath));
    if (!stage) {
        return;
    }
    getLayersToSaveFromStage(stage, objectPath, layersInfo);
}

} // namespace Serialization
} // namespace UsdLayerEditor