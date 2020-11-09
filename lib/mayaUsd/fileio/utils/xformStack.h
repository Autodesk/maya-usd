// Copyright 2017 Pixar
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
#ifndef PXRUSDMAYA_XFORM_STACK_H
#define PXRUSDMAYA_XFORM_STACK_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/refPtr.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <maya/MTransformationMatrix.h>

#include <limits>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Note: pivotTranslate is currently not used in MayaXformStack,
// CommonXformStack, or MatrixStack, so it should never occur
// at present, but there was some support for reading it, thus
// why it's here

// clang-format off
/// \hideinitializer
#define PXRUSDMAYA_XFORM_STACK_TOKENS \
    (translate) \
    (rotatePivotTranslate) \
    (rotatePivot) \
    (rotate) \
    (rotateAxis) \
    (scalePivotTranslate) \
    (scalePivot) \
    (shear) \
    (scale) \
    (pivot) \
    (pivotTranslate) \
    (transform)
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaXformStackTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_XFORM_STACK_TOKENS);

/// \class UsdMayaXformOpClassification
/// \brief Defines a named "class" of xform operation
///
/// Similar to UsdGeomXformOp, but without a specific attribute;
/// UsdGeomXformOps can be thought of as "instances" of a
/// UsdMayaXformOpDefinition "type"
class UsdMayaXformOpClassification
{
public:
    UsdMayaXformOpClassification(
        const TfToken&       name,
        UsdGeomXformOp::Type opType,
        bool                 isInvertedTwin = false);

    UsdMayaXformOpClassification();

    static UsdMayaXformOpClassification const& NullInstance();

    MAYAUSD_CORE_PUBLIC
    TfToken const& GetName() const;

    MAYAUSD_CORE_PUBLIC
    UsdGeomXformOp::Type GetOpType() const;

    MAYAUSD_CORE_PUBLIC
    bool IsInvertedTwin() const;
    ;

    /// Return True if the given op type is compatible with this
    /// OpClassification (ie, is the same, or is, say rotateX, when
    /// this op type is rotateXYZ)
    MAYAUSD_CORE_PUBLIC
    bool IsCompatibleType(UsdGeomXformOp::Type otherType) const;

    MAYAUSD_CORE_PUBLIC
    bool operator==(const UsdMayaXformOpClassification& other) const;

    MAYAUSD_CORE_PUBLIC
    bool IsNull() const;

    MAYAUSD_CORE_PUBLIC
    std::vector<TfToken> CompatibleAttrNames() const;

private:
    class _Data;

    // Because this is an immutable type, we keep a pointer to shared
    // data; this allows us to only have overhead associated with
    // a RefPtr, while having easy-python-wrapping (without overhead
    // of WeakPtr)
    typedef TfRefPtr<_Data> _DataRefPtr;
    _DataRefPtr             _sharedData;
};

/// \class UsdMayaXformStack
/// \brief Defines a standard list of xform operations.
///
/// Used to define the set and order of transforms that programs like
/// Maya use and understand.
class UsdMayaXformStack
{
public:
    typedef UsdMayaXformOpClassification            OpClass;
    typedef std::vector<OpClass>                    OpClassList;
    typedef std::pair<const OpClass, const OpClass> OpClassPair;

    // Internally, we use indices, because position sometimes matters...
    // should be safe, since _ops is const.
    static constexpr size_t           NO_INDEX = std::numeric_limits<size_t>::max();
    typedef std::pair<size_t, size_t> IndexPair;
    typedef std::unordered_map<TfToken, IndexPair, TfToken::HashFunctor> TokenIndexPairMap;
    typedef std::unordered_map<size_t, size_t>                           IndexMap;

    // Templated because we want it to work with both MEulerRotation::RotationOrder
    // and MTransformationMatrix::RotationOrder
    template <typename RotationOrder>
    static RotationOrder RotateOrderFromOpType(
        UsdGeomXformOp::Type opType,
        RotationOrder        defaultRotOrder = RotationOrder::kXYZ)
    {
        switch (opType) {
        case UsdGeomXformOp::TypeRotateXYZ: return RotationOrder::kXYZ; break;
        case UsdGeomXformOp::TypeRotateXZY: return RotationOrder::kXZY; break;
        case UsdGeomXformOp::TypeRotateYXZ: return RotationOrder::kYXZ; break;
        case UsdGeomXformOp::TypeRotateYZX: return RotationOrder::kYZX; break;
        case UsdGeomXformOp::TypeRotateZXY: return RotationOrder::kZXY; break;
        case UsdGeomXformOp::TypeRotateZYX: return RotationOrder::kZYX; break;
        default: return defaultRotOrder; break;
        }
    }

    UsdMayaXformStack(
        const OpClassList&            ops,
        const std::vector<IndexPair>& inversionTwins,
        bool                          nameMatters = true);

    // Don't want to accidentally make a copy, since the only instances are supposed
    // to be static!
    explicit UsdMayaXformStack(const UsdMayaXformStack& other) = default;
    explicit UsdMayaXformStack(UsdMayaXformStack&& other) = default;

