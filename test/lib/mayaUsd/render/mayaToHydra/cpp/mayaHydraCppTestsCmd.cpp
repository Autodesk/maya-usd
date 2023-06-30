#include "mayaHydraCppTestsCmd.h"
#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>
#include <gtest/gtest.h>

namespace 
{
constexpr auto _filter = "-f";
constexpr auto _filterLong = "-filter";

}

MSyntax mayaHydraCppTestCmd::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag(_filter, _filterLong, MSyntax::kString);
    return syntax;
}

std::vector<std::string> constructGoogleTestArgs(const MArgDatabase& database)
{
    std::vector<std::string> args;
    args.emplace_back("mayahydra_tests");

    MString filter = "*";

    if (database.isFlagSet("-f")) {
        if (database.getFlagArgument("-f", 0, filter)) { }
    }

    ::testing::GTEST_FLAG(filter) = filter.asChar();

    return args;
}

MStatus mayaHydraCppTestCmd::doIt( const MArgList& args ) 
{
    MStatus status;
    MArgDatabase db(syntax(), args, &status);
    if (!status) {
        return status;
    }

    std::vector<std::string> arguments = constructGoogleTestArgs(db);

    char**  argv = new char*[arguments.size()];
    int32_t argc(arguments.size());
    for (int32_t i = 0; i < argc; ++i) {
        argv[i] = (char*)arguments[i].c_str();
    }

    // By default, if no filter flag is given, all tests will run
    ::testing::InitGoogleTest(&argc, argv);
    if (RUN_ALL_TESTS() == 0 && ::testing::UnitTest::GetInstance()->test_to_run_count() > 0) {
        MGlobal::displayInfo("This test passed.");
        return MS::kSuccess;
    }

    MGlobal::displayInfo("This test failed.");
    return MS::kFailure;
}

MStatus initializePlugin( MObject obj ) 
{
    MFnPlugin plugin( obj, "Autodesk", "1.0", "Any" );
    plugin.registerCommand( "mayaHydraCppTest", mayaHydraCppTestCmd::creator, mayaHydraCppTestCmd::createSyntax );
    return MS::kSuccess;
}

MStatus uninitializePlugin( MObject obj ) 
{
    MFnPlugin plugin( obj );
    plugin.deregisterCommand( "mayaHydraCppTest" );
    return MS::kSuccess;
}