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
#ifndef PXRUSDMAYA_FALLBACK_PRIM_READER_H
#define PXRUSDMAYA_FALLBACK_PRIM_READER_H

#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

/// This is a special prim reader that is used whenever a typeless prim or prim
/// with unknown types is encountered when traversing USD.
class UsdMaya_FallbackPrimReader : public UsdMayaPrimReader
{
public:
    UsdMaya_FallbackPrimReader(const UsdMayaPrimReaderArgs& args);

    bool Read(UsdMayaPrimReaderContext& context) override;

    static UsdMayaPrimReaderRegistry::ReaderFactoryFn CreateFactory();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
