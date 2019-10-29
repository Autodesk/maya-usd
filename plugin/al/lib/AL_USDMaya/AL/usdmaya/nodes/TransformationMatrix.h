//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/usdmaya/Api.h"
#include "AL/usdmaya/TransformOperation.h"
#include "AL/usdmaya/nodes/BasicTransformationMatrix.h"

#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MPxTransform.h>
#include <maya/MPxTransformationMatrix.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

class Transform;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class provides a transformation matrix that allows you to apply tweaks over some
/// read only
///         transformation information extracted from a UsdPrim. Currently each tweak is a simple
///         offset over the values contained within the UsdPrim.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class TransformationMatrix : public BasicTransformationMatrix
{

    friend class Transform;

    UsdGeomXformable                m_xform;
    UsdTimeCode                     m_time;
    std::vector<UsdGeomXformOp>     m_xformops;
    std::vector<TransformOperation> m_orderedOps;

    // tweak values. These are applied on top of the USD transform values to produce the final
    // result.
    MVector        m_scaleTweak;
    MEulerRotation m_rotationTweak;
    MVector        m_translationTweak;
    MVector        m_shearTweak;
    MPoint         m_scalePivotTweak;
    MVector        m_scalePivotTranslationTweak;
    MPoint         m_rotatePivotTweak;
    MVector        m_rotatePivotTranslationTweak;
    MQuaternion    m_rotateOrientationTweak;

    // values read in from USD
    MVector        m_scaleFromUsd;
    MEulerRotation m_rotationFromUsd;
    MVector        m_translationFromUsd;
    MVector        m_shearFromUsd;
    MPoint         m_scalePivotFromUsd;
    MVector        m_scalePivotTranslationFromUsd;
    MPoint         m_rotatePivotFromUsd;
    MVector        m_rotatePivotTranslationFromUsd;
    MQuaternion    m_rotateOrientationFromUsd;

    // post-transform translation value applied in object space after all other transformations
    MVector m_localTranslateOffset;
    bool    m_enableUsdWriteback = true;

    void print() const
    {
        std::cout << "m_scaleTweak " << m_scaleTweak << '\n';
        std::cout << "m_rotationTweak " << m_rotationTweak << '\n';
        std::cout << "m_translationTweak " << m_translationTweak << '\n';
        std::cout << "m_shearTweak " << m_shearTweak << '\n';
        std::cout << "m_scalePivotTweak " << m_scalePivotTweak << '\n';
        std::cout << "m_scalePivotTranslationTweak " << m_scalePivotTranslationTweak << '\n';
        std::cout << "m_rotatePivotTweak " << m_rotatePivotTweak << '\n';
        std::cout << "m_rotatePivotTranslationTweak " << m_rotatePivotTranslationTweak << '\n';
        std::cout << "m_rotateOrientationTweak " << m_rotateOrientationTweak << '\n';
        std::cout << "m_scaleFromUsd " << m_scaleFromUsd << '\n';
        std::cout << "m_rotationFromUsd " << m_rotationFromUsd << '\n';
        std::cout << "m_translationFromUsd " << m_translationFromUsd << '\n';
        std::cout << "m_shearFromUsd " << m_shearFromUsd << '\n';
        std::cout << "m_scalePivotFromUsd " << m_scalePivotFromUsd << '\n';
        std::cout << "m_scalePivotTranslationFromUsd " << m_scalePivotTranslationFromUsd << '\n';
        std::cout << "m_rotatePivotFromUsd " << m_rotatePivotFromUsd << '\n';
        std::cout << "m_rotatePivotTranslationFromUsd " << m_rotatePivotTranslationFromUsd << '\n';
        std::cout << "m_rotateOrientationFromUsd " << m_rotateOrientationFromUsd << '\n'
                  << std::endl;
    }

    // methods that will insert a transform op into the ordered queue of operations, if for some.
    void insertTranslateOp();
    void insertScaleOp();
    void insertShearOp();
    void insertScalePivotOp();
    void insertScalePivotTranslationOp();
    void insertRotateOp();
    void insertRotatePivotOp();
    void insertRotatePivotTranslationOp();
    void insertRotateAxesOp();

    enum Flags
    {
        // describe which components are animated
        kAnimatedScale = 1 << 0,
        kAnimatedRotation = 1 << 1,
        kAnimatedTranslation = 1 << 2,
        kAnimatedMatrix = 1 << 3,
        kAnimatedShear = 1 << 4,

        // are the transform ops coming from a matrix, the PXR schema, or from the maya schema (no
        // flags set)
        kFromMatrix = 1 << 8,
        kFromMayaSchema = 1 << 9,

        // which transform components are present in the prim?
        kPrimHasScale = 1 << 16,
        kPrimHasRotation = 1 << 17,
        kPrimHasTranslation = 1 << 18,
        kPrimHasShear = 1 << 19,
        kPrimHasScalePivot = 1 << 20,
        kPrimHasScalePivotTranslate = 1 << 21,
        kPrimHasRotatePivot = 1 << 22,
        kPrimHasRotatePivotTranslate = 1 << 23,
        kPrimHasRotateAxes = 1 << 24,
        kPrimHasPivot = 1 << 25,
        kPrimHasTransform = 1 << 26,

        kPushToPrimEnabled = 1 << 28,
        kInheritsTransform = 1 << 29,

        kPushPrimToMatrix = 1 << 30,
        kReadAnimatedValues = 1 << 31,

        kAnimationMask = kAnimatedShear | kAnimatedScale | kAnimatedRotation | kAnimatedTranslation
            | kAnimatedMatrix,

        // Most of these flags are calculated based on reading the usd prim; however, a few are
        // driven "externally" (ie, from attributes on the controlling transform node), and should
        // NOT be reset when we're re-initializing (ie, in setPrim)
        kPreservationMask = kPushToPrimEnabled | kReadAnimatedValues
    };
    uint32_t m_flags = kReadAnimatedValues;

    bool internal_readVector(MVector& result, const UsdGeomXformOp& op)
    {
        return readVector(result, op, getTimeCode());
    }
    bool internal_readShear(MVector& result, const UsdGeomXformOp& op)
    {
        return readShear(result, op, getTimeCode());
    }
    bool internal_readPoint(MPoint& result, const UsdGeomXformOp& op)
    {
        return readPoint(result, op, getTimeCode());
    }
    bool internal_readRotation(MEulerRotation& result, const UsdGeomXformOp& op)
    {
        return readRotation(result, op, getTimeCode());
    }
    double internal_readDouble(const UsdGeomXformOp& op) { return readDouble(op, getTimeCode()); }
    bool   internal_readMatrix(MMatrix& result, const UsdGeomXformOp& op)
    {
        return readMatrix(result, op, getTimeCode());
    }

    bool internal_pushVector(const MVector& result, UsdGeomXformOp& op)
    {
        return pushVector(result, op, getTimeCode());
    }
    bool internal_pushPoint(const MPoint& result, UsdGeomXformOp& op)
    {
        return pushPoint(result, op, getTimeCode());
    }
    bool internal_pushRotation(const MEulerRotation& result, UsdGeomXformOp& op)
    {
        return pushRotation(result, op, getTimeCode());
    }
    void internal_pushDouble(const double result, UsdGeomXformOp& op)
    {
        pushDouble(result, op, getTimeCode());
    }
    bool internal_pushShear(const MVector& result, UsdGeomXformOp& op)
    {
        return pushShear(result, op, getTimeCode());
    }
    bool internal_pushMatrix(const MMatrix& result, UsdGeomXformOp& op)
    {
        return pushMatrix(result, op, getTimeCode());
    }

    /// \brief  checks to see whether the transform attribute is locked
    /// \return true if the translate attribute is locked
    bool isTranslateLocked()
    {
        MPlug plug(m_transformNode.object(), MPxTransform::translate);
        return plug.isLocked() || plug.child(0).isLocked() || plug.child(1).isLocked()
            || plug.child(2).isLocked();
    }

    /// \brief  checks to see whether the rotate attribute is locked
    /// \return true if the rotate attribute is locked
    bool isRotateLocked()
    {
        MPlug plug(m_transformNode.object(), MPxTransform::rotate);
        return plug.isLocked() || plug.child(0).isLocked() || plug.child(1).isLocked()
            || plug.child(2).isLocked();
    }

    /// \brief  checks to see whether the scale attribute is locked
    /// \return true if the scale attribute is locked
    bool isScaleLocked()
    {
        MPlug plug(m_transformNode.object(), MPxTransform::scale);
        return plug.isLocked() || plug.child(0).isLocked() || plug.child(1).isLocked()
            || plug.child(2).isLocked();
    }

    /// \brief  checks to see whether the shear attribute is locked
    /// \return true if the shear attribute is locked
    bool isShearLocked()
    {
        MPlug plug(m_transformNode.object(), MPxTransform::shear);
        return plug.isLocked() || plug.child(0).isLocked() || plug.child(1).isLocked()
            || plug.child(2).isLocked();
    }

    /// \brief  helper method. Reads a vector from the transform op specified at the requested
    /// timecode \param  result the returned result \param  op the transformation op to read from
    /// \param  timeCode the time at which to query the transform value
    /// return  true if read ok
    static bool readVector(
        MVector&              result,
        const UsdGeomXformOp& op,
        UsdTimeCode           timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Reads a shear value from the transform op specified at the requested
    /// timecode \param  result the returned result \param  op the transformation op to read from
    /// \param  timeCode the time at which to query the transform value
    /// return  true if read ok
    static bool readShear(
        MVector&              result,
        const UsdGeomXformOp& op,
        UsdTimeCode           timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Reads a point from the transform op specified at the requested
    /// timecode \param  result the returned result \param  op the transformation op to read from
    /// \param  timeCode the time at which to query the transform value
    /// return  true if read ok
    static bool readPoint(
        MPoint&               result,
        const UsdGeomXformOp& op,
        UsdTimeCode           timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Reads an euler rotation from the transform op specified at the
    /// requested timecode \param  result the returned result \param  op the transformation op to
    /// read from \param  timeCode the time at which to query the transform value return  true if
    /// read ok
    static bool readRotation(
        MEulerRotation&       result,
        const UsdGeomXformOp& op,
        UsdTimeCode           timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Reads a double from the transform op specified at the requested
    /// timecode (typically RotateX / rotateY values) \param  op the transformation op to read from
    /// \param  timeCode the time at which to query the transform value
    /// return  the returned value
    static double
    readDouble(const UsdGeomXformOp& op, UsdTimeCode timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Reads a matrix from the transform op specified at the requested
    /// timecode \param  result the returned result \param  op the transformation op to read from
    /// \param  timeCode the time at which to query the transform value
    /// return  true if read ok
    static bool readMatrix(
        MMatrix&              result,
        const UsdGeomXformOp& op,
        UsdTimeCode           timeCode = UsdTimeCode::EarliestTime());

    /// \brief  helper method. Pushes a vector into the transform op specified at the requested
    /// timecode \param  input the new value to insert into the transform operation \param  op the
    /// transformation op to write into \param  timeCode the time at which to set the transform
    /// value return  true if written ok
    static bool pushVector(
        const MVector&  input,
        UsdGeomXformOp& op,
        UsdTimeCode     timeCode = UsdTimeCode::Default());

    /// \brief  helper method. Pushes a point into the transform op specified at the requested
    /// timecode \param  input the new value to insert into the transform operation \param  op the
    /// transformation op to write into \param  timeCode the time at which to set the transform
    /// value return  true if written ok
    static bool pushPoint(
        const MPoint&   input,
        UsdGeomXformOp& op,
        UsdTimeCode     timeCode = UsdTimeCode::Default());

    /// \brief  helper method. Pushes a vector into the transform op specified at the requested
    /// timecode \param  input the new value to insert into the transform operation \param  op the
    /// transformation op to write into \param  timeCode the time at which to set the transform
    /// value return  true if written ok
    static bool pushRotation(
        const MEulerRotation& input,
        UsdGeomXformOp&       op,
        UsdTimeCode           timeCode = UsdTimeCode::Default());

    /// \brief  helper method. Pushes a double into the transform op specified at the requested
    /// timecode (typically for RotateX / RotateY) \param  input the new value to insert into the
    /// transform operation \param  op the transformation op to write into \param  timeCode the time
    /// at which to set the transform value return  true if written ok
    static void pushDouble(
        const double    input,
        UsdGeomXformOp& op,
        UsdTimeCode     timeCode = UsdTimeCode::Default());

    /// \brief  helper method. Pushes a shear into the transform op specified at the requested
    /// timecode \param  input the new value to insert into the transform operation \param  op the
    /// transformation op to write into \param  timeCode the time at which to set the transform
    /// value return  true if written ok
    static bool pushShear(
        const MVector&  input,
        UsdGeomXformOp& op,
        UsdTimeCode     timeCode = UsdTimeCode::Default());

    /// \brief  helper method. Pushes a matrix into the transform op specified at the requested
    /// timecode \param  input the new value to insert into the transform operation \param  op the
    /// transformation op to write into \param  timeCode the time at which to set the transform
    /// value return  true if written ok
    static bool pushMatrix(
        const MMatrix&  input,
        UsdGeomXformOp& op,
        UsdTimeCode     timeCode = UsdTimeCode::Default());

    /// \brief  If set to true, transform values will target the animated key-frame values in the
    /// prim. If set to false,
    ///         the transform values will target the default attribute values.
    /// \param  enabled true to target animated values, false to target the default.
    void enableReadAnimatedValues(bool enabled);

    /// \brief  Applies a local space translation offset to the computed matrix. Useful for
    /// positioning objects on a
    ///         table.
    /// \param  localTranslateOffset the local space offset to apply to the transform.
    inline void setLocalTranslationOffset(const MVector& localTranslateOffset)
    {
        m_localTranslateOffset = localTranslateOffset;
    }

    /// \brief  this method updates the internal transformation components to the given time. Only
    /// the Transform node
    ///         should need to call this method
    /// \param  time the new timecode
    void updateToTime(const UsdTimeCode& time);

    /// \brief  pushes any modifications on the matrix back onto the UsdPrim
    void pushToPrim();

    void notifyProxyShapeOfRedraw(GfMatrix4d& oldMatrix, bool oldResetsStack);

    /// \brief  sets the SRT values from a matrix
    void setFromMatrix(MObject thisNode, const MMatrix& m);

    //  Translation methods:
    MStatus translateTo(const MVector& vector, MSpace::Space = MSpace::kTransform) override;

    //  Scale methods:
    MStatus scaleTo(const MVector&, MSpace::Space = MSpace::kTransform) override;

    //  Shear methods:
    MStatus shearTo(const MVector& shear, MSpace::Space = MSpace::kTransform) override;

    //  Scale pivot methods:
    MStatus
    setScalePivot(const MPoint&, MSpace::Space = MSpace::kTransform, bool balance = true) override;
    MStatus
    setScalePivotTranslation(const MVector& vector, MSpace::Space = MSpace::kTransform) override;

    //  Rotate pivot methods:
    MStatus
    setRotatePivot(const MPoint&, MSpace::Space = MSpace::kTransform, bool balance = true) override;
    MStatus
    setRotatePivotTranslation(const MVector& vector, MSpace::Space = MSpace::kTransform) override;

    //  Rotate order methods:
    MStatus setRotationOrder(MTransformationMatrix::RotationOrder, bool preserve = true) override;

    //  Rotation methods:
    MStatus rotateTo(const MQuaternion& q, MSpace::Space = MSpace::kTransform) override;
    MStatus rotateTo(const MEulerRotation& e, MSpace::Space = MSpace::kTransform) override;
    MStatus setRotateOrientation(
        const MQuaternion& q,
        MSpace::Space = MSpace::kTransform,
        bool balance = true) override;
    MStatus setRotateOrientation(
        const MEulerRotation& euler,
        MSpace::Space = MSpace::kTransform,
        bool balance = true) override;

    /// \brief  If set to true, modifications to these transform attributes will be pushed back onto
    /// the original prim. \param  enabled true will cause changes to this transform update the
    /// values on the USD prim. False will mean that
    ///         the changes are simply cached internally.
    /// \param  enabled true to write values to the usd prim when changed, false to treat the usd
    /// prim as read only.
    void enablePushToPrim(bool enabled);

    //  Compute matrix result
    MMatrix asMatrix() const override;
    MMatrix asMatrix(double percent) const override;

public:
    /// \brief  the type ID of the transformation matrix
    AL_USDMAYA_PUBLIC
    static const MTypeId kTypeId;

    /// \brief  create an instance of  this transformation matrix
    /// \return a new instance of this transformation matrix
    AL_USDMAYA_PUBLIC
    static MPxTransformationMatrix* creator();

    /// \brief  ctor
    TransformationMatrix();

    /// \brief  ctor
    /// \param  prim the USD prim that this matrix should represent
    TransformationMatrix(const UsdPrim& prim);

    /// \brief  set the prim that this transformation matrix will read/write to.
    /// \param  prim the prim
    /// \param  transformNode the owning transform node
    void setPrim(const UsdPrim& prim, Scope* transformNode) override;

    /// \brief  Returns the timecode to use when pushing the transform values to the USD prim. If
    /// readFromTimeline flag
    ///         is set to true, then the timecode will be read from the incoming time attribute on
    ///         the transform node. If readFromTimeline is false, then the timecode will be the
    ///         magic 'modify default values' timecode, and animation data will not be affected
    ///         (only the default values found in the USD prim)
    /// \return the timecode to use when pushing transform values to the USD prim
    inline UsdTimeCode getTimeCode()
    {
        return readAnimatedValues() ? m_time : UsdTimeCode::Default();
    }

    //--------------------------------------------------------------------------------------------------------------------
    /// \name  Query flags
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  does the prim have animated scale?
    inline bool hasAnimation() const { return (kAnimationMask & m_flags) != 0; }

    /// \brief  does the prim have animated scale?
    inline bool hasAnimatedScale() const { return (kAnimatedScale & m_flags) != 0; }

    /// \brief  does the prim have animated shear?
    inline bool hasAnimatedShear() const { return (kAnimatedShear & m_flags) != 0; }

    /// \brief  does the prim have animated translation?
    inline bool hasAnimatedTranslation() const { return (kAnimatedTranslation & m_flags) != 0; }

    /// \brief  does the prim have animated rotation?
    inline bool hasAnimatedRotation() const { return (kAnimatedRotation & m_flags) != 0; }

    /// \brief  does the prim have an animated matrix only?
    inline bool hasAnimatedMatrix() const { return (kAnimatedMatrix & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a scale transform op?
    inline bool primHasScale() const { return (kPrimHasScale & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a rotation transform op?
    inline bool primHasRotation() const { return (kPrimHasRotation & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a translation transform op?
    inline bool primHasTranslation() const { return (kPrimHasTranslation & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a shear transform op?
    inline bool primHasShear() const { return (kPrimHasShear & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a scale pivot op?
    inline bool primHasScalePivot() const { return (kPrimHasScalePivot & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a scale pivot translate op?
    inline bool primHasScalePivotTranslate() const
    {
        return (kPrimHasScalePivotTranslate & m_flags) != 0;
    }

    /// \brief  does the UsdGeomXform have a rotate pivot op?
    inline bool primHasRotatePivot() const { return (kPrimHasRotatePivot & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a rotate pivot translate op?
    inline bool primHasRotatePivotTranslate() const
    {
        return (kPrimHasRotatePivotTranslate & m_flags) != 0;
    }

    /// \brief  does the UsdGeomXform have a rotation axes op?
    inline bool primHasRotateAxes() const { return (kPrimHasRotateAxes & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a pixar pivot op?
    inline bool primHasPivot() const { return (kPrimHasPivot & m_flags) != 0; }

    /// \brief  does the UsdGeomXform have a transform matrix op?
    inline bool primHasTransform() const { return (kPrimHasTransform & m_flags) != 0; }

    /// \brief  should we read the animated keyframes or the defaults?
    inline bool readAnimatedValues() const { return (kReadAnimatedValues & m_flags) != 0; }

    /// \brief  Is this transform set to write back onto the USD prim
    inline bool pushToPrimEnabled() const { return (kPushToPrimEnabled & m_flags) != 0; }

    /// \brief  Is this prim writing back to a matrix (true) or to components (false)
    inline bool pushPrimToMatrix() const { return (kPushPrimToMatrix & m_flags) != 0; }

    /// \brief  Is this transform set to write back onto the USD prim, and is it currently possible?
    bool pushToPrimAvailable() const override { return pushToPrimEnabled() && m_prim.IsValid(); }

    //--------------------------------------------------------------------------------------------------------------------
    /// \name  Convert To-From USD primitive
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  this method inspects the UsdGeomXform to find out:
    ///         -# what schema is being used (Maya, Pxr, or just a matrix)
    ///         -# which transformation components are present (e.g. scale, rotate, etc).
    ///         -# which, if any, of those components are animated.
    /// \param  readFromPrim if true, the maya attribute values will be updated from those found on
    /// the USD prim \param  node the transform node to which this matrix belongs (and where the USD
    /// prim will be extracted from)
    void initialiseToPrim(bool readFromPrim = true, Scope* node = 0) override;

    void pushTranslateToPrim();
    void pushPivotToPrim();
    void pushRotatePivotTranslateToPrim();
    void pushRotatePivotToPrim();
    void pushRotateToPrim();
    void pushRotateQuatToPrim();
    void pushRotateAxisToPrim();
    void pushScalePivotTranslateToPrim();
    void pushScalePivotToPrim();
    void pushScaleToPrim();
    void pushShearToPrim();
    void pushTransformToPrim();

    // Helper class.  Creating a variable of this class temporarily disables
    // push to prim after saving its original state.  When the variable goes
    // out of scope, the original push to prim state is restored by the
    // destructor.
    //
    class Scoped_DisablePushToPrim
    {
    public:
        Scoped_DisablePushToPrim(TransformationMatrix& tm)
            : m_transformationMatrix(tm)
        {
            m_IsPushToPrimEnabled = m_transformationMatrix.pushToPrimEnabled();
            m_transformationMatrix.m_flags &= ~TransformationMatrix::kPushToPrimEnabled;
        }

        ~Scoped_DisablePushToPrim()
        {
            if (m_IsPushToPrimEnabled) {
                m_transformationMatrix.m_flags |= TransformationMatrix::kPushToPrimEnabled;
            }
        }

    private:
        // The TransformationMatrix whose push to prim state is being affected.
        TransformationMatrix& m_transformationMatrix;

        // Holder for the original value of push to prim state.
        bool m_IsPushToPrimEnabled;
    };
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
