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
    const UsdTimeCode&    timeCode,
    const UsdStageRefPtr& stage)
    : _timeCode(timeCode)
    , _stage(stage)
{
}

void UsdMayaPrimUpdaterContext::Clear(const SdfPath&) { }

PXR_NAMESPACE_CLOSE_SCOPE
