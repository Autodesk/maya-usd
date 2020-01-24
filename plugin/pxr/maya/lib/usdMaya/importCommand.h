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

#include "pxr/pxr.h"

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>


PXR_NAMESPACE_OPEN_SCOPE


class UsdMaya_ReadJobWithSceneAssembly;

class UsdMayaImportCommand : public MPxCommand
{
  public:
    PXRUSDMAYA_API
    UsdMayaImportCommand();
    PXRUSDMAYA_API
    ~UsdMayaImportCommand() override;

    PXRUSDMAYA_API
    MStatus doIt(const MArgList& args) override;
    PXRUSDMAYA_API
    MStatus redoIt() override;
    PXRUSDMAYA_API
    MStatus undoIt() override;
    bool isUndoable() const override { return true; };

    PXRUSDMAYA_API
    static MSyntax createSyntax();
    PXRUSDMAYA_API
    static void* creator();

  private:
    UsdMaya_ReadJobWithSceneAssembly* mUsdReadJob;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
