#include <mayaUsdUtils/MergePrims.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/relationship.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {

const SdfPath primPath("/A");

const SdfPath childPath1("/A/B");
const SdfPath childPath2("/A/C");

const SdfPath targetPath1("/target1");
const SdfPath targetPath2("/target2");
const SdfPath targetPath3("/target3");

const TfToken testAttrName("test_attr");
const TfToken otherAttrName("other_attr");

const TfToken testRelName("test_rel");
const TfToken otherRelName("other_rel");

const SdfValueTypeName doubleType = SdfValueTypeNames->Double;

const bool mergeChildren = true;
const bool dontMergeChildren = false;

UsdPrim createPrim(UsdStageRefPtr& stage, const SdfPath& path)
{
    return stage->DefinePrim(SdfPath(path), TfToken("xform"));
}

UsdAttribute createAttr(UsdPrim& prim, const TfToken& attrName, double value)
{
    auto attr = prim.CreateAttribute(attrName, doubleType, true);
    attr.Set(value);
    return attr;
}

UsdAttribute createAttr(UsdPrim& prim, double value)
{
    return createAttr(prim, testAttrName, value);
}

UsdRelationship createRel(UsdPrim& prim, const TfToken& relName, const SdfPath& target)
{
    auto rel = prim.CreateRelationship(relName, true);
    rel.AddTarget(target);
    return rel;
}

UsdPrim createChild(UsdStageRefPtr& stage, const SdfPath& path, double value)
{
    auto child = createPrim(stage, path);
    createAttr(child, value);
    return child;
}

template <class ITER_RANGE>
size_t rangeSize(const ITER_RANGE& range)
{
    size_t count = 0;
    for (const auto& data : range)
        ++count;
    return count;
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
/// Children difference.

TEST(MergePrims, mergePrimsEmpty)
{
    // Test that prims with no attribute and no children are considered identical.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(0));
}

TEST(MergePrims, mergePrimsSameChildren)
{
    // Test that prims with no attribute and identical children are considered identical.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 1.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);
}

TEST(MergePrims, mergePrimsDiffChildren)
{
    // Test that prims with no attribute and children with different atttribute values are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 2.0);
    createChild(modifiedStage, childPath2, 3.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 2.);

    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 3.);
}

TEST(MergePrims, mergePrimsAbsentChild)
{
    // Test that prims with no attribute and a mssing child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    EXPECT_FALSE(baselineChild2.IsValid());
}

TEST(MergePrims, mergePrimsCreatedChild)
{
    // Test that prims with no attribute and an extra child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 2.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    auto baselineChild2 = baselineStage->GetPrimAtPath(childPath2);
    EXPECT_TRUE(baselineChild2.IsValid());
    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 2.);
}

//----------------------------------------------------------------------------------------------------------------------
/// Merging prim only: not merging children.

TEST(MergePrims, mergePrimsOnlySameChildren)
{
    // Test that prims with no attribute and identical children are untouched.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 1.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        dontMergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);
}

TEST(MergePrims, mergePrimsOnlyDiffChildren)
{
    // Test that prims with no attribute and children with different atttribute values are
    // left unchanged due to ignoring children.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 2.0);
    createChild(modifiedStage, childPath2, 3.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        dontMergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    // Verify children values have not been merged.

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);
}

TEST(MergePrims, mergePrimsOnlyAbsentChild)
{
    // Test that prims with no attribute and a mssing child are left unchanged due to ignoring
    // children.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);
    auto baselineChild2 = createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        dontMergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    // Verify both children still exist since we did not merge children.

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(2));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    EXPECT_EQ(baselineChild2.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild2.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);
}

TEST(MergePrims, mergePrimsOnlyCreatedChild)
{
    // Test that prims with no attribute and an extra child are left unchanged due to ignoring
    // children.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild1 = createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 2.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        dontMergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    // Verify both first child still exist but no other child was added since we did not merge
    // children.

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    double value = 0.;

    EXPECT_EQ(baselineChild1.GetAuthoredAttributes().size(), size_t(1));
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild1.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 1.);

    auto baselineChild2 = baselineStage->GetPrimAtPath(childPath2);
    EXPECT_FALSE(baselineChild2.IsValid());
}

//----------------------------------------------------------------------------------------------------------------------
/// Children attribute differences.

