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

/*! \brief flags used to create the dynamic attribute.
 **        By default it is readable, writable, storable and hidden.
 */
enum class DynamicAttrFlags : unsigned
{
    kNone = 0,

    kAppearance = 1 << 0,
    kCached = 1 << 1,
    kConnectable = 1 << 2,
    kFilename = 1 << 3,
    kHidden = 1 << 4,
    kKeyable = 1 << 5,
    kReadable = 1 << 6,
    kStorable = 1 << 7,
    kWorldspace = 1 << 8,
    kWritable = 1 << 9,

    kDefaults = kReadable | kWritable | kHidden | kStorable,

    kAll = unsigned(-1)
};

MAYAUSD_CORE_PUBLIC
inline DynamicAttrFlags operator|(const DynamicAttrFlags lhs, const DynamicAttrFlags rhs)
{
    return DynamicAttrFlags(unsigned(lhs) | unsigned(rhs));
}

MAYAUSD_CORE_PUBLIC
inline DynamicAttrFlags operator&(const DynamicAttrFlags lhs, const DynamicAttrFlags rhs)
{
    return DynamicAttrFlags(unsigned(lhs) & unsigned(rhs));
}

MAYAUSD_CORE_PUBLIC
inline DynamicAttrFlags operator^(const DynamicAttrFlags lhs, const DynamicAttrFlags rhs)
{
    return DynamicAttrFlags(unsigned(lhs) ^ unsigned(rhs));
}

MAYAUSD_CORE_PUBLIC
inline DynamicAttrFlags operator~(const DynamicAttrFlags flags)
{
    return DynamicAttrFlags(unsigned(DynamicAttrFlags::kAll) & ~unsigned(flags));
}

MAYAUSD_CORE_PUBLIC
inline bool isFlagSet(const DynamicAttrFlags lhs, const DynamicAttrFlags rhs)
{
    return unsigned(lhs & rhs) != 0;
}

/*! \brief verify if the named dynamic attribute is present on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
bool hasDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName);

/*! \brief create the named dynamic attribute on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus createDynamicAttribute(
    MFnDependencyNode&     depNode,
    const MString&         attrName,
    const DynamicAttrFlags flags = DynamicAttrFlags::kDefaults);

/*! \brief get the string value of the named dynamic attribute from the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus
getDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName, MString& value);

/*! \brief set the named dynamic attribute to the given string value on the Maya node.
 */
MAYAUSD_CORE_PUBLIC
MStatus setDynamicAttribute(
    MFnDependencyNode&     depNode,
    const MString&         attrName,
    const MString&         value,
    const DynamicAttrFlags flags = DynamicAttrFlags::kDefaults);

} // namespace MAYAUSD_NS_DEF

#endif
