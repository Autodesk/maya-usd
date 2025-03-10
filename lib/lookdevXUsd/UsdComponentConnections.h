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
#ifndef USD_COMPONENT_CONNECTIONS_H
#define USD_COMPONENT_CONNECTIONS_H

#include "Export.h"

#include <LookdevXUfe/ComponentConnections.h>

#include <ufe/sceneItem.h>

namespace LookdevXUsd
{

class LOOKDEVX_USD_EXPORT UsdComponentConnections : public LookdevXUfe::ComponentConnections
{
public:
    using Ptr = std::shared_ptr<UsdComponentConnections>;

    explicit UsdComponentConnections(const Ufe::SceneItem::Ptr& item);
    ~UsdComponentConnections() override = default;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdComponentConnections(const UsdComponentConnections&) = delete;
    UsdComponentConnections& operator=(const UsdComponentConnections&) = delete;
    UsdComponentConnections(UsdComponentConnections&&) = delete;
    UsdComponentConnections& operator=(UsdComponentConnections&&) = delete;
    //@}

    static UsdComponentConnections::Ptr create(const Ufe::SceneItem::Ptr& item);

    [[nodiscard]] std::vector<std::string> componentNames(const Ufe::Attribute::Ptr& attr) const override;

protected:
    [[nodiscard]] Ufe::Connections::Ptr connections(const Ufe::SceneItem::Ptr& sceneItem) const override;
}; // UsdComponentConnections

} // namespace LookdevXUsd

#endif
