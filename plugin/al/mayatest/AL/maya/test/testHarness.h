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
#include <maya/MPxCommand.h>
#include "AL/maya/test/Api.h"


namespace AL {
namespace maya {
namespace test {

class UnitTestHarness : public MPxCommand
{
public:

  AL_MAYA_TEST_PUBLIC static void* creator();
  AL_MAYA_TEST_PUBLIC static MSyntax createSyntax();
  AL_MAYA_TEST_PUBLIC static const MString kName;
  AL_MAYA_TEST_PUBLIC MStatus doIt(const MArgList& args) override;

private:
  void cleanTemporaryFiles() const;

};

//----------------------------------------------------------------------------------------------------------------------
} // test
} // maya
} // AL
//----------------------------------------------------------------------------------------------------------------------
