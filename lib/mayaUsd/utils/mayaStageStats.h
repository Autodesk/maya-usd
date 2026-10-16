//
// Copyright 2026 Sony Interactive Entertainment
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
#ifndef MAYAUSD_MAYA_STAGE_STATS_H
#define MAYAUSD_MAYA_STAGE_STATS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/utils/stageStatistics.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

MAYAUSD_CORE_PUBLIC
StageStats
computeMayaStageStats(const std::vector<std::string>& objects, const StageStatsOptions& requested);

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYA_STAGE_STATS_H
