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

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimUpdaterContext
/// \brief This class provides an interface for updater plugins to communicate
/// state back to the core usd maya logic.
class UsdMayaPrimUpdaterContext
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimUpdaterContext(const UsdTimeCode& timeCode, const UsdStageRefPtr& stage);

    /// \brief returns the time frame where data should be edited.
    const UsdTimeCode& GetTimeCode() const { return _timeCode; }

    /// \brief returns the usd stage that is being written to.
    UsdStageRefPtr GetUsdStage() const { return _stage; }

    MAYAUSD_CORE_PUBLIC
    virtual void Clear(const SdfPath&);

private:
    const UsdTimeCode& _timeCode;
    UsdStageRefPtr     _stage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
