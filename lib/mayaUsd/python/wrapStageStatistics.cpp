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
#include <mayaUsd/utils/mayaStageStats.h>
#include <mayaUsd/utils/stageStatistics.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyUtils.h>
#include <pxr/pxr.h>
#include <pxr_python.h>

#include <string>
#include <vector>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

dict _ComputeUsdDetails(
    const object& objects,
    bool          instanceProxies,
    bool          includeInactive,
    bool          includeClasses,
    bool          includeOvers,
    bool          byType)
{
    MayaUsd::StageStatsOptions options;
    options.traverseInstanceProxies = instanceProxies;
    options.includeInactive = includeInactive;
    options.includeClasses = includeClasses;
    options.includeOvers = includeOvers;
    options.countByType = byType;

    std::vector<std::string> objectNames;
    if (!TfPyIsNone(objects)) {
        objectNames = extract<std::vector<std::string>>(objects);
    }

    const MayaUsd::StageStats result = MayaUsd::ComputeMayaStageStats(objectNames, options);

    dict counts;
    for (const auto& entry : MayaUsd::StageStatsCounts(result)) {
        counts[entry.first] = entry.second;
    }

    dict types;
    for (const auto& entry : result.primsByType) {
        types[entry.first] = entry.second;
    }
    counts["types"] = types;

    return counts;
}

} // namespace

void wrapStageStatistics()
{
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::string>,
        TfPyContainerConversions::variable_capacity_policy>();

    def("ComputeUsdDetails",
        &_ComputeUsdDetails,
        (arg("objects") = object(),
         arg("instanceProxies") = true,
         arg("includeInactive") = false,
         arg("includeClasses") = false,
         arg("includeOvers") = false,
         arg("byType") = true));
}
