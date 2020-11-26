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
#ifndef PXRUSDMAYA_PRIMREADERARGS_H
#define PXRUSDMAYA_PRIMREADERARGS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/base/gf/interval.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimReaderArgs
/// \brief This class holds read-only arguments that are passed into reader plugins for
/// the usdMaya library.
///
/// \sa UsdMayaPrimReaderContext
class UsdMayaPrimReaderArgs
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimReaderArgs(const UsdPrim& prim, const UsdMayaJobImportArgs& jobArgs);

    /// \brief return the usd prim that should be read.
    MAYAUSD_CORE_PUBLIC
    const UsdPrim& GetUsdPrim() const;

    /// \brief return the initial job arguments, allowing a prim reader to
    /// execute a secondary prim reader.
    MAYAUSD_CORE_PUBLIC
    const UsdMayaJobImportArgs& GetJobArguments() const { return _jobArgs; }

    /// Returns the time interval over which to import animated data.
    /// An empty interval (<tt>GfInterval::IsEmpty()</tt>) means that no
    /// animated (time-sampled) data should be imported.
    MAYAUSD_CORE_PUBLIC
    GfInterval GetTimeInterval() const;

    MAYAUSD_CORE_PUBLIC
    const TfToken::Set& GetIncludeMetadataKeys() const;
    MAYAUSD_CORE_PUBLIC
    const TfToken::Set& GetIncludeAPINames() const;

    MAYAUSD_CORE_PUBLIC
    const TfToken::Set& GetExcludePrimvarNames() const;

    MAYAUSD_CORE_PUBLIC
    bool GetUseAsAnimationCache() const;

    bool ShouldImportUnboundShaders() const
    {
        // currently this is disabled.
        return false;
    }

private:
    const UsdPrim&              _prim;
    const UsdMayaJobImportArgs& _jobArgs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
