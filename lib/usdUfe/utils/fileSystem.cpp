//
// Copyright 2026 Autodesk
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
#include "fileSystem.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>
#include <pxr/usd/sdf/variantSetSpec.h>
#include <pxr/usd/sdf/variantSpec.h>

#include <ghc/fs_std.hpp>

#include <map>
#include <set>

namespace USDUFE_NS_DEF {
namespace FileSystem {

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct PostponedRelativeInfo
{
    std::set<fs::filesystem::path> paths;
    std::set<TfToken>              attrs;
};

using PostponedRelativePaths = std::map<SdfLayerHandle, PostponedRelativeInfo>;

PostponedRelativePaths& getPostponedRelativePaths()
{
    static PostponedRelativePaths sPaths;
    return sPaths;
}

template <typename TypePolicy>
void updatePathList(
    SdfListProxy<TypePolicy>                list,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr)
{
    for (auto proxy : list) {
        typename TypePolicy::value_type item = proxy;
        fs::filesystem::path            filePath(item.GetAssetPath());
        filePath = filePath.lexically_normal();

        auto it = layerEntry->second.paths.find(filePath);
        if (it == layerEntry->second.paths.end()) {
            continue;
        }

        item.SetAssetPath(getPathRelativeToDirectory(filePath.generic_string(), anchorDirStr));
        proxy = item;
    }
}

void updatePostponedRelativePathsForPrim(
    const SdfPrimSpecHandle&                primSpec,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr);

void updatePathsInVariantSets(
    const SdfVariantSetsProxy&              variantSets,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr)
{
    for (const SdfVariantSetsProxy::value_type& variantSet : variantSets) {
        for (const SdfVariantSpecHandle& variantSpec : variantSet.second->GetVariantList()) {
            updatePostponedRelativePathsForPrim(
                variantSpec->GetPrimSpec(), layerEntry, anchorDirStr);
            updatePathsInVariantSets(variantSpec->GetVariantSets(), layerEntry, anchorDirStr);
        }
    }
}

void updatePostponedRelativePathsForPrim(
    const SdfPrimSpecHandle&                primSpec,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr)
{
    for (const SdfPrimSpecHandle& child : primSpec->GetNameChildren()) {
        if (child->HasPayloads()) {
            auto payloadList = child->GetPayloadList();
            updatePathList(payloadList.GetExplicitItems(), layerEntry, anchorDirStr);
            updatePathList(payloadList.GetAddedItems(), layerEntry, anchorDirStr);
            updatePathList(payloadList.GetPrependedItems(), layerEntry, anchorDirStr);
            updatePathList(payloadList.GetAppendedItems(), layerEntry, anchorDirStr);
        }

        if (child->HasReferences()) {
            auto referenceList = child->GetReferenceList();
            updatePathList(referenceList.GetExplicitItems(), layerEntry, anchorDirStr);
            updatePathList(referenceList.GetAddedItems(), layerEntry, anchorDirStr);
            updatePathList(referenceList.GetPrependedItems(), layerEntry, anchorDirStr);
            updatePathList(referenceList.GetAppendedItems(), layerEntry, anchorDirStr);
        }

        for (auto attrPath : layerEntry->second.attrs) {
            auto attr = child->GetAttributes()[attrPath];
            if (!attr || !attr->HasDefaultValue()
                || attr->GetValueType() != TfType::Find<SdfAssetPath>()) {
                continue;
            }

            VtValue              filePathValue = attr->GetDefaultValue();
            auto                 filePathStr = filePathValue.Get<SdfAssetPath>().GetAssetPath();
            fs::filesystem::path filePath(filePathStr);
            auto                 it = layerEntry->second.paths.find(filePath);
            if (it == layerEntry->second.paths.end()) {
                continue;
            }

            std::string relativePath
                = getPathRelativeToDirectory(filePath.generic_string(), anchorDirStr);
            filePathValue = SdfAssetPath(relativePath);
            attr->SetDefaultValue(filePathValue);
        }

        updatePostponedRelativePathsForPrim(child, layerEntry, anchorDirStr);
        updatePathsInVariantSets(child->GetVariantSets(), layerEntry, anchorDirStr);
    }
}

} // namespace

std::string getDir(const std::string& fullFilePath)
{
    return fs::filesystem::path(fullFilePath).parent_path().string();
}

std::string getLayerFileDir(const SdfLayerHandle& layer)
{
    if (!layer)
        return std::string();

    const std::string layerFileName = layer->GetRealPath();
    if (layerFileName.empty())
        return std::string();

    return getDir(layerFileName);
}

std::pair<std::string, bool>
makePathRelativeTo(const std::string& fileName, const std::string& relativeToDir)
{
    fs::filesystem::path absolutePath(fileName);

    // If the anchor relative-to-directory doesn't exist yet, use the unchanged path,
    // but don't return a failure. The anchor path being empty is not considered
    // a failure. If the caller needs to detect this, they can verify that the
    // anchor path is empty themselves before calling this function.
    if (relativeToDir.empty()) {
        return std::make_pair(fileName, true);
    }

    // Paths that are already relative (and thus unresolvable) short-circuit to failure.
    if (!absolutePath.is_absolute()) {
        return std::make_pair(fileName, false);
    }

    fs::filesystem::path relativePath = fs::filesystem::relative(absolutePath, relativeToDir);

    if (relativePath.empty()) {
        return std::make_pair(fileName, false);
    }

    return std::make_pair(relativePath.generic_string(), true);
}

std::string
getPathRelativeToDirectory(const std::string& fileName, const std::string& relativeToDir)
{
    auto relativePathAndSuccess = makePathRelativeTo(fileName, relativeToDir);

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to its parent layer directory (%s), "
            "using the absolute path.",
            fileName.c_str(),
            relativeToDir.c_str());
    }

    return relativePathAndSuccess.first;
}

