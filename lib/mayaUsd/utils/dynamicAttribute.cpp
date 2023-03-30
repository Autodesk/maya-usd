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

#include "dynamicAttribute.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

bool hasDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName)
{
    return depNode.hasAttribute(attrName);
}

MStatus createDynamicAttribute(MFnDependencyNode& depNode, const MString& attrName)
{
    MStatus status;

    MFnTypedAttribute typedAttrFn;
    MObject           attr
        = typedAttrFn.create(attrName, attrName, MFnData::kString, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    typedAttrFn.setReadable(true);
    typedAttrFn.setWritable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setKeyable(false);
    typedAttrFn.setStorable(true);

    status = depNode.addAttribute(attr);
    return status;
}

MStatus
getDynamicAttribute(const MFnDependencyNode& depNode, const MString& attrName, MString& value)
{
    if (!depNode.hasAttribute(attrName))
        return MS::kNotFound;

    MStatus status = MS::kSuccess;
    MPlug   attr = depNode.findPlug(attrName);
    value = attr.asString(&status);
    return status;
}

MStatus
setDynamicAttribute(MFnDependencyNode& depNode, const MString& attrName, const MString& value)
{
    MStatus status = MS::kSuccess;

    if (!depNode.hasAttribute(attrName)) {
        status = createDynamicAttribute(depNode, attrName);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MPlug attr = depNode.findPlug(attrName);
    status = attr.setString(value);
    return status;
}

} // namespace MAYAUSD_NS_DEF
