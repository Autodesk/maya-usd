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

#include <pxr/usd/sdf/copyUtils.h>

#include <utility>

namespace MayaUsdUtils {

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

//----------------------------------------------------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Prints a layer / path/ field to the Maya console if the field exists in the prim.
void printAboutField(
    const SdfLayerHandle& layer,
    const SdfPath&        path,
    const TfToken&        field,
    const char*           message)
{
    TF_STATUS(
        "Layer [%s] / Path [%s] / Field [%s]: %s",
        layer->GetDisplayName().c_str(),
        path.GetText(),
        field.GetText(),
        message ? message : "");
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
/// Creates a functor to a function taking the source and destination root paths as two extra prefix
/// parameters.
template <class T>
auto makeFuncWithStagesAndRoots(
    UsdStageRefPtr srcStage,
    const SdfPath& srcRootPath,
    UsdStageRefPtr dstStage,
    const SdfPath& dstRootPath,
    T&&            func)
{
    namespace ph = std::placeholders;
    return std::bind(
        func,
        srcStage,
        std::cref(srcRootPath),
        dstStage,
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
// Copy Full Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Copies a prim fully without diff nor merge.
bool copyEntirePrims(
    const SdfLayerRefPtr srcLayer,
    const SdfPath&       srcPath,
    SdfLayerRefPtr       dstLayer,
    const SdfPath&       dstPath)
{
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath);
}

//----------------------------------------------------------------------------------------------------------------------
// Copy and Print Full Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Copies a value while printing its location to the Maya console.
bool copyAndPrintValue(
    const SdfPath&            srcRootPath,
    const SdfPath&            dstRootPath,
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
    if (fieldInSrc)
        printAboutField(srcLayer, srcPath, field, "modified value. ");

    return SdfShouldCopyValue(
        srcRootPath,
        dstRootPath,
        specType,
        field,
        srcLayer,
        srcPath,
        fieldInSrc,
        dstLayer,
        dstPath,
        fieldInDst,
        valueToCopy);
}

//----------------------------------------------------------------------------------------------------------------------
/// Copies a prim fully without diff nor merge, printing all fields that are copied to the Maya
/// console.
bool copyAndPrintPrims(
    const SdfLayerRefPtr srcLayer,
    const SdfPath&       srcPath,
    SdfLayerRefPtr       dstLayer,
    const SdfPath&       dstPath)
{
    auto copyValue = makeFuncWithRoots(srcPath, dstPath, copyAndPrintValue);
    auto copyChildren = makeFuncWithRoots(srcPath, dstPath, SdfShouldCopyChildren);
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath, copyValue, copyChildren);
}

//----------------------------------------------------------------------------------------------------------------------
// Merge Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Verifies if the data at the given path have been modified.
bool isDataAtPathsModified(
    UsdStageRefPtr srcStage,
    const SdfPath& srcPath,
    UsdStageRefPtr dstStage,
    const SdfPath& dstPath)
{
    UsdPrim srcPrim = srcStage->GetPrimAtPath(srcPath.GetPrimPath());
    UsdPrim dstPrim = dstStage->GetPrimAtPath(dstPath.GetPrimPath());
    if (!srcPrim.IsValid() || !dstPrim.IsValid())
        return srcPrim.IsValid() != dstPrim.IsValid();

    if (srcPath.IsPrimPropertyPath()) {
        const UsdProperty srcProp = srcPrim.GetPropertyAtPath(srcPath);
        const UsdProperty dstProp = dstPrim.GetPropertyAtPath(dstPath);
        if (!srcProp.IsValid() || !dstProp.IsValid())
            return srcProp.IsValid() != dstProp.IsValid();

        if (srcProp.Is<UsdAttribute>()) {
            const UsdAttribute srcAttr = srcProp.As<UsdAttribute>();
            const UsdAttribute dstAttr = dstProp.As<UsdAttribute>();
            return compareAttributes(srcAttr, dstAttr) != DiffResult::Same;
        } else {
            const UsdRelationship   srcRel = srcProp.As<UsdRelationship>();
            const UsdRelationship   dstRel = dstProp.As<UsdRelationship>();
            const DiffResultPerPath relDiffs = compareRelationships(srcRel, dstRel);
            return computeOverallResult(relDiffs) != DiffResult::Same;
        }
    } else {
        return comparePrims(srcPrim, dstPrim) != DiffResult::Same;
    }

    // If we can't identify the data, we assume it change. TODO: correct?
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge a value.
bool shouldMergeValue(
    UsdStageRefPtr            srcStage,
    const SdfPath&            srcRootPath,
    UsdStageRefPtr            dstStage,
    const SdfPath&            dstRootPath,
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
        srcRootPath,
        dstRootPath,
        specType,
        field,
        srcLayer,
        srcPath,
        fieldInSrc,
        dstLayer,
        dstPath,
        fieldInDst,
        valueToCopy);

    if (!isCopiable)
        return false;

    // TODO: if modified, return a custom data copy method in a SdfCopySpecsValueEdit.
    return isDataAtPathsModified(srcStage, srcPath, dstStage, dstPath);
}

template <class ChildPolicy>
bool filterTypedChildren(
    UsdStageRefPtr        srcStage,
    const SdfPath&        srcRootPath,
    UsdStageRefPtr        dstStage,
    const SdfPath&        dstRootPath,
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
        return true;
    }

    const ChildrenVector  emptyChildren;
    const ChildrenVector& srcChildren = srcChildrenValue.IsEmpty()
        ? emptyChildren
        : srcChildrenValue.UncheckedGet<ChildrenVector>();
    const ChildrenVector& dstChildren = dstChildrenValue.IsEmpty()
        ? emptyChildren
        : dstChildrenValue.UncheckedGet<ChildrenVector>();

    ChildrenVector srcFilteredChildren;
    ChildrenVector dstFilteredChildren;

    srcFilteredChildren.reserve(srcChildren.size());
    dstFilteredChildren.reserve(dstChildren.size());

    for (size_t i = 0; i < srcChildren.size(); ++i) {
        if (srcChildren[i].IsEmpty() || dstChildren[i].IsEmpty()) {
            continue;
        }

        const SdfPath srcChildPath = ChildPolicy::GetChildPath(srcPath, srcChildren[i]);
        const SdfPath dstChildPath = ChildPolicy::GetChildPath(dstPath, dstChildren[i]);

        if (isDataAtPathsModified(srcStage, srcChildPath, dstStage, dstChildPath)) {
            srcFilteredChildren.emplace_back(srcChildren[i]);
            dstFilteredChildren.emplace_back(dstChildren[i]);
        }
    }

    srcChildrenValue = srcFilteredChildren;
    dstChildrenValue = dstFilteredChildren;

    return srcFilteredChildren.size() > 0;
}

//----------------------------------------------------------------------------------------------------------------------
/// Filters the children.
bool filterChildren(
    UsdStageRefPtr        srcStage,
    const SdfPath&        srcRootPath,
    UsdStageRefPtr        dstStage,
    const SdfPath&        dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
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

    TF_CODING_ERROR("Unknown child field '%s'", childrenField.GetText());
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge children.
bool shouldMergeChildren(
    UsdStageRefPtr            srcStage,
    const SdfPath&            srcRootPath,
    UsdStageRefPtr            dstStage,
    const SdfPath&            dstRootPath,
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
    if (!SdfShouldCopyChildren(
            srcRootPath,
            dstRootPath,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren))
        return false;

    *srcChildren = srcLayer->GetField(srcPath, childrenField);
    *dstChildren = *srcChildren;

    if (!*srcChildren || !*dstChildren)
        return false;

    return filterChildren(
        srcStage,
        srcRootPath,
        dstStage,
        dstRootPath,
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
/// Copies a minimal prim using diff and merge.
bool mergeDiffPrims(
    UsdStageRefPtr       srcStage,
    const SdfLayerRefPtr srcLayer,
    const SdfPath&       srcPath,
    UsdStageRefPtr       dstStage,
    SdfLayerRefPtr       dstLayer,
    const SdfPath&       dstPath)
{
    auto copyValue
        = makeFuncWithStagesAndRoots(srcStage, srcPath, dstStage, dstPath, shouldMergeValue);
    auto copyChildren
        = makeFuncWithStagesAndRoots(srcStage, srcPath, dstStage, dstPath, shouldMergeChildren);
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath, copyValue, copyChildren);
}

//----------------------------------------------------------------------------------------------------------------------
// Merge And Print Prims
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge a value while printing its location to the Maya console.
bool mergeAndPrintValue(
    UsdStageRefPtr            srcStage,
    const SdfPath&            srcRootPath,
    UsdStageRefPtr            dstStage,
    const SdfPath&            dstRootPath,
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
    if (!shouldMergeValue(
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
            specType,
            field,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            valueToCopy))
        return false;

    printAboutField(srcLayer, srcPath, field, "modified value. ");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
/// Decides if we should merge children while printing its location to the Maya console.
bool mergeAndPrintChildren(
    UsdStageRefPtr            srcStage,
    const SdfPath&            srcRootPath,
    UsdStageRefPtr            dstStage,
    const SdfPath&            dstRootPath,
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
    if (!shouldMergeChildren(
            srcStage,
            srcRootPath,
            dstStage,
            dstRootPath,
            childrenField,
            srcLayer,
            srcPath,
            fieldInSrc,
            dstLayer,
            dstPath,
            fieldInDst,
            srcChildren,
            dstChildren))
        return false;

    printAboutField(srcLayer, srcPath, childrenField, "modified children. ");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
/// Copies a minimal prim using diff and merge, printing all fields that are copied to the Maya
/// console.
bool mergeAndPrintPrims(
    UsdStageRefPtr       srcStage,
    const SdfLayerRefPtr srcLayer,
    const SdfPath&       srcPath,
    UsdStageRefPtr       dstStage,
    SdfLayerRefPtr       dstLayer,
    const SdfPath&       dstPath)
{
    auto copyValue
        = makeFuncWithStagesAndRoots(srcStage, srcPath, dstStage, dstPath, mergeAndPrintValue);
    auto copyChildren
        = makeFuncWithStagesAndRoots(srcStage, srcPath, dstStage, dstPath, mergeAndPrintChildren);
    return SdfCopySpec(srcLayer, srcPath, dstLayer, dstPath, copyValue, copyChildren);
}

//----------------------------------------------------------------------------------------------------------------------
// Entrypoint of Merge
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
/// Available methods to merge or copy prims.

enum class MergeMethod
{
    FullCopy,
    FullCopyAndPrint,
    MergeDiff,
    MergeDiffAndPrint
};

MergeMethod mergeMethod = MergeMethod::MergeDiffAndPrint;

} // namespace

//----------------------------------------------------------------------------------------------------------------------
/// merges prims starting at a source path from a source layer and stage to a destination.
bool mergePrims(
    UsdStageRefPtr       srcStage,
    const SdfLayerRefPtr srcLayer,
    const SdfPath&       srcRootPath,
    UsdStageRefPtr       dstStage,
    SdfLayerRefPtr       dstLayer,
    const SdfPath&       dstRootPath)
{
    switch (mergeMethod) {
    default:
    case MergeMethod::FullCopy:
        return copyEntirePrims(srcLayer, srcRootPath, dstLayer, dstRootPath);
    case MergeMethod::FullCopyAndPrint:
        return copyAndPrintPrims(srcLayer, srcRootPath, dstLayer, dstRootPath);
    case MergeMethod::MergeDiffAndPrint:
        return mergeAndPrintPrims(srcStage, srcLayer, srcRootPath, dstStage, dstLayer, dstRootPath);
    case MergeMethod::MergeDiff:
        return mergeDiffPrims(srcStage, srcLayer, srcRootPath, dstStage, dstLayer, dstRootPath);
    }
}

} // namespace MayaUsdUtils
