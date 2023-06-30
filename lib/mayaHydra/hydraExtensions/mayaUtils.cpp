//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mayaUtils.h"

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>

namespace MAYAHYDRA_NS_DEF {

MStatus GetDagPathFromNodeName(const MString& nodeName, MDagPath& outDagPath)
{
    MSelectionList selectionList;
    MStatus        status = selectionList.add(nodeName);
    if (status) {
        status = selectionList.getDagPath(0, outDagPath);
    }
    return status;
}

MStatus GetMayaMatrixFromDagPath(const MDagPath& dagPath, MMatrix& outMatrix)
{
    MStatus status;
    outMatrix = dagPath.inclusiveMatrix(&status);
    return status;
}

bool IsUfeItemFromMayaUsd(const MDagPath& dagPath, MStatus* returnStatus)
{
    static const MString ufeRuntimeAttributeName = "ufeRuntime";
    static const MString mayaUsdUfeRuntimeName = "USD";

    MFnDagNode dagNode(dagPath);
    MStatus    ufePlugSearchStatus;
    MPlug ufeRuntimePlug = dagNode.findPlug(ufeRuntimeAttributeName, false, &ufePlugSearchStatus);
    if (returnStatus) {
        *returnStatus = ufePlugSearchStatus;
    }
    return ufePlugSearchStatus && ufeRuntimePlug.asString() == mayaUsdUfeRuntimeName;
}

bool IsUfeItemFromMayaUsd(const MObject& obj, MStatus* returnStatus)
{
    MDagPath dagPath;
    MStatus  dagPathSearchStatus = MDagPath::getAPathTo(obj, dagPath);
    if (!dagPathSearchStatus) {
        if (returnStatus) {
            *returnStatus = dagPathSearchStatus;
        }
        return false;
    }

    return IsUfeItemFromMayaUsd(dagPath, returnStatus);
}

} // namespace MAYAHYDRA_NS_DEF
