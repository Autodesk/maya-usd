//
// Copyright 2017 Animal Logic
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
#pragma once
#include "AL/maya/utils/ForwardDeclares.h"

#include <maya/MPxCommand.h>

namespace AL {
namespace maya {
namespace tests {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
// A very simple test command that adds in all combinations of command argument, used to test the
// CommandGuiHelper class.
//----------------------------------------------------------------------------------------------------------------------
#ifndef AL_GENERATING_DOCS
class CommandGuiHelperTestCMD : public MPxCommand
{
public:
    static const MString kName;
    static MSyntax       createSyntax();
    static void          makeGUI();
    static void*         creator();
    MStatus              doIt(const MArgList& args) override;
};
#endif

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace tests
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
