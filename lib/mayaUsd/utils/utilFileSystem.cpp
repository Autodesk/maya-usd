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

#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/ar/resolver.h>

#include <maya/MFileIO.h>
#include <maya/MFnReference.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>

#include <ghc/filesystem.hpp>

#include <random>
#include <system_error>

namespace {
std::string generateUniqueName()
{
    const auto  len { 6 };
    std::string uniqueName;
    uniqueName.reserve(len);

    const std::string alphaNum { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" };

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, alphaNum.size() - 1);

    for (auto i = 0; i < len; ++i) {
        uniqueName += (alphaNum[dis(gen)]);
    }
    return uniqueName;
}

struct PostponedRelativeInfo
{
    std::set<ghc::filesystem::path> paths;
    std::set<TfToken>               attrs;
};

using PostponedRelativePaths = std::map<PXR_NS::SdfLayerHandle, PostponedRelativeInfo>;

static PostponedRelativePaths& getPostponedRelativePaths()
{
    static PostponedRelativePaths sPaths;
    return sPaths;
}

} // namespace

PXR_NAMESPACE_USING_DIRECTIVE

std::string UsdMayaUtilFileSystem::resolvePath(const std::string& filePath)
{
    ArResolver& resolver = ArGetResolver();
#if AR_VERSION == 1
    resolver.ConfigureResolverForAsset(filePath);
#endif
    return resolver.Resolve(filePath);
}

std::string UsdMayaUtilFileSystem::getDir(const std::string& fullFilePath)
{
    return ghc::filesystem::path(fullFilePath).parent_path().string();
}

UsdMayaUtilFileSystem::TemporaryCurrentDir::TemporaryCurrentDir(const std::string& newCurDir)
{
    if (newCurDir.size() > 0) {
        _previousCurDir = ghc::filesystem::current_path().string();
        ghc::filesystem::current_path(newCurDir);
    }
}

UsdMayaUtilFileSystem::TemporaryCurrentDir::~TemporaryCurrentDir()
{
    try {
        restore();
    } catch (const std::exception&) {
        // Ignore exception in destructor.
    }
}

void UsdMayaUtilFileSystem::TemporaryCurrentDir::restore()
{
    if (_previousCurDir.size() > 0) {
        ghc::filesystem::current_path(_previousCurDir);
        _previousCurDir.clear();
    }
}

std::string UsdMayaUtilFileSystem::getMayaReferencedFileDir(const MObject& proxyShapeNode)
{
    // Can not use MFnDependencyNode(proxyShapeNode).isFromReferencedFile() to test if it is
    // reference node or not, which always return false even the proxyShape node is referenced...

    MStatus            stat;
    MFnReference       refFn;
    MItDependencyNodes dgIter(MFn::kReference, &stat);
    for (; !dgIter.isDone(); dgIter.next()) {
        MObject cRefNode = dgIter.thisNode();
        refFn.setObject(cRefNode);
        if (refFn.containsNodeExactly(proxyShapeNode, &stat)) {
            // According to Maya API document, the second argument is 'includePath' and set it to
            // true to include the file path. However, I have to set it to false to return the full
            // file path otherwise I get a file name only...
            MString refFilePath = refFn.fileName(true, false, false, &stat);
            if (!refFilePath.length())
                return std::string();

            std::string referencedFilePath = refFilePath.asChar();
            TF_DEBUG(USDMAYA_PROXYSHAPEBASE)
                .Msg(
                    "getMayaReferencedFileDir: The reference file that contains the proxyShape "
                    "node is : %s\n",
                    referencedFilePath.c_str());

            return getDir(referencedFilePath);
        }
    }

    return std::string();
}

std::string UsdMayaUtilFileSystem::getMayaSceneFileDir()
{
    std::string currentFile
        = std::string(MFileIO::currentFile().asChar(), MFileIO::currentFile().length());
    size_t filePathSize = currentFile.size();
    if (filePathSize < 4)
        return std::string();

    // If scene is untitled, the maya file will be MayaWorkspaceDir/untitled :
    constexpr char ma_ext[] = ".ma";
    constexpr char mb_ext[] = ".mb";
    auto           ext_start = currentFile.end() - 3;
    if (std::equal(ma_ext, ma_ext + 3, ext_start) || std::equal(mb_ext, mb_ext + 3, ext_start))
        return getDir(currentFile);

    return std::string();
}

