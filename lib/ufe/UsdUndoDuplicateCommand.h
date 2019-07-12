// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdSceneItem.h"

#include "ufe/path.h"
#include "ufe/undoableCommand.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDuplicateCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateCommand : public Ufe::UndoableCommand
{
public:
	typedef std::shared_ptr<UsdUndoDuplicateCommand> Ptr;

	UsdUndoDuplicateCommand(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath);
	~UsdUndoDuplicateCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdUndoDuplicateCommand(const UsdUndoDuplicateCommand&) = delete;
	UsdUndoDuplicateCommand& operator=(const UsdUndoDuplicateCommand&) = delete;
	UsdUndoDuplicateCommand(UsdUndoDuplicateCommand&&) = delete;
	UsdUndoDuplicateCommand& operator=(UsdUndoDuplicateCommand&&) = delete;

	//! Create a UsdUndoDuplicateCommand from a USD prim and UFE path.
	static UsdUndoDuplicateCommand::Ptr create(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath);

	const SdfPath& usdDstPath() const;

	//! Return the USD destination path and layer.
	static void primInfo(const UsdPrim& srcPrim, SdfPath& usdDstPath, SdfLayerHandle& srcLayer);

	//! Duplicate the prim hierarchy at usdSrcPath.
	//! \return True for success.
	static bool duplicate(const SdfLayerHandle& layer, const SdfPath& usdSrcPath, const SdfPath& usdDstPath);

	//! Return the USD destination path and layer.
	static void primInfo();

	// UsdUndoDuplicateCommand overrides
	void undo() override;
	void redo() override;

private:
	UsdPrim fSrcPrim;
	UsdStageWeakPtr fStage;
	SdfLayerHandle fLayer;
	Ufe::Path fUfeSrcPath;
	SdfPath fUsdDstPath;

}; // UsdUndoDuplicateCommand

} // namespace ufe
} // namespace MayaUsd
