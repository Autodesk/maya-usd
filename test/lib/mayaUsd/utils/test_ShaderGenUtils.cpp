#include <mayaUsd/render/MaterialXGenOgsXml/ShaderGenUtil.h>

#include <pxr/imaging/hdMtlx/hdMtlx.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenShader/Util.h>
#include <gtest/gtest.h>

#include <algorithm>
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
#if MX_COMBINED_VERSION < 13900
    mx::readFromXmlFile(doc, testPath / "topology_tests.mtlx", mx::EMPTY_STRING, &readOptions);
#else
    mx::readFromXmlFile(doc, testPath / "topology_tests_1_39.mtlx", mx::EMPTY_STRING, &readOptions);
#endif

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
#if MX_COMBINED_VERSION < 13900
    ASSERT_EQ(topoNetwork.getOriginalPath("NG0/N4"), "Ng9a/constant");
#else
    // Swizzle became explicit extract, so constant moved down one notch.
    ASSERT_EQ(topoNetwork.getOriginalPath("NG0/N5"), "Ng9a/constant");
#endif

    // Test watch list API:
    const auto& watchList = topoNetwork.getWatchList();
    auto        it = watchList.find(doc->getDescendant("Surf9"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eRegular);
#if MX_COMBINED_VERSION < 13900
    it = watchList.find(doc->getDescendant("Ng9b"));
    ASSERT_NE(it, watchList.end());
    ASSERT_EQ(it->second, sgu::TopoNeutralGraph::ElementType::eRegular);
    // Nodegraph itself is no longer topological in v1.39 since the swizzle was removed.
#endif
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

TEST(ShaderGenUtils, lobePruner)
{
    namespace sgu = MaterialXMaya::ShaderGenUtil;

    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    auto searchPath = PXR_NS::HdMtlxSearchPaths();
    mx::loadLibraries({}, searchPath, library);

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    auto lobePruner = sgu::LobePruner::create();
    lobePruner->setLibrary(doc);

    const auto attrVec = lobePruner->getOptimizedAttributeNames(
        doc->getNodeDef("ND_standard_surface_surfaceshader"));
    ASSERT_FALSE(attrVec.empty());
    ASSERT_TRUE(std::is_sorted(attrVec.begin(), attrVec.end()));
    ASSERT_TRUE(std::lower_bound(attrVec.begin(), attrVec.end(), "subsurface") != attrVec.end());

    const auto node = doc->addNode("standard_surface", "bob", "surfaceshader");

    auto optimizedNodeDef = lobePruner->getOptimizedNodeDef(*node);
    ASSERT_TRUE(optimizedNodeDef);
    // An x means can not optimize on that attribute
    // A 0 means we optimized due to this value being zero
    ASSERT_EQ(optimizedNodeDef->getNodeString(), "standard_surface_x0000x00x000");

    auto input = node->addInputFromNodeDef("subsurface");
    input->setValueString("1.0");
    optimizedNodeDef = lobePruner->getOptimizedNodeDef(*node);
    ASSERT_TRUE(optimizedNodeDef);
#if MX_COMBINED_VERSION < 13904
    // Now have a 1 for subsurface since we can also optimize the 1 value for mix nodes.
    ASSERT_EQ(optimizedNodeDef->getNodeString(), "standard_surface_x0000x00x010");
#else
    // Starting at 1.39.4 we have an X for subsurface since standard surface no longer uses a mix
    // node. See https://github.com/AcademySoftwareFoundation/MaterialX/pull/2483
    ASSERT_EQ(optimizedNodeDef->getNodeString(), "standard_surface_x0000x00x0x0");
#endif

    PXR_NS::HdMaterialNode2 usdNode;
    usdNode.nodeTypeId = PXR_NS::TfToken("ND_standard_surface_surfaceshader");
    auto optimizedNodeId = lobePruner->getOptimizedNodeId(usdNode);
    ASSERT_EQ(
        optimizedNodeId.GetString(),
        sgu::LobePruner::getOptimizedNodeDefPrefix()
            + "standard_surface_x0000x00x000_surfaceshader");
    ASSERT_TRUE(lobePruner->isOptimizedNodeId(optimizedNodeId));

    usdNode.nodeTypeId = PXR_NS::TfToken("ND_mix_surfaceshader");
    optimizedNodeId = lobePruner->getOptimizedNodeId(usdNode);
    ASSERT_TRUE(optimizedNodeId.IsEmpty());
}

TEST(ShaderGenUtils, lobePrunedTopoGraph)
{
    namespace sgu = MaterialXMaya::ShaderGenUtil;

    auto testPath = mx::FilePath(MATERIALX_TEST_DATA);

    auto library = mx::createDocument();
    ASSERT_TRUE(library != nullptr);
    auto searchPath = PXR_NS::HdMtlxSearchPaths();
    mx::loadLibraries({}, searchPath, library);

    auto doc = mx::createDocument();
    doc->importLibrary(library);

    auto lobePruner = sgu::LobePruner::create();
    lobePruner->setLibrary(doc);

    const mx::XmlReadOptions readOptions;
    mx::readFromXmlFile(doc, testPath / "MultiConnect1_topo.mtlx", mx::EMPTY_STRING, &readOptions);

    const auto originalCategory = doc->getNode("N1")->getCategory();

    auto topoNetwork
        = MaterialXMaya::ShaderGenUtil::TopoNeutralGraph(doc->getNode("N0"), lobePruner, false);

    // In thory, we should have an empty NodeGraph since we are in untextured mode:
    ASSERT_TRUE(topoNetwork.getNodeGraph()->getNodes().empty());

    // Should have only 2 nodes:
    ASSERT_EQ(topoNetwork.getDocument()->getNodes().size(), 2);

    // Surface should be optimized and fully unconnected:
    const auto surface = topoNetwork.getDocument()->getNode("N1");

    const auto surfaceCategory = surface->getCategory();

    // Should begin with standard_surface
    ASSERT_TRUE(surfaceCategory.rfind(originalCategory, 0) == 0);

    // But have a LobePruner optimization:
    ASSERT_TRUE(surfaceCategory.size() > originalCategory.size());

    // Which should be in the library of the LobePruner:
    topoNetwork.getDocument()->importLibrary(doc);
    const auto optNodeDef = surface->getNodeDef();
    ASSERT_TRUE(optNodeDef != nullptr);
}
