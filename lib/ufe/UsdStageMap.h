// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "ufe/path.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/hash.h"

#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD Stage Map
/*!
	Map of AL_usdmaya_ProxyShape UFE path to corresponding stage.

	Map of stage to corresponding AL_usdmaya_ProxyShape UFE path.  Ideally, we
	would support dynamically computing the path for the AL_usdmaya_ProxyShape
	node, but we assume here it will not be reparented.  We will also assume that
	a USD stage will not be instanced (even though nothing in the data model
	prevents it).
*/
class MAYAUSD_CORE_PUBLIC UsdStageMap
{
public:
	UsdStageMap() = default;
	~UsdStageMap() = default;

	// Delete the copy/move constructors assignment operators.
	UsdStageMap(const UsdStageMap&) = delete;
	UsdStageMap& operator=(const UsdStageMap&) = delete;
	UsdStageMap(UsdStageMap&&) = delete;
	UsdStageMap& operator=(UsdStageMap&&) = delete;

	//!Add the input Ufe path and USD Stage to the map.
	void addItem(const Ufe::Path& path, UsdStageWeakPtr stage);

	//! Get USD stage corresponding to argument Maya Dag path.
	UsdStageWeakPtr stage(const Ufe::Path& path) const;

	//! Return the ProxyShape node UFE path for the argument stage.
	Ufe::Path path(UsdStageWeakPtr stage) const;

	void clear();

private:
	// We keep two maps for fast lookup when there are many proxy shapes.
	std::unordered_map<Ufe::Path, UsdStageWeakPtr> fPathToStage;
	TfHashMap<UsdStageWeakPtr, Ufe::Path, TfHash> fStageToPath;

}; // UsdStageMap

} // namespace ufe
} // namespace MayaUsd
