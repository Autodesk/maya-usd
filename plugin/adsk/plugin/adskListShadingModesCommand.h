//
// Copyright 2020 Autodesk
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
#ifndef ADSK_MAYA_LIST_SHADING_MODES_COMMAND_H
#define ADSK_MAYA_LIST_SHADING_MODES_COMMAND_H

#include "base/api.h"

#include <mayaUsd/commands/baseListShadingModesCommand.h>

MAYAUSD_NS_DEF {

class MAYAUSD_PLUGIN_PUBLIC ADSKMayaUSDListShadingModesCommand : public MayaUsd::MayaUSDListShadingModesCommand
{
  public:

    static const MString commandName;

    static void* creator();
};

}

#endif
