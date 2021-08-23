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
#include "functorPrimReader.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_FunctorPrimReader::UsdMaya_FunctorPrimReader(
    const UsdMayaPrimReaderArgs&        args,
    UsdMayaPrimReaderRegistry::ReaderFn readerFn)
    : UsdMayaPrimReader(args)
    , _readerFn(readerFn)
{
}

bool UsdMaya_FunctorPrimReader::Read(UsdMayaPrimReaderContext& context)
{
    return _readerFn(_GetArgs(), context);
}

/* static */
UsdMayaPrimReaderSharedPtr UsdMaya_FunctorPrimReader::Create(
    const UsdMayaPrimReaderArgs&        args,
    UsdMayaPrimReaderRegistry::ReaderFn readerFn)
{
    return std::make_shared<UsdMaya_FunctorPrimReader>(args, readerFn);
}

/* static */
UsdMayaPrimReaderRegistry::ReaderFactoryFn
UsdMaya_FunctorPrimReader::CreateFactory(UsdMayaPrimReaderRegistry::ReaderFn readerFn)
{
    return [=](const UsdMayaPrimReaderArgs& args) { return Create(args, readerFn); };
}

PXR_NAMESPACE_CLOSE_SCOPE
