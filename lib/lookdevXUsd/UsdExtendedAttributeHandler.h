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
#ifndef USD_EXTENDED_ATTRIBUTE_HANDLER_H
#define USD_EXTENDED_ATTRIBUTE_HANDLER_H

#include "Export.h"

#include <LookdevXUfe/ExtendedAttributeHandler.h>

namespace LookdevXUsd
{

class LOOKDEVX_USD_EXPORT UsdExtendedAttributeHandler : public LookdevXUfe::ExtendedAttributeHandler
{
public:
    using Ptr = std::shared_ptr<UsdExtendedAttributeHandler>;

    //! Constructor.
    UsdExtendedAttributeHandler() = default;
    //! Destructor.
    ~UsdExtendedAttributeHandler() override = default;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdExtendedAttributeHandler(const UsdExtendedAttributeHandler&) = delete;
    UsdExtendedAttributeHandler& operator=(const UsdExtendedAttributeHandler&) = delete;
    UsdExtendedAttributeHandler(UsdExtendedAttributeHandler&&) = delete;
    UsdExtendedAttributeHandler& operator=(UsdExtendedAttributeHandler&&) = delete;
    //@}

    static UsdExtendedAttributeHandler::Ptr create();

    [[nodiscard]] bool isAuthoredAttribute(const Ufe::Attribute::Ptr& attribute) const override;

}; // UsdExtendedAttributeHandler

} // namespace LookdevXUsd

#endif
