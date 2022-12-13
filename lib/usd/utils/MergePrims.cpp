//
// Copyright 2021 Autodesk
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
#include "MergePrims.h"

#include "DiffPrims.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <algorithm>
#include <utility>

namespace MayaUsdUtils {

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Data used for merging passed to all helper functions.
struct MergeContext
{
    const MergePrimsOptions& options;
    const UsdStageRefPtr&    srcStage;
    const SdfPath&           srcRootPath;
    const UsdStageRefPtr&    dstStage;
    const SdfPath&           dstRootPath;
};

//----------------------------------------------------------------------------------------------------------------------
/// Description of a merge location: layer, path, field and if the field already exists at that
/// location.
struct MergeLocation
{
    const SdfLayerHandle& layer;
    const SdfPath&        path;
    const TfToken&        field;
    bool                  fieldExists;
};

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field to the Maya console with some messages.
void printAboutField(
    const MergeContext&  ctx,
    const MergeLocation& loc,
    MergeVerbosity       printVerbosity,
    const char*          message,
    const char*          message2 = nullptr)
{
    if (!contains(printVerbosity, ctx.options.verbosity))
        return;

    TF_STATUS(
        "Layer [%s] / Path [%s] / Field [%s]: %s%s",
        loc.layer->GetDisplayName().c_str(),
        loc.path.GetText(),
        loc.field.GetText(),
        message ? message : "",
        message2 ? message2 : "");
}

//----------------------------------------------------------------------------------------------------------------------
/// Print a layer / path / field when a rare failure occurs.
void printAboutFailure(
    const MergeContext&  ctx,
    const MergeLocation& loc,
    const char*          message,
    const char*          message2 = nullptr)
{
    printAboutField(ctx, loc, MergeVerbosity::Failure, message, message2);
}

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field when the list of children changed.
void printAboutChildren(
    const MergeContext&             ctx,
    const MergeLocation&            loc,
    const char*                     message,
    const std::vector<std::string>& childrenNames)
{
    if (!contains(MergeVerbosity::Children, ctx.options.verbosity))
        return;

    const std::string allNames = TfStringJoin(childrenNames);
    printAboutField(ctx, loc, MergeVerbosity::Children, message, allNames.c_str());
}

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field change status.
void printChangedField(
    const MergeContext&  ctx,
    const MergeLocation& loc,
    const char*          message,
    bool                 changed)
{
    printAboutField(
        ctx,
        loc,
        changed ? MergeVerbosity::Differ : MergeVerbosity::Same,
        message,
        changed ? ": changed. " : ": same. ");
}

//----------------------------------------------------------------------------------------------------------------------
/// Convert validity pair to a decriptive text.
const char* validitiesToText(bool srcValid, bool dstValid)
{
    if (srcValid && !dstValid)
        return ": created. ";

    if (!srcValid && dstValid)
        return ": removed. ";

    if (srcValid && dstValid)
        return ": all valid. ";

    return ": all invalid";
}

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field when the source or destination are invalid.
void printInvalidField(
    const MergeContext&  ctx,
    const MergeLocation& loc,
    const char*          message,
    bool                 srcValid,
    bool                 dstValid)
{
    printAboutField(
        ctx,
        loc,
        (srcValid != dstValid) ? MergeVerbosity::Differ : MergeVerbosity::Same,
        message,
        validitiesToText(srcValid, dstValid));
}

//----------------------------------------------------------------------------------------------------------------------
/// Creates a functor to a function taking the merge context as a prefix parameter.
template <class T> auto makeFuncWithContext(const MergeContext& ctx, T&& func)
{
    namespace ph = std::placeholders;
    return std::bind(
        func, ctx, ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, ph::_9);
}

//----------------------------------------------------------------------------------------------------------------------
// Special transform attributes handling.
//
// The goal is to detect that a prim transform has *not* changed even though its representation
// might have changed. For example it might use different attributes (RotateX vs RotateXYZ) or
// different transform operations order. (These are in the xformOpOrder attribute.)
//----------------------------------------------------------------------------------------------------------------------

bool isTransformProperty(const UsdProperty& prop)
{
    static const TfToken xformOpOrderToken("xformOpOrder");
    return prop.IsValid()
        && (UsdGeomXformOp::IsXformOp(prop.GetName()) || prop.GetBaseName() == xformOpOrderToken);
}

std::vector<UsdAttribute> getTransformAttributes(const UsdPrim& prim)
{
    UsdGeomXformable            xformable(prim);
    bool                        resetParentTrf = true;
    std::vector<UsdGeomXformOp> ops = xformable.GetOrderedXformOps(&resetParentTrf);

    std::vector<UsdAttribute> trfAttrs;
    for (const auto& op : ops)
        trfAttrs.emplace_back(op.GetAttr());

    return trfAttrs;
}

GfMatrix4d getLocalTransform(const UsdPrim prim, const UsdTimeCode& timeCode)
{
    GfMatrix4d primMtx(1);

    UsdGeomXformable xformable(prim);
    bool             resetParentTrf = true;
    if (!TF_VERIFY(xformable.GetLocalTransformation(&primMtx, &resetParentTrf, timeCode))) {
        return GfMatrix4d(1);
    }

    return primMtx;
}

bool isLocalTransformModified(
    const UsdPrim&     srcPrim,
    const UsdPrim&     dstPrim,
    const UsdTimeCode& timeCode)
{
    double epsilon = 1e-9;
    bool   similar = PXR_NS::GfIsClose(
        getLocalTransform(srcPrim, timeCode), getLocalTransform(dstPrim, timeCode), epsilon);
    return !similar;
}

bool isLocalTransformModified(const UsdPrim& srcPrim, const UsdPrim& dstPrim)
{
    // We will not compare the set of point-in-times themselves but the overall result
    // of the animated values. This takes care of trying to match time-samples: we
    // instead only care that the output result are the same.
    //
    // Note that the UsdAttribute API to get value automatically interpolates values
    // where samples are missing when queried.
    std::vector<double> times;
    {
        std::vector<UsdAttribute> trfAttrs = getTransformAttributes(srcPrim);
        std::vector<UsdAttribute> dstTrfAttrs = getTransformAttributes(dstPrim);
        trfAttrs.insert(trfAttrs.end(), dstTrfAttrs.begin(), dstTrfAttrs.end());

        // If retrieving the time codes somehow fails, be conservative and assume
        // the transform was modified.
        if (!UsdAttribute::GetUnionedTimeSamples(trfAttrs, &times))
            return true;
    }

    // If there are no time samples, then USD uses the default value.
    if (times.size() == 0)
        return isLocalTransformModified(srcPrim, dstPrim, UsdTimeCode::Default());

    for (const double time : times) {
        if (isLocalTransformModified(srcPrim, dstPrim, time)) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Special normal attributes handling.
//
// The goal is to detect that a prim normals has *not* changed even though its representation
// might have changed. The reason is that normals can be kept in the 'normals' attribute or in
// the 'primvars:normals' attribute, with the latter having priority. Maya export to USD always
// generates the 'normals' attribute, which would get hidden when merged. We now detect and correct
// that situation.
//----------------------------------------------------------------------------------------------------------------------

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    normalsTokens,

    (normals)
    ((pvNormals, "primvars:normals"))
    ((pvNormalsIndices, "primvars:normals:indices"))
);
// clang-format on

bool isNormalsProperty(const TfToken& name)
{
    return (name == normalsTokens->normals || name == normalsTokens->pvNormals);
}

bool isUndesiredNormalsProperty(const TfToken& name)
{
    return (name == normalsTokens->pvNormalsIndices || name == normalsTokens->pvNormals);
}

//----------------------------------------------------------------------------------------------------------------------
// Special metadata handling.
//
// There are some metadata that we know the merge-to-USD temporary export-to-USD
// will never produce. Others are generated only when needed, so their absence
// should always mean to remove them.
//
// isMetadataAlwaysPreserved() is for those metadata we know we never produce.
//
// isMetadataNeverPreserved() is for those metadata we know we sometimes omit
// to signify we want to remove them.
//----------------------------------------------------------------------------------------------------------------------

bool isMetadataAlwaysPreserved(const TfToken& metadata)
{
    static const std::set<TfToken> preserved = { TfToken("references") };

    return preserved.count(metadata) > 0;
}

bool isMetadataNeverPreserved(const TfToken& metadata)
{
    static const std::set<TfToken> never = {};

    return never.count(metadata) > 0;
}

bool isMetadataPreserved(const TfToken& metadata, MergeMissing missingHandling)
{
    if (contains(missingHandling, MergeMissing::Preserve))
        return !isMetadataNeverPreserved(metadata);
    else
        return isMetadataAlwaysPreserved(metadata);
}

//----------------------------------------------------------------------------------------------------------------------
// Merge Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Verifies if the metadata at the given path have been modified.
bool isMetadataAtPathModified(
    const MergeContext&  ctx,
    const MergeLocation& src,
    const MergeLocation& dst,
    const char* const    metadataType,
    const UsdObject&     modified,
    const UsdObject&     baseline,
    const MergeMissing   missingHandling)
{
    // Verify if both are missing. Should not happen, but let's be safe.
    // In that case, the metadata is not modified.
    if (!src.fieldExists && !dst.fieldExists) {
        const bool changed = false;
        printChangedField(ctx, src, metadataType, changed);
        return changed;
    }

    // If the metadata is missing in the source and we preserve missing,
    // return that the metadata is *not* modified so as to preserve it.
    if (!src.fieldExists) {
        const bool preserved = isMetadataPreserved(src.field, missingHandling);
        const bool changed = !preserved;
        printChangedField(ctx, src, metadataType, changed);
        return changed;
    }

    // If the metadata is missing in the destination and we create missing,
    // return that the metadata *is* modified so as to create it.
    if (!dst.fieldExists) {
        const bool created = contains(missingHandling, MergeMissing::Create);
        const bool changed = created;
        printChangedField(ctx, src, metadataType, changed);
        return changed;
    }

    // If both are present, we compare.
    //
    // Note: special references metadata handling.
    //
    // To support it fully, we would need:
    //
    //    - A generic SdfListOp<T> diff algorithm. That is because the references
    //      metadata is kept as a SdfListOp<SdfReference>.
    //
    //    - A custom copyValue callback, to be returned by the shouldMergeValue()
    //      function in the valueToCopy with custom code to merge a SdfListOp<T>
    //      into another.
    //
    // For the short term, we know that push-to-USD cannot generate references.
    // In other words, we will always go through the !src.fieldExists code above.
    // So we don't need to write the SdfListOp code yet.
    const bool changed = (compareMetadatas(modified, baseline, src.field) != DiffResult::Same);
    printChangedField(ctx, src, metadataType, changed);
    return changed;
}

//----------------------------------------------------------------------------------------------------------------------
/// Verifies if the data at the given path have been modified.
bool isDataAtPathsModified(
    const MergeContext&  ctx,
    const MergeLocation& src,
    const MergeLocation& dst)
{
    DiffResult quickDiff = DiffResult::Same;

    UsdPrim srcPrim
        = ctx.srcStage->GetPrimAtPath(src.path.GetPrimPath().StripAllVariantSelections());
    UsdPrim dstPrim
        = ctx.dstStage->GetPrimAtPath(dst.path.GetPrimPath().StripAllVariantSelections());
    if (!srcPrim.IsValid() || !dstPrim.IsValid()) {
        printInvalidField(ctx, src, "prim", srcPrim.IsValid(), dstPrim.IsValid());
        return srcPrim.IsValid() != dstPrim.IsValid();
    }

    if (src.path.ContainsPropertyElements()) {
        const UsdProperty srcProp = srcPrim.GetPropertyAtPath(src.path);

        // Note: we only return early for transform property if the local transform has
        //       not changed. The reason is that when the transform has changed, we *do*
        //       want to compare the individual properties in order to author only the
        //       minimum change.
        //
        //       The only reason we are using the local transformation comparison is for
        //       the somewhat common case where the transform has not changed but how it
        //       is encoded changed, because the Maya representation and the original
        //       representation differed, for example for USD data coming from another
        //       tool that use a different transform operation order.
        if (isTransformProperty(srcProp)) {
            const bool changed = isLocalTransformModified(srcPrim, dstPrim);
            if (!changed) {
                printChangedField(ctx, src, "transform prop local trf", changed);
                return changed;
            }
        }

        const UsdProperty dstProp = dstPrim.GetPropertyAtPath(dst.path);
        if (!srcProp.IsValid() || !dstProp.IsValid()) {
            printInvalidField(ctx, src, "prop", srcProp.IsValid(), dstProp.IsValid());
            return srcProp.IsValid() != dstProp.IsValid();
        }

        if (!src.field.IsEmpty()) {
            return isMetadataAtPathModified(
                ctx, src, dst, "prop metadata", srcProp, dstProp, ctx.options.propMetadataHandling);
        } else {
            if (srcProp.Is<UsdAttribute>()) {
                const UsdAttribute srcAttr = srcProp.As<UsdAttribute>();
                const UsdAttribute dstAttr = dstProp.As<UsdAttribute>();
                compareAttributes(srcAttr, dstAttr, &quickDiff);
                const bool changed = (quickDiff != DiffResult::Same);
                printChangedField(ctx, src, "attribute", changed);
                return changed;
            } else {
                const UsdRelationship srcRel = srcProp.As<UsdRelationship>();
                const UsdRelationship dstRel = dstProp.As<UsdRelationship>();
                compareRelationships(srcRel, dstRel, &quickDiff);
                const bool changed = (quickDiff != DiffResult::Same);
                printChangedField(ctx, src, "relationship", changed);
                return changed;
            }
        }
    } else {
        if (!src.field.IsEmpty()) {
            return isMetadataAtPathModified(
                ctx, src, dst, "prim metadata", srcPrim, dstPrim, ctx.options.propMetadataHandling);
        } else {
            comparePrimsOnly(srcPrim, dstPrim, &quickDiff);
            const bool changed = (quickDiff != DiffResult::Same);
            printChangedField(ctx, src, "prim", changed);
            return changed;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge a value.
bool shouldMergeValue(
    const MergeContext&       ctx,
    SdfSpecType               specType,
    const TfToken&            field,
    const SdfLayerHandle&     srcLayer,
    const SdfPath&            srcPath,
    bool                      fieldInSrc,
    const SdfLayerHandle&     dstLayer,
    const SdfPath&            dstPath,
    bool                      fieldInDst,
    boost::optional<VtValue>* valueToCopy)
{
    const bool isCopiable = SdfShouldCopyValue(
        ctx.srcRootPath,
        ctx.dstRootPath,
        specType,
        field,
        srcLayer,
        srcPath,
        fieldInSrc,
        dstLayer,
        dstPath,
        fieldInDst,
        valueToCopy);

    const MergeLocation src = { srcLayer, srcPath, field, fieldInSrc };
    if (!isCopiable) {
        printAboutFailure(ctx, src, "USD denies copying value. ");
        return false;
    }

    const MergeLocation dst = { dstLayer, dstPath, field, fieldInDst };
    return isDataAtPathsModified(ctx, src, dst);
}

//----------------------------------------------------------------------------------------------------------------------
/// Allow to do some custom filtering processing of children based on
/// the type of children being filtered. By default there is no custom
/// processing.
///
/// Because this is called by a templated function, it has to be templated
/// and the overrides is template-based with template specialization.

template <class ChildPolicy> class CustomChildrenFiltering
{
public:
    using FieldType = typename ChildPolicy::FieldType;
    using ChildrenVector = std::vector<FieldType>;

    /// Map some special fields to corresponding fields.
    const FieldType getCorrespondingChild(
        const MergeContext&,
        const MergeLocation&,
        const FieldType&,
        const FieldType& dstChild)
    {
        return dstChild;
    }

    /// Final post-filtering customizations.
    void postFiltering(
        const MergeContext&  ctx,
        const MergeMissing   missingHandling,
        const MergeLocation& src,
        const MergeLocation& dst,
        ChildrenVector&      srcFilteredChildren,
        ChildrenVector&      dstFilteredChildren)
    {
    }
};

//----------------------------------------------------------------------------------------------------------------------
/// Custom processing of properties.
///
/// Currently, only normals require special processing, but we wrote this
/// with the view that more will come and this call will become more sophisticated
/// as new cases are found.
///
/// we only map normals to primvars:normals since the latter has precedence.
/// We also remove some destination attributes for normals so that there is no
/// contradictory data left behind.
template <> class CustomChildrenFiltering<Sdf_PropertyChildPolicy>
{
public:
    using FieldType = TfToken;
    using ChildrenVector = std::vector<FieldType>;

    const TfToken getCorrespondingChild(
        const MergeContext&  ctx,
        const MergeLocation& dst,
        const TfToken&       srcChild,
        const TfToken&       dstChild)
    {
        if (isNormalsProperty(srcChild))
            srcHasNormals = true;

        return dstChild;
    }

    void postFiltering(
        const MergeContext&  ctx,
        const MergeMissing   missingHandling,
        const MergeLocation& src,
        const MergeLocation& dst,
        ChildrenVector&      srcChildren,
        ChildrenVector&      dstChildren)
    {
        if (!srcHasNormals)
            return;

        // Note: iterate from the end because we are going to remove items.
        for (size_t i = dstChildren.size() - 1; i < dstChildren.size(); --i) {
            if (!isUndesiredNormalsProperty(dstChildren[i]))
                continue;

            if (contains(ctx.options.verbosity, MergeVerbosity::Children)) {
                const MergeLocation childLoc = { dst.layer, dst.path, dstChildren[i], true };
                const char*         msg = "drop child to ensure correct normals transfer. ";
                printAboutField(ctx, childLoc, MergeVerbosity::Child, msg);
            }

            srcChildren.erase(srcChildren.begin() + i);
            dstChildren.erase(dstChildren.begin() + i);
        }
    }

private:
    bool srcHasNormals = false;
};

//----------------------------------------------------------------------------------------------------------------------
/// Filters the children based on their type.
template <class ChildPolicy>
bool filterTypedChildren(
    const MergeContext&  ctx,
    const MergeMissing   missingHandling,
    const MergeLocation& src,
    const MergeLocation& dst,
    VtValue&             srcChildrenValue,
    VtValue&             dstChildrenValue)
{
    typedef typename ChildPolicy::FieldType FieldType;
    typedef std::vector<FieldType>          ChildrenVector;

    if (!TF_VERIFY(srcChildrenValue.IsHolding<ChildrenVector>() || srcChildrenValue.IsEmpty())
        || !TF_VERIFY(dstChildrenValue.IsHolding<ChildrenVector>() || dstChildrenValue.IsEmpty())) {
        printAboutFailure(ctx, src, "invalid children vector. ");
        return true;
    }

    CustomChildrenFiltering<ChildPolicy> customFiltering;

    const ChildrenVector  emptyChildren;
    const ChildrenVector& srcChildren = srcChildrenValue.IsEmpty()
        ? emptyChildren
        : srcChildrenValue.UncheckedGet<ChildrenVector>();
    const ChildrenVector& dstChildren = dstChildrenValue.IsEmpty()
        ? emptyChildren
        : dstChildrenValue.UncheckedGet<ChildrenVector>();

    ChildrenVector           srcFilteredChildren;
    ChildrenVector           dstFilteredChildren;
    std::vector<std::string> childrenNames;

    srcFilteredChildren.reserve(srcChildren.size());
    dstFilteredChildren.reserve(dstChildren.size());

    for (size_t i = 0; i < srcChildren.size(); ++i) {
        const auto& srcChild = srcChildren[i];
        const auto  dstChild
            = customFiltering.getCorrespondingChild(ctx, dst, srcChild, dstChildren[i]);

        // Special handling when the source or destination are empty.
        if (srcChild.IsEmpty() || dstChild.IsEmpty()) {

            // Both empty: should not happen, but let's be safe.
            if (srcChild.IsEmpty() && dstChild.IsEmpty()) {
                printAboutFailure(ctx, src, "missing from both source and destination. ");
                continue;
            }

            // Check if we are  allowing missing attributes to be preserved.
            if (srcChild.IsEmpty() && !contains(missingHandling, MergeMissing::Preserve)) {
                printAboutFailure(ctx, dst, "not preserving child missing in source. ");
                continue;
            }

            // Check if we are not allowing creation of attributes.
            if (dstChild.IsEmpty() && !contains(missingHandling, MergeMissing::Create)) {
                printAboutFailure(ctx, src, "not creating child missing in destination. ");
                continue;
            }

            const SdfPath childPath = srcChild.IsEmpty()
                ? ChildPolicy::GetChildPath(dst.path, dstChild)
                : ChildPolicy::GetChildPath(src.path, srcChild);

            const MergeLocation childLoc
                = { srcChild.IsEmpty() ? dst.layer : src.layer, childPath, TfToken(), true };
            const char* msg
                = srcChild.IsEmpty() ? "preserving destination. " : "creating from source. ";
            printAboutField(ctx, childLoc, MergeVerbosity::Child, msg);

            if (contains(ctx.options.verbosity, MergeVerbosity::Children))
                childrenNames.emplace_back(childPath.GetName());

            srcFilteredChildren.emplace_back(srcChild);
            dstFilteredChildren.emplace_back(dstChild);
            continue;
        }

        const SdfPath srcChildPath = ChildPolicy::GetChildPath(src.path, srcChild);
        const SdfPath dstChildPath = ChildPolicy::GetChildPath(dst.path, dstChild);

        // Note: don't use location's field, since we're in a child path, the children field is
        // irrelevant. We will assume the child exists, but we will actually verify it just
        // below with a call to HasSpec().
        const MergeLocation childSrcLoc = { src.layer, srcChildPath, TfToken(), true };
        const MergeLocation childDstLoc = { dst.layer, dstChildPath, TfToken(), true };

        // Note: we cannot drop a children that already has an opinion at the
        // destination, otherwise SdfCopySpec() will delete that opinion!
        //
        // In other words, the list of children that we return is *not* merely
        // the list of children we want to copy over, but the final list of children
        // that will be in the destination when the copy is done.
        //
        // That is why we first check if the destination layer has a spec (opinion)
        // about the child.
        const char* childMessage = nullptr;
        if (dst.layer->HasSpec(dstChildPath)) {
            childMessage = "keep child. ";
            srcFilteredChildren.emplace_back(srcChild);
            dstFilteredChildren.emplace_back(dstChild);
            if (contains(ctx.options.verbosity, MergeVerbosity::Children))
                childrenNames.emplace_back(srcChildPath.GetName());
        } else {
            if (isDataAtPathsModified(ctx, childSrcLoc, childDstLoc)) {
                childMessage = "create child. ";
                srcFilteredChildren.emplace_back(srcChild);
                dstFilteredChildren.emplace_back(dstChild);
                if (contains(ctx.options.verbosity, MergeVerbosity::Children))
                    childrenNames.emplace_back(srcChildPath.GetName());
            } else {
                childMessage = "drop child, data identical from elsewhere. ";
            }
        }

        printAboutField(ctx, childSrcLoc, MergeVerbosity::Child, childMessage);
    }

    customFiltering.postFiltering(
        ctx, missingHandling, src, dst, srcFilteredChildren, dstFilteredChildren);

    const bool  shouldCopy = (srcFilteredChildren.size() > 0);
    const char* childrenMsg = nullptr;
    if (shouldCopy) {
        if (srcFilteredChildren.size() != srcChildren.size()) {
            childrenMsg = "subset of children: ";
        } else {
            childrenMsg = "keep all children: ";
        }

        // We copy the list even when the list size has not changed because
        // we sometimes edit the destination, see getCorrespondingChild().
        srcChildrenValue = srcFilteredChildren;
        dstChildrenValue = dstFilteredChildren;
    } else {
        childrenMsg = "no children: ";
    }
    printAboutChildren(ctx, src, childrenMsg, childrenNames);

    return shouldCopy;
}

//----------------------------------------------------------------------------------------------------------------------
/// Marge all children from both the source and destination.
template <class ChildPolicy>
void unionChildren(
    MergeMissing missingHandling,
    VtValue&     srcChildrenValue,
    VtValue&     dstChildrenValue)
{
    typedef typename ChildPolicy::FieldType FieldType;
    typedef std::vector<FieldType>          ChildrenVector;
    typedef std::set<FieldType>             ChildrenSet;

    if (!TF_VERIFY(srcChildrenValue.IsHolding<ChildrenVector>() || srcChildrenValue.IsEmpty()))
        return;

    if (!TF_VERIFY(dstChildrenValue.IsHolding<ChildrenVector>() || dstChildrenValue.IsEmpty()))
        return;

    // Extract children from source and destination.
    ChildrenVector srcChildren;
    if (!srcChildrenValue.IsEmpty())
        srcChildren = srcChildrenValue.UncheckedGet<ChildrenVector>();

    ChildrenVector dstChildren;
    if (!dstChildrenValue.IsEmpty())
        dstChildren = dstChildrenValue.UncheckedGet<ChildrenVector>();

    // Create sets for fast comparison and return if both sets are equal,
    // meaning the source and destination already have the same children.
    ChildrenSet srcSet(srcChildren.begin(), srcChildren.end());
    ChildrenSet dstSet(dstChildren.begin(), dstChildren.end());

    // Fill the source and destination to have the desired set
    // of children in the same order.
    ChildrenSet unionSet;
    if (contains(missingHandling, MergeMissing::Create))
        unionSet.insert(srcSet.begin(), srcSet.end());
    if (contains(missingHandling, MergeMissing::Preserve))
        unionSet.insert(dstSet.begin(), dstSet.end());

    srcChildren.clear();
    srcChildren.reserve(unionSet.size());

    dstChildren.clear();
    dstChildren.reserve(unionSet.size());

    for (const FieldType& child : unionSet) {
        // Note: source cannot be a field that does not exists. To preserve a destination
        // field, the source field must simply be invalid.
        srcChildren.emplace_back(srcSet.count(child) ? child : FieldType {});

        // For destrination field, we can put a field that does not exist in the destination.
        // It will be created.
        dstChildren.emplace_back(child);
    }

    srcChildrenValue = srcChildren;
    dstChildrenValue = dstChildren;
}

//----------------------------------------------------------------------------------------------------------------------
/// Filters the children.
bool filterChildren(
    const MergeContext&  ctx,
    const MergeLocation& src,
    const MergeLocation& dst,
    VtValue&             srcChildren,
    VtValue&             dstChildren)
{
    if (src.field == SdfChildrenKeys->ConnectionChildren) {
        const auto missingHandling = ctx.options.connectionsHandling;
        unionChildren<Sdf_AttributeConnectionChildPolicy>(
            missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_AttributeConnectionChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->MapperChildren) {
        const auto missingHandling = ctx.options.mappersHandling;
        unionChildren<Sdf_MapperChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_MapperChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->MapperArgChildren) {
        const auto missingHandling = ctx.options.mapperArgsHandling;
        unionChildren<Sdf_MapperArgChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_MapperArgChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->ExpressionChildren) {
        const auto missingHandling = ctx.options.expressionsHandling;
        unionChildren<Sdf_ExpressionChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_ExpressionChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->RelationshipTargetChildren) {
        const auto missingHandling = ctx.options.relationshipsHandling;
        unionChildren<Sdf_RelationshipTargetChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_RelationshipTargetChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->VariantChildren) {
        const auto missingHandling = ctx.options.variantsHandling;
        unionChildren<Sdf_VariantChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_VariantChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->VariantSetChildren) {
        const auto missingHandling = ctx.options.variantSetsHandling;
        unionChildren<Sdf_VariantSetChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_VariantSetChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->PropertyChildren) {
        const auto missingHandling = ctx.options.propertiesHandling;
        unionChildren<Sdf_PropertyChildPolicy>(missingHandling, srcChildren, dstChildren);
        return filterTypedChildren<Sdf_PropertyChildPolicy>(
            ctx, missingHandling, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->PrimChildren) {
        if (ctx.options.mergeChildren) {
            const auto missingHandling = ctx.options.primsHandling;
            unionChildren<Sdf_PrimChildPolicy>(missingHandling, srcChildren, dstChildren);
            return filterTypedChildren<Sdf_PrimChildPolicy>(
                ctx, missingHandling, src, dst, srcChildren, dstChildren);
        } else {
            return false;
        }
    }

    printAboutFailure(ctx, src, "unknown children field.");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge children.
bool shouldMergeChildren(
    const MergeContext&       ctx,
    const TfToken&            childrenField,
    const SdfLayerHandle&     srcLayer,
    const SdfPath&            srcPath,
    bool                      fieldInSrc,
    const SdfLayerHandle&     dstLayer,
    const SdfPath&            dstPath,
    bool                      fieldInDst,
    boost::optional<VtValue>* srcChildren,
    boost::optional<VtValue>* dstChildren)
{
    const bool shouldMerge = SdfShouldCopyChildren(
        ctx.srcRootPath,
        ctx.dstRootPath,
        childrenField,
        srcLayer,
        srcPath,
        fieldInSrc,
        dstLayer,
        dstPath,
        fieldInDst,
        srcChildren,
        dstChildren);

    const MergeLocation src = { srcLayer, srcPath, childrenField, fieldInSrc };

    if (!shouldMerge) {
        printAboutFailure(ctx, src, "USD denies copying children. ");
        return false;
    }

    // Protect against SdfShouldCopyChildren() not filling the children.
    if (!*srcChildren) {
        *srcChildren = srcLayer->GetField(srcPath, childrenField);
    }

    if (!*dstChildren) {
        *dstChildren = dstLayer->GetField(dstPath, childrenField);
    }

    if (!*srcChildren || !*dstChildren) {
        printAboutFailure(ctx, src, "no children to copy. ");
        return false;
    }

    const MergeLocation dst = { dstLayer, dstPath, childrenField, fieldInDst };
    return filterChildren(ctx, src, dst, **srcChildren, **dstChildren);
}

//----------------------------------------------------------------------------------------------------------------------
/// Copies a minimal prim using diff and merge, printing all fields that are copied to the Maya
/// console.
bool mergeDiffPrims(
    const MergePrimsOptions& options,
    const UsdStageRefPtr&    srcStage,
    const SdfLayerRefPtr&    srcLayer,
    const SdfPath&           srcPath,
    const UsdStageRefPtr&    dstStage,
    const SdfLayerRefPtr&    dstLayer,
    const SdfPath&           dstPath)
{
    const MergeContext ctx = { options, srcStage, srcPath, dstStage, dstPath };

    auto copyValue = makeFuncWithContext(ctx, shouldMergeValue);
    auto copyChildren = makeFuncWithContext(ctx, shouldMergeChildren);
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath, copyValue, copyChildren);
}

//----------------------------------------------------------------------------------------------------------------------
/// Create any missing parents as "over". Parents may be missing because we are targeting a
/// different layer than where the destination prim is authored. The SdfCopySpec function does not
/// automatically create the missing parent, unlike other functions like UsdStage::CreatePrim.
void createMissingParents(const SdfLayerRefPtr& dstLayer, const SdfPath& dstPath)
{
    SdfJustCreatePrimInLayer(dstLayer, dstPath.GetParentPath());
}

//----------------------------------------------------------------------------------------------------------------------
// Augment a USD SdfPath with the variants selections currently active at all levels.
std::pair<SdfPath, UsdEditTarget> augmentPathWithVariants(
    const UsdStageRefPtr& stage,
    const SdfLayerRefPtr& layer,
    const SdfPath&        path)
{
    SdfPath       pathWithVariants;
    UsdEditTarget target = stage->GetEditTarget();

    SdfPathVector rootToLeaf;
    path.GetPrefixes(&rootToLeaf);
    for (const SdfPath& prefix : rootToLeaf) {
        if (pathWithVariants.IsEmpty()) {
            pathWithVariants = prefix;
        } else {
            pathWithVariants = pathWithVariants.AppendChild(prefix.GetNameToken());
        }

        const UsdPrim prim = stage->GetPrimAtPath(prefix.StripAllVariantSelections());
        if (prim.IsValid()) {
            UsdVariantSets variants = prim.GetVariantSets();
            for (const std::string& setName : variants.GetNames()) {
                UsdVariantSet varSet = variants.GetVariantSet(setName);
                pathWithVariants = pathWithVariants.AppendVariantSelection(
                    varSet.GetName(), varSet.GetVariantSelection());
            }
        }
    }

    // Note: any trailing variant selection must be used to create a UsdEditContext,
    //       it must not be part of the destination path, otherwise SdfCopySpec()
    //       will fail: it does not handle destination paths ending with a variant
    //       selection correctly.
    //
    //       If SdfCopySpec() is called with a path ending with a variant selection,
    //       it assumes that it is inside its own iterations, about to add the
    //       selection name even though what is about to be added is a prim. So
    //       The GetParentPath() called in CreateSpec() in usd/sdf/childrenUtils.cpp
    //       around line 108 will strip the variant selection instead of stripping
    //       the prim (including variant selection). That will end-up causing an
    //       error when trying to create a field in SdfData::_GetOrCreateFieldValue
    //       in usd\sdf\data.cpp around line 260.
    if (pathWithVariants.IsPrimVariantSelectionPath()) {
        target = target.ForLocalDirectVariant(layer, pathWithVariants);
        pathWithVariants = pathWithVariants.GetPrimPath();
    }

    return std::make_pair(pathWithVariants, target);
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
// Entrypoint of Merge
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// merges prims starting at a source path from a source layer and stage to a destination.
bool mergePrims(
    const UsdStageRefPtr&    srcStage,
    const SdfLayerRefPtr&    srcLayer,
    const SdfPath&           srcPath,
    const UsdStageRefPtr&    dstStage,
    const SdfLayerRefPtr&    dstLayer,
    const SdfPath&           dstPath,
    const MergePrimsOptions& options)
{
    SdfPath       augmentedDstPath = dstPath;
    UsdEditTarget target = dstStage->GetEditTarget();

    if (!options.ignoreVariants) {
        const auto pathAndTarget = augmentPathWithVariants(dstStage, dstLayer, dstPath);
        augmentedDstPath = pathAndTarget.first;
        target = pathAndTarget.second;
    }

    UsdEditContext editCtx(dstStage, target);

    createMissingParents(dstLayer, augmentedDstPath);

    if (options.ignoreUpperLayerOpinions) {
        auto           tempStage = UsdStage::CreateInMemory();
        SdfLayerHandle tempLayer = tempStage->GetSessionLayer();

        tempLayer->TransferContent(dstLayer);

        const bool success = mergeDiffPrims(
            options, srcStage, srcLayer, srcPath, tempStage, tempLayer, augmentedDstPath);

        if (success)
            dstLayer->TransferContent(tempLayer);

        return success;
    } else {
        return mergeDiffPrims(
            options, srcStage, srcLayer, srcPath, dstStage, dstLayer, augmentedDstPath);
    }
}

} // namespace MayaUsdUtils
