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
#ifndef PXRUSDMAYA_MAYAPRIMREADER_H
#define PXRUSDMAYA_MAYAPRIMREADER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaPrimReader
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimReader(const UsdMayaPrimReaderArgs&);
    virtual ~UsdMayaPrimReader() {};

    /// Reads the USD prim given by the prim reader args into a Maya shape,
    /// modifying the prim reader context as a result.
    /// Returns true if successful.
    MAYAUSD_CORE_PUBLIC
    virtual bool Read(UsdMayaPrimReaderContext& context) = 0;

    /// Whether this prim reader specifies a PostReadSubtree step.
    MAYAUSD_CORE_PUBLIC
    virtual bool HasPostReadSubtree() const;

    /// An additional import step that runs after all descendants of this prim
    /// have been processed.
    /// For example, if we have prims /A, /A/B, and /C, then the import steps
    /// are run in the order:
    /// (1) Read A (2) Read B (3) PostReadSubtree B (4) PostReadSubtree A,
    /// (5) Read C (6) PostReadSubtree C
    MAYAUSD_CORE_PUBLIC
    virtual void PostReadSubtree(UsdMayaPrimReaderContext& context);

protected:
    /// Input arguments. Read data about the input USD prim from here.
    MAYAUSD_CORE_PUBLIC
    const UsdMayaPrimReaderArgs& _GetArgs();

private:
    const UsdMayaPrimReaderArgs _args;
};

typedef std::shared_ptr<UsdMayaPrimReader> UsdMayaPrimReaderSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
