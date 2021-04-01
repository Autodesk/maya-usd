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

#ifndef ADSK_MAYA_STAGE_LOAD_UNLOAD_COMMANDS_H
#define ADSK_MAYA_STAGE_LOAD_UNLOAD_COMMANDS_H

#include "base/api.h"

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MPxCommand.h>

namespace MAYAUSD_NS_DEF {

class ADSKMayaUsdStageLoadUnloadBase : public MPxCommand
{
public:
    static MSyntax createSyntax();

    MStatus doIt(const MArgList& args) override;

    bool isUndoable() const override { return true; }

protected:
    PXR_NS::UsdStageWeakPtr _stage;
    PXR_NS::SdfPathSet      _oldLoadSet;
};

class MAYAUSD_PLUGIN_PUBLIC ADSKMayaUsdStageLoadAllCommand : public ADSKMayaUsdStageLoadUnloadBase
{
public:
    static const MString commandName;

    static void* creator();

    MStatus redoIt() override;
    MStatus undoIt() override;
};

class MAYAUSD_PLUGIN_PUBLIC ADSKMayaUsdStageUnloadAllCommand : public ADSKMayaUsdStageLoadUnloadBase
{
public:
    static const MString commandName;

    static void* creator();

    MStatus redoIt() override;
    MStatus undoIt() override;
};

} // namespace MAYAUSD_NS_DEF

#endif
