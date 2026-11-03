//
// Copyright 2019 Autodesk
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
#include "utilFileSystem.h"

#include <usdLayerEditor/lib/layerEditorDCCFunctions.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usdUtils/dependencies.h>

#include <ghc/fs_std.hpp>

#include <algorithm>
#include <cctype>
#include <random>

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

std::string generateUniqueName()
{
    const auto  len { 6 };
    std::string uniqueName;
    uniqueName.reserve(len);

    const std::string alphaNum { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" };

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(alphaNum.size()) - 1);

    for (auto i = 0; i < len; ++i) {
        uniqueName += (alphaNum[dis(gen)]);
    }
    return uniqueName;
}

struct PostponedRelativeInfo
{
    std::set<fs::filesystem::path> paths;
};

using PostponedRelativePaths = std::map<PXR_NS::SdfLayerHandle, PostponedRelativeInfo>;

static PostponedRelativePaths& getPostponedRelativePaths()
{
    static PostponedRelativePaths sPaths;
    return sPaths;
}

} // namespace

namespace UsdLayerEditor {
namespace FileSystem {

PXR_NAMESPACE_USING_DIRECTIVE

std::string getDir(const std::string& fullFilePath)
{
    return fs::filesystem::path(fullFilePath).parent_path().string();
}

std::string getDCCSceneFileDir() { return UsdLayerEditor::getDCCSceneDir(); }

std::string getLayerFileDir(const PXR_NS::SdfLayerHandle& layer)
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

    fs::filesystem::path relativePath = absolutePath.lexically_relative(relativeToDir);

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

std::string getPathRelativeToDCCSceneFile(const std::string& fileName)
{
    auto relativePathAndSuccess = makePathRelativeTo(fileName, getDCCSceneFileDir());

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the Maya scene file, using the "
            "absolute path.",
            fileName.c_str());
    }

    return relativePathAndSuccess.first;
}

std::string
getPathRelativeToLayerFile(const std::string& fileName, const PXR_NS::SdfLayerHandle& layer)
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

void markPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath)
{
    fs::filesystem::path filePath(contentPath);
    auto&                postponedRelativePaths = getPostponedRelativePaths();
    postponedRelativePaths[layer].paths.insert(filePath);
}

void unmarkPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath)
{
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry != postponedRelativePaths.end()) {
        fs::filesystem::path filePath(contentPath);
        layerEntry->second.paths.erase(filePath);
    }
}

void updatePostponedRelativePaths(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            layerFileName)
{
    // Find the layer entry
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry == postponedRelativePaths.end()) {
        return;
    }

    auto anchorDir = fs::filesystem::path(layerFileName).remove_filename();
    auto anchorDirStr = anchorDir.generic_string();

    // Update sublayer paths
    auto subLayerPaths = layer->GetSubLayerPaths();
    for (size_t j = 0; j < subLayerPaths.size(); ++j) {
        const auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPaths[j]);
        if (!subLayer) {
            continue;
        }

        fs::filesystem::path filePath(subLayer->GetRealPath());

        auto it = layerEntry->second.paths.find(filePath);
        if (it == layerEntry->second.paths.end()) {
            continue;
        }

        subLayerPaths[j] = getPathRelativeToDirectory(filePath.generic_string(), anchorDirStr);
    }

    // Update references, payloads and asset path attributes
    PXR_NS::UsdUtilsModifyAssetPaths(
        layer,
        [&](const std::string& authored) {
            const fs::filesystem::path filePath(authored);
            if (layerEntry->second.paths.find(filePath) == layerEntry->second.paths.end()) {
                return authored;
            }
            return getPathRelativeToDirectory(filePath.generic_string(), anchorDirStr);
        },
        /*keepEmptyPathsInArrays*/ true);

    // Erase the layer entry
    postponedRelativePaths.erase(layerEntry);
}

