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
#ifndef MAYAUSD_DYNAMICATTRIBUTE_H
#define MAYAUSD_DYNAMICATTRIBUTE_H

#include <mayaUsd/base/api.h>

#include <maya/MApiNamespace.h>

namespace MAYAUSD_NS_DEF {

/*! \brief verify if the named dynamic attribute is present on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
bool hasDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName);

/*! \brief create the named dynamic attribute on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus createDynamicAttribute(MFnDependencyNode& depNode, const MString& attrName);

/*! \brief get the string value of the named dynamic attribute from the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus
getDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName, MString& value);

/*! \brief set the named dynamic attribute to the given string value on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus setDynamicAttribute(
    MFnDependencyNode& depNode,
    const MString&           attrName,
    const MString&           value);

} // namespace MAYAUSD_NS_DEF

#endif
