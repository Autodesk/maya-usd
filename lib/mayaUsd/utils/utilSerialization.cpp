//
// Copyright 2021 Autodesk
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
#include "utilSerialization.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/targetLayer.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <string>

namespace {

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
    const std::string&           proxyPath,
    const UsdStageRefPtr&        stage,
    SdfLayerRefPtr               layer,
    RecursionDetector*           recursionDetector,
    MayaUsd::utils::LayerInfos&  anonLayersToSave,
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
                proxyPath, stage, subLayer, recursionDetector, anonLayersToSave, dirtyLayersToSave);

            if (subLayer->IsAnonymous()) {
                MayaUsd::utils::LayerInfo info;
                info.stage = stage;
                info.layer = subLayer;
                info.parent._proxyPath = proxyPath;
                info.parent._layerParent = layer;
                anonLayersToSave.push_back(info);
            } else if (subLayer->IsDirty()) {
                dirtyLayersToSave.push_back(subLayer);
            }
        }
    }

    recursionDetector->pop();
}

void updateTargetLayer(const std::string& proxyNodeName, const SdfLayerRefPtr& layer)
{
    if (MayaUsdProxyShapeBase* proxyShape = UsdMayaUtil::GetProxyShapeByProxyName(proxyNodeName)) {
        proxyShape->getUsdStage()->SetEditTarget(layer);
    }
}

