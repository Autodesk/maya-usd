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
/// Verbosity level flags.

enum class Verbosity
{
    None = 0,
    Same = 1 << 0,
    Differ = 1 << 1,
    Child = 1 << 2,
    Children = 1 << 3,
    Failure = 1 << 4,
};

Verbosity operator|(Verbosity a, Verbosity b) { return Verbosity(uint32_t(a) | uint32_t(b)); }
Verbosity operator&(Verbosity a, Verbosity b) { return Verbosity(uint32_t(a) & uint32_t(b)); }
Verbosity operator^(Verbosity a, Verbosity b) { return Verbosity(uint32_t(a) ^ uint32_t(b)); }

//----------------------------------------------------------------------------------------------------------------------
// Data used for merging passed to all helper functions.
struct MergeContext
{
    const Verbosity       verbosity;
    const UsdStageRefPtr& srcStage;
    const SdfPath&        srcRootPath;
    const UsdStageRefPtr& dstStage;
    const SdfPath&        dstRootPath;
};

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field to the Maya console with some messages.
void printAboutField(
    const MergeContext&   ctx,
    Verbosity             printVerbosity,
    const SdfLayerHandle& layer,
    const SdfPath&        path,
    const TfToken&        field,
    const char*           message,
    const char*           message2 = nullptr)
{
    if ((printVerbosity & ctx.verbosity) == Verbosity::None)
        return;

    TF_STATUS(
        "Layer [%s] / Path [%s] / Field [%s]: %s%s",
        layer->GetDisplayName().c_str(),
        path.GetText(),
        field.GetText(),
        message ? message : "",
        message2 ? message2 : "");
}

//----------------------------------------------------------------------------------------------------------------------
/// Print a layer / path / field when a rare failure occurs.
void printAboutFailure(
    const MergeContext&   ctx,
    const SdfLayerHandle& layer,
    const SdfPath&        path,
    const TfToken&        field,
    const char*           message,
    const char*           message2 = nullptr)
{
    printAboutField(ctx, Verbosity::Failure, layer, path, field, message, message2);
}

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path / field to the Maya console with some messages.
void printAboutChildren(
    const MergeContext&             ctx,
    const SdfLayerHandle&           layer,
    const SdfPath&                  path,
    const TfToken&                  field,
    const char*                     message,
    const std::vector<std::string>& childrenNames)
{
    if ((Verbosity::Children & ctx.verbosity) == Verbosity::None)
        return;

    const std::string allNames = TfStringJoin(childrenNames);
    printAboutField(ctx, Verbosity::Children, layer, path, field, message, allNames.c_str());
}

/// Prints a layer / path / field change status.
void printChangedField(
    const MergeContext&   ctx,
    const SdfLayerHandle& layer,
    const SdfPath&        path,
    const TfToken&        field,
    const char*           message,
    bool                  changed)
{
    printAboutField(
        ctx,
        changed ? Verbosity::Differ : Verbosity::Same,
        layer,
        path,
        field,
        message,
        changed ? ": changed. " : ": same. ");
}

