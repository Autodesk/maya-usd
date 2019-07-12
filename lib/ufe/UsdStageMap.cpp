// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdStageMap.h"

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

UsdStageMap g_StageMap;

//------------------------------------------------------------------------------
// UsdStageMap
//------------------------------------------------------------------------------

void UsdStageMap::addItem(const Ufe::Path& path, UsdStageWeakPtr stage)
{
	fPathToStage[path] = stage;
	fStageToPath[stage] = path;
}

UsdStageWeakPtr UsdStageMap::stage(const Ufe::Path& path) const
{
	// A stage is bound to a single Dag proxy shape.
	auto iter = fPathToStage.find(path);
	if (iter != std::end(fPathToStage))
		return iter->second;
	return nullptr;
}

Ufe::Path UsdStageMap::path(UsdStageWeakPtr stage) const
{
	// A stage is bound to a single Dag proxy shape.
	auto iter = fStageToPath.find(stage);
	if (iter != std::end(fStageToPath))
		return iter->second;
	return Ufe::Path();
}

void UsdStageMap::clear()
{
	fPathToStage.clear();
	fStageToPath.clear();
}

} // namespace ufe
} // namespace MayaUsd
