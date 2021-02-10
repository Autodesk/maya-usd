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
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>

#include <maya/MGlobal.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

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

namespace {

std::string toForwardSlashes(const std::string& in_path)
{
    // it works better on windows if all the paths have forward slashes
    auto path = in_path;
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

// contains the logic to get the right path to use for SdfLayer:::FindOrOpen
// from a sublayerpath.
// this path could be absolute, relative, or be an anon layer
std::string computePathToLoadSublayer(
    const std::string&  subLayerPath,
    const std::string&  anchor,
    PXR_NS::ArResolver& resolver)
{
    std::string actualPath = subLayerPath;
    if (resolver.IsRelativePath(subLayerPath)) {
        auto subLayer = PXR_NS::SdfLayer::Find(subLayerPath); // note: finds in the cache
        if (subLayer) {
            if (!resolver.IsRelativePath(subLayer->GetIdentifier())) {
                actualPath = toForwardSlashes(subLayer->GetRealPath());
            }
        } else {
            actualPath = resolver.AnchorRelativePath(anchor, subLayerPath);
        }
    }
    return actualPath;
}

void populateChildren(
    SdfLayerRefPtr                                          layer,
    RecursionDetector*                                      recursionDetector,
    std::vector<std::pair<SdfLayerRefPtr, SdfLayerRefPtr>>& anonLayersToSave,
    std::vector<SdfLayerRefPtr>&                            dirtyLayersToSave)
{
    auto  subPaths = layer->GetSubLayerPaths();
    auto& resolver = ArGetResolver();
    auto  anchor = toForwardSlashes(layer->GetRealPath());

    RecursionDetector defaultDetector;
    if (!recursionDetector) {
        recursionDetector = &defaultDetector;
    }
    recursionDetector->push(layer->GetRealPath());

    for (auto const path : subPaths) {
        std::string actualPath = computePathToLoadSublayer(path, anchor, resolver);
        auto        subLayer = SdfLayer::FindOrOpen(actualPath);
        if (subLayer && !recursionDetector->contains(subLayer->GetRealPath())) {
            populateChildren(subLayer, recursionDetector, anonLayersToSave, dirtyLayersToSave);

            if (subLayer->IsAnonymous()) {
                anonLayersToSave.push_back(std::make_pair(subLayer, layer));
            } else if (subLayer->IsDirty()) {
                dirtyLayersToSave.push_back(subLayer);
            }
        }
    }

    recursionDetector->pop();
}

} // namespace

std::string UsdMayaSerialization::suggestedStartFolder(UsdStageRefPtr stage)
{
    SdfLayerRefPtr root = stage ? stage->GetRootLayer() : nullptr;
    if (!root && !root->IsAnonymous()) {
        return root->GetRealPath();
    }

    return UsdMayaSerialization::getSceneFolder();
}

std::string UsdMayaSerialization::getSceneFolder()
{
    std::string fileDir = UsdMayaUtilFileSystem::getMayaSceneFileDir();
    if (fileDir.empty()) {
        fileDir = UsdMayaUtilFileSystem::getMayaWorkspaceScenesDir();
    }

    return fileDir;
}

std::string UsdMayaSerialization::generateUniqueFileName(const std::string& basename)
{
    std::string newFileName = UsdMayaUtilFileSystem::getUniqueFileName(
        getSceneFolder(),
        !basename.empty() ? basename : "anonymous",
        UsdUsdFileFormatTokens->Id.GetText());
    return newFileName;
}

std::string UsdMayaSerialization::usdFormatArgOption()
{
    static const MString kSaveLayerFormatBinaryOption(
        MayaUsdOptionVars->SaveLayerFormatArgBinaryOption.GetText());
    bool binary = true;
    if (MGlobal::optionVarExists(kSaveLayerFormatBinaryOption)) {
        binary = MGlobal::optionVarIntValue(kSaveLayerFormatBinaryOption) != 0;
    } else {
        MGlobal::setOptionVarValue(kSaveLayerFormatBinaryOption, 1);
    }
    return binary ? UsdUsdcFileFormatTokens->Id.GetText() : UsdUsdaFileFormatTokens->Id.GetText();
}

/* static */
UsdMayaSerialization::USDUnsavedEditsOption UsdMayaSerialization::serializeUsdEditsLocationOption()
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

SdfLayerRefPtr UsdMayaSerialization::saveAnonymousLayer(
    SdfLayerRefPtr     anonLayer,
    SdfLayerRefPtr     parentLayer,
    const std::string& basename,
    std::string        formatArg)
{
    std::string newFileName = UsdMayaSerialization::generateUniqueFileName(basename);
    return saveAnonymousLayer(anonLayer, newFileName, parentLayer, formatArg);
}

SdfLayerRefPtr UsdMayaSerialization::saveAnonymousLayer(
    SdfLayerRefPtr     anonLayer,
    const std::string& path,
    SdfLayerRefPtr     parentLayer,
    std::string        formatArg)
{
    if (!anonLayer || !anonLayer->IsAnonymous()) {
        return nullptr;
    }

    PXR_NS::SdfFileFormat::FileFormatArguments args;
    if (!formatArg.empty()) {
        args["format"] = formatArg;
    } else {
        args["format"] = usdFormatArgOption();
    }

    anonLayer->Export(path, "", args);

    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(path);

    if (parentLayer) {
        parentLayer->GetSubLayerPaths().Replace(
            anonLayer->GetIdentifier(), newLayer->GetIdentifier());
    }

    return newLayer;
}

void UsdMayaSerialization::getLayersToSaveFromProxy(
    UsdStageRefPtr     stage,
    stageLayersToSave& layersInfo)
{
    auto root = stage->GetRootLayer();

    populateChildren(root, nullptr, layersInfo.anonLayers, layersInfo.dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        layersInfo.anonLayers.push_back(std::make_pair(root, nullptr));
    } else if (root->IsDirty()) {
        layersInfo.dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(session, nullptr, layersInfo.anonLayers, layersInfo.dirtyFileBackedLayers);
}
