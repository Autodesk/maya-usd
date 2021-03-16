//
// Copyright 2020 Autodesk
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
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/arch/hash.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/resolver.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <ghc/filesystem.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdUVTexture_Reader : public UsdMayaShaderReader
{
public:
    PxrMayaUsdUVTexture_Reader(const UsdMayaPrimReaderArgs&);

    bool Read(UsdMayaPrimReaderContext* context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXRUSDMAYA_REGISTER_SHADER_READER(UsdUVTexture, PxrMayaUsdUVTexture_Reader)

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya "file" node attribute names
    (file)
    (alphaGain)
    (alphaOffset)
    (colorGain)
    (colorOffset)
    (colorSpace)
    (defaultColor)
    (fileTextureName)
    (outAlpha)
    (outColor)
    (outColorR)
    (outColorG)
    (outColorB)
    (place2dTexture)
    (coverage)
    (translateFrame)
    (rotateFrame)
    (mirrorU)
    (mirrorV)
    (stagger)
    (wrapU)
    (wrapV)
    (repeatUV)
    (offset)
    (rotateUV)
    (noiseUV)
    (vertexUvOne)
    (vertexUvTwo)
    (vertexUvThree)
    (vertexCameraOne)

    // UsdUVTexture Input Names
    (bias)
    (fallback)
    (scale)
    (wrapS)
    (wrapT)

    // uv connections:
    (outUvFilterSize)
    (uvFilterSize)
    (outUV)
    (uvCoord)

    // Values for wrapS and wrapT
    (black)
    (mirror)
    (repeat)

    // UsdUVTexture Output Names
    ((RGBOutputName, "rgb"))
    ((RedOutputName, "r"))
    ((GreenOutputName, "g"))
    ((BlueOutputName, "b"))
    ((AlphaOutputName, "a"))

    // UDIM detection
    ((UDIMTag, "<UDIM>"))
    (uvTilingMode)
);
// clang-format on

static const TfTokenVector _Place2dTextureConnections = {
    _tokens->coverage,    _tokens->translateFrame, _tokens->rotateFrame,   _tokens->mirrorU,
    _tokens->mirrorV,     _tokens->stagger,        _tokens->wrapU,         _tokens->wrapV,
    _tokens->repeatUV,    _tokens->offset,         _tokens->rotateUV,      _tokens->noiseUV,
    _tokens->vertexUvOne, _tokens->vertexUvTwo,    _tokens->vertexUvThree, _tokens->vertexCameraOne
};

PxrMayaUsdUVTexture_Reader::PxrMayaUsdUVTexture_Reader(const UsdMayaPrimReaderArgs& readArgs)
    : UsdMayaShaderReader(readArgs)
{
}

/* virtual */
bool PxrMayaUsdUVTexture_Reader::Read(UsdMayaPrimReaderContext* context)
{
    const auto&    prim = _GetArgs().GetUsdPrim();
    UsdShadeShader shaderSchema = UsdShadeShader(prim);
    if (!shaderSchema) {
        return false;
    }

    MStatus           status;
    MObject           mayaObject;
    MFnDependencyNode depFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              MString(prim.GetName().GetText()),
              _tokens->file.GetText(),
              UsdMayaShadingNodeType::Texture,
              &status,
              &mayaObject)
          && depFn.setObject(mayaObject))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            _tokens->file.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    context->RegisterNewMayaNode(prim.GetPath().GetString(), mayaObject);

    // Create place2dTexture:
    MObject           uvObj;
    MFnDependencyNode uvDepFn;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              _tokens->place2dTexture.GetText(),
              _tokens->place2dTexture.GetText(),
              UsdMayaShadingNodeType::Utility,
              &status,
              &uvObj)
          && uvDepFn.setObject(uvObj))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create node of type '%s' for shader '%s'.\n",
            _tokens->place2dTexture.GetText(),
            prim.GetPath().GetText());
        return false;
    }

    // Connect manually (fileTexturePlacementConnect is not available in batch):
    {
        MPlug uvPlug = uvDepFn.findPlug(_tokens->outUV.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(_tokens->uvCoord.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }
    {
        MPlug uvPlug = uvDepFn.findPlug(_tokens->outUvFilterSize.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(_tokens->uvFilterSize.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }
    MString connectCmd;
    for (const TfToken& uvName : _Place2dTextureConnections) {
        MPlug uvPlug = uvDepFn.findPlug(uvName.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(uvName.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }

    VtValue       val;
    MPlug         mayaAttr;
    UsdShadeInput usdInput = shaderSchema.GetInput(_tokens->file);
    if (usdInput && usdInput.Get(&val) && val.IsHolding<SdfAssetPath>()) {
        std::string filePath = val.UncheckedGet<SdfAssetPath>().GetResolvedPath();
        if (!filePath.empty() && !ArIsPackageRelativePath(filePath)) {
            // Maya has issues with relative paths, especially if deep inside a
            // nesting of referenced assets. Use absolute path instead if USD was
            // able to resolve. A better fix will require providing an asset
            // resolver to Maya that can resolve the file correctly using the
            // MPxFileResolver API. We also make sure the path is not expressed
            // as a relationship like texture paths inside USDZ assets.
            val = SdfAssetPath(filePath);
        }
        // Re-fetch the file name in case it is UDIM-tagged
        std::string unresolvedFilePath = val.UncheckedGet<SdfAssetPath>().GetAssetPath();
        mayaAttr = depFn.findPlug(_tokens->fileTextureName.GetText(), true, &status);
        if (status != MS::kSuccess) {
            TF_RUNTIME_ERROR(
                "Could not find the built-in attribute fileTextureName on a Maya file node: %s! "
                "Something is seriously wrong with your current Maya session.",
                depFn.name().asChar());
            return false;
        }
        // Handle UDIM texture files:
        std::string::size_type udimPos = unresolvedFilePath.rfind(_tokens->UDIMTag.GetString());
        if (udimPos != std::string::npos) {
            MPlug tilingAttr = depFn.findPlug(_tokens->uvTilingMode.GetText(), true, &status);
            if (status == MS::kSuccess) {
                tilingAttr.setInt(3);

                // USD did not resolve the path to absolute because the file name was not an
                // actual file on disk. We need to find the first tile to help Maya find the
                // other ones.
                std::string udimPath(unresolvedFilePath.substr(0, udimPos));
                udimPath += "1001";
                udimPath
                    += unresolvedFilePath.substr(udimPos + _tokens->UDIMTag.GetString().size());

                Usd_Resolver res(&prim.GetPrimIndex());
                for (; res.IsValid(); res.NextLayer()) {
                    std::string resolvedName
                        = SdfComputeAssetPathRelativeToLayer(res.GetLayer(), udimPath);

                    if (!resolvedName.empty() && !ArIsPackageRelativePath(resolvedName)
                        && resolvedName != udimPath) {
                        udimPath = resolvedName;
                        break;
                    }
                }
                val = SdfAssetPath(udimPath);
            }
        }

        const UsdMayaJobImportArgs& jobArgs = this->_GetArgs().GetJobArguments();
        if (jobArgs.importUSDZTextures && !jobArgs.importUSDZTexturesFilePath.empty()
            && !filePath.empty() && ArIsPackageRelativePath(filePath)) {
            // NOTE: (yliangsiew) Package-relatve path means that we are inside of a USDZ file.
            ArResolver& arResolver = ArGetResolver(); // NOTE: (yliangsiew) This is cached.
            std::shared_ptr<ArAsset> assetPtr = arResolver.OpenAsset(filePath);
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
                        checkPath
                            = TfStringPrintf("%s_%d%s", checkPath.c_str(), counter, ext.c_str());
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
            val = SdfAssetPath(extractedFilePath);
        }
        UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);

        // colorSpace:
        if (usdInput.GetAttr().HasColorSpace()) {
            MString colorSpace = usdInput.GetAttr().GetColorSpace().GetText();
            mayaAttr = depFn.findPlug(_tokens->colorSpace.GetText(), true, &status);
            if (status == MS::kSuccess) {
                mayaAttr.setString(colorSpace);
            }
        }
    }

    // The Maya file node's 'colorGain' and 'alphaGain' attributes map to the
    // UsdUVTexture's scale input.
    usdInput = shaderSchema.GetInput(_tokens->scale);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f scale = val.UncheckedGet<GfVec4f>();
            mayaAttr = depFn.findPlug(_tokens->colorGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(scale[0], scale[1], scale[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(_tokens->alphaGain.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = scale[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // The Maya file node's 'colorOffset' and 'alphaOffset' attributes map to
    // the UsdUVTexture's bias input.
    usdInput = shaderSchema.GetInput(_tokens->bias);
    if (usdInput) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f bias = val.UncheckedGet<GfVec4f>();
            mayaAttr = depFn.findPlug(_tokens->colorOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                val = GfVec3f(bias[0], bias[1], bias[2]);
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
            }
            mayaAttr = depFn.findPlug(_tokens->alphaOffset.GetText(), true, &status);
            if (status == MS::kSuccess) {
                // The alpha is not scaled
                val = bias[3];
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    // Default color
    usdInput = shaderSchema.GetInput(_tokens->fallback);
    mayaAttr = depFn.findPlug(_tokens->defaultColor.GetText(), true, &status);
    if (usdInput && status == MS::kSuccess) {
        if (usdInput.Get(&val) && val.IsHolding<GfVec4f>()) {
            GfVec4f fallback = val.UncheckedGet<GfVec4f>();
            val = GfVec3f(fallback[0], fallback[1], fallback[2]);
            UsdMayaReadUtil::SetMayaAttr(mayaAttr, val, /*unlinearizeColors*/ false);
        }
    }

    // Wrap U/V
    const TfToken wrapMirrorTriples[2][3] { { _tokens->wrapU, _tokens->mirrorU, _tokens->wrapS },
                                            { _tokens->wrapV, _tokens->mirrorV, _tokens->wrapT } };
    for (auto wrapMirrorTriple : wrapMirrorTriples) {
        auto wrapUVToken = wrapMirrorTriple[0];
        auto mirrorUVToken = wrapMirrorTriple[1];
        auto wrapSTToken = wrapMirrorTriple[2];

        usdInput = shaderSchema.GetInput(wrapSTToken);
        if (usdInput) {
            if (usdInput.Get(&val) && val.IsHolding<TfToken>()) {
                TfToken wrapVal = val.UncheckedGet<TfToken>();
                TfToken plugName;

                if (wrapVal == _tokens->repeat) {
                    // do nothing - will repeat by default
                    continue;
                } else if (wrapVal == _tokens->mirror) {
                    plugName = mirrorUVToken;
                    val = true;
                } else {
                    plugName = wrapUVToken;
                    val = false;
                }
                mayaAttr = uvDepFn.findPlug(plugName.GetText(), true, &status);
                if (status != MS::kSuccess) {
                    continue;
                }
                UsdMayaReadUtil::SetMayaAttr(mayaAttr, val);
            }
        }
    }

    return true;
}

/* virtual */
TfToken PxrMayaUsdUVTexture_Reader::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    TfToken               usdOutputName;
    UsdShadeAttributeType attrType;
    std::tie(usdOutputName, attrType) = UsdShadeUtils::GetBaseNameAndType(usdAttrName);

    if (attrType == UsdShadeAttributeType::Output) {
        if (usdOutputName == _tokens->RGBOutputName) {
            return _tokens->outColor;
        } else if (usdOutputName == _tokens->RedOutputName) {
            return _tokens->outColorR;
        } else if (usdOutputName == _tokens->GreenOutputName) {
            return _tokens->outColorG;
        } else if (usdOutputName == _tokens->BlueOutputName) {
            return _tokens->outColorB;
        } else if (usdOutputName == _tokens->AlphaOutputName) {
            return _tokens->outAlpha;
        }
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
