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
#include "AL/maya/utils/CommandGuiHelper.h"

#include "AL/maya/tests/mayaplugintest/utils/CommandGuiHelperTest.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

#include <cmath>
#include <iostream>

namespace AL {
namespace maya {
namespace tests {
namespace utils {
const MString CommandGuiHelperTestCMD::kName = "AL_usdmaya_CommandGuiHelperTest";

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelperTestCMD::makeGUI()
{
    // Currently this tests all of the individual data types, all using the 'persist' parameter to
    // ensure they get added into the optionVars correctly, and can be retrieved and set by the gui.
    // There aren't exactly loads of tests regarding the non-persistent command args. I will need to
    // go test those at some point.
    AL::maya::utils::CommandGuiHelper commandOpts(
        "AL_usdmaya_CommandGuiHelperTest",
        "Unit Testy Stuff",
        "Test",
        "USD/Tests/CommandGuiHelperTest");
    commandOpts.addFlagOption("qflag", "Flag", false, true);
    commandOpts.addBoolOption("qbool", "Bool", false, true);
    commandOpts.addIntOption("qint", "Int", 42, true);
    commandOpts.addIntSliderOption("qintSlider", "Int Slider", -42, 69, 42, true);
    commandOpts.addInt2Option("qint2", "Int2", 1, 2, true);
    commandOpts.addInt3Option("qint3", "Int3", 3, 4, 5, true);
    commandOpts.addInt4Option("qint4", "Int4", 6, 7, 8, 9, true);
    commandOpts.addDoubleOption("qdouble", "Double", 2.3, true);
    commandOpts.addDoubleSliderOption("qdoubleSlider", "Double Slider", -1.2, 4.5, 2.3, true);
    commandOpts.addVec2Option("qdouble2", "Double2", 0.1, 0.4, true);
    commandOpts.addVec3Option("qdouble3", "Double3", 0.5, 0.6, 0.9, true);
    commandOpts.addVec4Option("qdouble4", "Double4", 0.8, 0.5, 0.6, 0.9, true);
    commandOpts.addColourOption("qcolour", "Colour", 0.15, 0.16, 0.19, true);

    const int32_t     values[4] = { 4, 3, 2, 1 };
    const char* const strings[5] = { "never", "eat", "shredded", "wheat", 0 };
    commandOpts.addEnumOption("qenum1", "Enum Passed as Index", 0, strings, 0, true, false);
    commandOpts.addEnumOption("qenum2", "Enum Passed as String", 0, strings, 0, true, true);
    commandOpts.addEnumOption(
        "qenum3", "Enum Passed as Mutated Index", 0, strings, values, true, false);
    commandOpts.addRadioButtonGroupOption(
        "qradio1", "Radio Passed as Index", 0, strings, 0, true, false);
    commandOpts.addRadioButtonGroupOption(
        "qradio2", "Radio Passed as String", 0, strings, 0, true, true);
    commandOpts.addRadioButtonGroupOption(
        "qradio3", "Radio Passed as Mutated Index", 0, strings, values, true, false);
    commandOpts.addStringOption(
        "qstring",
        "String",
        "hello",
        true,
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandOpts.addFilePathOption(
        "fp1",
        "File Path Load",
        AL::maya::utils::CommandGuiHelper::kLoad,
        "All Files (*.*) (*.*)",
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandOpts.addFilePathOption(
        "fp2",
        "File Path Save",
        AL::maya::utils::CommandGuiHelper::kSave,
        "All Files (*.*) (*.*)",
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandOpts.addFilePathOption(
        "fp3",
        "File Path Dir",
        AL::maya::utils::CommandGuiHelper::kDirectory,
        "All Files (*.*) (*.*)",
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandOpts.addFilePathOption(
        "fp4",
        "File Path Dir + File",
        AL::maya::utils::CommandGuiHelper::kDirectoryWithFiles,
        "All Files (*.*) (*.*)",
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    commandOpts.addFilePathOption(
        "fp5",
        "File Path Multi File",
        AL::maya::utils::CommandGuiHelper::kMultiLoad,
        "All Files (*.*) (*.*)",
        AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
}

//----------------------------------------------------------------------------------------------------------------------
MSyntax CommandGuiHelperTestCMD::createSyntax()
{
    MSyntax syn;
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wfg", "-qflag", MSyntax::kNoArg), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wb", "-qbool", MSyntax::kBoolean), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wi", "-qint", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wis", "-qintSlider", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag("-wi2", "-qint2", MSyntax::kLong, MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag("-wi3", "-qint3", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong),
        "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag(
            "-wi4", "-qint4", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong, MSyntax::kLong),
        "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wd", "-qdouble", MSyntax::kDouble), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wds", "-qdoubleSlider", MSyntax::kDouble), "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag("-wd2", "-qdouble2", MSyntax::kDouble, MSyntax::kDouble), "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag("-wd3", "-qdouble3", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble),
        "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag(
            "-wd4",
            "-qdouble4",
            MSyntax::kDouble,
            MSyntax::kDouble,
            MSyntax::kDouble,
            MSyntax::kDouble),
        "syntaxError");
    AL_MAYA_CHECK_ERROR2(
        syn.addFlag("-wc", "-qcolour", MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble),
        "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-we1", "-qenum1", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-we2", "-qenum2", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-we3", "-qenum3", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wr1", "-qradio1", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wr2", "-qradio2", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-wr3", "-qradio3", MSyntax::kLong), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-ws", "-qstring", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-fp1", "-fap1", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-fp2", "-fap2", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-fp3", "-fap3", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-fp4", "-fap4", MSyntax::kString), "syntaxError");
    AL_MAYA_CHECK_ERROR2(syn.addFlag("-fp5", "-fap5", MSyntax::kString), "syntaxError");
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
void* CommandGuiHelperTestCMD::creator() { return new CommandGuiHelperTestCMD; }

//----------------------------------------------------------------------------------------------------------------------
MStatus CommandGuiHelperTestCMD::doIt(const MArgList& args)
{
    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        return status;
    }

    bool    b;
    int     i;
    int     i2[2];
    int     i3[3];
    int     i4[4];
    double  d;
    double  d2[2];
    double  d3[3];
    double  d4[4];
    double  c[3];
    int     e1;
    MString e2;
    int     e3;
    int     r1;
    MString r2;
    int     r3;
    MString s;

    const char* const testFail = "test fail";
    if (argData.isFlagSet("qbool")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qbool", 0, b), testFail);
        std::cout << "qbool " << b << std::endl;
    } else {
        std::cout << "qbool not set" << std::endl;
    }

    if (argData.isFlagSet("qint")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint", 0, i), testFail);
        std::cout << "qint " << i << std::endl;
    } else {
        std::cout << "qint not set" << std::endl;
    }

    if (argData.isFlagSet("qint2")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint2", 0, i2[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint2", 1, i2[1]), testFail);
        std::cout << "qint2 " << i2[0] << " " << i2[1] << std::endl;
    } else {
        std::cout << "int2 not set" << std::endl;
    }

