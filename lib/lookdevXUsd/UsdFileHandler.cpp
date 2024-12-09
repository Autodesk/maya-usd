//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "UsdFileHandler.h"

#include <mayaUsdAPI/utils.h>

#include <pxr/usd/sdf/layerUtils.h>

#include <LookdevXUfe/UfeUtils.h>

#include <algorithm>
#include <string_view>

namespace LookdevXUsd
{

namespace
{
std::string getRelativePath(const Ufe::AttributeFilename::Ptr& fnAttr, const std::string& path)
{
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == MayaUsdAPI::getUsdRunTimeId())
    {
        auto stage = MayaUsdAPI::usdStage(fnAttr);
        if (stage && stage->GetEditTarget().GetLayer())
        {
            const std::string layerDirPath = MayaUsdAPI::getDir(stage->GetEditTarget().GetLayer()->GetRealPath());

            auto relativePathAndSuccess = MayaUsdAPI::makePathRelativeTo(path, layerDirPath);

            if (relativePathAndSuccess.second && relativePathAndSuccess.first != path)
            {
                return relativePathAndSuccess.first;
            }
        }
    }
    return path;
}

// This function is basically:
//    return prefix + middle + suffix;
// But in a way that allocates memory only once and keeps clang-tidy happy.
//  See: performance-inefficient-string-concatenation in clang-tidy docs.
std::string buildFullPath(const std::string_view& prefix,
                          const std::string_view& middle,
                          const std::string_view& suffix)
{
    std::string path;
    path.reserve(prefix.size() + middle.size() + suffix.size() + 1);
    path.append(prefix);
    path.append(middle);
    path.append(suffix);
    return path;
}

std::string resolveUdimPath(const std::string& path, const Ufe::Attribute::Ptr& attribute)
{
    constexpr auto kUdimMin = 1001;
    constexpr auto kUdimMax = 1100;
    constexpr auto kUdimNumSize = 4;
    constexpr std::string_view kUdimTag = "<UDIM>";

    const auto udimPos = path.rfind(kUdimTag);
    if (udimPos == std::string::npos)
    {
        return {};
    }

    const auto stage = MayaUsdAPI::usdStage(attribute);
    if (!stage || !stage->GetEditTarget().GetLayer())
    {
        return {};
    }

    // Going for minimal alloc code using string_view to the fullest :)
    const auto udimPrefix = std::string_view{path.data(), udimPos};
    const auto udimEndPos = udimPos + kUdimTag.size();

    const auto udimPostfix =
        std::string_view{path.data() + static_cast<ptrdiff_t>(udimEndPos), path.size() - udimEndPos};

    // Build numericPath once and re-use it in the loop:
    auto numericPath = buildFullPath(udimPrefix, std::to_string(kUdimMin), udimPostfix);
    const auto numericStart = numericPath.begin() + static_cast<std::string::difference_type>(udimPrefix.size());
    const auto numericEnd = numericStart + static_cast<std::string::difference_type>(kUdimNumSize);

    for (auto i = kUdimMin; i < kUdimMax; ++i)
    {
        numericPath.replace(numericStart, numericEnd, std::to_string(i));

        const auto resolved =
            PXR_NS::SdfComputeAssetPathRelativeToLayer(stage->GetEditTarget().GetLayer(), numericPath);
        if (resolved.size() != numericPath.size())
        {
            // Restore the <UDIM> tag in the resolved path:
            const auto prefixLength = resolved.size() - udimPostfix.size() - kUdimNumSize;
            return buildFullPath({resolved.data(), prefixLength}, kUdimTag, udimPostfix);
        }
    }

    return {};
}

} // namespace

UsdFileHandler::UsdFileHandler() : LookdevXUfe::FileHandler()
{
}

UsdFileHandler::~UsdFileHandler() = default;

std::string UsdFileHandler::getResolvedPath(const Ufe::AttributeFilename::Ptr& fnAttr) const
{
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == MayaUsdAPI::getUsdRunTimeId())
    {
        auto attributeType = MayaUsdAPI::usdAttributeType(fnAttr);
        if (attributeType == PXR_NS::SdfValueTypeNames->Asset)
        {
            PXR_NS::VtValue vt;
            if (MayaUsdAPI::getUsdValue(fnAttr, vt, MayaUsdAPI::getTime(fnAttr->sceneItem()->path())) &&
                vt.IsHolding<PXR_NS::SdfAssetPath>())
            {
                const auto& assetPath = vt.UncheckedGet<PXR_NS::SdfAssetPath>();
                auto path = assetPath.GetResolvedPath();
                if (path.empty())
                {
                    path = resolveUdimPath(assetPath.GetAssetPath(), fnAttr);
                }
#ifdef _WIN32
                std::replace(path.begin(), path.end(), '\\', '/');
#endif
                return path;
            }
        }
    }

    return {};
}

Ufe::UndoableCommand::Ptr UsdFileHandler::convertPathToAbsoluteCmd(const Ufe::AttributeFilename::Ptr& fnAttr) const
{
    std::string storedPath = fnAttr->get();
    std::string absolutePath = getResolvedPath(fnAttr);
    if (!absolutePath.empty() && absolutePath != storedPath)
    {
        return fnAttr->setCmd(absolutePath);
    }
    return {};
}

Ufe::UndoableCommand::Ptr UsdFileHandler::convertPathToRelativeCmd(const Ufe::AttributeFilename::Ptr& fnAttr) const
{
    auto relativePath = getRelativePath(fnAttr, fnAttr->get());
    if (!relativePath.empty() && relativePath != fnAttr->get())
    {
        return fnAttr->setCmd(relativePath);
    }
    return {};
}

Ufe::UndoableCommand::Ptr UsdFileHandler::setPreferredPathCmd(const Ufe::AttributeFilename::Ptr& fnAttr,
                                                              const std::string& path) const
{
    if (MayaUsdAPI::requireUsdPathsRelativeToEditTargetLayer())
    {
        auto relativePath = getRelativePath(fnAttr, path);

        if (!relativePath.empty() && relativePath != path)
        {
            return fnAttr->setCmd(LookdevXUfe::UfeUtils::insertUdimTagInFilename(relativePath));
        }
    }
    return fnAttr->setCmd(LookdevXUfe::UfeUtils::insertUdimTagInFilename(path));
}

std::string UsdFileHandler::openFileDialog(const Ufe::AttributeFilename::Ptr& fnAttr) const
{
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == MayaUsdAPI::getUsdRunTimeId())
    {
        auto fileHandler = LookdevXUfe::FileHandler::get(fnAttr->sceneItem()->path().popSegment().runTimeId());
        if (!fileHandler)
        {
            return {};
        }

        std::string relativeRoot;
        auto stage = MayaUsdAPI::usdStage(fnAttr);
        if (stage && stage->GetEditTarget().GetLayer())
        {
            relativeRoot = MayaUsdAPI::getDir(stage->GetEditTarget().GetLayer()->GetRealPath());
        }

        // Delegate to the DCC FileHandler:
        std::string pickedPath = fileHandler->openImageFileDialog(getResolvedPath(fnAttr), true, relativeRoot);

        // Mark the path as potentially relative if it was added in an anonymous layer:
        if (stage && stage->GetEditTarget().GetLayer())
        {
            pickedPath = MayaUsdAPI::handleAssetPathThatMaybeRelativeToLayer(
                pickedPath, fnAttr->name(), stage->GetEditTarget().GetLayer(),
                "mayaUsd_MakePathRelativeToImageEditTargetLayer");
        }

        return pickedPath;
    }
    return {};
}
} // namespace LookdevXUsd
