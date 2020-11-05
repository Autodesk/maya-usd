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
#ifndef PXRUSDMAYA_FUNCTORPRIMREADER_H
#define PXRUSDMAYA_FUNCTORPRIMREADER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/primReaderRegistry.h>

#include <pxr/pxr.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMaya_FunctorPrimReader
/// \brief This class is scaffolding to hold bare prim reader functions.
///
/// It is used by the PXRUSDMAYA_DEFINE_READER macro.
class UsdMaya_FunctorPrimReader final : public UsdMayaPrimReader
{
public:
    UsdMaya_FunctorPrimReader(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderRegistry::ReaderFn);

    bool Read(UsdMayaPrimReaderContext* context) override;

    static UsdMayaPrimReaderSharedPtr
    Create(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderRegistry::ReaderFn readerFn);

    static UsdMayaPrimReaderRegistry::ReaderFactoryFn
    CreateFactory(UsdMayaPrimReaderRegistry::ReaderFn readerFn);

private:
    UsdMayaPrimReaderRegistry::ReaderFn _readerFn;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