TEST(MergePrims, mergePrimsAbsentChildAttribute)
{
    // Test that prims with no attribute and the same children but with a missing attribute are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    createAttr(baselineChild, otherAttrName, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 2.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    double value = 0.;

    EXPECT_EQ(baselineChild.GetAuthoredAttributes().size(), size_t(1));

    EXPECT_TRUE(baselineChild.GetAttribute(testAttrName).IsValid());
    EXPECT_TRUE(baselineChild.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 2.);

    EXPECT_FALSE(baselineChild.GetAttribute(otherAttrName).IsValid());
}

TEST(MergePrims, mergePrimsCreatedChildAttribute)
{
    // Test that prims with no attribute and the same children but with an extra attribute are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 2.0);
    createAttr(modifiedChild, otherAttrName, 1.0);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    double value = 0.;

    EXPECT_EQ(baselineChild.GetAuthoredAttributes().size(), size_t(2));
    EXPECT_TRUE(baselineChild.GetAttribute(testAttrName).Get(&value));
    EXPECT_EQ(value, 2.);

    EXPECT_TRUE(baselineChild.GetAttribute(otherAttrName).IsValid());
    EXPECT_TRUE(baselineChild.GetAttribute(otherAttrName).Get(&value));
    EXPECT_EQ(value, 1.);
}

//----------------------------------------------------------------------------------------------------------------------
/// Children relationship differences.

TEST(MergePrims, mergePrimsAbsentChildRelationship)
{
    // Test that prims with the same children but with a missing relationship are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    auto baselineRel1 = createRel(baselineChild, testRelName, targetPath1);
    auto baselineRel2 = createRel(baselineChild, otherRelName, targetPath2);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);
    auto modifiedRel1 = createRel(modifiedChild, testRelName, targetPath1);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    SdfPathVector targets;

    EXPECT_EQ(baselineChild.GetAuthoredRelationships().size(), size_t(1));

    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).IsValid());
    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).GetTargets(&targets));
    EXPECT_EQ(targets.size(), size_t(1));
    EXPECT_EQ(targets[0], targetPath1);
}

TEST(MergePrims, mergePrimsCreatedChildRelationship)
{
    // Test that prims with no relationship and the same children but with an extra relationship are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    auto baselineRel1 = createRel(baselineChild, testRelName, targetPath1);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);
    createRel(modifiedChild, testRelName, targetPath1);
    createRel(modifiedChild, otherRelName, targetPath2);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    SdfPathVector targets;

    EXPECT_EQ(baselineChild.GetAuthoredRelationships().size(), size_t(2));

    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).IsValid());
    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).GetTargets(&targets));
    EXPECT_EQ(targets.size(), size_t(1));
    EXPECT_EQ(targets[0], targetPath1);

    EXPECT_TRUE(baselineChild.GetRelationship(otherRelName).IsValid());
    EXPECT_TRUE(baselineChild.GetRelationship(otherRelName).GetTargets(&targets));
    EXPECT_EQ(targets.size(), size_t(1));
    EXPECT_EQ(targets[0], targetPath2);
}

TEST(MergePrims, mergePrimsChildRelationshipAddTarget)
{
    // Test that prims with no relationship and the same children but with an extra relationship are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    auto baselineRel1 = createRel(baselineChild, testRelName, targetPath1);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);
    auto modifiedRel1 = createRel(modifiedChild, testRelName, targetPath1);
    modifiedRel1.AddTarget(targetPath3);

    const bool result = mergePrims(
        modifiedStage,
        modifiedStage->GetRootLayer(),
        modifiedPrim.GetPath(),
        baselineStage,
        baselineStage->GetRootLayer(),
        baselinePrim.GetPath(),
        mergeChildren,
        MergeVerbosity::Failure);

    EXPECT_TRUE(result);

    EXPECT_EQ(baselinePrim.GetAuthoredProperties().size(), size_t(0));
    EXPECT_EQ(rangeSize(baselinePrim.GetChildren()), size_t(1));

    SdfPathVector targets;

    EXPECT_EQ(baselineChild.GetAuthoredRelationships().size(), size_t(1));

    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).IsValid());
    EXPECT_TRUE(baselineChild.GetRelationship(testRelName).GetTargets(&targets));
    EXPECT_EQ(targets.size(), size_t(2));
    EXPECT_EQ(targets[0], targetPath1);
    EXPECT_EQ(targets[1], targetPath3);
}
