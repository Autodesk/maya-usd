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

TEST(ShaderGenUtils, topoChannels)
{
    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto searchPath = PXR_NS::HdMtlxSearchPaths();
#if PXR_VERSION > 2311
    auto library = PXR_NS::HdMtlxStdLibraries();
#else
    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    mx::loadLibraries({}, searchPath, library);
#endif

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    const mx::XmlReadOptions readOptions;
    mx::readFromXmlFile(doc, testPath / "topology_tests.mtlx", mx::EMPTY_STRING, &readOptions);

    for (const mx::NodePtr& material : doc->getMaterialNodes()) {
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
        if (material->getName().rfind("Broken", 0) != 0) {
            ASSERT_TRUE(outputDoc->validate(&message)) << message;
        }
    }
}

TEST(ShaderGenUtils, topoGraphAPI)
{
    namespace sgu = MaterialXMaya::ShaderGenUtil;

    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    auto searchPath = PXR_NS::HdMtlxSearchPaths();
    mx::loadLibraries({}, searchPath, library);

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    const mx::XmlReadOptions readOptions;
    mx::readFromXmlFile(doc, testPath / "topology_tests.mtlx", mx::EMPTY_STRING, &readOptions);

    const mx::NodePtr& material = doc->getNode("Interface2");
    auto               topoNetwork = sgu::TopoNeutralGraph(material);

    // Test remapping API:
    ASSERT_EQ(topoNetwork.getOriginalPath("N1"), "Surf9");
    ASSERT_EQ(topoNetwork.getOriginalPath("NG0/N2"), "Ng9b/add9b");
    ASSERT_EQ(topoNetwork.getOriginalPath("NG0/N3"), "add9a");
    ASSERT_EQ(topoNetwork.getOriginalPath("NG0/N4"), "Ng9a/constant");

    // Test watch list API:
    const auto& watchList = topoNetwork.getWatchList();
    auto        it = watchList.find(doc->getDescendant("Surf9"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eRegular);
    it = watchList.find(doc->getDescendant("Ng9b"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eRegular);
    it = watchList.find(doc->getDescendant("Ng9b/add9b"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eRegular);
    it = watchList.find(doc->getDescendant("Ng9a/constant"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eTopological);
}

#if MX_COMBINED_VERSION >= 13811
TEST(ShaderGenUtils, topoGraphAPI_defaultgeomprop)
{
    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto searchPath = PXR_NS::HdMtlxSearchPaths();
#if PXR_VERSION > 2311
    auto library = PXR_NS::HdMtlxStdLibraries();
#else
    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    mx::loadLibraries({}, searchPath, library);
#endif

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    const mx::XmlReadOptions readOptions;
    mx::readFromXmlFile(
        doc, testPath / "defaultgeomprop_topo.mtlx", mx::EMPTY_STRING, &readOptions);

    for (const mx::NodePtr& material : doc->getMaterialNodes()) {
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
        if (material->getName().rfind("Broken", 0) != 0) {
            ASSERT_TRUE(outputDoc->validate(&message)) << message;
        }
    }
}
#endif
