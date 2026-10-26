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

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/utils/fileSystem.h>

#include <pxr/usd/ar/resolver.h>

#include <maya/MFileIO.h>
#include <maya/MFnReference.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>

#include <ghc/fs_std.hpp>

#include <cstdio>
#include <system_error>

PXR_NAMESPACE_USING_DIRECTIVE

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

            return UsdUfe::FileSystem::getDir(referencedFilePath);
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
        return UsdUfe::FileSystem::getDir(currentFile);

    return std::string();
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

    auto relativePathAndSuccess = UsdUfe::FileSystem::makePathRelativeTo(fileName, projectPath);

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
    const auto pathAndSuccess = UsdUfe::FileSystem::makePathRelativeTo(fileName, projectPath);
    if (!pathAndSuccess.second)
        return {};

    // Make the path absolute but relative to the project folder. That is an absolute
    // path that starts with the project path.
    return UsdUfe::FileSystem::appendPaths(projectPath, pathAndSuccess.first);
}

std::string UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(const std::string& fileName)
{
    auto relativePathAndSuccess
        = UsdUfe::FileSystem::makePathRelativeTo(fileName, getMayaSceneFileDir());

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the Maya scene file, using the "
            "absolute path.",
            fileName.c_str());
    }

    return relativePathAndSuccess.first;
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
            fileName = UsdUfe::FileSystem::getPathRelativeToLayerFile(fileName, layer);
        } else {
            UsdUfe::FileSystem::markPathAsPostponedRelative(layer, fileName);
            UsdUfe::FileSystem::markAttributeAsPostponedRelative(layer, attrName);
        }
    } else {
        UsdUfe::FileSystem::unmarkPathAsPostponedRelative(layer, fileName);
    }

    return fileName;
}

bool UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile()
{
    static const MString MAKE_PATH_RELATIVE_TO_SCENE_FILE = "mayaUsd_MakePathRelativeToSceneFile";
    return MGlobal::optionVarExists(MAKE_PATH_RELATIVE_TO_SCENE_FILE)
        && MGlobal::optionVarIntValue(MAKE_PATH_RELATIVE_TO_SCENE_FILE);
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
    static const MString REFERENCED_PRIM_PATH = "mayaUsd_ReferencedPrimPath";
    if (!MGlobal::optionVarExists(REFERENCED_PRIM_PATH))
        return {};
    return MGlobal::optionVarStringValue(REFERENCED_PRIM_PATH).asChar();
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
    auto            path = fs::filesystem::canonical(
        fs::filesystem::path(currentFileDir).append(relativeFilePath), errorCode);

    if (errorCode) {
        // file does not exist
        return std::string();
    }

    return path.string();
}

std::string UsdMayaUtilFileSystem::resolvePath(const std::string& filePath)
{
    ArResolver& resolver = ArGetResolver();
#if AR_VERSION == 1
    resolver.ConfigureResolverForAsset(filePath);
#endif
    return resolver.Resolve(filePath);
}

bool UsdMayaUtilFileSystem::requireUsdPathsRelativeToEditTargetLayer()
{
    static const MString MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE
        = "mayaUsd_MakePathRelativeToEditTargetLayer";
    return MGlobal::optionVarExists(MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE)
        && MGlobal::optionVarIntValue(MAKE_PATH_RELATIVE_TO_EDIT_TARGET_LAYER_FILE);
}

bool UsdMayaUtilFileSystem::pathAppendPath(std::string& a, const std::string& b)
{
    if (!fs::filesystem::is_directory(a)) {
        return false;
    }
    fs::filesystem::path aPath(a);
    fs::filesystem::path bPath(b);
    aPath /= b;
    a.assign(aPath.string());
    return true;
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
    fs::filesystem::path p(filePath);
    fs::filesystem::path filename = p.filename();
    filePath.assign(filename.string());
    return;
}

void UsdMayaUtilFileSystem::pathRemoveExtension(std::string& filePath)
{
    fs::filesystem::path p(filePath);
    fs::filesystem::path dir = p.parent_path();
    fs::filesystem::path finalPath = dir / p.stem();
    filePath.assign(finalPath.string());
    return;
}

std::string UsdMayaUtilFileSystem::pathFindExtension(std::string& filePath)
{
    fs::filesystem::path p(filePath);
    if (!p.has_extension()) {
        return std::string();
    }
    fs::filesystem::path ext = p.extension();
    return ext.string();
}

void UsdMayaUtilFileSystem::updatePostponedRelativePaths(const PXR_NS::SdfLayerHandle& layer)
{
    if (!layer)
        return;

    UsdUfe::FileSystem::updatePostponedRelativePaths(layer, layer->GetRealPath());
}

bool UsdMayaUtilFileSystem::isPathInside(const std::string& parentDir, const std::string& childPath)
{
    fs::filesystem::path parent = fs::filesystem::weakly_canonical(parentDir);
    fs::filesystem::path child = fs::filesystem::weakly_canonical(childPath);

    // Iterate up from child to root
    for (fs::filesystem::path p = child; !p.empty(); p = p.parent_path()) {
        if (p == parent)
            return true;

        fs::filesystem::path next = p.parent_path();
        if (next == p) // reached root (ex "C:\")
            break;
    }
    return false;
}
