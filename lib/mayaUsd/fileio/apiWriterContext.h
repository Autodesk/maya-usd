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
#ifndef PXRUSDMAYA_APIWRITERCONTEXT_H
#define PXRUSDMAYA_APIWRITERCONTEXT_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <maya/MDagPath.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;
class UsdTimeCode;

/// Class that provides an interface for API writer plugins.
/// \sa UsdMayaApiWriterRegistry
class UsdMayaApiWriterContext
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaApiWriterContext(
        const MDagPath&    dagPath,
        const UsdPrim&     usdPrim,
        const UsdTimeCode& timeCode);

    /// Returns the MDagPath that corresponds to the \p usdPrim that was
    /// exported that the API is being applied to.
    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetMDagPath() const;

    /// Returns the UsdPrim that was just exported.
    MAYAUSD_CORE_PUBLIC
    const UsdPrim& GetUsdPrim() const;

    /// Returns the time code.
    MAYAUSD_CORE_PUBLIC
    const UsdTimeCode& GetTimeCode() const;

private:
    const MDagPath&    _dagPath;
    const UsdPrim&     _usdPrim;
    const UsdTimeCode& _timeCode;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
