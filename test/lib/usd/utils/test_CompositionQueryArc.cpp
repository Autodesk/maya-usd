#include <mayaUsdUtils/util.h>

#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>

#include <gtest/gtest.h>

TEST(Util, layersWithContribution)
{
    // creates a new stage only in memory
    auto stage = pxr::UsdStage::CreateInMemory();
    auto rootLayer = stage->GetRootLayer();

    // create a new prim whose typeName is Xform
    auto basePrim = stage->DefinePrim(pxr::SdfPath("/base"), pxr::TfToken("Xform"));

    // authoring new transformation
    pxr::UsdGeomXformable xformable(basePrim);
    pxr::UsdGeomXformOp trans = xformable.AddTranslateOp();
    trans.Set(pxr::GfVec3d(0.0, 5.0, 0.0));

    // authoring new radius and color
    auto spherePrim = stage->DefinePrim(pxr::SdfPath("/base/green_sphere"), pxr::TfToken("Sphere"));

    auto radiusAttr = spherePrim.GetAttribute(pxr::TfToken("radius"));
    radiusAttr.Set(pxr::VtValue(1.2));

    auto displayColorAttr = spherePrim.GetAttribute(pxr::TfToken("primvars:displayColor"));
    pxr::VtVec3fArray color(1, pxr::GfVec3f(0.0,1.0,0.0));
    displayColorAttr.Set(color);

    // expected to have one composition arc Pcp.ArcTypeRoot
    auto layers = MayaUsdUtils::layersWithContribution(spherePrim);
    EXPECT_EQ(layers.size(), 1);
    EXPECT_EQ(layers[0]->GetDisplayName(), "tmp.usda");

    auto refPrim = stage->OverridePrim(SdfPath("/base/green_sphere_reference"));
    refPrim.GetReferences().AddReference(SdfReference(rootLayer->GetIdentifier(), SdfPath("/base/green_sphere")));

    // expected to have two composition arc ( Pcp.ArcTypeRoot and PcpArcTypeReference )
    auto layers2 = MayaUsdUtils::layersWithContribution(refPrim);
    EXPECT_EQ(layers2.size(), 2);
}


