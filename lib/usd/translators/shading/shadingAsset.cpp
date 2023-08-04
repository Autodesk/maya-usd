//
// Copyright 2023 Autodesk
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

#include "shadingAsset.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/arch/hash.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/resolver.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/shader.h>

#include <ghc/filesystem.hpp>

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

static void
updateAssetPath(std::string assetPath, std::string resolvedPath, SdfAssetPath* resolvedAssetPath)
{
    assetPath = TfStringReplace(assetPath, "\\", "/");

    if (resolvedPath.empty())
        resolvedPath = assetPath;
    else
        resolvedPath = TfStringReplace(resolvedPath, "\\", "/");

    *resolvedAssetPath = SdfAssetPath(assetPath, resolvedPath);
}

static SdfAssetPath handleShaderInput(const UsdShadeInput& usdInput)
{
    VtValue val;
    if (!usdInput.Get(&val))
        return {};

    if (!val.IsHolding<SdfAssetPath>())
        return {};

    return val.UncheckedGet<SdfAssetPath>();
}

static void handleMissingResolvedPath(SdfAssetPath* resolvedAssetPath)
{
    const std::string& filePath = resolvedAssetPath->GetResolvedPath();
    if (!filePath.empty() && !ArIsPackageRelativePath(filePath)) {
        // Maya has issues with relative paths, especially if deep inside a
        // nesting of referenced assets. Use absolute path instead if USD was
        // able to resolve. A better fix will require providing an asset
        // resolver to Maya that can resolve the file correctly using the
        // MPxFileResolver API. We also make sure the path is not expressed
        // as a relationship like texture paths inside USDZ assets.
        *resolvedAssetPath = SdfAssetPath(filePath, filePath);
    }
}

// Note: return false only on import errors. Currently always returns true.
static bool
handleUDIM(const UsdPrim& prim, MFnDependencyNode& depFn, SdfAssetPath* resolvedAssetPath)
{
    const std::string unresolvedFilePath = resolvedAssetPath->GetAssetPath();

    std::string::size_type udimPos = unresolvedFilePath.rfind(TrMayaTokens->UDIMTag.GetString());
    if (udimPos == std::string::npos)
        return true;

    MStatus status;
    MPlug   tilingAttr = depFn.findPlug(TrMayaTokens->uvTilingMode.GetText(), true, &status);
    if (status != MS::kSuccess)
        return true;

    tilingAttr.setInt(3);

    // USD did not resolve the path to absolute because the file name was not an
    // actual file on disk. We need to find the first tile to help Maya find the
    // other ones.
    std::string udimPath(unresolvedFilePath.substr(0, udimPos));
    udimPath += "1001";
    udimPath += unresolvedFilePath.substr(udimPos + TrMayaTokens->UDIMTag.GetString().size());

    Usd_Resolver res(&prim.GetPrimIndex());
    for (; res.IsValid(); res.NextLayer()) {
        std::string resolvedName = SdfComputeAssetPathRelativeToLayer(res.GetLayer(), udimPath);

        if (!resolvedName.empty() && !ArIsPackageRelativePath(resolvedName)
            && resolvedName != udimPath) {
            udimPath = resolvedName;
            break;
        }
    }

    const std::string absPath = resolvedAssetPath->GetResolvedPath();
    updateAssetPath(udimPath, absPath, resolvedAssetPath);
    return true;
}

