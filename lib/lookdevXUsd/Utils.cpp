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
#include "Utils.h"

#include <LookdevXUfe/UfeUtils.h>

#include <mayaUsdAPI/utils.h>

#include <ufe/attributes.h>
#include <ufe/runTimeMgr.h>

using namespace LookdevXUfe;
using namespace PXR_NS;

namespace LookdevXUsdUtils
{
SdfLayerRefPtr getSessionLayer(const Ufe::SceneItem::Ptr& item)
{
    if (!item)
    {
        return {};
    }

    auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
    auto sessionLayer = prim.GetStage()->GetSessionLayer();
    return sessionLayer;
}

TfToken getShaderSourceType(const Ufe::Attribute::Ptr& attr)
{
    std::string shaderSource;

    auto deepAttr = UfeUtils::getConnectedSource(attr);
    if (deepAttr)
    {
        auto nodeDef = UfeUtils::getNodeDef(deepAttr->sceneItem());
        if (nodeDef && nodeDef->nbClassifications() != 0)
        {
            shaderSource = TfToken(nodeDef->classification(nodeDef->nbClassifications() - 1));
        }
    }

    return TfToken(shaderSource);
}

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    return std::any_of(connectedAttrs.begin(), connectedAttrs.end(),
                       [&srcUsdAttr](const auto& path) { return path == srcUsdAttr.GetPath(); });
}

Strings splitString(const std::string& str, const std::string& separators)
{
    Strings split;

    if (str.empty())
    {
        return split;
    }

    std::string::size_type lastPos = str.find_first_not_of(separators, 0);
    std::string::size_type pos = str.find_first_of(separators, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos)
    {
        split.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(separators, pos);
        pos = str.find_first_of(separators, lastPos);
    }

    return split;
}

} // namespace LookdevXUsdUtils
