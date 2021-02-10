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
#ifndef PXRUSDMAYA_IMPORT_COMMAND_H
#define PXRUSDMAYA_IMPORT_COMMAND_H

/// \file usdMaya/importCommand.h

#include "usdMaya/api.h"

#include <mayaUsd/commands/baseImportCommand.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMaya_ReadJobWithSceneAssembly;

class PxrMayaUSDImportCommand : public MayaUsd::MayaUSDImportCommand
{
public:
    //
    // Command flags are a mix of Arg Tokens defined in readJob.h
    // and some that are defined by this command itself.
    // All short forms of the Maya flag names are defined here.
    // All long forms of flags defined by the command are also here.
    // All long forms of flags defined by the Arg Tokens are queried
    // for and set when creating the MSyntax object.
    // Derived classes can use the short forms of the flags when
    // calling Maya functions like argData.isFlagSet()
    //
    // The list of short forms of flags defined as Arg Tokens:
    static constexpr auto kAssemblyRepFlag = "ar";

    PXRUSDMAYA_API
    static MSyntax createSyntax();

    PXRUSDMAYA_API
    static void* creator();

protected:
    std::unique_ptr<UsdMaya_ReadJob>
    initializeReadJob(const MayaUsd::ImportData&, const UsdMayaJobImportArgs&) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