// Note: return false only on import errors.
static bool handleUSDZTexture(
    const UsdPrim&              prim,
    MFnDependencyNode&          depFn,
    const UsdMayaJobImportArgs& jobArgs,
    SdfAssetPath*               resolvedAssetPath)
{
    const std::string filePath = resolvedAssetPath->GetResolvedPath();

    if (filePath.empty())
        return true;

    if (!ArIsPackageRelativePath(filePath))
        return true;

    if (!jobArgs.importUSDZTextures) {
        TF_WARN(
            "Imported USD file contains an USDZ archive but the importUSDZTextures flag is off.");
        return true;
    }

    if (jobArgs.importUSDZTexturesFilePath.empty()) {
        TF_WARN("Imported USD file contains an USDZ archive but no importUSDZTexturesFilePath flag "
                "were provided.");
        return true;
    }

    std::string unresolvedFilePath = resolvedAssetPath->GetAssetPath();

    // NOTE: (yliangsiew) Package-relatve path means that we are inside of a USDZ file.
    ArResolver&              arResolver = ArGetResolver(); // NOTE: (yliangsiew) This is cached.
    std::shared_ptr<ArAsset> assetPtr = arResolver.OpenAsset(ArResolvedPath(filePath));
    if (assetPtr == nullptr) {
        TF_WARN(
            "The file: %s could not be found within the USDZ archive for extraction.",
            filePath.c_str());
        return false;
    }

    ArAsset*                    asset = assetPtr.get();
    std::shared_ptr<const char> fileData = asset->GetBuffer();
    const size_t                fileSize = asset->GetSize();

    bool     needsUniqueFilename = false;
    uint64_t spookyHash = ArchHash64(fileData.get(), fileSize);
    std::unordered_map<std::string, uint64_t>::iterator itExistingHash
        = UsdMayaReadUtil::mapFileHashes.find(unresolvedFilePath);
    if (itExistingHash
        == UsdMayaReadUtil::mapFileHashes.end()) { // NOTE: (yliangsiew) Means that the
                                                   // texture hasn't been extracted before.
        UsdMayaReadUtil::mapFileHashes.insert(
            { unresolvedFilePath,
              spookyHash }); // NOTE: (yliangsiew) This _should_ be the common case.
    } else {
        uint64_t existingHash = itExistingHash->second;
        if (spookyHash == existingHash) {
            TF_WARN(
                "A duplicate texture: %s was found, skipping extraction of it and re-using "
                "the existing one.",
                unresolvedFilePath.c_str());
            unresolvedFilePath = itExistingHash->first;
        } else {
            // NOTE: (yliangsiew) Means that a duplicate texture with the same name but with
            // different contents was found. Instead of failing, continue extraction with a
            // different filename instead and point to that one.
            needsUniqueFilename = true;
        }
    }

    // NOTE: (yliangsiew) Write the file to disk now.
    std::string filename(unresolvedFilePath);
    UsdMayaUtilFileSystem::pathStripPath(filename);
    std::string extractedFilePath(jobArgs.importUSDZTexturesFilePath);
    bool        bStat = UsdMayaUtilFileSystem::pathAppendPath(extractedFilePath, filename);
    TF_VERIFY(bStat);

    if (needsUniqueFilename) {
        int         counter = 0;
        std::string checkPath(extractedFilePath);
        while (ghc::filesystem::is_regular_file(checkPath)) {
            checkPath.assign(extractedFilePath);
            std::string filenameNoExt(checkPath);
            std::string ext = UsdMayaUtilFileSystem::pathFindExtension(checkPath);
            UsdMayaUtilFileSystem::pathRemoveExtension(checkPath);
            checkPath = TfStringPrintf("%s_%d%s", checkPath.c_str(), counter, ext.c_str());
            ++counter;
        }
        extractedFilePath.assign(checkPath);
        TF_WARN(
            "A file was duplicated within the archive, but was unique in content. Writing "
            "file with a suffix instead: %s",
            extractedFilePath.c_str());
    }

    // NOTE: (yliangsiew) Check if the texture already exists on disk and skip overwriting
    // it if necessary. This is because what happens if two USDZ files are imported, but
    // they have textures with the same names in them? We can't overwrite them....
    // If the texture exists on disk already and it is has the same contents, however, we
    // skip overwriting it.
    bool needsWrite = true;
    if (ghc::filesystem::is_regular_file(extractedFilePath)) {
        FILE* pFile = fopen(extractedFilePath.c_str(), "rb");
        fseek(pFile, 0, SEEK_END);
        long fileSize = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);
        char* buf = (char*)malloc(sizeof(char) * fileSize);
        fread(buf, fileSize, 1, pFile);
        uint64_t existingSpookyHash = ArchHash64(buf, fileSize);
        fclose(pFile);
        free(buf);
        if (spookyHash == existingSpookyHash) {
            TF_WARN(
                "The texture: %s already on disk is the same, skipping overwriting it.",
                extractedFilePath.c_str());
            needsWrite = false;
        } else {
            int         counter = 0;
            std::string checkPath(extractedFilePath);
            while (ghc::filesystem::is_regular_file(checkPath)) {
                checkPath.assign(extractedFilePath);
                std::string filenameNoExt(checkPath);
                std::string ext = UsdMayaUtilFileSystem::pathFindExtension(checkPath);
                UsdMayaUtilFileSystem::pathRemoveExtension(checkPath);
                checkPath = TfStringPrintf("%s_%d%s", checkPath.c_str(), counter, ext.c_str());
                ++counter;
            }
            extractedFilePath.assign(checkPath);
            TF_WARN(
                "A duplicate file exists, but was unique in content. Writing a new"
                "file with a suffix instead: %s",
                extractedFilePath.c_str());
        }
    }

    if (needsWrite) {
        // TODO: (yliangsiew) Support undo/redo of mayaUSDImport command...though this might
        // be too risky compared to just having the end-user delete the textures manually
        // when needed.
        size_t bytesWritten = UsdMayaUtilFileSystem::writeToFilePath(
            extractedFilePath.c_str(), fileData.get(), fileSize);
        if (bytesWritten != fileSize) {
            TF_WARN(
                "Failed to write out texture: %s to disk. Check that there is enough disk "
                "space available",
                extractedFilePath.c_str());
            return false;
        }
    }

    // NOTE: (yliangsiew) Continue setting the texture file node attribute to point to the
    // new file that was written to disk.
    updateAssetPath(extractedFilePath, extractedFilePath, resolvedAssetPath);
    return true;
}

