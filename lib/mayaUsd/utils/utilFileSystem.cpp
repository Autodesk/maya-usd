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
} // namespace

PXR_NAMESPACE_USING_DIRECTIVE

std::string UsdMayaUtilFileSystem::resolvePath(const std::string& filePath)
{
    ArResolver& resolver = ArGetResolver();
    return resolver.Resolve(filePath);
}

std::string UsdMayaUtilFileSystem::getDir(const std::string& fullFilePath)
{
    return ghc::filesystem::path(fullFilePath).parent_path().string();
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
    if (relativeFilePath.length() < 3)
        return relativeFilePath;

    std::string currentFileDir = getMayaReferencedFileDir(proxyShape);

    if (currentFileDir.empty())
        currentFileDir = getMayaSceneFileDir();

    if (currentFileDir.empty())
        return relativeFilePath;

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

bool UsdMayaUtilFileSystem::isDirectory(const std::string& dirPath)
{
    ghc::filesystem::path p(dirPath);
    return ghc::filesystem::is_directory(p);
}

bool UsdMayaUtilFileSystem::isFile(const std::string& filePath)
{
    bool bStat = ghc::filesystem::is_regular_file(filePath);
    return bStat;
}

bool UsdMayaUtilFileSystem::pathAppendPath(std::string& a, const std::string& b)
{
    if (!ghc::filesystem::is_directory(a)) {
        return false;
    }
    ghc::filesystem::path aPath(a);
    ghc::filesystem::path bPath(b);
    aPath /= b;
    a.assign(aPath.c_str());
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
    ghc::filesystem::path p(filePath);
    ghc::filesystem::path filename = p.filename();
    filePath.assign(filename.c_str());
    return;
}

void UsdMayaUtilFileSystem::pathRemoveExtension(std::string& filePath)
{
    ghc::filesystem::path p(filePath);
    ghc::filesystem::path dir = p.parent_path();
    ghc::filesystem::path finalPath = dir / p.stem();
    filePath.assign(finalPath.c_str());
    return;
}

std::string UsdMayaUtilFileSystem::pathFindExtension(std::string& filePath)
{
    ghc::filesystem::path p(filePath);
    if (!p.has_extension()) {
        return std::string();
    }
    ghc::filesystem::path ext = p.extension();
    return std::string(ext.c_str());
}
