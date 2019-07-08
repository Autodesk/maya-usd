//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXRUSDMAYA_PRIMUPDATERCONTEXT_H
#define PXRUSDMAYA_PRIMUPDATERCONTEXT_H

/// \file usdMaya/primUpdaterContext.h

#include "../base/api.h"

#include "pxr/pxr.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMayaPrimUpdaterContext
/// \brief This class provides an interface for updater plugins to communicate
/// state back to the core usd maya logic.
class UsdMayaPrimUpdaterContext
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimUpdaterContext(
            const UsdTimeCode& timeCode,
            const UsdStageRefPtr& stage);

    /// \brief returns the time frame where data should be edited.
    MAYAUSD_CORE_PUBLIC
    const UsdTimeCode& GetTimeCode() const;

    /// \brief returns the usd stage that is being written to.
    MAYAUSD_CORE_PUBLIC
    UsdStageRefPtr GetUsdStage() const; 

    MAYAUSD_CORE_PUBLIC
    virtual void Clear(const SdfPath&);
  
private:
    const UsdTimeCode&  _timeCode;
    UsdStageRefPtr      _stage;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