namespace {

bool isIdentifierChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// "${VAR}", "$VAR" and "%VAR%": resolved by the asset resolver or the environment at load
// time, so the authored text must be kept as is.
bool hasTokenOrEnvironmentVariable(const std::string& path)
{
    if (path.find('$') != std::string::npos) {
        return true;
    }
    for (size_t open = path.find('%'); open != std::string::npos; open = path.find('%', open + 1)) {
        const size_t close = path.find('%', open + 1);
        if (close == std::string::npos) {
            break;
        }
        if (close > open + 1
            && std::all_of(path.begin() + open + 1, path.begin() + close, isIdentifierChar)) {
            return true;
        }
    }
    return false;
}

// "https://...": not a file system path.
bool hasUriScheme(const std::string& path)
{
    const size_t separator = path.find("://");
    return separator != std::string::npos && separator > 1
        && std::all_of(path.begin(), path.begin() + separator, [](char c) {
               return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-'
                   || c == '.';
           });
}

} // namespace

RewrittenAssetPaths
makeAssetPathsRelativeToFile(const PXR_NS::SdfLayerHandle& layer, const std::string& targetFilePath)
{
    RewrittenAssetPaths result;
    if (!layer || targetFilePath.empty()) {
        return result;
    }

    const auto targetDir = fs::filesystem::path(targetFilePath).parent_path().lexically_normal();
    if (targetDir.empty()) {
        return result;
    }

    const std::string currentFile = layer->GetRealPath();
    const auto        currentDir = currentFile.empty()
               ? fs::filesystem::path()
               : fs::filesystem::path(currentFile).parent_path().lexically_normal();

    // Relative paths only need re-anchoring when the layer ends up in another folder.
    const bool reanchor = !currentDir.empty() && currentDir != targetDir;

    PXR_NS::UsdUtilsModifyAssetPaths(
        layer,
        [&](const std::string& authored) {
            if (authored.empty() || hasTokenOrEnvironmentVariable(authored)
                || hasUriScheme(authored)) {
                return authored;
            }

            fs::filesystem::path absolute(authored);
            if (!absolute.is_absolute()) {
                if (!reanchor) {
                    return authored;
                }
                absolute = currentDir / absolute;
            }

            const auto relative = makePathRelativeTo(
                absolute.lexically_normal().generic_string(), targetDir.generic_string());
            if (!relative.second) {
                TF_WARN(
                    "Asset path '%s' cannot be made relative to '%s'; keeping it as authored.",
                    authored.c_str(),
                    targetFilePath.c_str());
                return authored;
            }

            std::string converted = relative.first;
            if (converted.rfind("..", 0) != 0) {
                converted.insert(0, "./");
            }
            if (converted == authored) {
                return authored;
            }

            result.changes.emplace_back(converted, authored);
            return converted;
        },
        /*keepEmptyPathsInArrays*/ true);

    return result;
}

void restoreAssetPaths(const PXR_NS::SdfLayerHandle& layer, const RewrittenAssetPaths& rewritten)
{
    if (!layer || rewritten.changes.empty()) {
        return;
    }

    // When two originals converted to the same text, the first one wins: both name the same
    // file.
    std::map<std::string, std::string> originalByConverted;
    for (const auto& [converted, original] : rewritten.changes) {
        originalByConverted.emplace(converted, original);
    }

    PXR_NS::UsdUtilsModifyAssetPaths(
        layer,
        [&](const std::string& authored) {
            const auto found = originalByConverted.find(authored);
            return found == originalByConverted.end() ? authored : found->second;
        },
        /*keepEmptyPathsInArrays*/ true);
}

bool prepareLayerSaveUILayer(const PXR_NS::SdfLayerHandle& layer, bool useSceneFileForRoot)
{
    std::string layerFileDir;
    if (layer) {
        layerFileDir = getLayerFileDir(layer);
    } else if (useSceneFileForRoot) {
        layerFileDir = getDCCSceneFileDir();
    }

    return prepareLayerSaveUILayer(layerFileDir);
}

bool prepareLayerSaveUILayer(const std::string& relativeAnchor)
{
    return UsdLayerEditor::prepareLayerSaveUILayer(relativeAnchor);
}

bool requireUsdPathsRelativeToDCCSceneFile()
{
    return UsdLayerEditor::requireUsdPathsRelativeToSceneFile();
}

