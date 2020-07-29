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
#include "primWriterContext.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimWriterContext::UsdMayaPrimWriterContext(
    const UsdTimeCode&    timeCode,
    const SdfPath&        authorPath,
    const UsdStageRefPtr& stage)
    : _timeCode(timeCode)
    , _authorPath(authorPath)
    , _stage(stage)
    , _exportsGprims(false)
    , _pruneChildren(false)
{
}

const UsdTimeCode& UsdMayaPrimWriterContext::GetTimeCode() const { return _timeCode; }

const SdfPath& UsdMayaPrimWriterContext::GetAuthorPath() const { return _authorPath; }

UsdStageRefPtr UsdMayaPrimWriterContext::GetUsdStage() const { return _stage; }

bool UsdMayaPrimWriterContext::GetExportsGprims() const { return _exportsGprims; }

void UsdMayaPrimWriterContext::SetExportsGprims(bool exportsGprims)
{
    _exportsGprims = exportsGprims;
}

void UsdMayaPrimWriterContext::SetPruneChildren(bool pruneChildren)
{
    _pruneChildren = pruneChildren;
}

bool UsdMayaPrimWriterContext::GetPruneChildren() const { return _pruneChildren; }

const SdfPathVector& UsdMayaPrimWriterContext::GetModelPaths() const { return _modelPaths; }

void UsdMayaPrimWriterContext::SetModelPaths(const SdfPathVector& modelPaths)
{
    _modelPaths = modelPaths;
}

void UsdMayaPrimWriterContext::SetModelPaths(SdfPathVector&& modelPaths)
{
    _modelPaths = std::move(modelPaths);
}

PXR_NAMESPACE_CLOSE_SCOPE
