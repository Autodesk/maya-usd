//
// Copyright 2018 Pixar
// Copyright 2019 Autodesk
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
#include "primUpdater.h"

#include <maya/MFnDagNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimUpdater::UsdMayaPrimUpdater(const MFnDependencyNode& depNodeFn, const SdfPath& usdPath)
    : _dagPath(UsdMayaUtil::getDagPath(depNodeFn))
    , _mayaObject(depNodeFn.object())
    , _usdPath(usdPath)
    , _baseDagToUsdPaths(UsdMayaUtil::getDagPathMap(depNodeFn, usdPath))
{
}

bool UsdMayaPrimUpdater::Push(UsdMayaPrimUpdaterContext* context) { return false; }

bool UsdMayaPrimUpdater::Pull(UsdMayaPrimUpdaterContext* context) { return false; }

void UsdMayaPrimUpdater::Clear(UsdMayaPrimUpdaterContext* context) { }

const MDagPath& UsdMayaPrimUpdater::GetDagPath() const { return _dagPath; }

const MObject& UsdMayaPrimUpdater::GetMayaObject() const { return _mayaObject; }

const SdfPath& UsdMayaPrimUpdater::GetUsdPath() const { return _usdPath; }

PXR_NAMESPACE_CLOSE_SCOPE
