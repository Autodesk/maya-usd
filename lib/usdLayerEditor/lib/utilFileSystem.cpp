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

#include "layerEditorDCCFunctions.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include <pxr/usd/ar/resolver.h>

#include <filesystem>
#include <random>

namespace {
std::function<bool(std::string)> writeAccessCheckFunc;
std::function<std::string()> dccSceneSaveLocationFunc;
std::function<std::string()> dccWorkspaceSceneSaveLocationFunc;
std::function<bool(const std::string&)> prepareLayerSaveUILayerFn;
}

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
    std::set<std::filesystem::path> paths;
    std::set<TfToken>               attrs;
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

std::string resolvePath(const std::string& filePath)
{
    ArResolver& resolver = ArGetResolver();
#if AR_VERSION == 1
    resolver.ConfigureResolverForAsset(filePath);
#endif
    return resolver.Resolve(filePath);
}

std::string getDir(const std::string& fullFilePath)
{
    return std::filesystem::path(fullFilePath).parent_path().string();
}

std::string getDCCSceneFileDir()
{
    if (!dccSceneSaveLocationFunc)
        return {};
    return dccSceneSaveLocationFunc();
}

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
    std::filesystem::path absolutePath(fileName);

    // If the anchor relative-to-directory doesn't exist yet, use the unchanged path,
    // but don't return a failure. The anchor path being empty is not considered
    // a failure. If the caller needs to detect this, they can verify that the
    // anchor path is empty themselves before calling this function.
    if (relativeToDir.empty()) {
        return std::make_pair(fileName, true);
    }

    std::filesystem::path relativePath = absolutePath.lexically_relative(relativeToDir);

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
    std::filesystem::path filePath(contentPath);
    auto&                 postponedRelativePaths = getPostponedRelativePaths();
    postponedRelativePaths[layer].paths.insert(filePath);
}

void unmarkPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath)
{
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry != postponedRelativePaths.end()) {
        std::filesystem::path filePath(contentPath);
        layerEntry->second.paths.erase(filePath);
    }
}