void updateRootLayer(
    const std::string&    proxy,
    const std::string&    layerPath,
    const SdfLayerRefPtr& layer,
    bool                  isTargetLayer)
{
    // Upda the root layer of the given proxy shape
    if (layerPath.empty() || proxy.empty())
        return;

#ifdef _WIN32
    // Building a string that includes a file path for an executeCommand call
    // can be problematic, easier to just switch the path separator.
    const std::string fp = TfStringReplace(layerPath, "\\", "/");
#else
    const std::string& fp = layerPath;
#endif

    MayaUsd::utils::setNewProxyPath(
        MString(proxy.c_str()), MString(fp.c_str()), layer, isTargetLayer);
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
    for (UsdStageCache& cache : UsdMayaStageCache::GetAllCaches()) {
        UsdStageCacheContext        ctx(cache);
        std::vector<UsdStageRefPtr> stages = cache.FindAllMatching(originalLayer);
        for (const auto& stage : stages) {
            auto sessionLayer = stage->GetSessionLayer();
            auto newStage = UsdStage::UsdStage::Open(
                newLayer, sessionLayer, UsdStage::InitialLoadSet::LoadNone);
            newStage->SetLoadRules(stage->GetLoadRules());
            newStage->MuteAndUnmuteLayers(stage->GetMutedLayers(), {});
        }
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace utils {

std::string suggestedStartFolder(PXR_NS::UsdStageRefPtr stage)
{
    PXR_NS::SdfLayerRefPtr root = stage ? stage->GetRootLayer() : nullptr;
    if (!root && !root->IsAnonymous()) {
        return root->GetRealPath();
    }

    return getSceneFolder();
}

std::string getSceneFolder()
{
    std::string fileDir = UsdMayaUtilFileSystem::getMayaSceneFileDir();
    if (fileDir.empty()) {
        fileDir = UsdMayaUtilFileSystem::getMayaWorkspaceScenesDir();
    }

    return fileDir;
}

std::string generateUniqueFileName(const std::string& basename)
{
    std::string newFileName = UsdMayaUtilFileSystem::getUniqueFileName(
        getSceneFolder(),
        !basename.empty() ? basename : "anonymous",
        PXR_NS::UsdUsdFileFormatTokens->Id.GetText());
    return newFileName;
}

std::string generateUniqueLayerFileName(const std::string& basename, const SdfLayerRefPtr& layer)
{
    std::string layerNumber("1");
    if (layer)
        layerNumber = UsdMayaUtilFileSystem::getNumberSuffix(layer->GetDisplayName());

    const std::string ext = PXR_NS::UsdUsdFileFormatTokens->Id.GetText();
    const std::string layerFilename = basename + "-layer" + layerNumber + "." + ext;
    const std::string dir = getSceneFolder();

    return UsdMayaUtilFileSystem::ensureUniqueFileName(
        UsdMayaUtilFileSystem::appendPaths(dir, layerFilename));
}

std::string usdFormatArgOption()
{
    static const MString kSaveLayerFormatBinaryOption(
        MayaUsdOptionVars->SaveLayerFormatArgBinaryOption.GetText());
    bool binary = true;
    if (MGlobal::optionVarExists(kSaveLayerFormatBinaryOption)) {
        binary = MGlobal::optionVarIntValue(kSaveLayerFormatBinaryOption) != 0;
    } else {
        MGlobal::setOptionVarValue(kSaveLayerFormatBinaryOption, 1);
    }
    return binary ? PXR_NS::UsdUsdcFileFormatTokens->Id.GetText()
                  : PXR_NS::UsdUsdaFileFormatTokens->Id.GetText();
}

/* static */
USDUnsavedEditsOption serializeUsdEditsLocationOption()
{
    static const MString kSerializedUsdEditsLocation(
        MayaUsdOptionVars->SerializedUsdEditsLocation.GetText());

    bool optVarExists = true;
    int  saveOption = MGlobal::optionVarIntValue(kSerializedUsdEditsLocation, &optVarExists);

    // Default is to save back to .usd files, set it to that if the optionVar doesn't exist yet.
    // optionVar's are also just ints so make sure the value is a correct one.
    // If we end up initializing the value then write it back to the optionVar itself.
    if (!optVarExists || (saveOption < kSaveToUSDFiles || saveOption > kIgnoreUSDEdits)) {
        saveOption = kSaveToUSDFiles;
        MGlobal::setOptionVarValue(kSerializedUsdEditsLocation, saveOption);
    }

    if (saveOption == kSaveToMayaSceneFile) {
        return kSaveToMayaSceneFile;
    } else if (saveOption == kIgnoreUSDEdits) {
        return kIgnoreUSDEdits;
    } else {
        return kSaveToUSDFiles;
    }
} // namespace MAYAUSD_NS_DEF

void setNewProxyPath(
    const MString&        proxyNodeName,
    const MString&        newRootLayerPath,
    const SdfLayerRefPtr& layer,
    bool                  isTargetLayer)
{
    const bool  needRelativePath = UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
    const char* filePathCmd = "setAttr -type \"string\" ^1s.filePath \"^2s\"; "
                              "setAttr ^1s.filePathRelative ^3s; ";

    MString script;
    script.format(filePathCmd, proxyNodeName, newRootLayerPath, needRelativePath ? "1" : "0");
    MGlobal::executeCommand(
        script,
        /*display*/ true,
        /*undo*/ false);

    if (isTargetLayer)
        updateTargetLayer(proxyNodeName.asChar(), layer);
}

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

    const TfToken underlyingFormat = UsdUsdFileFormat::GetUnderlyingFormatForLayer(*layer);
    if (underlyingFormat.size()) {
        return underlyingFormat == formatArg;
    } else {
        const SdfFileFormat::FileFormatArguments currentFormatArgs
            = layer->GetFileFormatArguments();

        // If we cannot find the format argument then we cannot validate that the file format match
        // so we err to the side of safety and claim they don't match.
        const auto keyAndValue = currentFormatArgs.find("format");
        if (keyAndValue == currentFormatArgs.end())
            return false;

        return keyAndValue->second == formatArg;
    }
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

    if (isCompatibleWithSave(layer, filePath, formatArg)) {
        if (!layer->Save()) {
            return false;
        }
    } else {
        PXR_NS::SdfFileFormat::FileFormatArguments args;
        args["format"] = formatArg;
        if (!layer->Export(filePath, "", args)) {
            return false;
        }
    }

    updateAllCachedStageWithLayer(layer, filePath);

    return true;
}

SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr     stage,
    SdfLayerRefPtr     anonLayer,
    LayerParent        parent,
    const std::string& basename,
    std::string        formatArg)
{
    PathInfo pathInfo;
    pathInfo.absolutePath = generateUniqueLayerFileName(basename, anonLayer);
    return saveAnonymousLayer(stage, anonLayer, pathInfo, parent, formatArg);
}

SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr  stage,
    SdfLayerRefPtr  anonLayer,
    const PathInfo& pathInfo,
    LayerParent     parent,
    std::string     formatArg)
{
    if (!anonLayer || !anonLayer->IsAnonymous()) {
        return nullptr;
    }

    std::string filePath(pathInfo.absolutePath);
    ensureUSDFileExtension(filePath);

    const bool wasTargetLayer = (stage->GetEditTarget().GetLayer() == anonLayer);

    if (!saveLayerWithFormat(anonLayer, filePath, formatArg)) {
        return nullptr;
    }

    const bool  isSubLayer = (parent._layerParent != nullptr);
    std::string relativePathAnchor;

    if (pathInfo.savePathAsRelative) {
        if (!pathInfo.customRelativeAnchor.empty()) {
            relativePathAnchor = pathInfo.customRelativeAnchor;
            filePath
                = UsdMayaUtilFileSystem::makePathRelativeTo(filePath, relativePathAnchor).first;
        } else if (isSubLayer) {
            filePath
                = UsdMayaUtilFileSystem::getPathRelativeToLayerFile(filePath, parent._layerParent);
            relativePathAnchor = UsdMayaUtilFileSystem::getLayerFileDir(parent._layerParent);
        } else {
            filePath = UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(filePath);
            relativePathAnchor = UsdMayaUtilFileSystem::getMayaSceneFileDir();
        }
    }

    // When the filePath was made relative, we need to help FindOrOpen to locate
    //      the sub-layers when using relative paths. We temporarily chande the
    //      current directory to the location the file path is relative to.
    UsdMayaUtilFileSystem::TemporaryCurrentDir tempCurDir(relativePathAnchor);
    SdfLayerRefPtr                             newLayer = SdfLayer::FindOrOpen(filePath);
    tempCurDir.restore();

    if (newLayer) {
        if (isSubLayer) {
            updateSubLayer(parent._layerParent, anonLayer, filePath);
            updateTargetLayer(parent._proxyPath, newLayer);
        } else if (!parent._proxyPath.empty()) {
            updateRootLayer(parent._proxyPath, filePath, newLayer, wasTargetLayer);
        }
    }

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
    subLayers.Replace(oldAbsPath, newSubLayerPath);

    const std::string oldRelPath
        = UsdMayaUtilFileSystem::getPathRelativeToLayerFile(oldAbsPath, parentLayer);
    subLayers.Replace(oldRelPath, newSubLayerPath);
}

void ensureUSDFileExtension(std::string& filePath)
{
    const std::string& extension = SdfFileFormat::GetFileExtension(filePath);
    const std::string  defaultExt(UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText());
    const std::string  usdCrateExt(UsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText());
    const std::string  usdASCIIExt(UsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText());
    if (extension != defaultExt && extension != usdCrateExt && extension != usdASCIIExt) {
        filePath.append(".");
        filePath.append(defaultExt.c_str());
    }
}

void getLayersToSaveFromProxy(const std::string& proxyPath, StageLayersToSave& layersInfo)
{
    auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);
    if (!stage) {
        return;
    }

    auto root = stage->GetRootLayer();
    populateChildren(
        proxyPath, stage, root, nullptr, layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        LayerInfo info;
        info.stage = stage;
        info.layer = root;
        info.parent._proxyPath = proxyPath;
        info.parent._layerParent = nullptr;
        layersInfo._anonLayers.push_back(info);
    } else if (root->IsDirty()) {
        layersInfo._dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(
        proxyPath,
        stage,
        session,
        nullptr,
        layersInfo._anonLayers,
        layersInfo._dirtyFileBackedLayers);
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
