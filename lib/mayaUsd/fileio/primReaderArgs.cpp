//
// Copyright 2016 Pixar
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
#include "primReaderArgs.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimReaderArgs::UsdMayaPrimReaderArgs(
    const UsdPrim&              prim,
    const UsdMayaJobImportArgs& jobArgs)
    : _prim(prim)
    , _jobArgs(jobArgs)
{
}
const UsdPrim& UsdMayaPrimReaderArgs::GetUsdPrim() const { return _prim; }

GfInterval UsdMayaPrimReaderArgs::GetTimeInterval() const { return _jobArgs.timeInterval; }

const TfToken::Set& UsdMayaPrimReaderArgs::GetIncludeMetadataKeys() const
{
    return _jobArgs.includeMetadataKeys;
}

const TfToken::Set& UsdMayaPrimReaderArgs::GetIncludeAPINames() const
{
    return _jobArgs.includeAPINames;
}

const TfToken::Set& UsdMayaPrimReaderArgs::GetExcludePrimvarNames() const
{
    return _jobArgs.excludePrimvarNames;
}

bool UsdMayaPrimReaderArgs::GetUseAsAnimationCache() const { return _jobArgs.useAsAnimationCache; }

PXR_NAMESPACE_CLOSE_SCOPE
