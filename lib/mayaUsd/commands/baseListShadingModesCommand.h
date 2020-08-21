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
#ifndef MAYA_LIST_SHADING_MODES_COMMAND_H
#define MAYA_LIST_SHADING_MODES_COMMAND_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

MAYAUSD_NS_DEF {

class MAYAUSD_CORE_PUBLIC MayaUSDListShadingModesCommand : public MPxCommand
{
public:
    MStatus doIt(const MArgList& args) override;
    bool  isUndoable () const override { return false; };

    static MSyntax  createSyntax();
    static void* creator();
};

}

#endif
