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
#ifndef MAYAUSD_LOADRULES_H
#define MAYAUSD_LOADRULES_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageLoadRules.h>

#include <maya/MObject.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

/*! \brief modify the stage load rules so that the rules governing fromPath are replicated for
 * destPath.
 */
MAYAUSD_CORE_PUBLIC
void duplicateLoadRules(
    PXR_NS::UsdStage&      stage,
    const PXR_NS::SdfPath& fromPath,
    const PXR_NS::SdfPath& destPath);

/*! \brief modify the stage load rules so that all rules governing the path are removed.
 */
MAYAUSD_CORE_PUBLIC
void removeRulesForPath(PXR_NS::UsdStage& stage, const PXR_NS::SdfPath& path);

/*! \brief convert the stage load rules to a text format.
 */
MAYAUSD_CORE_PUBLIC
MString convertLoadRulesToText(const PXR_NS::UsdStage& stage);

/*! \brief set the stage load rules from a text format.
 */
MAYAUSD_CORE_PUBLIC
void setLoadRulesFromText(PXR_NS::UsdStage& stage, const MString& text);

/*! \brief convert the load rules to a text format.
 */
MAYAUSD_CORE_PUBLIC
MString convertLoadRulesToText(const PXR_NS::UsdStageLoadRules& rules);

/*! \brief create load rules from a text format.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdStageLoadRules createLoadRulesFromText(const MString& text);

/*! \brief verify if there is a dynamic attribute on the object for load rules.
 */
MAYAUSD_CORE_PUBLIC
bool hasLoadRulesAttribute(const MObject& obj);

/*! \brief copy the stage load rules in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus copyLoadRulesToAttribute(const PXR_NS::UsdStage& stage, MObject& obj);

/*! \brief set the stage load rules from data in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus copyLoadRulesFromAttribute(const MObject& obj, PXR_NS::UsdStage& stage);

} // namespace MAYAUSD_NS_DEF

#endif
