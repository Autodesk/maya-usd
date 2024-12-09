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
#include "UsdDebugHandler.h"

#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/prim.h>

#include <mayaUsdAPI/utils.h>

#include <any>

namespace LookdevXUsd
{

std::string UsdDebugHandler::dumpPrim(const PXR_NS::UsdPrim& prim)
{
    // There's no export to string for primitives, so instead we grab the stage for
    // the prim and then copy the isolated prim to an empty layer and export that
    // layer to a string.
    PXR_NS::UsdStagePtr shapeStage = prim.GetStage();
    PXR_NS::SdfLayerRefPtr sourceLayer = shapeStage->Flatten();
    PXR_NS::SdfLayerRefPtr primLayer = PXR_NS::SdfLayer::CreateAnonymous();

    std::string isolatedPrimPath = prim.GetName();
    isolatedPrimPath = "/" + isolatedPrimPath;
    PXR_NS::SdfCopySpec(sourceLayer, prim.GetPath(), primLayer, PXR_NS::SdfPath(isolatedPrimPath));

    std::string primLayerString;
    primLayer->ExportToString(&primLayerString);
    return primLayerString;
}

std::string UsdDebugHandler::exportToString(Ufe::SceneItem::Ptr sceneItem)
{
    auto prim = MayaUsdAPI::getPrimForUsdSceneItem(sceneItem);
    if (prim.IsValid())
    {
        return dumpPrim(prim);
    }
    return "";
}

void UsdDebugHandler::runCommand(const std::string& /*command*/,
                                 const std::unordered_map<std::string, std::any>& /*args*/)
{
}

bool UsdDebugHandler::hasViewportSupport(const Ufe::NodeDef::Ptr& /*nodeDef*/) const
{
    // TODO(LOOKDEVX-2713): Add viewport support indication for USD.
    // Currently MayaUSD doesn't render Arnold nodes in the viewport, please refer to the ticket for more details.
    return true;
}

} // namespace LookdevXUsd
