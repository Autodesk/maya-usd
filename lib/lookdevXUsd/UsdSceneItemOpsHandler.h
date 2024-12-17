//**************************************************************************/
// Copyright 2024 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
#ifndef USD_SCENE_ITEM_OPS_HANDLER_H
#define USD_SCENE_ITEM_OPS_HANDLER_H

#include "Export.h"

#include <ufe/sceneItemOpsHandler.h>

namespace LookdevXUsd
{
class LOOKDEVX_USD_EXPORT UsdSceneItemOpsHandler : public Ufe::SceneItemOpsHandler
{
public:
    using Ptr = std::shared_ptr<UsdSceneItemOpsHandler>;

    explicit UsdSceneItemOpsHandler(Ufe::SceneItemOpsHandler::Ptr mayaUsdSceneItemOpsHandler);
    ~UsdSceneItemOpsHandler() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdSceneItemOpsHandler(const UsdSceneItemOpsHandler&) = delete;
    UsdSceneItemOpsHandler& operator=(const UsdSceneItemOpsHandler&) = delete;
    UsdSceneItemOpsHandler(UsdSceneItemOpsHandler&&) = delete;
    UsdSceneItemOpsHandler& operator=(UsdSceneItemOpsHandler&&) = delete;
    //@}

    // Ufe::SceneItemOpsHandler overrides
    [[nodiscard]] Ufe::SceneItemOps::Ptr sceneItemOps(const Ufe::SceneItem::Ptr& item) const override;

    //! Create a UsdSceneItemOpsHandler.
    static UsdSceneItemOpsHandler::Ptr create(const Ufe::SceneItemOpsHandler::Ptr& mayaUsdSceneItemOpsHandler);

private:
    Ufe::SceneItemOpsHandler::Ptr m_mayaUsdSceneItemOpsHandler;

}; // UsdSceneItemOpsHandler

} // namespace LookdevXUsd
#endif