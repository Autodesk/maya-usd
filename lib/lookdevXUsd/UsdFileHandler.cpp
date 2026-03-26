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

#include <pxr/usd/usdShade/udimUtils.h>

#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/Global.h>

#include <LookdevXUfe/UfeUtils.h>

#include <algorithm>

#ifdef LDX_USD_USE_MAYAUSDAPI
#include <mayaUsdAPI/utils.h>
#endif

namespace LookdevXUsd
{

namespace
{
#ifdef LDX_USD_USE_MAYAUSDAPI
std::string getRelativePath(const Ufe::AttributeFilename::Ptr& fnAttr, const std::string& path)
{
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == UsdUfe::getUsdRunTimeId())
    {
        auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(fnAttr);
        auto stage = usdAttribute ? usdAttribute->usdPrim().GetStage() : nullptr;
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
#endif

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
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == UsdUfe::getUsdRunTimeId())
    {
        auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(fnAttr);
        auto attributeType = usdAttribute ? usdAttribute->usdAttributeType() : PXR_NS::SdfValueTypeName();
        if (attributeType == PXR_NS::SdfValueTypeNames->Asset)
        {
            auto usdAttrItem = UsdUfe::downcast(fnAttr->sceneItem());
            auto prim = usdAttrItem ? usdAttrItem->prim() : PXR_NS::UsdPrim();
            const auto usdAttribute = prim.GetAttribute(PXR_NS::TfToken(fnAttr->name()));
            const auto attrQuery = PXR_NS::UsdAttributeQuery(usdAttribute);
            const auto time = UsdUfe::getTime(fnAttr->sceneItem()->path());
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
#ifdef LDX_USD_USE_MAYAUSDAPI
    auto relativePath = getRelativePath(fnAttr, fnAttr->get());
    if (!relativePath.empty() && relativePath != fnAttr->get())
    {
        return fnAttr->setCmd(relativePath);
    }
#endif
    return {};
}

Ufe::UndoableCommand::Ptr UsdFileHandler::setPreferredPathCmd(const Ufe::AttributeFilename::Ptr& fnAttr,
                                                              const std::string& path) const
{
#ifdef LDX_USD_USE_MAYAUSDAPI
    if (MayaUsdAPI::requireUsdPathsRelativeToEditTargetLayer())
    {
        auto relativePath = getRelativePath(fnAttr, path);

        if (!relativePath.empty() && relativePath != path)
        {
            return fnAttr->setCmd(LookdevXUfe::UfeUtils::insertUdimTagInFilename(relativePath));
        }
    }
#endif
    return fnAttr->setCmd(LookdevXUfe::UfeUtils::insertUdimTagInFilename(path));
}

std::string UsdFileHandler::openFileDialog(const Ufe::AttributeFilename::Ptr& fnAttr) const
{
#ifdef LDX_USD_USE_MAYAUSDAPI
    if (fnAttr && fnAttr->sceneItem()->runTimeId() == UsdUfe::getUsdRunTimeId())
    {
        auto fileHandler = LookdevXUfe::FileHandler::get(fnAttr->sceneItem()->path().popSegment().runTimeId());
        if (!fileHandler)
        {
            return {};
        }

        std::string relativeRoot;

        auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(fnAttr);
        auto stage = usdAttribute ? usdAttribute->usdPrim().GetStage() : nullptr;
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
#endif
    return {};
}
} // namespace LookdevXUsd