std::string getPathRelativeToLayerFile(const std::string& fileName, const SdfLayerHandle& layer)
{
    if (!layer)
        return fileName;

    const std::string layerDirPath = getLayerFileDir(layer);
    if (layerDirPath.empty()) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative since its parent layer is not saved,"
            " using the absolute path instead.",
            fileName.c_str());

        return fileName;
    }

    auto relativePathAndSuccess = makePathRelativeTo(fileName, layerDirPath);

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to its parent layer directory (%s), "
            "using the absolute path instead.",
            fileName.c_str(),
            layerDirPath.c_str());
    }

    return relativePathAndSuccess.first;
}

void markPathAsPostponedRelative(const SdfLayerHandle& layer, const std::string& contentPath)
{
    fs::filesystem::path filePath(contentPath);
    auto&                postponedRelativePaths = getPostponedRelativePaths();
    postponedRelativePaths[layer].paths.insert(filePath.lexically_normal());
}

void markAttributeAsPostponedRelative(const SdfLayerHandle& layer, const std::string& attrName)
{
    getPostponedRelativePaths()[layer].attrs.insert(TfToken(attrName));
}

void unmarkPathAsPostponedRelative(const SdfLayerHandle& layer, const std::string& contentPath)
{
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry != postponedRelativePaths.end()) {
        fs::filesystem::path filePath(contentPath);
        layerEntry->second.paths.erase(filePath.lexically_normal());
    }
}

void updatePostponedRelativePaths(const SdfLayerHandle& layer, const std::string& layerFileName)
{
    // Find the layer entry
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry == postponedRelativePaths.end()) {
        return;
    }

    auto anchorDir = fs::filesystem::path(layerFileName).lexically_normal().remove_filename();
    auto anchorDirStr = anchorDir.generic_string();

    // Update sublayer paths
    auto subLayerPaths = layer->GetSubLayerPaths();
    for (size_t j = 0; j < subLayerPaths.size(); ++j) {
        const auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPaths[j]);
        if (!subLayer) {
            continue;
        }

        fs::filesystem::path filePath(subLayer->GetRealPath());
        filePath = filePath.lexically_normal();

        auto it = layerEntry->second.paths.find(filePath);
        if (it == layerEntry->second.paths.end()) {
            continue;
        }

        subLayerPaths[j] = getPathRelativeToDirectory(filePath.generic_string(), anchorDirStr);
    }

    // Update references, payloads and asset path attributes
    updatePostponedRelativePathsForPrim(layer->GetPseudoRoot(), layerEntry, anchorDirStr);

    // Erase the layer entry
    postponedRelativePaths.erase(layerEntry);
}

std::string appendPaths(const std::string& a, const std::string& b)
{
    fs::filesystem::path aPath(a);
    fs::filesystem::path bPath(b);
    aPath /= b;

    return aPath.string();
}

} // namespace FileSystem
} // namespace USDUFE_NS_DEF