bool requireUsdPathsRelativeToParentLayer()
{
    return UsdLayerEditor::requireUsdPathsRelativeToParentLayer();
}

void setRequireUsdPathsRelativeToDCCSceneFile(bool value)
{
    UsdLayerEditor::setRequireUsdPathsRelativeToSceneFile(value);
}

void setRequireUsdPathsRelativeToParentLayer(bool value)
{
    UsdLayerEditor::setRequireUsdPathsRelativeToParentLayer(value);
}

std::string getDCCWorkspaceScenesDir() { return UsdLayerEditor::getDCCWorkspaceScenesDir(); }

std::string
getUniqueFileName(const std::string& dir, const std::string& basename, const std::string& ext)
{
    const std::string fileNameModel = basename + '-' + generateUniqueName() + '.' + ext;

    fs::filesystem::path pathModel(dir);
    pathModel.append(fileNameModel);

    return pathModel.generic_string();
}

std::string ensureUniqueFileName(const std::string& filename)
{
    std::string uniqueName = filename;
    while (true) {
        if (!fs::filesystem::exists(fs::filesystem::path(uniqueName)))
            return uniqueName;

        // Algorithm to generate a unique name:
        //    1. Remove the extension
        //    2. Replace the filename with the filename plus random text
        //    3. Put the extension back.

        fs::filesystem::path uniquePath(filename);

        const std::string extOnly = uniquePath.extension().generic_string();
        uniquePath = uniquePath.replace_extension();

        const std::string nameOnly = uniquePath.filename().generic_string();
        uniquePath = uniquePath.replace_filename(nameOnly + "-" + generateUniqueName());

        uniquePath = uniquePath.replace_extension(extOnly);

        uniqueName = uniquePath.generic_string();
    }
}

size_t getNumberSuffixPosition(const std::string& text)
{
    const size_t length = text.size();

    if (length <= 1)
        return 0;

    size_t nonDigitPos = length - 1;
    while (nonDigitPos != 0 && std::isdigit(text[nonDigitPos]))
        --nonDigitPos;

    return nonDigitPos + 1;
}

std::string getNumberSuffix(const std::string& text)
{
    return text.substr(getNumberSuffixPosition(text));
}

std::string increaseNumberSuffix(const std::string& text)
{
    const size_t      suffixPos = getNumberSuffixPosition(text);
    const std::string numberText = text.substr(suffixPos);
    const std::string prefixText = text.substr(0, suffixPos);

    const int nextNumber = TfUnstringify<int>(numberText) + 1;
    return prefixText + TfStringify(nextNumber);
}

std::string appendPaths(const std::string& a, const std::string& b)
{
    fs::filesystem::path aPath(a);
    fs::filesystem::path bPath(b);
    aPath /= b;

    return aPath.string();
}

bool checkWriteAccess(const std::string& filePath)
{
    return UsdLayerEditor::checkWriteAccess(filePath);
}

FileBackup::FileBackup(const std::string& filename)
    : _filename(filename)
{
    backup();
}

FileBackup::~FileBackup()
{
    // If commited, we don't restore the old file.
    if (_commited)
        return;

    try {
        restore();
    } catch (...) {
        // Don't allow exceptions out of a destructor.
    }
}

std::string FileBackup::getBackupFilename() const { return _filename + ".backup"; }

void FileBackup::backup()
{
    if (!fs::filesystem::exists(fs::filesystem::path(_filename)))
        return;

    const std::string backupFileName = getBackupFilename();
    remove(backupFileName.c_str());
    if (rename(_filename.c_str(), backupFileName.c_str()) != 0)
        return;

    _backed = true;
}

void FileBackup::commit()
{
    // Once commited, the backup will not be put back into the original file.
    _commited = true;
}

void FileBackup::restore()
{
    if (!_backed)
        return;

    remove(_filename.c_str());
    const std::string backupFileName = getBackupFilename();
    rename(backupFileName.c_str(), _filename.c_str());
}

} // namespace FileSystem
} // namespace UsdLayerEditor