std::string UsdMayaUtilFileSystem::getLayerFileDir(const PXR_NS::SdfLayerHandle& layer)
{
    if (!layer)
        return std::string();

    const std::string layerFileName = layer->GetRealPath();
    if (layerFileName.empty())
        return std::string();

    return UsdMayaUtilFileSystem::getDir(layerFileName);
}

std::pair<std::string, bool> UsdMayaUtilFileSystem::makePathRelativeTo(
    const std::string& fileName,
    const std::string& relativeToDir)
{
    ghc::filesystem::path absolutePath(fileName);

    // If the anchor relative-to-directory doesn't exist yet, use the unchanged path,
    // but don't return a failure. The anchor path being empty is not considered
    // a failure. If the caller needs to detect this, they can verify that the
    // anchor path is empty themselves before calling this function.
    if (relativeToDir.empty()) {
        return std::make_pair(fileName, true);
    }

    ghc::filesystem::path relativePath = absolutePath.lexically_relative(relativeToDir);

    if (relativePath.empty()) {
        return std::make_pair(fileName, false);
    }

    return std::make_pair(relativePath.generic_string(), true);
}

std::string UsdMayaUtilFileSystem::getPathRelativeToProject(const std::string& fileName)
{
    if (fileName.empty())
        return {};

    const std::string projectPath(UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar());
    if (projectPath.empty())
        return {};

    // Note: we do *not* use filesystem function to attempt to make the
    //       path relative sinceit would succeed as long as both paths
    //       are on the same drive. We really only want to know if the
    //       project path is the prefix of the file path. Maya will
    //       preserve paths entered manually with relative folder ("..")
    //       by keping an absolute path with ".." embedded in them,
    //       so this works even in this situation.
    const auto pos = fileName.rfind(projectPath, 0);
    if (pos != 0)
        return {};

    auto relativePathAndSuccess = makePathRelativeTo(fileName, projectPath);

    if (!relativePathAndSuccess.second)
        return {};

    return relativePathAndSuccess.first;
}

std::string UsdMayaUtilFileSystem::makeProjectRelatedPath(const std::string& fileName)
{
    const std::string projectPath(UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar());
    if (projectPath.empty())
        return {};

    // Attempt to create a relative path relative to the project folder.
    // If that fails, we cannot create the project-relative path.
    const auto pathAndSuccess = UsdMayaUtilFileSystem::makePathRelativeTo(fileName, projectPath);
    if (!pathAndSuccess.second)
        return {};

    // Make the path absolute but relative to the project folder. That is an absolute
    // path that starts with the project path.
    return UsdMayaUtilFileSystem::appendPaths(projectPath, pathAndSuccess.first);
}

std::string UsdMayaUtilFileSystem::getPathRelativeToDirectory(
    const std::string& fileName,
    const std::string& relativeToDir)
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

std::string UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(const std::string& fileName)
{
    auto relativePathAndSuccess = makePathRelativeTo(fileName, getMayaSceneFileDir());

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the Maya scene file, using the "
            "absolute path.",
            fileName.c_str());
    }

    return relativePathAndSuccess.first;
}

std::string UsdMayaUtilFileSystem::getPathRelativeToLayerFile(
    const std::string&            fileName,
    const PXR_NS::SdfLayerHandle& layer)
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

void UsdMayaUtilFileSystem::markPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath)
{
    ghc::filesystem::path filePath(contentPath);
    auto&                 postponedRelativePaths = getPostponedRelativePaths();
    postponedRelativePaths[layer].paths.insert(filePath.lexically_normal());
}

void UsdMayaUtilFileSystem::unmarkPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath)
{
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry != postponedRelativePaths.end()) {
        ghc::filesystem::path filePath(contentPath);
        layerEntry->second.paths.erase(filePath.lexically_normal());
    }
}

std::string UsdMayaUtilFileSystem::handleAssetPathThatMaybeRelativeToLayer(
    std::string                   fileName,
    const std::string&            attrName,
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            optionVarName)
{
    if (!layer) {
        return fileName;
    }

    const MString optionVarString = optionVarName.c_str();
    const bool    needPathRelative
        = MGlobal::optionVarExists(optionVarString) && MGlobal::optionVarIntValue(optionVarString);

    if (needPathRelative) {
        if (!layer->IsAnonymous()) {
            fileName = getPathRelativeToLayerFile(fileName, layer);
        } else {
            markPathAsPostponedRelative(layer, fileName);
            getPostponedRelativePaths()[layer].attrs.insert(TfToken(attrName));
        }
    } else {
        unmarkPathAsPostponedRelative(layer, fileName);
    }

    return fileName;
}

