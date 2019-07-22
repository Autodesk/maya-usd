//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "primUpdater.h"

#include <maya/MFnDagNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

static
MDagPath
_GetDagPath(const MFnDependencyNode& depNodeFn, const bool reportError = true)
{
    try {
        const MFnDagNode& dagNodeFn =
            dynamic_cast<const MFnDagNode&>(depNodeFn);

        MStatus status;
        const MDagPath dagPath = dagNodeFn.dagPath(&status);
        if (status == MS::kSuccess) {
            const bool dagPathIsValid = dagPath.isValid(&status);
            if (status == MS::kSuccess && dagPathIsValid) {
                return dagPath;
            }
        }

        if (reportError) {
            TF_CODING_ERROR(
                "Invalid MDagPath for MFnDagNode '%s'. Verify that it was "
                "constructed using an MDagPath.",
                dagNodeFn.fullPathName().asChar());
        }
    }
    catch (const std::bad_cast& /* e */) {
        // This is not a DAG node, so it can't have a DAG path.
    }

    return MDagPath();
}

static
UsdMayaUtil::MDagPathMap<SdfPath>
_GetDagPathMap(const MFnDependencyNode& depNodeFn, const SdfPath& usdPath)
{
    const MDagPath dagPath = _GetDagPath(depNodeFn, /* reportError = */ false);
    if (dagPath.isValid()) {
        return UsdMayaUtil::MDagPathMap<SdfPath>({ {dagPath, usdPath} });
    }

    return UsdMayaUtil::MDagPathMap<SdfPath>({});
}

UsdMayaPrimUpdater::UsdMayaPrimUpdater(
    const MFnDependencyNode& depNodeFn,
    const SdfPath& usdPath) :
    _dagPath(_GetDagPath(depNodeFn)),
    _mayaObject(depNodeFn.object()),
    _usdPath(usdPath),
    _baseDagToUsdPaths(_GetDagPathMap(depNodeFn, usdPath))
{
}

bool UsdMayaPrimUpdater::Push(UsdMayaPrimUpdaterContext* context)
{
    return false;
}

bool UsdMayaPrimUpdater::Pull(UsdMayaPrimUpdaterContext* context)
{
    return false;
}

void UsdMayaPrimUpdater::Clear(UsdMayaPrimUpdaterContext* context)
{
}

const MDagPath&
UsdMayaPrimUpdater::GetDagPath() const
{
    return _dagPath;
}

const MObject&
UsdMayaPrimUpdater::GetMayaObject() const
{
    return _mayaObject;
}

const SdfPath&
UsdMayaPrimUpdater::GetUsdPath() const
{
    return _usdPath;
}

PXR_NAMESPACE_CLOSE_SCOPE
