//
// Copyright 2019 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

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
