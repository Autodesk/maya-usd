//
// Copyright 2022 Autodesk
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

#include "loadRules.h"

namespace MAYAUSD_NS_DEF {

void duplicateLoadRules(
    PXR_NS::UsdStage&      stage,
    const PXR_NS::SdfPath& fromPath,
    const PXR_NS::SdfPath& destPath)
{
    // Note: get a *copy* of the rules since we are going to insert new rules as we iterate.
    auto loadRules = stage.GetLoadRules();

    // Retrieve the effective rule for the source path.
    //
    // The reason we retrieve the effetive rule is that even
    // though we will modify all rules specific to that path,
    // its actual effective rule might be dictated by an ancestor.
    //
    // For example, there might be *no* rules at all for the path
    // while its parent has a rule to unload it. So its effective
    // rule would be "unloaded".
    //
    // In that case, we will have to reproduce that rule at the
    // destination as a path-specific rule.
    const auto desiredRule = loadRules.GetEffectiveRuleForPath(fromPath);

    // Analyze the reasons the source path was loaded or unloaded and
    // replicate them to the destination.
    //
    // The case we need to explicitly handle is when the path is controlled
    // by a rule on itself or a descendent and not from an ancestor. Then we
    // need to duplicate the load or unload rule.
    //
    // We do this by iterating over all rules and duplicating all rules that
    // contain the source path to create rules with the destination path.

    auto oldRules = loadRules.GetRules();
    for (const auto& rule : oldRules) {
        const PXR_NS::SdfPath& rulePath = rule.first;
        if (rulePath.HasPrefix(fromPath)) {
            const auto newPath = rulePath.ReplacePrefix(fromPath, destPath);
            loadRules.AddRule(newPath, rule.second);
        }
    }

    // Verify if the effective rule at the destination was covered by the
    // modified rules above. If not, add a specific rule that will give us
    // the desired behaviour.
    //
    // The reason we don't simply add this specific rule for all cases and
    // avoid the above work is that sub-paths might have rules and we need
    // to preserve those rules. So we would need to do the above work anyway.
    //
    // Given that, a common case is that we don't need to add an additional
    // rule. Always adding it would add unnecessary rules.
    //
    // Note: the UsdStageLoadRules has a Minimize function that simplifies
    // rules, but we don't want to change rules the user might have set.
    // The user may expect those rules to exists for some future purpose
    // even though they are not currently used. As a general principle
    // we try to not change user data unless necessary.
    if (desiredRule != loadRules.GetEffectiveRuleForPath(destPath)) {
        loadRules.AddRule(destPath, desiredRule);
    }

    // Update the rules in the load rules object and then in the stage
    // since we were operating on a copy.
    stage.SetLoadRules(loadRules);
}

void removeRulesForPath(PXR_NS::UsdStage& stage, const PXR_NS::SdfPath& path)
{
    // Note: get a *copy* of the rules since we are going to remove rules.
    auto loadRules = stage.GetLoadRules();
    auto rules = loadRules.GetRules();

    // Remove all rules that match the given path.
    auto newEnd = std::remove_if(rules.begin(), rules.end(), [&path](const auto& rule) {
        const PXR_NS::SdfPath& rulePath = rule.first;
        return rulePath.HasPrefix(path);
    });
    rules.erase(newEnd, rules.end());

    // Update the rules in the load rules object and then in the stage
    // since we were operating on a copy.
    loadRules.SetRules(rules);
    stage.SetLoadRules(loadRules);
}

} // namespace MAYAUSD_NS_DEF
