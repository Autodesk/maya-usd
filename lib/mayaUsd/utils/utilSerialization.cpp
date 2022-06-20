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
#include <mayaUsd/utils/stageCache.h>
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
    SdfLayerRefPtr                                                       layer,
    RecursionDetector*                                                   recursionDetector,
    std::vector<std::pair<SdfLayerRefPtr, MayaUsd::utils::LayerParent>>& anonLayersToSave,
    std::vector<SdfLayerRefPtr>&                                         dirtyLayersToSave)
{
    auto subPaths = layer->GetSubLayerPaths();

    RecursionDetector defaultDetector;
    if (!recursionDetector) {
        recursionDetector = &defaultDetector;
    }
    recursionDetector->push(layer->GetRealPath());

    for (auto const path : subPaths) {
        std::string actualPath = PXR_NS::SdfComputeAssetPathRelativeToLayer(layer, path);
        auto        subLayer = PXR_NS::SdfLayer::FindOrOpen(actualPath);
        if (subLayer && !recursionDetector->contains(subLayer->GetRealPath())) {
            populateChildren(subLayer, recursionDetector, anonLayersToSave, dirtyLayersToSave);

            if (subLayer->IsAnonymous()) {
                MayaUsd::utils::LayerParent p;
                p._proxyPath.clear();
                p._layerParent = layer;
                anonLayersToSave.push_back(std::make_pair(subLayer, p));
            } else if (subLayer->IsDirty()) {
                dirtyLayersToSave.push_back(subLayer);
            }
        }
    }

    recursionDetector->pop();
}

bool saveRootLayer(SdfLayerRefPtr layer, const std::string& proxy)
{
    if (!layer || proxy.empty() || layer->IsAnonymous()) {
        return false;
    }

    std::string fp = layer->GetRealPath();
#ifdef _WIN32
    // Building a string that includes a file path for an executeCommand call
    // can be problematic, easier to just switch the path separator.
    fp = TfStringReplace(fp, "\\", "/");
#endif

    MayaUsd::utils::setNewProxyPath(MString(proxy.c_str()), MString(fp.c_str()));

    return true;
}

void updateAllCachedStageWithLayer(SdfLayerRefPtr originalLayer, const std::string& newFilePath)
{
    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(newFilePath);
    for (UsdStageCache& cache : UsdMayaStageCache::GetAllCaches()) {
        UsdStageCacheContext        ctx(cache);
        std::vector<UsdStageRefPtr> stages = cache.FindAllMatching(originalLayer);
        for (const auto& stage : stages) {
            auto sessionLayer = stage->GetSessionLayer();
            auto newStage = UsdStage::UsdStage::Open(
                newLayer, sessionLayer, UsdStage::InitialLoadSet::LoadNone);
            newStage->SetLoadRules(stage->GetLoadRules());
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

void setNewProxyPath(const MString& proxyNodeName, const MString& newValue)
{
    MString script;
    script.format("setAttr -type \"string\" ^1s.filePath \"^2s\"", proxyNodeName, newValue);
    MGlobal::executeCommand(
        script,
        /*display*/ true,
        /*undo*/ false);
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
    SdfLayerRefPtr     anonLayer,
    LayerParent        parent,
    const std::string& basename,
    std::string        formatArg)
{
    std::string newFileName = generateUniqueFileName(basename);
    return saveAnonymousLayer(anonLayer, newFileName, parent, formatArg);
}

SdfLayerRefPtr saveAnonymousLayer(
    SdfLayerRefPtr     anonLayer,
    const std::string& path,
    LayerParent        parent,
    std::string        formatArg)
{
    if (!anonLayer || !anonLayer->IsAnonymous()) {
        return nullptr;
    }

    saveLayerWithFormat(anonLayer, path, formatArg);

    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(path);
    if (newLayer) {
        if (parent._layerParent) {
            parent._layerParent->GetSubLayerPaths().Replace(
                anonLayer->GetIdentifier(), newLayer->GetIdentifier());
        } else if (!parent._proxyPath.empty()) {
            saveRootLayer(newLayer, parent._proxyPath);
        }
    }
    return newLayer;
}

void getLayersToSaveFromProxy(const std::string& proxyPath, stageLayersToSave& layersInfo)
{
    auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);
    if (!stage) {
        return;
    }

    auto root = stage->GetRootLayer();
    populateChildren(root, nullptr, layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        LayerParent p;
        p._proxyPath = proxyPath;
        p._layerParent = nullptr;
        layersInfo._anonLayers.push_back(std::make_pair(root, p));
    } else if (root->IsDirty()) {
        layersInfo._dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(session, nullptr, layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
