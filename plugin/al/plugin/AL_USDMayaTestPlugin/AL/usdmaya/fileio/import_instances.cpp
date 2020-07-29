#include "AL/usdmaya/Metadata.h"
#include "test_usdmaya.h"

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>

#include <fstream>
#include <set>
#include <string>

using AL::maya::test::buildTempPath;

extern const char* const g_merged;
extern const char* const g_unmerged;

TEST(import_instances, merged)
{
    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_mergedInstances.usda");
    {
        std::ofstream ofs(temp_path);
        ofs << g_merged;
    }

    MString importStr = "file -import -type \"AL usdmaya import\"  -ignoreVersion -ra true "
                        "-mergeNamespacesOnClash false -namespace \"instanced\" -options "
                        "\"Parent_Path=;Import_Animations=1;Import_Dynamic_Attributes=1;Load_None="
                        "0;Read_Default_Values=1;Activate_all_Plugin_Translators=1;Active_"
                        "Translator_List=;Inactive_Translator_List=;Import_Curves=1;Import_Meshes="
                        "1;\" -pr -importFrameRate true -importTimeRange \"override\" \"";
    importStr += temp_path.c_str();
    importStr += "\";";
    MGlobal::executeCommand(importStr);

    MStringArray xformResults;
    MGlobal::executeCommand("ls -type transform", xformResults);

    // filter out the default TM's that maya creates
    for (uint32_t i = 0; i < xformResults.length();) {
        if (xformResults[i] == "front" || xformResults[i] == "side" || xformResults[i] == "persp"
            || xformResults[i] == "top") {
            xformResults.remove(i);
        } else {
            ++i;
        }
    }

    std::set<std::string> expectedTransforms = { "InstanceParent1",
                                                 "InstanceParent1|FirstInstanceLevel",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1|pCube1",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1|pCube2",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes2",
                                                 "InstanceParent2",
                                                 "Root" };
    ASSERT_EQ(expectedTransforms.size(), xformResults.length());

    // with any luck, all the transforms should match
    uint32_t i = 0;
    for (; i < xformResults.length(); ++i) {
        uint32_t removed = expectedTransforms.erase(xformResults[i].asChar());
        ASSERT_TRUE(removed != 0);
    }

    EXPECT_EQ(0u, expectedTransforms.size());

    MStringArray meshResults;
    MGlobal::executeCommand("ls -type mesh", meshResults);
    EXPECT_EQ(2u, meshResults.length());
}

TEST(import_instances, unmerged)
{
    MFileIO::newFile(true);

    const std::string temp_path = buildTempPath("AL_USDMayaTests_unmergedInstances.usda");
    {
        std::ofstream ofs(temp_path);
        ofs << g_unmerged;
    }

    MString importStr = "file -import -type \"AL usdmaya import\"  -ignoreVersion -ra true "
                        "-mergeNamespacesOnClash false -namespace \"instanced\" -options "
                        "\"Parent_Path=;Import_Animations=1;Import_Dynamic_Attributes=1;Load_None="
                        "0;Read_Default_Values=1;Activate_all_Plugin_Translators=1;Active_"
                        "Translator_List=;Inactive_Translator_List=;Import_Curves=1;Import_Meshes="
                        "1;\" -pr -importFrameRate true -importTimeRange \"override\" \"";
    importStr += temp_path.c_str();
    importStr += "\";";

    MGlobal::executeCommand(importStr);

    MStringArray xformResults;
    MGlobal::executeCommand("ls -type transform", xformResults);

    // filter out the default TM's that maya creates
    for (uint32_t i = 0; i < xformResults.length();) {
        if (xformResults[i] == "front" || xformResults[i] == "side" || xformResults[i] == "persp"
            || xformResults[i] == "top") {
            xformResults.remove(i);
        } else {
            ++i;
        }
    }

    std::set<std::string> expectedTransforms = { "InstanceParent1",
                                                 "InstanceParent1|FirstInstanceLevel",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1|pCube1",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes1|pCube2",
                                                 "InstanceParent1|FirstInstanceLevel|Boxes2",
                                                 "InstanceParent2",
                                                 "Root" };
    ASSERT_EQ(expectedTransforms.size(), xformResults.length());

    // with any luck, all the transforms should match
    uint32_t i = 0;
    for (; i < xformResults.length(); ++i) {
        uint32_t removed = expectedTransforms.erase(xformResults[i].asChar());
        ASSERT_TRUE(removed != 0);
    }

    EXPECT_EQ(0u, expectedTransforms.size());

    MStringArray meshResults;
    MGlobal::executeCommand("ls -type mesh", meshResults);
    EXPECT_EQ(2u, meshResults.length());
}

const char* const g_merged =
    R"(#usda 1.0

