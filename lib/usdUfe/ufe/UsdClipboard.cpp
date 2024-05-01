//
// Copyright 2024 Autodesk
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

#include "UsdClipboard.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#include <pxr/usd/usdShade/connectableAPI.h>

#if (__cplusplus < 201703L)
#error "Compiling ClipboardHandler requires C++17"
#endif
#include <filesystem> // Requires C++17

namespace USDUFE_NS_DEF {

// Ensure that UsdClipboard is properly setup.
static_assert(!std::is_copy_constructible<UsdClipboard>::value);
static_assert(!std::is_copy_assignable<UsdClipboard>::value);
static_assert(!std::is_move_constructible<UsdClipboard>::value);
static_assert(!std::is_move_assignable<UsdClipboard>::value);

UsdClipboard::UsdClipboard()
{
    setClipboardPath(std::filesystem::temp_directory_path().string());
    setClipboardFileFormat(PXR_NS::UsdUsdcFileFormatTokens->Id.GetText()); // Default = binary
}

UsdClipboard::~UsdClipboard() { cleanClipboard(); }

void UsdClipboard::setClipboardData(const PXR_NS::UsdStageWeakPtr& clipboardData)
{
    // Note: if a clipboard file already exists, it automatically gets overridden, so there is no
    // need to clear it.
    // Note: export the root layer directly as the stage export will flatten which removes
    //       variant sets, payloads, etc.
    PXR_NS::SdfFileFormat::FileFormatArguments args;
    args[PXR_NS::UsdUsdFileFormatTokens->FormatArg] = _clipboardFileFormat;
    if (!clipboardData->GetRootLayer()->Export(_clipboardFilePath, "UsdUfe clipboard", args)) {
        const std::string error
            = "Failed to export Clipboard stage with destination: " + _clipboardFilePath + ".";
        throw std::runtime_error(error);
    }

    // Unload the stage, otherwise when we try to set and get the next clipboard data we end up with
    // the old stage.
    clipboardData->Unload();
}

PXR_NS::UsdStageWeakPtr UsdClipboard::getClipboardData()
{
    // Check if the layer exists
    auto layer = PXR_NS::SdfLayer::FindOrOpen(_clipboardFilePath);
    if (!layer)
        return {};

    PXR_NS::UsdStageRefPtr clipboardStage = PXR_NS::UsdStage::Open(_clipboardFilePath);

    cleanClipboardStageCache();

    // Add the clipboard USD stage to UsdUtilsStageCache, otherwise it is destroyed once out of
    // scope.
    _clipboardStageCacheId = PXR_NS::UsdUtilsStageCache::Get().Insert(clipboardStage);

    // Force the new stage to reload, so we don't end up with the old stage.
    clipboardStage->Reload();

    return clipboardStage;
}

void UsdClipboard::setClipboardPath(const std::string& clipboardPath)
{
    auto tmpPath = std::filesystem::path(clipboardPath);
    tmpPath.append(clipboardFileName);
    setClipboardFilePath(tmpPath.string());
}

void UsdClipboard::setClipboardFilePath(const std::string& clipboardFilePath)
{
    _clipboardFilePath = clipboardFilePath;
}

void UsdClipboard::setClipboardFileFormat(const std::string& formatTag)
{
    _clipboardFileFormat = formatTag;
}

void UsdClipboard::cleanClipboard()
{
    cleanClipboardStageCache();
    removeClipboardFile();
}

void UsdClipboard::cleanClipboardStageCache()
{
    // Erase the clipboard stage from the cache.
    if (_clipboardStageCacheId.IsValid()) {
        auto clipboardStageRef = PXR_NS::UsdUtilsStageCache::Get().Find(_clipboardStageCacheId);
        PXR_NS::UsdUtilsStageCache::Get().Erase(clipboardStageRef);
    }
}

void UsdClipboard::removeClipboardFile() { std::filesystem::remove(_clipboardFilePath); }

} // namespace USDUFE_NS_DEF
