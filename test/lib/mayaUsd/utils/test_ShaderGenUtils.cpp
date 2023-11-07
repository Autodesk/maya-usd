#include <mayaUsd/render/MaterialXGenOgsXml/ShaderGenUtil.h>

#include <pxr/imaging/hdMtlx/hdMtlx.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/Util.h>
#include <gtest/gtest.h>

#include <iostream>

constexpr auto pythonDiff = R"D(
import difflib
import sys

with open(r'^1s', 'r') as mxBaseline:
    with open(r'^1s', 'r') as mxOutput:
        diff = difflib.unified_diff(
            mxBaseline.readlines(),
            mxOutput.readlines(),
            fromfile='baseline',
            tofile='output',
        )
        [str(line) for line in diff]
)D";

TEST(ShaderGenUtils, topoChannels)
{
    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    auto searchPath = PXR_NS::HdMtlxSearchPaths();
    mx::loadLibraries({}, searchPath, library);

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    const mx::XmlReadOptions readOptions;
    mx::readFromXmlFile(doc, testPath / "topology_tests.mtlx", mx::EMPTY_STRING, &readOptions);

    for (const mx::NodePtr& material : doc->getMaterialNodes()) {
        if (material->getName() != "Interface2") {
            continue;
        }
        auto topoNetwork = MaterialXMaya::ShaderGenUtil::TopoNeutralGraph(material);

        const std::string& expectedFileName = material->getAttribute("topo");
        ASSERT_FALSE(expectedFileName.empty());

        auto baseline = mx::createDocument();
        auto baselinePath = testPath / expectedFileName;
        mx::readFromXmlFile(baseline, baselinePath, mx::EMPTY_STRING, &readOptions);

        auto outputDoc = topoNetwork.getDocument();

        if (*baseline != *outputDoc) {
            const std::string baselineStr = mx::writeToXmlString(baseline);
            const std::string outputStr = mx::writeToXmlString(outputDoc);
            ASSERT_EQ(baselineStr, outputStr) << "While testing: " << material->getName()
                                              << " against baseline " << expectedFileName;
        }

        outputDoc->importLibrary(library);
        std::string message;
        ASSERT_TRUE(outputDoc->validate(&message)) << message;
    }
}
