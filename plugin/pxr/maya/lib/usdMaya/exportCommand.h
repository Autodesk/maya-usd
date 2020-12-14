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
#ifndef PXRUSDMAYA_EXPORT_COMMAND_H
#define PXRUSDMAYA_EXPORT_COMMAND_H

/// \file usdMaya/exportCommand.h

#include "usdMaya/api.h"

#include <mayaUsd/commands/baseExportCommand.h>

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUSDExportCommand : public MayaUsd::MayaUSDExportCommand
{
public:
    PXRUSDMAYA_API
    static void* creator();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
