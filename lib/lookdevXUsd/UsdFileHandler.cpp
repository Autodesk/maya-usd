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

#include <pxr/usd/usdShade/udimUtils.h>

#include <LookdevXUfe/UfeUtils.h>

#include <algorithm>

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

// From USD's materialParamsUtil.cpp:
// We need to find the first layer that changes the value
// of the parameter so that we anchor relative paths to that.
static
PXR_NS::SdfLayerHandle 
findLayerHandle(const PXR_NS::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time) {
    for (const auto& spec: attr.GetPropertyStack(time)) {
        if (spec->HasDefaultValue() ||
            spec->GetLayer()->GetNumTimeSamplesForPath(
                spec->GetPath()) > 0) {
            return spec->GetLayer();
        }
    }
    return PXR_NS::TfNullPtr;
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
            const auto prim = MayaUsdAPI::getPrimForUsdSceneItem(fnAttr->sceneItem());
            const auto usdAttribute = prim.GetAttribute(PXR_NS::TfToken(fnAttr->name()));
            const auto attrQuery = PXR_NS::UsdAttributeQuery(usdAttribute);
            const auto time = MayaUsdAPI::getTime(fnAttr->sceneItem()->path());
            PXR_NS::SdfAssetPath assetPath;

            if (attrQuery.Get(&assetPath, time))
            {
                auto path = assetPath.GetResolvedPath();
                if (path.empty() && PXR_NS::UsdShadeUdimUtils::IsUdimIdentifier(assetPath.GetAssetPath()))
                {
                    path = 
                        PXR_NS::UsdShadeUdimUtils::ResolveUdimPath(
                            assetPath.GetAssetPath(), findLayerHandle(usdAttribute, time));
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
