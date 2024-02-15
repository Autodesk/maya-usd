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
#ifndef USDUFE_LOADRULES_H
#define USDUFE_LOADRULES_H

#include <usdUfe/base/api.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageLoadRules.h>

#include <string.h>

namespace USDUFE_NS_DEF {

/*! \brief modify the stage load rules so that the rules governing fromPath are replicated for
 * destPath.
 */
USDUFE_PUBLIC
void duplicateLoadRules(
    PXR_NS::UsdStage&      stage,
    const PXR_NS::SdfPath& fromPath,
    const PXR_NS::SdfPath& destPath);

/*! \brief modify the stage load rules so that all rules governing the path are removed.
 */
USDUFE_PUBLIC
void removeRulesForPath(PXR_NS::UsdStage& stage, const PXR_NS::SdfPath& path);

/*! \brief convert the stage load rules to a text format.
 */
USDUFE_PUBLIC
std::string convertLoadRulesToText(const PXR_NS::UsdStage& stage);

/*! \brief set the stage load rules from a text format.
 */
USDUFE_PUBLIC
void setLoadRulesFromText(PXR_NS::UsdStage& stage, const std::string& text);

/*! \brief set the stage load rules if they are different from the current ones.
 */
USDUFE_PUBLIC
void setLoadRules(PXR_NS::UsdStage& stage, const PXR_NS::UsdStageLoadRules& newLoadRules);

/*! \brief convert the load rules to a text format.
 */
USDUFE_PUBLIC
std::string convertLoadRulesToText(const PXR_NS::UsdStageLoadRules& rules);

/*! \brief create load rules from a text format.
 */
USDUFE_PUBLIC
PXR_NS::UsdStageLoadRules createLoadRulesFromText(const std::string& text);

} // namespace USDUFE_NS_DEF

#endif
