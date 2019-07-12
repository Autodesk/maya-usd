// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdSceneItemOps.h"

#include "ufe/sceneItemOpsHandler.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdSceneItemOps interface object.
class MAYAUSD_CORE_PUBLIC UsdSceneItemOpsHandler : public Ufe::SceneItemOpsHandler
{
public:
	typedef std::shared_ptr<UsdSceneItemOpsHandler> Ptr;

	UsdSceneItemOpsHandler();
	~UsdSceneItemOpsHandler() override;

	// Delete the copy/move constructors assignment operators.
	UsdSceneItemOpsHandler(const UsdSceneItemOpsHandler&) = delete;
	UsdSceneItemOpsHandler& operator=(const UsdSceneItemOpsHandler&) = delete;
	UsdSceneItemOpsHandler(UsdSceneItemOpsHandler&&) = delete;
	UsdSceneItemOpsHandler& operator=(UsdSceneItemOpsHandler&&) = delete;

	//! Create a UsdSceneItemOpsHandler.
	static UsdSceneItemOpsHandler::Ptr create();

	// Ufe::SceneItemOpsHandler overrides
	Ufe::SceneItemOps::Ptr sceneItemOps(const Ufe::SceneItem::Ptr& item) const override;

private:
	UsdSceneItemOps::Ptr fUsdSceneItemOps;

}; // UsdSceneItemOpsHandler

} // namespace ufe
} // namespace MayaUsd
