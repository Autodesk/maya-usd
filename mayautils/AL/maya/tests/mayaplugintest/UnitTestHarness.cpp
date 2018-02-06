//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/maya/tests/mayaplugintest/UnitTestHarness.h"

#include <gtest/gtest.h>
#include "maya/MSyntax.h"
#include "maya/MArgDatabase.h"
#include "maya/MGlobal.h"

const char* happy_cat =
"\n"
"    \\    /\\ \n"
"     )  ( ^)\n"
"    (  /  )\n"
"     \\(__)|\n"
"\e[39m";

const char* angry_cat =
"\n"
"         // \n"
"        ( >)\n"
"   /\\  /  )\n"
"  /  \\(__)|\n"
"\e[39m";

//----------------------------------------------------------------------------------------------------------------------
const MString UnitTestHarness::kName = "MayaUtils_UnitTestHarness";

//----------------------------------------------------------------------------------------------------------------------
MSyntax UnitTestHarness::createSyntax()
{
  MSyntax syn;
  syn.addFlag("-f", "-filter", MSyntax::kString);
  syn.addFlag("-o", "-output", MSyntax::kString);
  syn.addFlag("-ff", "-flag_file", MSyntax::kString);
  syn.addFlag("-l", "-list");
  syn.addFlag("-bof", "-break_on_failure");
  syn.addFlag("-ne", "-no_catch_exceptions");
  syn.addFlag("-nc", "-no_colour");
  syn.addFlag("-nt", "-no_time");
  syn.addFlag("-rs", "-random_seed", MSyntax::kLong);
  syn.addFlag("-rp", "-repeat", MSyntax::kLong);
  syn.addFlag("-std", "-stack_trace_depth", MSyntax::kLong);
  syn.addFlag("-tof", "-throw_on_failure");
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<std::string> constructGoogleTestArgs(MArgDatabase& database)
{
  std::cout << "CONSTRUCTING GOOGLE TESTS" << std::endl;
  std::vector<std::string> args;
  args.emplace_back("maya_tests");

  MString filter = "*";
  MString output = "";
  MString color = "yes";
  int rs = 0;
  int rp = 1;
  int sd = 100;

  if(database.isFlagSet("-ff"))
  {
    MString flag_file;
    if(database.getFlagArgument("-ff", 0, flag_file))
    {
      std::string str("--gtest_flagfile=");
      str += flag_file.asChar();
      args.emplace_back(std::move(str));
    }
  }

  if(database.isFlagSet("-nc"))
  {
    color = "no";
  }

  if(database.isFlagSet("-f"))
  {
    if(database.getFlagArgument("-f", 0, filter)) {}
  }

  if(database.isFlagSet("-o"))
  {
    if(database.getFlagArgument("-o", 0, output)) {}
  }

  if(database.isFlagSet("-rs"))
  {
    if(database.getFlagArgument("-rs", 0, rs)) {}
  }

  if(database.isFlagSet("-rp"))
  {
    if(database.getFlagArgument("-rp", 0, rp)) {}
  }

  if(database.isFlagSet("-std"))
  {
    if(database.getFlagArgument("-std", 0, sd)) {}
  }

  ::testing::GTEST_FLAG(catch_exceptions) = !database.isFlagSet("-ne");
  ::testing::GTEST_FLAG(print_time) = !database.isFlagSet("-nt");
  ::testing::GTEST_FLAG(list_tests) = database.isFlagSet("-l");
  ::testing::GTEST_FLAG(throw_on_failure) = database.isFlagSet("-tof");
  ::testing::GTEST_FLAG(filter) = filter.asChar();
  ::testing::GTEST_FLAG(output) = output.asChar();
  ::testing::GTEST_FLAG(color) = color.asChar();
  ::testing::GTEST_FLAG(random_seed) = rs;
  ::testing::GTEST_FLAG(repeat) = rp;
  ::testing::GTEST_FLAG(stack_trace_depth) = sd;

  return args;
}


//----------------------------------------------------------------------------------------------------------------------
void* UnitTestHarness::creator()
{
  return new UnitTestHarness;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus UnitTestHarness::doIt(const MArgList& args)
{
  MStatus status;
  MArgDatabase database(syntax(), args, &status);
  if(!status)
    return status;

  // the unit tests cycle manipulate the timeline quite a bit. Disable GL refresh to speed them up a bit.
  if(MGlobal::kInteractive == MGlobal::mayaState())
    MGlobal::executeCommand("refresh -suspend true");

  std::vector<std::string> arguments = constructGoogleTestArgs(database);

  char** argv = new char*[arguments.size()];
  int argc(arguments.size());
  for(size_t i = 0; i < argc; ++i)
  {
    argv[i] = (char*)arguments[i].c_str();
  }

  ::testing::InitGoogleTest(&argc, argv);
  int error_code = -1;
  if(RUN_ALL_TESTS() == 0)
  {
    error_code = 0;
  }
  delete [] argv;
  setResult(error_code);

  cleanTemporaryFiles();

  if(MGlobal::kInteractive == MGlobal::mayaState())
    MGlobal::executeCommand("refresh -suspend false");

  if(error_code)
  {
    if(::testing::GTEST_FLAG(color) != "no") std::cout << "\e[31m";
    std::cout << angry_cat;
  }
  else
  {
    if(::testing::GTEST_FLAG(color) != "no") std::cout << "\e[32m";
    std::cout << happy_cat;
  }
  return MS::kSuccess;
}

//------------------------------------------------------------------------------
void UnitTestHarness::cleanTemporaryFiles() const
{
  MString cmd(
      "import glob;"
      "import os;"
      "[os.remove(x) for x in glob.glob('/tmp/AL_USDMayaTests*.usda')];"
      "[os.remove(x) for x in glob.glob('/tmp/AL_USDMayaTests*.ma')]"
      );

  MStatus stat = MGlobal::executePythonCommand(cmd);

  if(stat != MStatus::kSuccess) {
    MGlobal::displayWarning("Unable to remove temporary test files");
  }
}