template <typename TypePolicy>
void updatePathList(
    SdfListProxy<TypePolicy>                list,
    const PostponedRelativePaths::iterator& layerEntry,
    const std::string&                      anchorDirStr)
{
    for (auto proxy : list) {
        typename TypePolicy::value_type item = proxy;
        ghc::filesystem::path           filePath(item.GetAssetPath());
        filePath = filePath.lexically_normal();

        auto it = layerEntry->second.paths.find(filePath);
        if (it == layerEntry->second.paths.end()) {
            continue;
        }

        item.SetAssetPath(UsdMayaUtilFileSystem::getPathRelativeToDirectory(
            filePath.generic_string(), anchorDirStr));
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
            ghc::filesystem::path filePath(filePathStr);
            auto                  it = layerEntry->second.paths.find(filePath);
            if (it == layerEntry->second.paths.end()) {
                continue;
            }

            std::string relativePath = UsdMayaUtilFileSystem::getPathRelativeToDirectory(
                filePath.generic_string(), anchorDirStr);
            filePathValue = SdfAssetPath(relativePath);
            attr->SetDefaultValue(filePathValue);
        }

        updatePostponedRelativePathsForPrim(child, layerEntry, anchorDirStr);
        updatePathsInVariantSets(child->GetVariantSets(), layerEntry, anchorDirStr);
    }
}

void UsdMayaUtilFileSystem::updatePostponedRelativePaths(const PXR_NS::SdfLayerHandle& layer)
{
    if (!layer)
        return;

    updatePostponedRelativePaths(layer, layer->GetRealPath());
}

void UsdMayaUtilFileSystem::updatePostponedRelativePaths(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            layerFileName)
{
    // Find the layer entry
    auto& postponedRelativePaths = getPostponedRelativePaths();
    auto  layerEntry = postponedRelativePaths.find(layer);
    if (layerEntry == postponedRelativePaths.end()) {
        return;
    }

    auto anchorDir = ghc::filesystem::path(layerFileName).lexically_normal().remove_filename();
    auto anchorDirStr = anchorDir.generic_string();

    // Update sublayer paths
    auto subLayerPaths = layer->GetSubLayerPaths();
    for (size_t j = 0; j < subLayerPaths.size(); ++j) {
        const auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPaths[j]);
        if (!subLayer) {
            continue;
        }

        ghc::filesystem::path filePath(subLayer->GetRealPath());
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

bool UsdMayaUtilFileSystem::prepareLayerSaveUILayer(
    const PXR_NS::SdfLayerHandle& layer,
    bool                          useSceneFileForRoot)
{
    std::string layerFileDir;
    if (layer) {
        layerFileDir = getLayerFileDir(layer);
    } else if (useSceneFileForRoot) {
        layerFileDir = getMayaSceneFileDir();
    }

    return prepareLayerSaveUILayer(layerFileDir);
}

bool UsdMayaUtilFileSystem::prepareLayerSaveUILayer(const std::string& relativeAnchor)
{
    const char* script = "import mayaUsd_USDRootFileRelative as murel\n"
                         "murel.usdFileRelative.setRelativeFilePathRoot(r'''%s''')";

    const std::string commandString = TfStringPrintf(script, relativeAnchor.c_str());
    return MGlobal::executePythonCommand(commandString.c_str());
}

bool UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile()
{
    static const MString MAKE_PATH_RELATIVE_TO_SCENE_FILE = "mayaUsd_MakePathRelativeToSceneFile";
    return MGlobal::optionVarExists(MAKE_PATH_RELATIVE_TO_SCENE_FILE)
        && MGlobal::optionVarIntValue(MAKE_PATH_RELATIVE_TO_SCENE_FILE);
}

bool UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer()
{
    static const MString MAKE_PATH_RELATIVE_TO_PARENT_LAYER_FILE
        = "mayaUsd_MakePathRelativeToParentLayer";
    return MGlobal::optionVarExists(MAKE_PATH_RELATIVE_TO_PARENT_LAYER_FILE)
        && MGlobal::optionVarIntValue(MAKE_PATH_RELATIVE_TO_PARENT_LAYER_FILE);
}

bool UsdMayaUtilFileSystem::requireUsdPathsRelativeToEditTargetLayer()
{
    static const MString MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE
        = "mayaUsd_MakePathRelativeToEditTargetLayer";
    return MGlobal::optionVarExists(MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE)
        && MGlobal::optionVarIntValue(MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE);
}

bool UsdMayaUtilFileSystem::wantReferenceCompositionArc()
{
    static const MString WANT_REFERENCE_COMPOSITION_ARC = "mayaUsd_WantReferenceCompositionArc";
    return MGlobal::optionVarExists(WANT_REFERENCE_COMPOSITION_ARC)
        && MGlobal::optionVarIntValue(WANT_REFERENCE_COMPOSITION_ARC);
}

bool UsdMayaUtilFileSystem::wantPrependCompositionArc()
{
    static const MString WANT_PREPEND_COMPOSITION_ARC = "mayaUsd_WantPrependCompositionArc";
    return MGlobal::optionVarExists(WANT_PREPEND_COMPOSITION_ARC)
        && MGlobal::optionVarIntValue(WANT_PREPEND_COMPOSITION_ARC);
}

bool UsdMayaUtilFileSystem::wantPayloadLoaded()
{
    static const MString WANT_PAYLOAD_LOADED = "mayaUsd_WantPayloadLoaded";
    return MGlobal::optionVarExists(WANT_PAYLOAD_LOADED)
        && MGlobal::optionVarIntValue(WANT_PAYLOAD_LOADED);
}

std::string UsdMayaUtilFileSystem::getReferencedPrimPath()
{
    static const MString WANT_REFERENCE_COMPOSITION_ARC = "mayaUsd_ReferencedPrimPath";
    if (!MGlobal::optionVarExists(WANT_REFERENCE_COMPOSITION_ARC))
        return {};
    return MGlobal::optionVarStringValue(WANT_REFERENCE_COMPOSITION_ARC).asChar();
}

const char* getScenesFolderScript = R"(
global proc string UsdMayaUtilFileSystem_GetScenesFolder()
{
    string $workspaceLocation = `workspace -q -fn`;
    string $scenesFolder = `workspace -q -fileRuleEntry "scene"`;
    $sceneFolder = $workspaceLocation + "/" + $scenesFolder;

    return $sceneFolder;
}
UsdMayaUtilFileSystem_GetScenesFolder;
)";

