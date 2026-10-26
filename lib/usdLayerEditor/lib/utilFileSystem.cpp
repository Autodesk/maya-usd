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

#include <usdUfe/utils/fileSystem.h>

#include <pxr/usd/ar/resolver.h>

#include <ghc/fs_std.hpp>

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

} // namespace

namespace UsdLayerEditor {
namespace FileSystem {

PXR_NAMESPACE_USING_DIRECTIVE

std::string getDCCSceneFileDir() { return UsdLayerEditor::getDCCSceneDir(); }

std::string getPathRelativeToDCCSceneFile(const std::string& fileName)
{
    auto relativePathAndSuccess
        = UsdUfe::FileSystem::makePathRelativeTo(fileName, getDCCSceneFileDir());

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the Maya scene file, using the "
            "absolute path.",
            fileName.c_str());
    }

    return relativePathAndSuccess.first;
}

bool prepareLayerSaveUILayer(const PXR_NS::SdfLayerHandle& layer, bool useSceneFileForRoot)
{
    std::string layerFileDir;
    if (layer) {
        layerFileDir = UsdUfe::FileSystem::getLayerFileDir(layer);
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
