
#include "testUtils.h"

#include <mayaHydraLib/mayaUtils.h>
#include <mayaHydraLib/utils.h>

#include <pxr/imaging/hd/tokens.h>

#include <maya/MViewport2Renderer.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace MAYAHYDRA_NS;

TEST(MayaUsdUfeItems, skipUsdUfeItems)
{
    // Setup inspector for the first scene index
    const SceneIndicesVector& sceneIndices = GetTerminalSceneIndices();
    ASSERT_GT(sceneIndices.size(), static_cast<size_t>(0));
    SceneIndexInspector inspector(sceneIndices.front());

    // Check if there are UFE prims that contain data
    FindPrimPredicate findCubePrimPredicate
        = [](const HdSceneIndexBasePtr& sceneIndex, const SdfPath& primPath) -> bool {
        std::string      primPathString = primPath.GetAsString();
        bool             isUfeItem = primPathString.find("ufe", 0) < std::string::npos;
        HdSceneIndexPrim prim = sceneIndex->GetPrim(primPath);
        bool             containsData = HdContainerDataSource::Cast(prim.dataSource) != nullptr;
        return isUfeItem && containsData;
    };
    PrimEntriesVector foundPrims = inspector.FindPrims(findCubePrimPredicate, 1);
    EXPECT_EQ(foundPrims.size(), static_cast<size_t>(0));
}