std::string UsdMayaUtilFileSystem::getMayaWorkspaceScenesDir()
{
    MString scenesFolder;
    MGlobal::executeCommand(
        getScenesFolderScript,
        scenesFolder,
        /*display*/ false,
        /*undo*/ false);

    return UsdMayaUtil::convert(scenesFolder);
}

std::string UsdMayaUtilFileSystem::resolveRelativePathWithinMayaContext(
    const MObject&     proxyShape,
    const std::string& relativeFilePath)
{
    if (relativeFilePath.length() < 3) {
        return relativeFilePath;
    }

    std::string currentFileDir = getMayaReferencedFileDir(proxyShape);

    if (currentFileDir.empty()) {
        currentFileDir = getMayaSceneFileDir();
    }

    if (currentFileDir.empty()) {
        return relativeFilePath;
    }

    std::error_code errorCode;
    auto            path = ghc::filesystem::canonical(
        ghc::filesystem::path(currentFileDir).append(relativeFilePath), errorCode);

    if (errorCode) {
        // file does not exist
        return std::string();
    }

    return path.string();
}

std::string UsdMayaUtilFileSystem::getUniqueFileName(
    const std::string& dir,
    const std::string& basename,
    const std::string& ext)
{
    const std::string fileNameModel = basename + '-' + generateUniqueName() + '.' + ext;

    ghc::filesystem::path pathModel(dir);
    pathModel.append(fileNameModel);

    return pathModel.generic_string();
}

std::string UsdMayaUtilFileSystem::ensureUniqueFileName(const std::string& filename)
{
    std::string uniqueName = filename;
    while (true) {
        if (!ghc::filesystem::exists(ghc::filesystem::path(uniqueName)))
            return uniqueName;

        // Algorithm to generate a unique name:
        //    1. Remove the extension
        //    2. Replace the filename with the filename plus random text
        //    3. Put the extension back.

        ghc::filesystem::path uniquePath(filename);

        const std::string extOnly = uniquePath.extension().generic_string();
        uniquePath = uniquePath.replace_extension();

        const std::string nameOnly = uniquePath.filename().generic_string();
        uniquePath = uniquePath.replace_filename(nameOnly + "-" + generateUniqueName());

        uniquePath = uniquePath.replace_extension(extOnly);

        uniqueName = uniquePath.generic_string();
    }
}

