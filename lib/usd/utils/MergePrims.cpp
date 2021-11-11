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
    const MergeVerbosity  verbosity;
    const bool            mergeChildren;
    const UsdStageRefPtr& srcStage;
    const SdfPath&        srcRootPath;
    const UsdStageRefPtr& dstStage;
    const SdfPath&        dstRootPath;
};

//----------------------------------------------------------------------------------------------------------------------
/// Description of a merge location: layer, path and if the field already exists at that location.
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
    if (!contains(printVerbosity, ctx.verbosity))
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
    if (!contains(MergeVerbosity::Children, ctx.verbosity))
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
// Merge Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Verifies if the data at the given path have been modified.
bool isDataAtPathsModified(
    const MergeContext&  ctx,
    const MergeLocation& src,
    const MergeLocation& dst)
{
    DiffResult quickDiff = DiffResult::Same;

    UsdPrim srcPrim = ctx.srcStage->GetPrimAtPath(src.path.GetPrimPath());
    UsdPrim dstPrim = ctx.dstStage->GetPrimAtPath(dst.path.GetPrimPath());
    if (!srcPrim.IsValid() || !dstPrim.IsValid()) {
        printInvalidField(ctx, src, "prim", srcPrim.IsValid(), dstPrim.IsValid());
        return srcPrim.IsValid() != dstPrim.IsValid();
    }

    if (src.path.ContainsPropertyElements()) {
        const UsdProperty srcProp = srcPrim.GetPropertyAtPath(src.path);
        const UsdProperty dstProp = dstPrim.GetPropertyAtPath(dst.path);
        if (!srcProp.IsValid() || !dstProp.IsValid()) {
            printInvalidField(ctx, src, "prop", srcProp.IsValid(), dstProp.IsValid());
            return srcProp.IsValid() != dstProp.IsValid();
        }

        if (!src.field.IsEmpty()) {
            const bool changed
                = (compareMetadatas(srcProp, dstProp, src.field) != DiffResult::Same);
            printChangedField(ctx, src, "prop metadata", changed);
            return changed;
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
            const bool changed
                = (compareMetadatas(srcPrim, dstPrim, src.field) != DiffResult::Same);
            printChangedField(ctx, src, "prim metadata", changed);
            return changed;
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

template <class ChildPolicy>
bool filterTypedChildren(
    const MergeContext&  ctx,
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
        if (srcChildren[i].IsEmpty() || dstChildren[i].IsEmpty()) {
            printAboutFailure(ctx, src, "empty child. ");
            continue;
        }

        const SdfPath srcChildPath = ChildPolicy::GetChildPath(src.path, srcChildren[i]);
        const SdfPath dstChildPath = ChildPolicy::GetChildPath(dst.path, dstChildren[i]);

        // Note: don't use location's field, since we're in a child path, the children field is
        // irrelevant. We will assume the child exists, but we will actually verify it just
        // below with a call to HasSpec().
        const MergeLocation childSrc = { src.layer, srcChildPath, TfToken(), true };
        const MergeLocation childDst = { dst.layer, dstChildPath, TfToken(), true };

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
            srcFilteredChildren.emplace_back(srcChildren[i]);
            dstFilteredChildren.emplace_back(dstChildren[i]);
            if (contains(ctx.verbosity, MergeVerbosity::Children))
                childrenNames.emplace_back(srcChildPath.GetName());
        } else {
            if (isDataAtPathsModified(ctx, childSrc, childDst)) {
                childMessage = "create child. ";
                srcFilteredChildren.emplace_back(srcChildren[i]);
                dstFilteredChildren.emplace_back(dstChildren[i]);
                if (contains(ctx.verbosity, MergeVerbosity::Children))
                    childrenNames.emplace_back(srcChildPath.GetName());
            } else {
                childMessage = "drop child. ";
            }
        }

        printAboutField(ctx, childSrc, MergeVerbosity::Child, childMessage);
    }

    const bool  shouldCopy = (srcFilteredChildren.size() > 0);
    const char* childrenMsg = nullptr;
    if (shouldCopy) {
        if (srcFilteredChildren.size() != srcChildren.size()) {
            childrenMsg = "subset of children: ";
            srcChildrenValue = srcFilteredChildren;
            dstChildrenValue = dstFilteredChildren;
        } else {
            childrenMsg = "keep all children: ";
        }
    } else {
        childrenMsg = "no children: ";
    }
    printAboutChildren(ctx, src, childrenMsg, childrenNames);

    return shouldCopy;
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
        return filterTypedChildren<Sdf_AttributeConnectionChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->MapperChildren) {
        return filterTypedChildren<Sdf_MapperChildPolicy>(ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->MapperArgChildren) {
        return filterTypedChildren<Sdf_MapperArgChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->ExpressionChildren) {
        return filterTypedChildren<Sdf_ExpressionChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->RelationshipTargetChildren) {
        return filterTypedChildren<Sdf_RelationshipTargetChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->VariantChildren) {
        return filterTypedChildren<Sdf_VariantChildPolicy>(ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->VariantSetChildren) {
        return filterTypedChildren<Sdf_VariantSetChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->PropertyChildren) {
        return filterTypedChildren<Sdf_PropertyChildPolicy>(
            ctx, src, dst, srcChildren, dstChildren);
    }
    if (src.field == SdfChildrenKeys->PrimChildren) {
        if (ctx.mergeChildren)
            return filterTypedChildren<Sdf_PrimChildPolicy>(
                ctx, src, dst, srcChildren, dstChildren);
        else
            return false;
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
    if (!*srcChildren || !*dstChildren) {
        *srcChildren = srcLayer->GetField(srcPath, childrenField);
        *dstChildren = *srcChildren;

        if (!*srcChildren || !*dstChildren) {
            printAboutFailure(ctx, src, "no children to copy. ");
            return false;
        }
    }

    const MergeLocation dst = { dstLayer, dstPath, childrenField, fieldInDst };
    return filterChildren(ctx, src, dst, **srcChildren, **dstChildren);
}

//----------------------------------------------------------------------------------------------------------------------
/// Copies a minimal prim using diff and merge, printing all fields that are copied to the Maya
/// console.
bool mergeDiffPrims(
    MergeVerbosity        verbosity,
    bool                  mergeChildren,
    const UsdStageRefPtr& srcStage,
    const SdfLayerRefPtr& srcLayer,
    const SdfPath&        srcPath,
    const UsdStageRefPtr& dstStage,
    const SdfLayerRefPtr& dstLayer,
    const SdfPath&        dstPath)
{
    MergeContext ctx = { verbosity, mergeChildren, srcStage, srcPath, dstStage, dstPath };
    auto         copyValue = makeFuncWithContext(ctx, shouldMergeValue);
    auto         copyChildren = makeFuncWithContext(ctx, shouldMergeChildren);
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath, copyValue, copyChildren);
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
// Entrypoint of Merge
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// merges prims starting at a source path from a source layer and stage to a destination.
bool mergePrims(
    const UsdStageRefPtr& srcStage,
    const SdfLayerRefPtr& srcLayer,
    const SdfPath&        srcPath,
    const UsdStageRefPtr& dstStage,
    const SdfLayerRefPtr& dstLayer,
    const SdfPath&        dstPath,
    bool                  mergeChildren,
    MergeVerbosity        verbosity)
{
    // TODO: only create and transfer layer if: not the top layer (easy) or higher layers have an
    // opinion (might be tricky).
    const bool useTempStage = true;

    if (useTempStage) {
        auto           tempStage = UsdStage::CreateInMemory();
        SdfLayerHandle tempLayer = tempStage->GetSessionLayer();

        tempLayer->TransferContent(dstLayer);

        const bool success = mergeDiffPrims(
            verbosity, mergeChildren, srcStage, srcLayer, srcPath, tempStage, tempLayer, dstPath);

        if (success)
            dstLayer->TransferContent(tempLayer);

        return success;
    } else {
        return mergeDiffPrims(
            verbosity, mergeChildren, srcStage, srcLayer, srcPath, dstStage, dstLayer, dstPath);
    }
}

} // namespace MayaUsdUtils
