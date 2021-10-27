//
// Copyright 2016 Pixar
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
#include "primUpdaterContext.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimUpdaterContext::UsdMayaPrimUpdaterContext(
    const UsdTimeCode&            timeCode,
    const UsdStageRefPtr&         stage,
    const VtDictionary&           userArgs,
    const UsdPathToDagPathMapPtr& pathMap)
    : _timeCode(timeCode)
    , _stage(stage)
    , _pathMap(pathMap)
    , _userArgs(userArgs)
    , _args(UsdMayaPrimUpdaterArgs::createFromDictionary(userArgs))
{
}

MDagPath UsdMayaPrimUpdaterContext::MapSdfPathToDagPath(const SdfPath& sdfPath) const
{
    if (!_pathMap || sdfPath.IsEmpty()) {
        return MDagPath();
    }

    auto found = _pathMap->find(sdfPath);
    return found == _pathMap->end() ? MDagPath() : found->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