class "inner"
{
def Mesh "pCube1"
{
    int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
    int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
    normal3f[] normals (
        interpolation = "faceVarying"
    )
    normal3f[] normals.timeSamples = {
        1: [(0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, -1, 0), (0, -1, 0), (0, -1, 0), (0, -1, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0)],
    }
    point3f[] points.timeSamples = {
        1: [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)],
    }
    texCoord2f[] primvars:st = [(0.33, 0), (0.66333336, 0), (0.33, 0.25), (0.66333336, 0.25), (0.33, 0.5), (0.66333336, 0.5), (0.33, 0.75), (0.66333336, 0.75), (0.33, 1), (0.66333336, 1), (1, 0), (1, 0.25), (0, 0), (0, 0.25)] (
        interpolation = "faceVarying"
    )
    int[] primvars:st:indices.timeSamples = {
        1: [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 9, 8, 1, 10, 11, 3, 12, 0, 2, 13],
    }
}
def Mesh "pCube2"
{
    int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
    int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
    normal3f[] normals (
        interpolation = "faceVarying"
    )
    normal3f[] normals.timeSamples = {
        1: [(0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, -1, 0), (0, -1, 0), (0, -1, 0), (0, -1, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0)],
    }
    point3f[] points.timeSamples = {
        1: [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)],
    }
    texCoord2f[] primvars:st = [(0.33, 0), (0.66333336, 0), (0.33, 0.25), (0.66333336, 0.25), (0.33, 0.5), (0.66333336, 0.5), (0.33, 0.75), (0.66333336, 0.75), (0.33, 1), (0.66333336, 1), (1, 0), (1, 0.25), (0, 0), (0, 0.25)] (
        interpolation = "faceVarying"
    )
    int[] primvars:st:indices.timeSamples = {
        1: [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 9, 8, 1, 10, 11, 3, 12, 0, 2, 13],
    }
    Vec3d xformOp:translate = (0, 1.5, 0)
    uniform token[] xformOpOrder = ["xformOp:translate"]
}
}

class "outer"
{
def "FirstInstanceLevel"
{
    def Xform "Boxes1" (
        instanceable = true
	inherits = </inner>
    )
    {
        Vec3d xformOp:translate = (-6.07907941905379, 0.582578518187711, 4.61637983370438)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
    def Xform "Boxes2" (
        instanceable = true
	inherits = </inner>
    )
    {
        Vec3d xformOp:translate = (-6.07907941905379, 0.582578518187711, -4.61637983370438)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}
}
def "Root"
{
    def Xform "InstanceParent1" (
        instanceable = true
	inherits = </outer>
    )
    {
        Vec3d xformOp:translate = (4, 0, 0)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
    def Xform "InstanceParent2" (
        instanceable = true
	inherits = </outer>
    )
    {
        Vec3d xformOp:translate = (-4, 0, 0)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}
)";

const char* const g_unmerged =
    R"(#usda 1.0

class "inner"
{

def Xform "pCube1" (
    al_usdmaya_mergedTransform = "unmerged"
)
{
    uniform token[] xformOpOrder = []
def Mesh "pCubeShape1"
{
    int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
    int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
    normal3f[] normals (
        interpolation = "faceVarying"
    )
    normal3f[] normals.timeSamples = {
        1: [(0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, -1, 0), (0, -1, 0), (0, -1, 0), (0, -1, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0)],
    }
    point3f[] points.timeSamples = {
        1: [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)],
    }
    texCoord2f[] primvars:st = [(0.33, 0), (0.66333336, 0), (0.33, 0.25), (0.66333336, 0.25), (0.33, 0.5), (0.66333336, 0.5), (0.33, 0.75), (0.66333336, 0.75), (0.33, 1), (0.66333336, 1), (1, 0), (1, 0.25), (0, 0), (0, 0.25)] (
        interpolation = "faceVarying"
    )
    int[] primvars:st:indices.timeSamples = {
        1: [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 9, 8, 1, 10, 11, 3, 12, 0, 2, 13],
    }
}
}

def Xform "pCube2" (
    al_usdmaya_mergedTransform = "unmerged"
)
{
    Vec3d xformOp:translate = (0, 1.5, 0)
    uniform token[] xformOpOrder = ["xformOp:translate"]
def Mesh "pCube2Shape"
{
    int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
    int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
    normal3f[] normals (
        interpolation = "faceVarying"
    )
    normal3f[] normals.timeSamples = {
        1: [(0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 0, 1), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, 0, -1), (0, -1, 0), (0, -1, 0), (0, -1, 0), (0, -1, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0), (-1, 0, 0)],
    }
    point3f[] points.timeSamples = {
        1: [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)],
    }
    texCoord2f[] primvars:st = [(0.33, 0), (0.66333336, 0), (0.33, 0.25), (0.66333336, 0.25), (0.33, 0.5), (0.66333336, 0.5), (0.33, 0.75), (0.66333336, 0.75), (0.33, 1), (0.66333336, 1), (1, 0), (1, 0.25), (0, 0), (0, 0.25)] (
        interpolation = "faceVarying"
    )
    int[] primvars:st:indices.timeSamples = {
        1: [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 9, 8, 1, 10, 11, 3, 12, 0, 2, 13],
    }
}
}
}

class "outer"
{
def "FirstInstanceLevel"
{
    def Xform "Boxes1" (
        instanceable = true
	inherits = </inner>
    )
    {
        Vec3d xformOp:translate = (-6.07907941905379, 0.582578518187711, 4.61637983370438)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
    def Xform "Boxes2" (
        instanceable = true
	inherits = </inner>
    )
    {
        Vec3d xformOp:translate = (-6.07907941905379, 0.582578518187711, -4.61637983370438)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}
}
def "Root"
{
    def Xform "InstanceParent1" (
        instanceable = true
	inherits = </outer>
    )
    {
        Vec3d xformOp:translate = (4, 0, 0)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
    def Xform "InstanceParent2" (
        instanceable = true
	inherits = </outer>
    )
    {
        Vec3d xformOp:translate = (-4, 0, 0)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }
}


)";
