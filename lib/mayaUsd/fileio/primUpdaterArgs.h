//
// Copyright 2021 Autodesk
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
#ifndef PXRUSDMAYA_PRIMUPDATERARGS_H
#define PXRUSDMAYA_PRIMUPDATERARGS_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define PXRUSDMAYA_UPDATER_ARGS_TOKENS \
    /* Dictionary keys */ \
    (copyOperation)
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaPrimUpdaterArgsTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_UPDATER_ARGS_TOKENS);

/// \class UsdMayaPrimUpdaterArgs
/// \brief Arguments that configure the updater.
struct UsdMayaPrimUpdaterArgs
{
    const bool _copyOperation { false };

    MAYAUSD_CORE_PUBLIC
    static UsdMayaPrimUpdaterArgs createFromDictionary(const VtDictionary& userArgs);

    MAYAUSD_CORE_PUBLIC
    static const VtDictionary& getDefaultDictionary();

private:
    UsdMayaPrimUpdaterArgs(const VtDictionary& userArgs);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