    if (argData.isFlagSet("qint3")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint3", 0, i3[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint3", 1, i3[1]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint3", 2, i3[2]), testFail);
        std::cout << "qint3 " << i3[0] << " " << i3[1] << " " << i3[2] << std::endl;
    } else {
        std::cout << "qint3 not set" << std::endl;
    }

    if (argData.isFlagSet("qint4")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint4", 0, i4[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint4", 1, i4[1]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint4", 2, i4[2]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qint4", 3, i4[3]), testFail);
        std::cout << "qint4 " << i4[0] << " " << i4[1] << " " << i4[2] << " " << i4[3] << std::endl;
    } else {
        std::cout << "qint4 not set" << std::endl;
    }

    if (argData.isFlagSet("qdouble")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble", 0, d), testFail);
        std::cout << "qdouble " << i << std::endl;
    } else {
        std::cout << "qdouble not set" << std::endl;
    }

    if (argData.isFlagSet("qdouble2")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble2", 0, d2[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble2", 1, d2[1]), testFail);
        std::cout << "qdouble2 " << d2[0] << " " << d2[1] << std::endl;
    } else {
        std::cout << "qdouble2 not set" << std::endl;
    }

    if (argData.isFlagSet("qdouble3")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble3", 0, d3[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble3", 1, d3[1]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble3", 2, d3[2]), testFail);
        std::cout << "qdouble3 " << d3[0] << " " << d3[1] << " " << d3[2] << std::endl;
    } else {
        std::cout << "qdouble3 not set" << std::endl;
    }

    if (argData.isFlagSet("qdouble4")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble4", 0, d4[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble4", 1, d4[1]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble4", 2, d4[2]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qdouble4", 3, d4[3]), testFail);
        std::cout << "qdouble4 " << d4[0] << " " << d4[1] << " " << d4[2] << " " << d4[3]
                  << std::endl;
    } else {
        std::cout << "qdouble4 not set" << std::endl;
    }

    if (argData.isFlagSet("qcolour")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qcolour", 0, c[0]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qcolour", 1, c[1]), testFail);
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qcolour", 2, c[2]), testFail);
        std::cout << "qcolour " << c[0] << " " << c[1] << " " << c[2] << std::endl;
    } else {
        std::cout << "qcolour not set" << std::endl;
    }

    if (argData.isFlagSet("qenum1")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qenum1", 0, e1), testFail);
        std::cout << "qenum1 " << e1 << std::endl;
    } else {
        std::cout << "qenum1 not set" << std::endl;
    }

    if (argData.isFlagSet("qenum2")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qenum2", 0, e2), testFail);
        std::cout << "qenum2 " << e2.asChar() << std::endl;
    } else {
        std::cout << "qenum2 not set" << std::endl;
    }

    if (argData.isFlagSet("qenum3")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qenum3", 0, e3), testFail);
        std::cout << "qenum3 " << e3 << std::endl;
    } else {
        std::cout << "qenum3 not set" << std::endl;
    }

    if (argData.isFlagSet("qradio1")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qradio1", 0, r1), testFail);
        std::cout << "qradio1 " << r1 << std::endl;
    } else {
        std::cout << "qradio1 not set" << std::endl;
    }

    if (argData.isFlagSet("qradio2")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qradio2", 0, r2), testFail);
        std::cout << "qradio2 " << r2.asChar() << std::endl;
    } else {
        std::cout << "qradio2 not set" << std::endl;
    }

    if (argData.isFlagSet("qradio3")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qradio3", 0, r3), testFail);
        std::cout << "qradio3 " << r3 << std::endl;
    } else {
        std::cout << "qradio3 not set" << std::endl;
    }

    if (argData.isFlagSet("qstring")) {
        AL_MAYA_CHECK_ERROR(argData.getFlagArgument("qstring", 0, s), testFail);
        std::cout << "qstring " << s.asChar() << std::endl;
    } else {
        std::cout << "qstring not set" << std::endl;
    }

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool test_CommandGuiHelper()
{
    const MString polyCube_constructionHistory = MString("polyCube_constructionHistory");
    const MString polyCube_width = MString("polyCube_width");
    const MString polyCube_height = MString("polyCube_height");
    const MString polyCube_depth = MString("polyCube_depth");
    const MString polyCube_subdivisionsX = MString("polyCube_subdivisionsX");
    const MString polyCube_subdivisionsY = MString("polyCube_subdivisionsY");
    const MString polyCube_subdivisionsZ = MString("polyCube_subdivisionsZ");
    const MString polyCube_name = MString("polyCube_name");
    const MString polyCube_axis = MString("polyCube_axis");

    // make sure these don't already exist (messing with our tests)
    {
        if (MGlobal::optionVarExists(polyCube_constructionHistory))
            MGlobal::removeOptionVar(polyCube_constructionHistory);
        if (MGlobal::optionVarExists(polyCube_width))
            MGlobal::removeOptionVar(polyCube_width);
        if (MGlobal::optionVarExists(polyCube_height))
            MGlobal::removeOptionVar(polyCube_height);
        if (MGlobal::optionVarExists(polyCube_depth))
            MGlobal::removeOptionVar(polyCube_depth);
        if (MGlobal::optionVarExists(polyCube_subdivisionsX))
            MGlobal::removeOptionVar(polyCube_subdivisionsX);
        if (MGlobal::optionVarExists(polyCube_subdivisionsY))
            MGlobal::removeOptionVar(polyCube_subdivisionsY);
        if (MGlobal::optionVarExists(polyCube_subdivisionsZ))
            MGlobal::removeOptionVar(polyCube_subdivisionsZ);
        if (MGlobal::optionVarExists(polyCube_name))
            MGlobal::removeOptionVar(polyCube_name);
        if (MGlobal::optionVarExists(polyCube_axis))
            MGlobal::removeOptionVar(polyCube_name);
    }

    // generate the GUI.
    {
        // see: http://help.autodesk.com/cloudhelp/2016/ENU/Maya-Tech-Docs/Commands/polyCube.html
        AL::maya::utils::CommandGuiHelper options(
            "polyCube", "Create Polygon Cube", "Create", "USD/polygons/Create Cube");
        options.addBoolOption("constructionHistory", "Construction History", true, true);
        options.addDoubleOption("width", "Width", 1.0f, true);
        options.addDoubleOption("height", "Height", 1.1f, true);
        options.addDoubleOption("depth", "Depth", 1.2f, true);
        options.addIntOption("subdivisionsX", "Subdivisions in X", 1, true);
        options.addIntOption("subdivisionsY", "Subdivisions in Y", 2, true);
        options.addIntOption("subdivisionsZ", "Subdivisions in Z", 3, true);
        options.addStringOption("name", "Name", "", false);
        const double defaultAxis[3] = { 1, 0, 0 };
        options.addVec3Option("axis", "Axis", defaultAxis, true);
        const char* enumStrings[] = { "No UVs",         "No normalization",  "Each face separately",
                                      "Normalized UVs", "Non distorted UVs", 0 };
        const int32_t enumValues[] = { 0, 1, 2, 3, 4 };
        options.addEnumOption("createUVs", "Create UVs", 4, enumStrings, enumValues, true, false);
    }

    // generate a GUI for a light (to test colour params).
    {
        AL::maya::utils::CommandGuiHelper options(
            "pointLight", "Create Point Light", "Create", "USD/lights/Create Point Light");
        double colour[3] = { 1, 1, 1 };
        options.addColourOption("rgb", "Colour", colour, true);
        double shadowColour[3] = { 0, 0, 0 };
        options.addColourOption("sc", "Shadow Colour", shadowColour, true);
    }

    // Does load set our option vars correctly?
    bool result = true;

    result = MGlobal::executeCommand("init_polyCube_optionGUI;") && result;
    bool exists = false;

    if (MGlobal::optionVarIntValue(polyCube_constructionHistory, &exists)) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_constructionHistory failed" << std::endl;
    }

    if (fabs(1.0f - MGlobal::optionVarDoubleValue(polyCube_width, &exists)) < 1e-5f) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_width failed" << std::endl;
    }

    if (fabs(1.1f - MGlobal::optionVarDoubleValue(polyCube_height, &exists)) < 1e-5f) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_height failed" << std::endl;
    }

    if (fabs(1.2f - MGlobal::optionVarDoubleValue(polyCube_depth, &exists)) < 1e-5f) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_depth failed" << std::endl;
    }

    if (1 == MGlobal::optionVarIntValue(polyCube_subdivisionsX, &exists)) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_subdivisionsX failed" << std::endl;
    }

    if (2 == MGlobal::optionVarIntValue(polyCube_subdivisionsY, &exists)) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_subdivisionsY failed" << std::endl;
    }

    if (3 == MGlobal::optionVarIntValue(polyCube_subdivisionsZ, &exists)) {
        result = exists && result;
    } else {
        result = false;
        std::cout << "polyCube_subdivisionsZ failed" << std::endl;
    }

    // create up after ourselves
    if (0) {
        if (MGlobal::optionVarExists(polyCube_constructionHistory))
            MGlobal::removeOptionVar(polyCube_constructionHistory);
        if (MGlobal::optionVarExists(polyCube_width))
            MGlobal::removeOptionVar(polyCube_width);
        if (MGlobal::optionVarExists(polyCube_height))
            MGlobal::removeOptionVar(polyCube_height);
        if (MGlobal::optionVarExists(polyCube_depth))
            MGlobal::removeOptionVar(polyCube_depth);
        if (MGlobal::optionVarExists(polyCube_subdivisionsX))
            MGlobal::removeOptionVar(polyCube_subdivisionsX);
        if (MGlobal::optionVarExists(polyCube_subdivisionsY))
            MGlobal::removeOptionVar(polyCube_subdivisionsY);
        if (MGlobal::optionVarExists(polyCube_subdivisionsZ))
            MGlobal::removeOptionVar(polyCube_subdivisionsZ);
        if (MGlobal::optionVarExists(polyCube_name))
            MGlobal::removeOptionVar(polyCube_name);
        if (MGlobal::optionVarExists(polyCube_axis))
            MGlobal::removeOptionVar(polyCube_axis);
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace tests
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