size_t UsdMayaUtilFileSystem::getNumberSuffixPosition(const std::string& text)
{
    const size_t length = text.size();

    if (length <= 1)
        return 0;

    size_t nonDigitPos = length - 1;
    while (nonDigitPos != 0 && std::isdigit(text[nonDigitPos]))
        --nonDigitPos;

    return nonDigitPos + 1;
}

std::string UsdMayaUtilFileSystem::getNumberSuffix(const std::string& text)
{
    return text.substr(getNumberSuffixPosition(text));
}

std::string UsdMayaUtilFileSystem::increaseNumberSuffix(const std::string& text)
{
    const size_t      suffixPos = getNumberSuffixPosition(text);
    const std::string numberText = text.substr(suffixPos);
    const std::string prefixText = text.substr(0, suffixPos);

    const int nextNumber = TfUnstringify<int>(numberText) + 1;
    return prefixText + TfStringify(nextNumber);
}

bool UsdMayaUtilFileSystem::pathAppendPath(std::string& a, const std::string& b)
{
    if (!ghc::filesystem::is_directory(a)) {
        return false;
    }
    ghc::filesystem::path aPath(a);
    ghc::filesystem::path bPath(b);
    aPath /= b;
    a.assign(aPath.string());
    return true;
}

std::string UsdMayaUtilFileSystem::appendPaths(const std::string& a, const std::string& b)
{
    ghc::filesystem::path aPath(a);
    ghc::filesystem::path bPath(b);
    aPath /= b;

    return aPath.string();
}

size_t
UsdMayaUtilFileSystem::writeToFilePath(const char* filePath, const void* buffer, const size_t size)
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

void UsdMayaUtilFileSystem::pathStripPath(std::string& filePath)
{
    ghc::filesystem::path p(filePath);
    ghc::filesystem::path filename = p.filename();
    filePath.assign(filename.string());
    return;
}

void UsdMayaUtilFileSystem::pathRemoveExtension(std::string& filePath)
{
    ghc::filesystem::path p(filePath);
    ghc::filesystem::path dir = p.parent_path();
    ghc::filesystem::path finalPath = dir / p.stem();
    filePath.assign(finalPath.string());
    return;
}

std::string UsdMayaUtilFileSystem::pathFindExtension(std::string& filePath)
{
    ghc::filesystem::path p(filePath);
    if (!p.has_extension()) {
        return std::string();
    }
    ghc::filesystem::path ext = p.extension();
    return ext.string();
}

UsdMayaUtilFileSystem::FileBackup::FileBackup(const std::string& filename)
    : _filename(filename)
{
    backup();
}

UsdMayaUtilFileSystem::FileBackup::~FileBackup()
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

std::string UsdMayaUtilFileSystem::FileBackup::getBackupFilename() const
{
    return _filename + ".backup";
}

void UsdMayaUtilFileSystem::FileBackup::backup()
{
    if (!ghc::filesystem::exists(ghc::filesystem::path(_filename)))
        return;

    const std::string backupFileName = getBackupFilename();
    remove(backupFileName.c_str());
    if (rename(_filename.c_str(), backupFileName.c_str()) != 0)
        return;

    _backed = true;
}

void UsdMayaUtilFileSystem::FileBackup::commit()
{
    // Once commited, the backup will not be put back into the original file.
    _commited = true;
}

void UsdMayaUtilFileSystem::FileBackup::restore()
{
    if (!_backed)
        return;

    remove(_filename.c_str());
    const std::string backupFileName = getBackupFilename();
    rename(backupFileName.c_str(), _filename.c_str());
}

bool UsdMayaUtilFileSystem::isPathInside(const std::string& parentDir, const std::string& childPath)
{
    ghc::filesystem::path parent = ghc::filesystem::weakly_canonical(parentDir);
    ghc::filesystem::path child = ghc::filesystem::weakly_canonical(childPath);

    // Iterate up from child to root
    for (ghc::filesystem::path p = child; !p.empty(); p = p.parent_path()) {
        if (p == parent)
            return true;

        ghc::filesystem::path next = p.parent_path();
        if (next == p) // reached root (ex "C:\")
            break;
    }
    return false;
}