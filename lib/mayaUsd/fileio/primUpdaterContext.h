//
// Copyright 2016 Pixar
// Copyright 2019 Autodesk
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
#ifndef PXRUSDMAYA_PRIMUPDATERCONTEXT_H
#define PXRUSDMAYA_PRIMUPDATERCONTEXT_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primUpdaterArgs.h>

#include <pxr/base/tf/hashmap.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MDagPath.h>

#include <memory> // shared_ptr

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimUpdaterContext
/// \brief This class provides an interface for updater plugins to communicate
/// state back to the core usd maya logic.
//
// Gives access to shared state across prim updaters.
class UsdMayaPrimUpdaterContext
{
public:
    using UsdPathToDagPathMap = TfHashMap<SdfPath, MDagPath, SdfPath::Hash>;
    using UsdPathToDagPathMapPtr = std::shared_ptr<UsdPathToDagPathMap>;

    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimUpdaterContext(
        const UsdTimeCode&            timeCode,
        const UsdStageRefPtr&         stage,
        const VtDictionary&           userArgs,
        const UsdPathToDagPathMapPtr& pathMap = nullptr);

    /// \brief returns the time frame where data should be edited.
    const UsdTimeCode& GetTimeCode() const { return _timeCode; }

    /// \brief returns the usd stage that is being written to.
    UsdStageRefPtr GetUsdStage() const { return _stage; }

    /// \brief Return dictionary with user defined arguments. Can contain a mix of reader/writer and
    /// updater args
    const VtDictionary& GetUserArgs() const { return _userArgs; }

    /// \brief Return updater arguments
    const UsdMayaPrimUpdaterArgs& GetArgs() const { return _args; }

    /// \brief Returns the Maya Dag path corresponding to a pulled USD path.  The Dag path will be
    /// empty if no correspondence exists.
    MAYAUSD_CORE_PUBLIC
    MDagPath MapSdfPathToDagPath(const SdfPath& sdfPath) const;

private:
    const UsdTimeCode&           _timeCode;
    const UsdStageRefPtr         _stage;
    const UsdPathToDagPathMapPtr _pathMap;

    const VtDictionary&          _userArgs;
    const UsdMayaPrimUpdaterArgs _args;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
