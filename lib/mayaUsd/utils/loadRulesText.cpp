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

#include <maya/MStringArray.h>

namespace MAYAUSD_NS_DEF {

MString convertLoadRulesToText(const PXR_NS::UsdStage& stage)
{
    return convertLoadRulesToText(stage.GetLoadRules());
}

void setLoadRulesFromText(PXR_NS::UsdStage& stage, const MString& text)
{
    const auto newLoadRules = createLoadRulesFromText(text);
    if (stage.GetLoadRules() != newLoadRules)
        stage.SetLoadRules(newLoadRules);
}

static MString convertRuleToText(const PXR_NS::UsdStageLoadRules::Rule rule)
{
    // Note: using namespace required for the TF_WARN macro.
    using namespace PXR_NS;

    switch (rule) {
    case UsdStageLoadRules::AllRule: return "all";
    case UsdStageLoadRules::OnlyRule: return "only";
    case UsdStageLoadRules::NoneRule: return "none";
    default: TF_WARN("convert rule to text: invalid rule: %d", (int)rule); return "all";
    }
}

static MString
convertPerPathRuleToText(const PXR_NS::SdfPath& path, const PXR_NS::UsdStageLoadRules::Rule rule)
{
    MString text;
    text.format("^1s=^2s", path.GetAsString().c_str(), convertRuleToText(rule));
    return text;
}

MString convertLoadRulesToText(const PXR_NS::UsdStageLoadRules& rules)
{
    MString text;

    const auto& perPathRules = rules.GetRules();
    for (const auto& pathAndRule : perPathRules) {
        if (text.length() > 0)
            text += ";";

        text += convertPerPathRuleToText(pathAndRule.first, pathAndRule.second);
    }

    return text;
}

static PXR_NS::UsdStageLoadRules::Rule createRuleFromText(const MString& text)
{
    // Note: using namespace required for the TF_WARN macro.
    using namespace PXR_NS;

    if (text == "all")
        return UsdStageLoadRules::AllRule;
    if (text == "only")
        return UsdStageLoadRules::OnlyRule;
    if (text == "none")
        return UsdStageLoadRules::NoneRule;

    TF_WARN("Convert text to rule: invalid rule: %s", text.asChar());
    return PXR_NS::UsdStageLoadRules::AllRule;
}

PXR_NS::UsdStageLoadRules createLoadRulesFromText(const MString& text)
{
    PXR_NS::UsdStageLoadRules rules;

    MStringArray perPathRules;
    text.split(';', perPathRules);
    for (const auto& item : perPathRules) {
        MStringArray pathAndRule;
        item.split('=', pathAndRule);
        if (pathAndRule.length() != 2)
            continue;

        auto path = PXR_NS::SdfPath(pathAndRule[0].asChar());
        auto rule = createRuleFromText(pathAndRule[1]);

        rules.AddRule(path, rule);
    }

    return rules;
}

} // namespace MAYAUSD_NS_DEF