//----------------------------------------------------------------------------------------------------------------------
/// Creates a functor to a function taking the source and destination root paths as two extra prefix
/// parameters.
template <class T>
auto makeFuncWithRoots(const SdfPath& srcRootPath, const SdfPath& dstRootPath, T&& func)
{
    namespace ph = std::placeholders;
    return std::bind(
        func,
        std::cref(srcRootPath),
        std::cref(dstRootPath),
        ph::_1,
        ph::_2,
        ph::_3,
        ph::_4,
        ph::_5,
        ph::_6,
        ph::_7,
        ph::_8,
        ph::_9);
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
    const MergeContext&   ctx,
    const SdfLayerRefPtr& srcLayer,
    const SdfPath&        srcPath,
    const SdfLayerRefPtr& dstLayer,
    const SdfPath&        dstPath,
    const TfToken&        field = TfToken())
{
    DiffResult quickDiff = DiffResult::Same;

    UsdPrim srcPrim = ctx.srcStage->GetPrimAtPath(srcPath.GetPrimPath());
    UsdPrim dstPrim = ctx.dstStage->GetPrimAtPath(dstPath.GetPrimPath());
    if (!srcPrim.IsValid() || !dstPrim.IsValid()) {
        const bool changed = (srcPrim.IsValid() != dstPrim.IsValid());
        printChangedField(ctx, srcLayer, srcPath, field, "invalid prim", changed);
        return changed;
    }

    if (srcPath.ContainsPropertyElements()) {
        const UsdProperty srcProp = srcPrim.GetPropertyAtPath(srcPath);
        const UsdProperty dstProp = dstPrim.GetPropertyAtPath(dstPath);
        if (!srcProp.IsValid() || !dstProp.IsValid()) {
            const bool changed = (srcProp.IsValid() != dstProp.IsValid());
            printChangedField(ctx, srcLayer, srcPath, field, "invalid prop", changed);
            return changed;
        }

        if (!field.IsEmpty()) {
            const bool changed = (compareMetadatas(srcProp, dstProp, field) != DiffResult::Same);
            printChangedField(ctx, srcLayer, srcPath, field, "prop metadata", changed);
            return changed;
        } else {
            // TODO: should we detect more than properties?

            if (srcProp.Is<UsdAttribute>()) {
                const UsdAttribute srcAttr = srcProp.As<UsdAttribute>();
                const UsdAttribute dstAttr = dstProp.As<UsdAttribute>();
                compareAttributes(srcAttr, dstAttr, &quickDiff);
                const bool changed = (quickDiff != DiffResult::Same);
                printChangedField(ctx, srcLayer, srcPath, field, "attribute", changed);
                return changed;
            } else {
                const UsdRelationship   srcRel = srcProp.As<UsdRelationship>();
                const UsdRelationship   dstRel = dstProp.As<UsdRelationship>();
                compareRelationships(srcRel, dstRel, &quickDiff);
                const bool changed = (quickDiff != DiffResult::Same);
                printChangedField(ctx, srcLayer, srcPath, field, "relationship", changed);
                return changed;
            }
        }
    } else {
        if (!field.IsEmpty()) {
            const bool changed = (compareMetadatas(srcPrim, dstPrim, field) != DiffResult::Same);
            printChangedField(ctx, srcLayer, srcPath, field, "prim metadata", changed);
            return changed;
        } else {
            comparePrims(srcPrim, dstPrim, &quickDiff);
            const bool changed = (quickDiff != DiffResult::Same);
            printChangedField(ctx, srcLayer, srcPath, field, "prim", changed);
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

    if (!isCopiable) {
        printAboutFailure(ctx, srcLayer, srcPath, field, "USD denies copying value. ");
        return false;
    }

    // TODO: if modified, return a custom data copy method in a SdfCopySpecsValueEdit.
    return isDataAtPathsModified(ctx, srcLayer, srcPath, dstLayer, dstPath, field);
}

template <class ChildPolicy>
bool filterTypedChildren(
    const MergeContext&   ctx,
    const TfToken&        childrenField,
    const SdfLayerHandle& srcLayer,
    const SdfPath&        srcPath,
    bool                  fieldInSrc,
    const SdfLayerHandle& dstLayer,
    const SdfPath&        dstPath,
    bool                  fieldInDst,
    VtValue&              srcChildrenValue,
    VtValue&              dstChildrenValue)
{
    typedef typename ChildPolicy::FieldType FieldType;
    typedef std::vector<FieldType>          ChildrenVector;

    if (!TF_VERIFY(srcChildrenValue.IsHolding<ChildrenVector>() || srcChildrenValue.IsEmpty())
        || !TF_VERIFY(dstChildrenValue.IsHolding<ChildrenVector>() || dstChildrenValue.IsEmpty())) {
        printAboutFailure(ctx, srcLayer, srcPath, childrenField, "invalid children vector. ");
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
    childrenNames.reserve(srcChildren.size() + 2);
    childrenNames.emplace_back("[");

    for (size_t i = 0; i < srcChildren.size(); ++i) {
        if (srcChildren[i].IsEmpty() || dstChildren[i].IsEmpty()) {
            printAboutFailure(ctx, srcLayer, srcPath, childrenField, "empty child. ");
            continue;
        }

        const SdfPath srcChildPath = ChildPolicy::GetChildPath(srcPath, srcChildren[i]);
        const SdfPath dstChildPath = ChildPolicy::GetChildPath(dstPath, dstChildren[i]);

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
        if (dstLayer->HasSpec(dstChildPath)) {
            childMessage = "keep child. ";
            srcFilteredChildren.emplace_back(srcChildren[i]);
            dstFilteredChildren.emplace_back(dstChildren[i]);
            childrenNames.emplace_back(srcChildPath.GetName());
        } else {
            if (isDataAtPathsModified(ctx, srcLayer, srcChildPath, dstLayer, dstChildPath)) {
                childMessage = "create child. ";
                srcFilteredChildren.emplace_back(srcChildren[i]);
                dstFilteredChildren.emplace_back(dstChildren[i]);
                childrenNames.emplace_back(srcChildPath.GetName());
            } else {
                childMessage = "drop child. ";
            }
        }
        printAboutField(ctx, Verbosity::Child, srcLayer, srcChildPath, childrenField, childMessage);
    }
    childrenNames.emplace_back("]");

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
    printAboutChildren(ctx, srcLayer, srcPath, childrenField, childrenMsg, childrenNames);

    return shouldCopy;
}

//----------------------------------------------------------------------------------------------------------------------
/// Filters the children.
bool filterChildren(
    const MergeContext&   ctx,
    const TfToken&        childrenField,
    const SdfLayerHandle& srcLayer,
    const SdfPath&        srcPath,
    bool                  fieldInSrc,
    const SdfLayerHandle& dstLayer,
    const SdfPath&        dstPath,
    bool                  fieldInDst,
    VtValue&              srcChildren,
    VtValue&              dstChildren)
{
    if (childrenField == SdfChildrenKeys->ConnectionChildren) {
        return filterTypedChildren<Sdf_AttributeConnectionChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->MapperChildren) {
        return filterTypedChildren<Sdf_MapperChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->MapperArgChildren) {
        return filterTypedChildren<Sdf_MapperArgChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->ExpressionChildren) {
        return filterTypedChildren<Sdf_ExpressionChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->RelationshipTargetChildren) {
        return filterTypedChildren<Sdf_RelationshipTargetChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->VariantChildren) {
        return filterTypedChildren<Sdf_VariantChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->VariantSetChildren) {
        return filterTypedChildren<Sdf_VariantSetChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->PropertyChildren) {
        return filterTypedChildren<Sdf_PropertyChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }
    if (childrenField == SdfChildrenKeys->PrimChildren) {
        return filterTypedChildren<Sdf_PrimChildPolicy>(
            ctx,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren);
    }

    printAboutFailure(ctx, srcLayer, srcPath, childrenField, "unknown children field.");
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

    if (!shouldMerge) {
        printAboutFailure(ctx, srcLayer, srcPath, childrenField, "USD denies copying children. ");
        return false;
    }

    // Protect against SdfShouldCopyChildren() not filling the children.
    if (!*srcChildren || !*dstChildren) {
        *srcChildren = srcLayer->GetField(srcPath, childrenField);
        *dstChildren = *srcChildren;

        if (!*srcChildren || !*dstChildren) {
            printAboutFailure(ctx, srcLayer, srcPath, childrenField, "no children to copy. ");
        }
    }

    return filterChildren(
        ctx,
        childrenField,
        srcLayer,
        srcPath,
        fieldInSrc,
        dstLayer,
        dstPath,
        fieldInDst,
        **srcChildren,
        **dstChildren);
}

//----------------------------------------------------------------------------------------------------------------------
/// Copies a minimal prim using diff and merge, printing all fields that are copied to the Maya
/// console.
bool mergeDiffPrims(
    const UsdStageRefPtr& srcStage,
    const SdfLayerRefPtr& srcLayer,
    const SdfPath&        srcPath,
    const UsdStageRefPtr& dstStage,
    const SdfLayerRefPtr& dstLayer,
    const SdfPath&        dstPath)
{
    MergeContext ctx = { Verbosity::Differ | Verbosity::Children | Verbosity::Failure,
                         srcStage,
                         srcPath,
                         dstStage,
                         dstPath };
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
    const SdfPath&        srcRootPath,
    const UsdStageRefPtr& dstStage,
    const SdfLayerRefPtr& dstLayer,
    const SdfPath&        dstRootPath)
{
    return mergeDiffPrims(srcStage, srcLayer, srcRootPath, dstStage, dstLayer, dstRootPath);
}

} // namespace MayaUsdUtils
