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
#include "testHarness.h"

#include "testHelpers.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

#include <gtest/gtest.h>

namespace AL {
namespace maya {
namespace test {

#ifdef _WIN32
#define RESET_COLOUR
#else
#define RESET_COLOUR "\e[39m"
#endif

const char* happy_dino = "               __\n"
                         "              /\"_)\n"
                         "     _.----._/ /\n"
                         "    /         /\n"
                         " __/ (  | (  |\n"
                         "/__.-'|_|--|_|\n" RESET_COLOUR;

const char* angry_dino = "               __\n"
                         "              /x_)\n"
                         "     _/\\/\\/\\_/ /\n"
                         "   _|         /\n"
                         " _|  (  | (  |\n"
                         "/__.-'|_|--|_|\n" RESET_COLOUR;

const char* happy_cat = "\n"
                        "    \\    /\\ \n"
                        "     )  ( ^)\n"
                        "    (  /  )\n"
                        "     \\(__)|\n" RESET_COLOUR;

const char* angry_cat = "\n"
                        "         // \n"
                        "        ( >)\n"
                        "   /\\  /  )\n"
                        "  /  \\(__)|\n" RESET_COLOUR;

//----------------------------------------------------------------------------------------------------------------------
const MString UnitTestHarness::kName = "AL_maya_test_UnitTestHarness";

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
    syn.addFlag("-ktf", "-keep_temp_files");
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<std::string> constructGoogleTestArgs(MArgDatabase& database)
{
    std::vector<std::string> args;
    args.emplace_back("maya_tests");

    MString filter = "*";
    MString output = "";
    MString color = "yes";
    int     rs = 0;
    int     rp = 1;
    int     sd = 100;

    if (database.isFlagSet("-ff")) {
        MString flag_file;
        if (database.getFlagArgument("-ff", 0, flag_file)) {
            std::string str("--gtest_flagfile=");
            str += flag_file.asChar();
            args.emplace_back(std::move(str));
        }
    }

    if (database.isFlagSet("-nc")) {
        color = "no";
    }

    if (database.isFlagSet("-f")) {
        if (database.getFlagArgument("-f", 0, filter)) { }
    }

    if (database.isFlagSet("-o")) {
        if (database.getFlagArgument("-o", 0, output)) { }
    }

    if (database.isFlagSet("-rs")) {
        if (database.getFlagArgument("-rs", 0, rs)) { }
    }

    if (database.isFlagSet("-rp")) {
        if (database.getFlagArgument("-rp", 0, rp)) { }
    }

    if (database.isFlagSet("-std")) {
        if (database.getFlagArgument("-std", 0, sd)) { }
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
void* UnitTestHarness::creator() { return new UnitTestHarness; }

//----------------------------------------------------------------------------------------------------------------------
MStatus UnitTestHarness::doIt(const MArgList& args)
{
    MStatus      status;
    MArgDatabase database(syntax(), args, &status);
    if (!status)
        return status;

    // the unit tests cycle manipulate the timeline quite a bit. Disable GL refresh to speed them up
    // a bit.
    if (MGlobal::kInteractive == MGlobal::mayaState())
        MGlobal::executeCommand("refresh -suspend true");

    std::vector<std::string> arguments = constructGoogleTestArgs(database);

    char**  argv = new char*[arguments.size()];
    int32_t argc(arguments.size());
    for (int32_t i = 0; i < argc; ++i) {
        argv[i] = (char*)arguments[i].c_str();
    }

    ::testing::InitGoogleTest(&argc, argv);
    int error_code = -1;
    if (RUN_ALL_TESTS() == 0 && ::testing::UnitTest::GetInstance()->test_to_run_count() > 0) {
        error_code = 0;
    }
    delete[] argv;
    setResult(error_code);

    if (!database.isFlagSet("-ktf")) {
        cleanTemporaryFiles();
    }

    if (MGlobal::kInteractive == MGlobal::mayaState())
        MGlobal::executeCommand("refresh -suspend false");

    if (error_code) {
#ifndef _WIN32
        if (::testing::GTEST_FLAG(color) != "no")
            std::cout << "\e[31m";
#endif
        std::cout << angry_dino;
    } else {
#ifndef _WIN32
        if (::testing::GTEST_FLAG(color) != "no")
            std::cout << "\e[32m";
#endif
        std::cout << happy_dino;
    }
    // return the status based on the error code
    status = (error_code == 0) ? MS::kSuccess : MS::kFailure;
    return status;
}

//------------------------------------------------------------------------------
void UnitTestHarness::cleanTemporaryFiles() const
{
    const MString temp_path = buildTempPath("AL_USDMayaTests*.*");
    MString       cmd(
        "import glob;"
        "import os;"
        "[os.remove(x) for x in glob.glob('"
        + temp_path + "')];");

    MStatus stat = MGlobal::executePythonCommand(cmd);

    if (stat != MStatus::kSuccess) {
        MGlobal::displayWarning("Unable to remove temporary test files");
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace test
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