static bool handleMakeRelative(
    const UsdMayaJobImportArgs& jobArgs,
    const SdfAssetPath&         originalAssetPath,
    SdfAssetPath*               resolvedAssetPath)
{
    const std::string& relativeMode = jobArgs.importRelativeTextures;
    if (relativeMode == UsdMayaJobImportArgsTokens->none)
        return true;

    bool makeRelative = (relativeMode == UsdMayaJobImportArgsTokens->relative);
    bool makeAbsolute = (relativeMode == UsdMayaJobImportArgsTokens->absolute);

    // When in automatic mode (neither relative nor absolute), select a mode based on
    // the input texture filename. Maya always keeps paths as absolute paths internally,
    // so we need to detect if the path is in the Maya project folders.
    const bool isForced = (makeRelative || makeAbsolute);
    if (!isForced) {
        const std::string& fileName = originalAssetPath.GetAssetPath();
        const bool         isAbs = ghc::filesystem::path(fileName).is_absolute();
        if (isAbs) {
            makeAbsolute = true;
        } else {
            makeRelative = true;
        }
    }

    // Make the path relative to the project if requested.
    if (makeAbsolute) {
        std::error_code       ec;
        ghc::filesystem::path absolutePath
            = ghc::filesystem::absolute(resolvedAssetPath->GetAssetPath(), ec);
        if (!ec && !absolutePath.empty()) {
            const std::string absPath = absolutePath.generic_string();
            updateAssetPath(absPath, absPath, resolvedAssetPath);
        }
    } else if (makeRelative) {
        std::string absPath = resolvedAssetPath->GetResolvedPath();
        if (absPath.empty())
            absPath = resolvedAssetPath->GetAssetPath();
        const std::string relToProject = UsdMayaUtilFileSystem::makeProjectRelatedPath(absPath);
        if (!relToProject.empty()) {
            updateAssetPath(relToProject, absPath, resolvedAssetPath);
        } else {
            TF_WARN("Could not make texture file path relative for [%s].", absPath.c_str());
        }
    }

    return true;
}

bool ResolveTextureAssetPath(
    const UsdPrim&              prim,
    const UsdShadeShader&       shaderSchema,
    MFnDependencyNode&          depFn,
    const UsdMayaJobImportArgs& jobArgs)
{
    // Note: not having a shader input is not an error.
    UsdShadeInput usdInput = shaderSchema.GetInput(TrUsdTokens->file);
    if (!usdInput)
        return true;

    const SdfAssetPath originalAssetPath = handleShaderInput(usdInput);
    if (originalAssetPath == SdfAssetPath())
        return true;

    SdfAssetPath resolvedAssetPath = originalAssetPath;

    handleMissingResolvedPath(&resolvedAssetPath);

    // Handle UDIM texture files:
    if (!handleUDIM(prim, depFn, &resolvedAssetPath))
        return false;

    if (!handleUSDZTexture(prim, depFn, jobArgs, &resolvedAssetPath))
        return false;

    // Note: current implementation of handleMakeRelative always succeeds,
    //       checking return value in case it evolves to have real fatal error
    //       conditions in the future.
    if (!handleMakeRelative(jobArgs, originalAssetPath, &resolvedAssetPath))
        return false;

    if (resolvedAssetPath != SdfAssetPath()) {
        MStatus status;
        MPlug   mayaAttr = depFn.findPlug(TrMayaTokens->fileTextureName.GetText(), true, &status);
        if (status != MS::kSuccess) {
            TF_RUNTIME_ERROR(
                "Could not find the built-in attribute fileTextureName on a Maya file node: "
                "%s! "
                "Something is seriously wrong with your current Maya session.",
                depFn.name().asChar());
            return false;
        }
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, VtValue(resolvedAssetPath));
    }

    // colorSpace:
    if (usdInput.GetAttr().HasColorSpace()) {
        MStatus status;
        MString colorSpace = usdInput.GetAttr().GetColorSpace().GetText();
        MPlug   mayaAttr = depFn.findPlug(TrMayaTokens->colorSpace.GetText(), true, &status);
        if (status == MS::kSuccess) {
            mayaAttr.setString(colorSpace);
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