    MAYAUSD_CORE_PUBLIC
    OpClassList const& GetOps() const;

    MAYAUSD_CORE_PUBLIC
    const std::vector<IndexPair>& GetInversionTwins() const;

    MAYAUSD_CORE_PUBLIC
    bool GetNameMatters() const;

    MAYAUSD_CORE_PUBLIC
    UsdMayaXformOpClassification const& operator[](const size_t index) const;

    MAYAUSD_CORE_PUBLIC
    size_t GetSize() const;

    /// \brief  Finds the index of the Op Classification with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// \param  isInvertedTwin the returned op classification object must match
    ///         this param for it's IsInvertedTwin() - if an op is found that matches
    ///         the name, but has the wrong invertedTwin status, NO_INDEX is returned
    /// return  Index to the op classification object with the given name (and
    ///         inverted twin state); will be NO_INDEX if no match could be found.
    MAYAUSD_CORE_PUBLIC
    size_t FindOpIndex(const TfToken& opName, bool isInvertedTwin = false) const;

    /// \brief  Finds the Op Classification with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// \param  isInvertedTwin the returned op classification object must match
    ///         this param for it's IsInvertedTwin() - if an op is found that matches
    ///         the name, but has the wrong invertedTwin status, OpClass::NullInstance
    ///         is returned
    /// return  Reference to the op classification object with the given name (and
    ///         inverted twin state); will be a reference to OpClass::NullInstance
    ///         if no match could be found.
    MAYAUSD_CORE_PUBLIC
    const OpClass& FindOp(const TfToken& opName, bool isInvertedTwin = false) const;

    /// \brief  Finds the indices of Op Classification(s) with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// return  A pair of indices to op classification objects with the given name;
    ///         if the objects are part of an inverted twin pair, then both are
    ///         returned (in the order they appear in this stack). If found, but
    ///         not as part of an inverted twin pair, the first result will point
    ///         to the found classification, and the second will be NO_INDEX.  If
    ///         no matches are found, both will be NO_INDEX.
    MAYAUSD_CORE_PUBLIC
    const IndexPair& FindOpIndexPair(const TfToken& opName) const;

    /// \brief  Finds the Op Classification(s) with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// return  A pair classification objects with the given name; if the objects
    ///         are part of an inverted twin pair, then both are returned (in the
    ///         order they appear in this stack). If found, but not as part of an
    ///         inverted twin pair, the first result will point to the found
    ///         classification, and the second will be OpClass::NullInstance.  If
    ///         no matches are found, both results will be OpClass::NullInstance.
    MAYAUSD_CORE_PUBLIC
    const OpClassPair FindOpPair(const TfToken& opName) const;

    /// \brief Returns a list of matching XformOpDefinitions for this stack
    ///
    /// For each xformop, we want to find the corresponding op within this
    /// stack that it matches.  There are 3 requirements:
    ///  - to be considered a match, the name and type must match an op in this stack
    ///  - the matches for each xformop must have increasing indexes in the stack
    ///  - inversionTwins must either both be matched or neither matched.
    ///
    /// This returns a vector of the matching XformOpDefinitions in this stack. The
    /// size of this vector will be 0 if no complete match is found, or xformops.size()
    /// if a complete match is found.
    MAYAUSD_CORE_PUBLIC
    OpClassList MatchingSubstack(const std::vector<UsdGeomXformOp>& xformops) const;

    /// \brief The standard Maya xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    rotatePivotTranslate
    ///    rotatePivot
    ///    rotate
    ///    rotateAxis
    ///    rotatePivot^-1 (inverted twin)
    ///    scalePivotTranslate
    ///    scalePivot
    ///    shear
    ///    scale
    ///    scalePivot^-1 (inverted twin)
    MAYAUSD_CORE_PUBLIC
    static const UsdMayaXformStack& MayaStack();

    /// \brief The Common API xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    pivot
    ///    rotate
    ///    scale
    ///    pivot^-1 (inverted twin)
    MAYAUSD_CORE_PUBLIC
    static const UsdMayaXformStack& CommonStack();

    /// \brief xform "stack" consisting of only a single matrix xform
    ///
    /// This stack will match any list of xform ops that consist of a single
    /// matrix "transform" op, regardless of name.
    /// Consists of these xform operators:
    ///    transform
    MAYAUSD_CORE_PUBLIC
    static const UsdMayaXformStack& MatrixStack();

    /// \brief Runs MatchingSubstack against the given list of stacks
    ///
    /// Returns the first non-empty result it finds; if all stacks
    /// return an empty vector, an empty vector is returned.
    MAYAUSD_CORE_PUBLIC
    static OpClassList FirstMatchingSubstack(
        const std::vector<UsdMayaXformStack const*>& stacks,
        const std::vector<UsdGeomXformOp>&           xformops);

private:
    class _Data;

    // Because this is an immutable type, we keep a pointer to shared
    // data; this allows us to only have overhead associated with
    // a RefPtr, while having easy-python-wrapping (without overhead
    // of WeakPtr)
    typedef TfRefPtr<_Data> _DataRefPtr;
    _DataRefPtr             _sharedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