template <typename TypePolicy>
void updatePathList(
    SdfListProxy<TypePolicy>                list,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr)
{
    for (auto proxy : list) {
        typename TypePolicy::value_type item = proxy;
        std::filesystem::path           filePath(item.GetAssetPath());
        filePath = filePath;

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

            VtValue               filePathValue = attr->GetDefaultValue();
            auto                  filePathStr = filePathValue.Get<SdfAssetPath>().GetAssetPath();
            std::filesystem::path filePath(filePathStr);
            auto                  it = layerEntry->second.paths.find(filePath);
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

    auto anchorDir = std::filesystem::path(layerFileName).remove_filename();
    auto anchorDirStr = anchorDir.generic_string();

    // Update sublayer paths
    auto subLayerPaths = layer->GetSubLayerPaths();
    for (size_t j = 0; j < subLayerPaths.size(); ++j) {
        const auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPaths[j]);
        if (!subLayer) {
            continue;
        }

        std::filesystem::path filePath(subLayer->GetRealPath());
        filePath = filePath;

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
    if (!prepareLayerSaveUILayerFn)
        return true;
    return prepareLayerSaveUILayerFn(relativeAnchor);
}

bool requireUsdPathsRelativeToDCCSceneFile()
{
    return UsdLayerEditor::requireUsdPathsRelativeToSceneFile();
}

bool requireUsdPathsRelativeToParentLayer()
{
    return UsdLayerEditor::requireUsdPathsRelativeToParentLayer();
}

bool requireUsdPathsRelativeToEditTargetLayer()
{
    return UsdLayerEditor::requireUsdPathsRelativeToEditTargetLayer();
}

bool wantReferenceCompositionArc() { return UsdLayerEditor::wantReferenceCompositionArc(); }

bool wantPrependCompositionArc() { return UsdLayerEditor::wantPrependCompositionArc(); }

bool wantPayloadLoaded() { return UsdLayerEditor::wantPayloadLoaded(); }

std::string getReferencedPrimPath() { return UsdLayerEditor::getReferencedPrimPath(); }

void setRequireUsdPathsRelativeToDCCSceneFile(bool value)
{
    UsdLayerEditor::setRequireUsdPathsRelativeToSceneFile(value);
}

void setRequireUsdPathsRelativeToParentLayer(bool value)
{
    UsdLayerEditor::setRequireUsdPathsRelativeToParentLayer(value);
}

std::string getDCCWorkspaceScenesDir()
{
    if (!dccWorkspaceSceneSaveLocationFunc)
        return {};
    return dccWorkspaceSceneSaveLocationFunc();
}

std::string
getUniqueFileName(const std::string& dir, const std::string& basename, const std::string& ext)
{
    const std::string fileNameModel = basename + '-' + generateUniqueName() + '.' + ext;

    std::filesystem::path pathModel(dir);
    pathModel.append(fileNameModel);

    return pathModel.generic_string();
}

std::string ensureUniqueFileName(const std::string& filename)
{
    std::string uniqueName = filename;
    while (true) {
        if (!std::filesystem::exists(std::filesystem::path(uniqueName)))
            return uniqueName;

        // Algorithm to generate a unique name:
        //    1. Remove the extension
        //    2. Replace the filename with the filename plus random text
        //    3. Put the extension back.

        std::filesystem::path uniquePath(filename);

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

bool pathAppendPath(std::string& a, const std::string& b)
{
    if (!std::filesystem::is_directory(a)) {
        return false;
    }
    std::filesystem::path aPath(a);
    std::filesystem::path bPath(b);
    aPath /= b;
    a.assign(aPath.string());
    return true;
}

std::string appendPaths(const std::string& a, const std::string& b)
{
    std::filesystem::path aPath(a);
    std::filesystem::path bPath(b);
    aPath /= b;

    return aPath.string();
}

size_t writeToFilePath(const char* filePath, const void* buffer, const size_t size)
{
    std::FILE* stream = std::fopen(filePath, "w");
    if (stream == nullptr) {
        return 0;
    }
    size_t numObjectsWritten = std::fwrite(buffer, size, 1, stream);
    if (numObjectsWritten != 1) {
        return 0;
    }
    int stat = std::fclose(stream);
    if (stat != 0) {
        return 0;
    }

    return size;
}

void pathStripPath(std::string& filePath)
{
    std::filesystem::path p(filePath);
    std::filesystem::path filename = p.filename();
    filePath.assign(filename.string());
    return;
}

void pathRemoveExtension(std::string& filePath)
{
    std::filesystem::path p(filePath);
    std::filesystem::path dir = p.parent_path();
    std::filesystem::path finalPath = dir / p.stem();
    filePath.assign(finalPath.string());
    return;
}

std::string pathFindExtension(std::string& filePath)
{
    std::filesystem::path p(filePath);
    if (!p.has_extension()) {
        return std::string();
    }
    std::filesystem::path ext = p.extension();
    return ext.string();
}

void setFileWriteAccessFunction(WriteAccessCheckFn checkFileWriteAccess)
{
    writeAccessCheckFunc = checkFileWriteAccess;
}

bool checkWriteAccess(const std::string& filePath)
{
    if (writeAccessCheckFunc) {
        return writeAccessCheckFunc(filePath);
    }
    TF_CODING_ERROR("No implementation provided for FileSystem::checkWriteAccess()");
    return false;
}

void setDCCSceneLocationFunc(std::function<std::string()> fn)
{
    dccSceneSaveLocationFunc = fn;
}

void setDCCWorkspaceSceneLocationFunc(std::function<std::string()> fn)
{
    dccWorkspaceSceneSaveLocationFunc = fn;
}

void setPrepareLayerSaveUILayerFn(std::function<bool(const std::string&)> fn)
{
    prepareLayerSaveUILayerFn = fn;
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
    }
    catch (...) {
        // Don't allow exceptions out of a destructor.
    }
}

std::string FileBackup::getBackupFilename() const
{
    return _filename + ".backup";
}

void FileBackup::backup()
{
    if (!std::filesystem::exists(std::filesystem::path(_filename)))
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
