//
// Copyright 2018 Pixar
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
#include "primReader.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimReader::UsdMayaPrimReader(const UsdMayaPrimReaderArgs& args)
    : _args(args)
{
}

bool UsdMayaPrimReader::HasPostReadSubtree() const { return false; }

void UsdMayaPrimReader::PostReadSubtree(UsdMayaPrimReaderContext*) { }

const UsdMayaPrimReaderArgs& UsdMayaPrimReader::_GetArgs() { return _args; }

PXR_NAMESPACE_CLOSE_SCOPE
