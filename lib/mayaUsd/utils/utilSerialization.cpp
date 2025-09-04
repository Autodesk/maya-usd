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
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/utils/layerLocking.h>
#include <mayaUsd/utils/layerMuting.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/targetLayer.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/stageCacheContext.h>
#if PXR_VERSION < 2508
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#else
#include <pxr/usd/sdf/usdFileFormat.h>
#include <pxr/usd/sdf/usdaFileFormat.h>
#include <pxr/usd/sdf/usdcFileFormat.h>
#endif
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <ghc/filesystem.hpp>

#include <string>

namespace {

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

void populateChildren(
    const std::string&           proxyPath,
    const UsdStageRefPtr&        stage,
    SdfLayerRefPtr               layer,
    RecursionDetector*           recursionDetector,
    MayaUsd::utils::LayerInfos&  anonLayersToSave,
    std::vector<SdfLayerRefPtr>& dirtyLayersToSave)
{
    auto subPaths = layer->GetSubLayerPaths();

    RecursionDetector defaultDetector;
    if (!recursionDetector) {
        recursionDetector = &defaultDetector;
    }
    recursionDetector->push(layer->GetRealPath());

    for (auto iter = subPaths.rbegin(); iter != subPaths.rend(); ++iter) {
        std::string path = *iter;
        std::string actualPath = PXR_NS::SdfComputeAssetPathRelativeToLayer(layer, path);
        auto        subLayer = PXR_NS::SdfLayer::FindOrOpen(actualPath);
        if (subLayer && !recursionDetector->contains(subLayer->GetRealPath())) {
            populateChildren(
                proxyPath, stage, subLayer, recursionDetector, anonLayersToSave, dirtyLayersToSave);

            if (subLayer->IsAnonymous()) {
                MayaUsd::utils::LayerInfo info;
                info.stage = stage;
                info.layer = subLayer;
                info.parent._proxyPath = proxyPath;
                info.parent._layerParent = layer;
                anonLayersToSave.push_back(info);
            } else if (subLayer->IsDirty()) {
                dirtyLayersToSave.push_back(subLayer);
            }
        }
    }

    recursionDetector->pop();
}

void updateMutedLayers(
    const UsdStageRefPtr& stage,
    const SdfLayerRefPtr& oldLayer,
    const SdfLayerRefPtr& newLayer)
{
    if (!stage)
        return;
    if (!oldLayer)
        return;
    if (!newLayer)
        return;

    if (stage->IsLayerMuted(oldLayer->GetIdentifier())) {
        MayaUsd::addMutedLayer(newLayer);
        stage->MuteLayer(newLayer->GetIdentifier());
    }
}

void updateLockedLayers(
    const std::string&    proxyPath,
    const SdfLayerRefPtr& oldLayer,
    const SdfLayerRefPtr& newLayer)
{
    if (!oldLayer)
        return;
    if (!newLayer)
        return;

    if (MayaUsd::isLayerSystemLocked(oldLayer)) {
        MayaUsd::lockLayer(proxyPath, newLayer, MayaUsd::LayerLock_SystemLocked);
    } else if (MayaUsd::isLayerLocked(oldLayer)) {
        MayaUsd::lockLayer(proxyPath, newLayer, MayaUsd::LayerLock_Locked);
    }
}

void updateTargetLayer(const std::string& proxyNodeName, const SdfLayerRefPtr& layer)
{
    if (MayaUsdProxyShapeBase* proxyShape = UsdMayaUtil::GetProxyShapeByProxyName(proxyNodeName)) {
        proxyShape->getUsdStage()->SetEditTarget(layer);
    }
}

void updateRootLayer(
    const std::string&            proxy,
    const std::string&            layerPath,
    MayaUsd::utils::ProxyPathMode proxyPathMode,
    const SdfLayerRefPtr&         layer,
    bool                          isTargetLayer)
{
    // Upda the root layer of the given proxy shape
    if (layerPath.empty() || proxy.empty())
        return;

#ifdef _WIN32
    // Building a string that includes a file path for an executeCommand call
    // can be problematic, easier to just switch the path separator.
    const std::string fp = TfStringReplace(layerPath, "\\", "/");
#else
    const std::string& fp = layerPath;
#endif

    MayaUsd::utils::setNewProxyPath(
        MString(proxy.c_str()), MString(fp.c_str()), proxyPathMode, layer, isTargetLayer);
}

void updateAllCachedStageWithLayer(SdfLayerRefPtr originalLayer, const std::string& newFilePath)
{
    // Update all known stage caches managed by the Maya USD plugin that contained
    // stages using the original anonymous layer so that new stagesusing new saved
    // layer are created with the load rules and the muted layers of the original
    // stage.
    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(newFilePath);
    if (!newLayer) {
        TF_WARN("The filename %s is an invalid file name for a layer.", newFilePath.c_str());
        return;
    }
    for (UsdStageCache& cache : UsdMayaStageCache::GetAllCaches()) {
        UsdStageCacheContext        ctx(cache);
        std::vector<UsdStageRefPtr> stages = cache.FindAllMatching(originalLayer);
        for (const auto& stage : stages) {
            auto sessionLayer = stage->GetSessionLayer();
            // Note: See comments in lib\mayaUsd\nodes\proxyShapeBase.cpp, in the
            //       function computeInStageDataCached() about requirements about
            //       matching UsdStage::Open() arguments to find a stage.
            auto newStage = UsdStage::UsdStage::Open(
                newLayer, sessionLayer, UsdStage::InitialLoadSet::LoadNone);
            newStage->SetLoadRules(stage->GetLoadRules());
            newStage->MuteAndUnmuteLayers(stage->GetMutedLayers(), {});
        }
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace utils {

std::string suggestedStartFolder(PXR_NS::UsdStageRefPtr stage)
{
    PXR_NS::SdfLayerRefPtr root = stage ? stage->GetRootLayer() : nullptr;
    if (!root && !root->IsAnonymous()) {
        return root->GetRealPath();
    }

    return getSceneFolder();
}

std::string getSceneFolder()
{
    std::string fileDir = UsdMayaUtilFileSystem::getMayaSceneFileDir();
    if (fileDir.empty()) {
        fileDir = UsdMayaUtilFileSystem::getMayaWorkspaceScenesDir();
    }

    return fileDir;
}

std::string generateUniqueFileName(const std::string& basename)
{
    std::string newFileName = UsdMayaUtilFileSystem::getUniqueFileName(
        getSceneFolder(),
        !basename.empty() ? basename : "anonymous",
#if PXR_VERSION < 2508
        PXR_NS::UsdUsdFileFormatTokens->Id.GetText());
#else
        PXR_NS::SdfUsdFileFormatTokens->Id.GetText());
#endif
    return newFileName;
}

std::string generateUniqueLayerFileName(const std::string& basename, const SdfLayerRefPtr& layer)
{
    std::string layerNumber("1");
    if (layer)
        layerNumber = UsdMayaUtilFileSystem::getNumberSuffix(layer->GetDisplayName());

#if PXR_VERSION < 2508
    const std::string ext = PXR_NS::UsdUsdFileFormatTokens->Id.GetText();
#else
    const std::string ext = PXR_NS::SdfUsdFileFormatTokens->Id.GetText();
#endif
    const std::string layerFilename = basename + "-layer" + layerNumber + "." + ext;
    const std::string dir = getSceneFolder();

    return UsdMayaUtilFileSystem::ensureUniqueFileName(
        UsdMayaUtilFileSystem::appendPaths(dir, layerFilename));
}

std::string usdFormatArgOption()
{
    static const MString kSaveLayerFormatBinaryOption(
        MayaUsdOptionVars->SaveLayerFormatArgBinaryOption.GetText());
    bool binary = true;
    if (MGlobal::optionVarExists(kSaveLayerFormatBinaryOption)) {
        binary = MGlobal::optionVarIntValue(kSaveLayerFormatBinaryOption) != 0;
    } else {
        MGlobal::setOptionVarValue(kSaveLayerFormatBinaryOption, 1);
    }
#if PXR_VERSION < 2508
    return binary ? PXR_NS::UsdUsdcFileFormatTokens->Id.GetText()
                  : PXR_NS::UsdUsdaFileFormatTokens->Id.GetText();
#else
    return binary ? PXR_NS::SdfUsdcFileFormatTokens->Id.GetText()
                  : PXR_NS::SdfUsdaFileFormatTokens->Id.GetText();
#endif
}

/* static */
USDUnsavedEditsOption serializeUsdEditsLocationOption()
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

bool isProxyShapePathRelative(MayaUsdProxyShapeBase& proxyShape)
{
    MStatus           status;
    MFnDependencyNode depNode(proxyShape.thisMObject(), &status);
    if (!status)
        return false;

    MPlug filePathRelativePlug = depNode.findPlug(MayaUsdProxyShapeBase::filePathRelativeAttr);
    return filePathRelativePlug.asBool();
}

bool isProxyPathModeRelative(ProxyPathMode proxyPathMode, const MString& proxyNodeName)
{
    if (kProxyPathRelative == proxyPathMode)
        return true;

    if (kProxyPathAbsolute == proxyPathMode)
        return false;

    if (kProxyPathFollowProxyShape == proxyPathMode) {
        // Note: if we fail to find the proxy shape, we will fallback on
        //       using the options var preference instead.
        MayaUsdProxyShapeBase* proxyShape
            = UsdMayaUtil::GetProxyShapeByProxyName(proxyNodeName.asChar());
        if (proxyShape) {
            return isProxyShapePathRelative(*proxyShape);
        }
    }

    return UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
}

void setNewProxyPath(
    const MString&        proxyNodeName,
    const MString&        newRootLayerPath,
    ProxyPathMode         proxyPathMode,
    const SdfLayerRefPtr& layer,
    bool                  isTargetLayer)
{
    const bool  needRelativePath = isProxyPathModeRelative(proxyPathMode, proxyNodeName);
    const char* filePathCmd = "setAttr -type \"string\" ^1s.filePath \"^2s\"; "
                              "setAttr ^1s.filePathRelative ^3s; ";

    MString script;
    script.format(filePathCmd, proxyNodeName, newRootLayerPath, needRelativePath ? "1" : "0");
    MGlobal::executeCommand(
        script,
        /*display*/ true,
        /*undo*/ false);

    if (isTargetLayer)
        updateTargetLayer(proxyNodeName.asChar(), layer);
}

static bool isCompatibleWithSave(
    SdfLayerRefPtr     layer,
    const std::string& filePath,
    const std::string& formatArg)
{
    if (!layer)
        return false;

    // Save cannot specify the filename, so the file name must match to use save.
    if (layer->GetRealPath() != filePath)
        return false;

#if PXR_VERSION < 2508
    const TfToken underlyingFormat = UsdUsdFileFormat::GetUnderlyingFormatForLayer(*layer);
#else
    const TfToken underlyingFormat = SdfUsdFileFormat::GetUnderlyingFormatForLayer(*layer);
#endif
    if (underlyingFormat.size()) {
        return underlyingFormat == formatArg;
    } else {
        const SdfFileFormat::FileFormatArguments currentFormatArgs
            = layer->GetFileFormatArguments();

        // If we cannot find the format argument then we cannot validate that the file format match
        // so we err to the side of safety and claim they don't match.
        const auto keyAndValue = currentFormatArgs.find("format");
        if (keyAndValue == currentFormatArgs.end())
            return false;

        return keyAndValue->second == formatArg;
    }
}

void setLayerUpAxisAndUnits(const SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    // Don't try to author the metadata on non-editable layers.
    if (!layer->PermissionToEdit())
        return;

    const PXR_NS::TfToken upAxis
        = MGlobal::isZAxisUp() ? PXR_NS::UsdGeomTokens->z : PXR_NS::UsdGeomTokens->y;
    const double metersPerUnit
        = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(MDistance::internalUnit());

    // Note: code similar to what UsdGeomSetStageUpAxis -> UsdStage::SetMetadata end-up doing,
    // but without having to have a stage. We basically set metadata on the virtual root object
    // of the layer.
    layer->SetField(
        PXR_NS::SdfPath::AbsoluteRootPath(), PXR_NS::UsdGeomTokens->metersPerUnit, metersPerUnit);
    layer->SetField(PXR_NS::SdfPath::AbsoluteRootPath(), PXR_NS::UsdGeomTokens->upAxis, upAxis);
}

bool saveLayerWithFormat(
    SdfLayerRefPtr     layer,
    const std::string& requestedFilePath,
    const std::string& requestedFormatArg)
{
    const std::string& filePath
        = requestedFilePath.empty() ? layer->GetRealPath() : requestedFilePath;

    const std::string& formatArg
        = requestedFormatArg.empty() ? usdFormatArgOption() : requestedFormatArg;

    UsdMayaUtilFileSystem::updatePostponedRelativePaths(layer, filePath);

    if (isCompatibleWithSave(layer, filePath, formatArg)) {
        if (!layer->Save()) {
            return false;
        }
    } else {
        PXR_NS::SdfFileFormat::FileFormatArguments args;
#if PXR_VERSION < 2508
        args[UsdUsdFileFormatTokens->FormatArg] = formatArg;
#else
        args[SdfUsdFileFormatTokens->FormatArg] = formatArg;
#endif
        if (!layer->Export(filePath, "", args)) {
            return false;
        }
    }

    // Update all known stage caches if the layer was saved to a new file path.
    // Skip this step when the layer's file path hasn't changed to avoid unnecessary stage
    // recompositions.
    if (!requestedFilePath.empty()) {
        updateAllCachedStageWithLayer(layer, filePath);
    }

    return true;
}

SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr     stage,
    SdfLayerRefPtr     anonLayer,
    LayerParent        parent,
    const std::string& basename,
    std::string        formatArg,
    std::string*       errorMsg)
{
    PathInfo pathInfo;
    pathInfo.absolutePath = generateUniqueLayerFileName(basename, anonLayer);
    return saveAnonymousLayer(stage, anonLayer, pathInfo, parent, formatArg, errorMsg);
}

static void formatErrorMsg(
    const char*           message,
    const SdfLayerRefPtr& anonLayer,
    const std::string     absPath,
    std::string*          errorMsg)
{
    if (!errorMsg)
        return;

    MString text;
    text.format(message, anonLayer->GetDisplayName().c_str(), absPath.c_str());
    *errorMsg = text.asChar();
}

SdfLayerRefPtr saveAnonymousLayer(
    UsdStageRefPtr  stage,
    SdfLayerRefPtr  anonLayer,
    const PathInfo& pathInfo,
    LayerParent     parent,
    std::string     formatArg,
    std::string*    errorMsg)
{
    UsdMayaUtilFileSystem::FileBackup backup(pathInfo.absolutePath);
    std::string                       filePath(pathInfo.absolutePath);

    if (!anonLayer) {
        formatErrorMsg("No layer provided to save to \"^2s\"", anonLayer, filePath, errorMsg);
        return nullptr;
    }

    if (!anonLayer->IsAnonymous()) {
        formatErrorMsg(
            "Cannot save non-anonymous layer \"^1\" under a different file name",
            anonLayer,
            filePath,
            errorMsg);
        return nullptr;
    }

    if (isLayerSystemLocked(anonLayer)) {
        formatErrorMsg(
            "Cannot save layer \"^1\" when system-locked", anonLayer, filePath, errorMsg);
        return nullptr;
    }

    // Only set up-axis and units metadata on the root layer
    // and only if it is anonymous before being saved.
    if (stage->GetRootLayer() == anonLayer) {
        setLayerUpAxisAndUnits(anonLayer);
    }

    ensureUSDFileExtension(filePath);

    const bool wasTargetLayer = (stage->GetEditTarget().GetLayer() == anonLayer);

    if (!saveLayerWithFormat(anonLayer, filePath, formatArg)) {
        formatErrorMsg("Failed to save layer \"^1\" to \"^2s\"", anonLayer, filePath, errorMsg);
        return nullptr;
    }

    auto       parentLayer = parent._layerParent;
    const bool isSubLayer = (parentLayer != nullptr);

    if (pathInfo.savePathAsRelative) {
        if (!pathInfo.customRelativeAnchor.empty()) {
            std::string relativePathAnchor = pathInfo.customRelativeAnchor;
            filePath
                = UsdMayaUtilFileSystem::makePathRelativeTo(filePath, relativePathAnchor).first;
        } else if (isSubLayer) {
            filePath = UsdMayaUtilFileSystem::getPathRelativeToLayerFile(filePath, parentLayer);
            if (ghc::filesystem::path(filePath).is_absolute()) {
                UsdMayaUtilFileSystem::markPathAsPostponedRelative(parentLayer, filePath);
            }
        } else {
            filePath = UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(filePath);
        }
    } else {
        if (isSubLayer) {
            UsdMayaUtilFileSystem::unmarkPathAsPostponedRelative(parentLayer, filePath);
        }
    }

    // Note: we need to open the layer with the absolute path. The relative path is only
    //       used by the parent layer to refer to the sub-layer relative to itself. When
    //       opening the layer in isolation, we need to use the absolute path. Failure to
    //       do so will make finding the layer by its own identifier fail! A symptom of
    //       this failure is that drag-and-drop in the Layer Manager UI fails immediately
    //       after saving a layer with a relative path.
    SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(pathInfo.absolutePath);

    if (!newLayer) {
        formatErrorMsg("Failed to reload layer \"^1\" from \"^2\"", anonLayer, filePath, errorMsg);
        return nullptr;
    }

    // Now replace the layer in the parent, using a relative path if requested.
    if (isSubLayer) {
        updateSubLayer(parentLayer, anonLayer, filePath);
    } else if (!parent._proxyPath.empty()) {
        updateRootLayer(
            parent._proxyPath,
            filePath,
            pathInfo.savePathAsRelative ? kProxyPathRelative : kProxyPathAbsolute,
            newLayer,
            wasTargetLayer);
    }

    updateTargetLayer(parent._proxyPath, newLayer);
    updateMutedLayers(stage, anonLayer, newLayer);
    updateLockedLayers(parent._proxyPath, anonLayer, newLayer);

    backup.commit();

    return newLayer;
}

void updateSubLayer(
    const SdfLayerRefPtr& parentLayer,
    const SdfLayerRefPtr& oldSubLayer,
    const std::string&    newSubLayerPath)
{
    if (!parentLayer)
        return;

    if (!oldSubLayer)
        return;

    // Note: we don't know if the old sub-layer was referenced with an absolute
    //       or relative path, so we try replacing both and its identifier.
    SdfSubLayerProxy subLayers = parentLayer->GetSubLayerPaths();

    subLayers.Replace(oldSubLayer->GetIdentifier(), newSubLayerPath);

    const std::string oldAbsPath = oldSubLayer->GetRealPath();
    if (oldAbsPath.length() > 0) {
        subLayers.Replace(oldAbsPath, newSubLayerPath);

        const std::string oldRelPath
            = UsdMayaUtilFileSystem::getPathRelativeToLayerFile(oldAbsPath, parentLayer);
        subLayers.Replace(oldRelPath, newSubLayerPath);
    }
}

void ensureUSDFileExtension(std::string& filePath)
{
    const std::string& extension = SdfFileFormat::GetFileExtension(filePath);
    const std::string  defaultExt(UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText());
    const std::string  usdCrateExt(UsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText());
    const std::string  usdASCIIExt(UsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText());
    const std::string  usdPackageExt(UsdMayaTranslatorTokens->UsdFileExtensionPackage.GetText());
    if (extension != defaultExt && extension != usdCrateExt && extension != usdASCIIExt
        && extension != usdPackageExt) {
        filePath.append(".");
        filePath.append(defaultExt.c_str());
    }
}

void getLayersToSaveFromProxy(const std::string& proxyPath, StageLayersToSave& layersInfo)
{
    auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);
    if (!stage) {
        return;
    }

    auto root = stage->GetRootLayer();
    populateChildren(
        proxyPath, stage, root, nullptr, layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        LayerInfo info;
        info.stage = stage;
        info.layer = root;
        info.parent._proxyPath = proxyPath;
        info.parent._layerParent = nullptr;
        layersInfo._anonLayers.push_back(info);
    } else if (root->IsDirty()) {
        layersInfo._dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(
        proxyPath,
        stage,
        session,
        nullptr,
        layersInfo._anonLayers,
        layersInfo._dirtyFileBackedLayers);
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
