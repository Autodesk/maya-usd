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
#include "usdMaya/importCommand.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include "usdMaya/readJobWithSceneAssembly.h"

PXR_NAMESPACE_OPEN_SCOPE

/* static */
void*
PxrMayaUSDImportCommand::creator()
{
    return new PxrMayaUSDImportCommand();
}

/* virtual */
std::unique_ptr<UsdMaya_ReadJob> PxrMayaUSDImportCommand::initializeReadJob(const MayaUsd::ImportData & data, 
        const UsdMayaJobImportArgs & args)
{
    return std::unique_ptr<UsdMaya_ReadJob>(new UsdMaya_ReadJobWithSceneAssembly(data, args));
}

PXR_NAMESPACE_CLOSE_SCOPE
