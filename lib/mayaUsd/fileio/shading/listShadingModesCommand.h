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
#ifndef PXRUSDMAYA_LIST_SHADING_MODES_COMMAND_H
#define PXRUSDMAYA_LIST_SHADING_MODES_COMMAND_H

/// \file listShadingModesCommand.h

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaListShadingModesCommand : public MPxCommand
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaListShadingModesCommand();
    MAYAUSD_CORE_PUBLIC
    ~UsdMayaListShadingModesCommand() override;

    MAYAUSD_CORE_PUBLIC
    MStatus doIt(const MArgList& args) override;
    bool  isUndoable () const override { return false; };

    MAYAUSD_CORE_PUBLIC
    static MSyntax  createSyntax();
    MAYAUSD_CORE_PUBLIC
    static void* creator();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
