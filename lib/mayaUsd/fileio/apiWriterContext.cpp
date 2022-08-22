//
// Copyright 2022 Pixar
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

#include "apiWriterContext.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaApiWriterContext::UsdMayaApiWriterContext(
    const MDagPath&    dagPath,
    const UsdPrim&     usdPrim,
    const UsdTimeCode& timeCode)
    : _dagPath(dagPath)
    , _usdPrim(usdPrim)
    , _timeCode(timeCode)
{
}

const MDagPath& UsdMayaApiWriterContext::GetMDagPath() const { return _dagPath; }

const UsdPrim& UsdMayaApiWriterContext::GetUsdPrim() const { return _usdPrim; }

const UsdTimeCode& UsdMayaApiWriterContext::GetTimeCode() const { return _timeCode; }

PXR_NAMESPACE_CLOSE_SCOPE
