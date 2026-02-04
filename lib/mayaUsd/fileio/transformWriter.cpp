//
// Copyright 2016 Pixar
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
#include "transformWriter.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/splineUtils.h>
#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTransform.h>
#include <maya/MString.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(transform, UsdMayaTransformWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(transform, UsdGeomXform);

void UsdMayaTransformWriter::_AnimChannel::setXformOp(
    const GfVec3d&             value,
    const GfMatrix4d&          matrix,
    const UsdTimeCode&         usdTime,
    FlexibleSparseValueWriter* valueWriter) const
{
    if (!op) {
        TF_CODING_ERROR("Xform op is not valid");
        return;
    }

    VtValue vtValue;
    if (valueType == _ValueType::Matrix) {
        vtValue = matrix;
    } else if (valueType == _ValueType::Value) {
        bool isDouble = UsdGeomXformOp::GetPrecisionFromValueTypeName(op.GetAttr().GetTypeName())
            == UsdGeomXformOp::PrecisionDouble;
        vtValue = isDouble ? VtValue(value[valueIndex])
                           : VtValue(static_cast<float>(value[valueIndex]));
    } else if (opType == _XformType::Shear) {
        GfMatrix4d shearXForm(1.0);
        shearXForm[1][0] = value[0]; // xyVal
        shearXForm[2][0] = value[1]; // xzVal
        shearXForm[2][1] = value[2]; // yzVal
        vtValue = shearXForm;
    } else if (
        UsdGeomXformOp::GetPrecisionFromValueTypeName(op.GetAttr().GetTypeName())
        == UsdGeomXformOp::PrecisionDouble) {
        vtValue = VtValue(value);
    } else {
        // float precision
        vtValue = VtValue(GfVec3f(value));
    }
    valueWriter->SetAttribute(op.GetAttr(), vtValue, usdTime);
}

void UsdMayaTransformWriter::_ComputeXformOps(
    const std::vector<_AnimChannel>&           animChanList,
    const UsdTimeCode&                         usdTime,
    const bool                                 eulerFilter,
    UsdMayaTransformWriter::_TokenRotationMap* previousRotates,
    FlexibleSparseValueWriter*                 valueWriter,
    double                                     distanceConversionScalar)
{
    if (!TF_VERIFY(previousRotates)) {
        return;
    }

    // Iterate over each _AnimChannel, retrieve the default value and pull the
    // Maya data if needed. Then store it on the USD Ops
    for (const auto& animChannel : animChanList) {

        if (animChannel.isInverse) {
            continue;
        }

        GfVec3d            value = animChannel.defValue;
        GfMatrix4d         matrix = animChannel.defMatrix;
        bool               hasAnimated = false;
        bool               hasStatic = false;
        const unsigned int plugCount = animChannel.valueType == _ValueType::Matrix ? 1u : 3u;
        if (animChannel.valueType != _ValueType::Value) {
            for (unsigned int i = 0u; i < plugCount; ++i) {
                if (animChannel.sampleType[i] == _SampleType::Animated) {
                    if (animChannel.valueType == _ValueType::Matrix) {
                        matrix = animChannel.GetSourceData(i).Get<GfMatrix4d>();
                    } else {
                        value[i] = animChannel.GetSourceData(i).Get<double>();
                    }
                    hasAnimated = true;
                } else if (animChannel.sampleType[i] == _SampleType::Static) {
                    hasStatic = true;
                }
            }
        }

        // If the channel is not animated AND has non identity value, we are
        // computing default time, then set the values.
        //
        // If the channel is animated(connected) and we are not setting default
        // time, then set the values.
        //
        // This to make sure static channels are setting their default while
        // animating ones are actually animating
        if ((usdTime == UsdTimeCode::Default() && hasStatic && !hasAnimated)
            || (usdTime != UsdTimeCode::Default() && hasAnimated)) {

            if (animChannel.opType == _XformType::Rotate) {
                if (hasAnimated && eulerFilter) {
                    const TfToken& lookupName = animChannel.suffix.IsEmpty()
                        ? UsdGeomXformOp::GetOpTypeToken(animChannel.usdOpType)
                        : animChannel.suffix;
                    auto findResult = previousRotates->find(lookupName);
                    if (findResult == previousRotates->end()) {
                        MEulerRotation::RotationOrder rotOrder
                            = UsdMayaXformStack::RotateOrderFromOpType(
                                animChannel.usdOpType, MEulerRotation::kXYZ);
                        MEulerRotation currentRotate(value[0], value[1], value[2], rotOrder);
                        (*previousRotates)[lookupName] = currentRotate;
                    } else {
                        MEulerRotation&               previousRotate = findResult->second;
                        MEulerRotation::RotationOrder rotOrder
                            = UsdMayaXformStack::RotateOrderFromOpType(
                                animChannel.usdOpType, previousRotate.order);
                        MEulerRotation currentRotate(value[0], value[1], value[2], rotOrder);
                        currentRotate.setToClosestSolution(previousRotate);
                        for (unsigned int i = 0; i < 3; i++) {
                            value[i] = currentRotate[i];
                        }
                        (*previousRotates)[lookupName] = currentRotate;
                    }
                }
                for (unsigned int i = 0; i < 3; i++) {
                    value[i] = GfRadiansToDegrees(value[i]);
                }
            } else if (animChannel.opType == _XformType::Translate) {
                // Scale the translation as needed to fit the desired metersPerUnit
                if (distanceConversionScalar != 1.0) {
                    value = value * distanceConversionScalar;
                }
            }
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
            if (_GetExportArgs().animationType != UsdMayaJobExportArgsTokens->curves) {
                animChannel.setXformOp(value, matrix, usdTime, valueWriter);
            }
#else
            animChannel.setXformOp(value, matrix, usdTime, valueWriter);
#endif
        }

#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
        if (animChannel.valueType == _ValueType::Value && usdTime == UsdTimeCode::Default()) {
            if (UsdGeomXformOp::GetPrecisionFromValueTypeName(
                    animChannel.op.GetAttr().GetTypeName())
                == UsdGeomXformOp::PrecisionDouble) {
                UsdMayaSplineUtils::WriteSplineAttribute<double>(
                    MFnDependencyNode(GetMayaObject()),
                    GetUsdPrim(),
                    animChannel.valueAttrName,
                    animChannel.op.GetAttr().GetName(),
                    // For translation, we need to apply the distanceConversionScalar
                    animChannel.opType == _XformType::Translate ? distanceConversionScalar : 1.f);
            } else {
                UsdMayaSplineUtils::WriteSplineAttribute<float>(
                    MFnDependencyNode(GetMayaObject()),
                    GetUsdPrim(),
                    animChannel.valueAttrName,
                    animChannel.op.GetAttr().GetName(),
                    // For rotations, we need to convert radians to degrees
                    animChannel.opType == _XformType::Rotate ? 180.0 / M_PI : 1.f);
            }
        }
#endif
    }
}

VtValue UsdMayaTransformWriter::_AnimChannel::GetSourceData(unsigned int i) const
{
    if (valueType == _ValueType::Matrix) {
        const MPlug&   attrPlug = plug[i];
        MFnMatrixData  matrixDataFn(attrPlug.asMObject());
        const MMatrix& mayaMatrix = matrixDataFn.matrix();
        GfMatrix4d     matrix;
        MayaUsd::TypedConverter<MMatrix, GfMatrix4d>::convert(mayaMatrix, matrix);
        return VtValue(matrix);
    } else {
        return VtValue(plug[i].asDouble());
    }
}

/* static */
bool UsdMayaTransformWriter::_GatherAnimChannel(
    const _XformType           opType,
    const MFnTransform&        iTrans,
    const TfToken&             mayaAttrName,
    const MString&             xName,
    const MString&             yName,
    const MString&             zName,
    std::vector<_AnimChannel>* oAnimChanList,
    const bool                 isWritingAnimation,
    const bool                 useSuffix,
    const TfToken&             animType,
    const bool                 isMatrix)
{
    _AnimChannel chan;
    chan.opType = opType;
    chan.isInverse = false;
    chan.valueType = isMatrix ? _ValueType::Matrix : _ValueType::Vector;
    if (useSuffix) {
        chan.suffix = mayaAttrName;
    }
    MString mayaAttrNameMStr = mayaAttrName.GetText();

    // We default to single precision (later we set the main translate op and
    // shear to double)
    chan.precision = UsdGeomXformOp::PrecisionFloat;

    bool hasValidComponents = false;

    // this is to handle the case where there is a connection to the parent
    // plug but not to the child plugs, if the connection is there and you are
    // not forcing static, then all of the children are considered animated
    int parentSample = UsdMayaUtil::getSampledType(iTrans.findPlug(mayaAttrNameMStr), false);

    // Determine what plug are needed based on default value & being
    // connected/animated
    MStringArray suffixes;
    suffixes.append(xName);
    suffixes.append(yName);
    suffixes.append(zName);

    GfVec3d            nullValue(opType == _XformType::Scale ? 1.0 : 0.0);
    const unsigned int plugCount = isMatrix ? 1u : 3u;
    for (unsigned int i = 0; i < plugCount; i++) {
        // Find the plug and retrieve the data as the channel default value. It
        // won't be updated if the channel is NOT ANIMATED
        if (isMatrix) {
            chan.plug[i] = iTrans.findPlug(mayaAttrNameMStr);
            chan.defMatrix = chan.GetSourceData(i).Get<GfMatrix4d>();
        } else {
            chan.plug[i] = iTrans.findPlug(mayaAttrNameMStr + suffixes[i]);
            chan.defValue[i] = chan.GetSourceData(i).Get<double>();
        }
        chan.sampleType[i] = _SampleType::None;
        // If we allow animation and either the parent sample or local sample is
        // not 0 then we have an Animated sample else we have a scale and the
        // value is NOT 1 or if the value is NOT 0 then we have a static xform
        if ((parentSample != 0 || UsdMayaUtil::getSampledType(chan.plug[i], true) != 0)
            && isWritingAnimation) {
            chan.sampleType[i] = _SampleType::Animated;
            hasValidComponents = true;
        } else {
            const bool isNullValue = isMatrix ? GfIsClose(chan.defMatrix, GfMatrix4d(1.0), 1e-7)
                                              : GfIsClose(chan.defValue[i], nullValue[i], 1e-7);
            if (!isNullValue) {
                chan.sampleType[i] = _SampleType::Static;
                hasValidComponents = true;
            }
        }
    }

    // If there are valid component, then we will add the animation channel.
    if (hasValidComponents) {
        if (opType == _XformType::Scale) {
            chan.usdOpType = UsdGeomXformOp::TypeScale;
        } else if (opType == _XformType::Translate) {
            chan.usdOpType = UsdGeomXformOp::TypeTranslate;
            // The main translate is set to double precision
            if (mayaAttrName == UsdMayaXformStackTokens->translate) {
                chan.precision = UsdGeomXformOp::PrecisionDouble;
            }
        } else if (opType == _XformType::Rotate) {
            chan.usdOpType = UsdGeomXformOp::TypeRotateXYZ;
            // Rotation Order ONLY applies to the "rotate" attribute
            if (mayaAttrName == UsdMayaXformStackTokens->rotate) {
                switch (iTrans.rotationOrder()) {
                case MTransformationMatrix::kYZX:
                    chan.usdOpType = UsdGeomXformOp::TypeRotateYZX;
                    break;
                case MTransformationMatrix::kZXY:
                    chan.usdOpType = UsdGeomXformOp::TypeRotateZXY;
                    break;
                case MTransformationMatrix::kXZY:
                    chan.usdOpType = UsdGeomXformOp::TypeRotateXZY;
                    break;
                case MTransformationMatrix::kYXZ:
                    chan.usdOpType = UsdGeomXformOp::TypeRotateYXZ;
                    break;
                case MTransformationMatrix::kZYX:
                    chan.usdOpType = UsdGeomXformOp::TypeRotateZYX;
                    break;
                default: break;
                }
            }
        } else if (opType == _XformType::Shear) {
            chan.usdOpType = UsdGeomXformOp::TypeTransform;
            chan.precision = UsdGeomXformOp::PrecisionDouble;
        } else if (opType == _XformType::Transform) {
            chan.usdOpType = UsdGeomXformOp::TypeTransform;
            chan.precision = UsdGeomXformOp::PrecisionDouble;
        } else {
            return false;
        }
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
        // when using usd spline animation, we need to break down the transform elements as it's
        // smallest components. USD spline only supports floating point numbers and vec2.
        if (animType != UsdMayaJobExportArgsTokens->timesamples) {
            chan.valueType = _ValueType::Value;
            auto chanX = chan, chanY = chan, chanZ = chan;
            chanX.valueAttrName = mayaAttrName.GetString() + xName.asChar();
            chanY.valueIndex = 1;
            chanY.valueAttrName = mayaAttrName.GetString() + yName.asChar();
            chanZ.valueIndex = 2;
            chanZ.valueAttrName = mayaAttrName.GetString() + zName.asChar();

            // add channels for each component of the transform
            if (opType == _XformType::Rotate) {
                chanX.usdOpType = UsdGeomXformOp::TypeRotateX;
                chanY.usdOpType = UsdGeomXformOp::TypeRotateY;
                chanZ.usdOpType = UsdGeomXformOp::TypeRotateZ;

                switch (iTrans.rotationOrder()) {
                case MTransformationMatrix::kYZX:
                    oAnimChanList->push_back(chanX);
                    oAnimChanList->push_back(chanZ);
                    oAnimChanList->push_back(chanY);
                    break;
                case MTransformationMatrix::kZXY:
                    oAnimChanList->push_back(chanY);
                    oAnimChanList->push_back(chanX);
                    oAnimChanList->push_back(chanZ);
                    break;
                case MTransformationMatrix::kXZY:
                    oAnimChanList->push_back(chanY);
                    oAnimChanList->push_back(chanZ);
                    oAnimChanList->push_back(chanX);
                    break;
                case MTransformationMatrix::kXYZ:
                    oAnimChanList->push_back(chanZ);
                    oAnimChanList->push_back(chanY);
                    oAnimChanList->push_back(chanX);
                    break;
                case MTransformationMatrix::kYXZ:
                    oAnimChanList->push_back(chanZ);
                    oAnimChanList->push_back(chanX);
                    oAnimChanList->push_back(chanY);
                    break;
                case MTransformationMatrix::kZYX:
                    oAnimChanList->push_back(chanX);
                    oAnimChanList->push_back(chanY);
                    oAnimChanList->push_back(chanZ);
                    break;
                default: break;
                }
            } else if (opType == _XformType::Translate) {
                chanX.usdOpType = UsdGeomXformOp::TypeTranslateX;
                chanY.usdOpType = UsdGeomXformOp::TypeTranslateY;
                chanZ.usdOpType = UsdGeomXformOp::TypeTranslateZ;
                oAnimChanList->push_back(chanX);
                oAnimChanList->push_back(chanY);
                oAnimChanList->push_back(chanZ);
            } else if (opType == _XformType::Scale) {
                chanX.usdOpType = UsdGeomXformOp::TypeScaleX;
                chanY.usdOpType = UsdGeomXformOp::TypeScaleY;
                chanZ.usdOpType = UsdGeomXformOp::TypeScaleZ;
                oAnimChanList->push_back(chanX);
                oAnimChanList->push_back(chanY);
                oAnimChanList->push_back(chanZ);
            }
        } else {
            oAnimChanList->push_back(chan);
        }
#else
        oAnimChanList->push_back(chan);
#endif
        return true;
    }
    return false;
}

void UsdMayaTransformWriter::_MakeAnimChannelsUnique(const UsdGeomXformable& usdXformable)
{
    using OpName = TfToken;
    std::set<OpName> existingOps;
    bool             xformReset = false;
    for (const UsdGeomXformOp& op : usdXformable.GetOrderedXformOps(&xformReset)) {
        existingOps.emplace(op.GetOpName());
    }

    for (_AnimChannel& channel : _animChannels) {
        // We will put a upper limit on the number of similar transform operations
        // that a prim can use. Having 1000 separate translations on a single prim
        // seems both generous. Having more is highly improbable.
        for (int suffixIndex = 1; suffixIndex < 1000; ++suffixIndex) {
            TfToken channelOpName
                = UsdGeomXformOp::GetOpName(channel.usdOpType, channel.suffix, channel.isInverse);
            if (existingOps.count(channelOpName) == 0) {
                existingOps.emplace(channelOpName);
                break;
            }
            std::ostringstream oss;
            oss << "channel" << suffixIndex;
            channel.suffix = TfToken(oss.str());
        }
    }
}

void UsdMayaTransformWriter::_PushTransformStack(
    const MDagPath&         dagPath,
    const MFnTransform&     iTrans,
    const UsdGeomXformable& usdXformable,
    const bool              writeAnim,
    const bool              worldspace)
{
    // NOTE: I think this logic and the logic in MayaTransformReader
    // should be merged so the concept of "CommonAPI" stays centralized.
    //
    // By default we assume that the xform conforms to the common API
    // (xlate,pivot,rotate,scale,pivotINVERTED) As soon as we encounter any
    // additional xform (compensation translates for pivots, rotateAxis or
    // shear) we are not conforming anymore
    bool conformsToCommonAPI = true;

    // Keep track of where we have rotate and scale Pivots and their inverse so
    // that we can combine them later if possible
    unsigned int rotPivotIdx = -1, rotPivotINVIdx = -1, scalePivotIdx = -1, scalePivotINVIdx = -1;

    // Check if the Maya prim inherits-transform or needs world-space positioning.
    MPlug inheritPlug = iTrans.findPlug("inheritsTransform");
    if (!inheritPlug.isNull() && !inheritPlug.asBool()) {
        usdXformable.SetResetXformStack(true);
    } else if (worldspace) {
        MDagPath parentDagPath = dagPath;
        if (parentDagPath.pop() == MStatus::kSuccess && parentDagPath.isValid()) {
            MObject parentObj = parentDagPath.node();
            if (parentObj.apiType() != MFn::Type::kWorld) {
                MFnTransform parentTrans(parentObj);
                _PushTransformStack(
                    parentDagPath, parentTrans, usdXformable, writeAnim, worldspace);
            }
        }
    }

    const auto animType = _GetExportArgs().animationType;
    if (_GatherAnimChannel(
            _XformType::Transform,
            iTrans,
            UsdMayaXformStackTokens->offsetParentMatrix,
            "",
            "",
            "",
            &_animChannels,
            writeAnim,
            true,
            animType,
            true)) {
        conformsToCommonAPI = false;
    }

    // inspect the translate, no suffix to be closer compatibility with common API
    _GatherAnimChannel(
        _XformType::Translate,
        iTrans,
        UsdMayaXformStackTokens->translate,
        "X",
        "Y",
        "Z",
        &_animChannels,
        writeAnim,
        false,
        animType);

    // inspect the rotate pivot translate
    if (_GatherAnimChannel(
            _XformType::Translate,
            iTrans,
            UsdMayaXformStackTokens->rotatePivotTranslate,
            "X",
            "Y",
            "Z",
            &_animChannels,
            writeAnim,
            true,
            animType)) {
        conformsToCommonAPI = false;
    }

    // inspect the rotate pivot
    bool hasRotatePivot = _GatherAnimChannel(
        _XformType::Translate,
        iTrans,
        UsdMayaXformStackTokens->rotatePivot,
        "X",
        "Y",
        "Z",
        &_animChannels,
        writeAnim,
        true,
        animType);
    if (hasRotatePivot) {
        rotPivotIdx = _animChannels.size() - 1;
    }

    // inspect the rotate, no suffix to be closer compatibility with common API
    _GatherAnimChannel(
        _XformType::Rotate,
        iTrans,
        UsdMayaXformStackTokens->rotate,
        "X",
        "Y",
        "Z",
        &_animChannels,
        writeAnim,
        false,
        animType);

    // inspect the rotateAxis/orientation
    if (_GatherAnimChannel(
            _XformType::Rotate,
            iTrans,
            UsdMayaXformStackTokens->rotateAxis,
            "X",
            "Y",
            "Z",
            &_animChannels,
            writeAnim,
            true,
            animType)) {
        conformsToCommonAPI = false;
    }

    // invert the rotate pivot
    if (hasRotatePivot) {
        _AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.suffix = UsdMayaXformStackTokens->rotatePivot;
        chan.isInverse = true;
        _animChannels.push_back(chan);
        rotPivotINVIdx = _animChannels.size() - 1;
    }

    // inspect the scale pivot translation
    if (_GatherAnimChannel(
            _XformType::Translate,
            iTrans,
            UsdMayaXformStackTokens->scalePivotTranslate,
            "X",
            "Y",
            "Z",
            &_animChannels,
            writeAnim,
            true,
            animType)) {
        conformsToCommonAPI = false;
    }

    // inspect the scale pivot point
    bool hasScalePivot = _GatherAnimChannel(
        _XformType::Translate,
        iTrans,
        UsdMayaXformStackTokens->scalePivot,
        "X",
        "Y",
        "Z",
        &_animChannels,
        writeAnim,
        true,
        animType);
    if (hasScalePivot) {
        scalePivotIdx = _animChannels.size() - 1;
    }

    // inspect the shear. Even if we have one xform on the xform list, it represents a share so we
    // should name it
    if (_GatherAnimChannel(
            _XformType::Shear,
            iTrans,
            UsdMayaXformStackTokens->shear,
            "XY",
            "XZ",
            "YZ",
            &_animChannels,
            writeAnim,
            true,
            animType)) {
        conformsToCommonAPI = false;
    }

    // add the scale. no suffix to be closer compatibility with common API
    _GatherAnimChannel(
        _XformType::Scale,
        iTrans,
        UsdMayaXformStackTokens->scale,
        "X",
        "Y",
        "Z",
        &_animChannels,
        writeAnim,
        false,
        animType);

    // inverse the scale pivot point
    if (hasScalePivot) {
        _AnimChannel chan;
        chan.usdOpType = UsdGeomXformOp::TypeTranslate;
        chan.precision = UsdGeomXformOp::PrecisionFloat;
        chan.suffix = UsdMayaXformStackTokens->scalePivot;
        chan.isInverse = true;
        _animChannels.push_back(chan);
        scalePivotINVIdx = _animChannels.size() - 1;
    }

    // If still potential common API, check if the pivots are the same and NOT animated/connected
    if (hasRotatePivot != hasScalePivot) {
        conformsToCommonAPI = false;
    }

    if (conformsToCommonAPI && hasRotatePivot && hasScalePivot) {
        _AnimChannel rotPivChan, scalePivChan;
        rotPivChan = _animChannels[rotPivotIdx];
        scalePivChan = _animChannels[scalePivotIdx];
        // If they have different sampleType or are animated, then this does not
        // conformsToCommonAPI anymore
        for (unsigned int i = 0; i < 3; i++) {
            if (rotPivChan.sampleType[i] != scalePivChan.sampleType[i]
                || rotPivChan.sampleType[i] == _SampleType::Animated) {
                conformsToCommonAPI = false;
            }
        }

        // If The defaultValue is not the same, does not conformsToCommonAPI anymore
        if (!GfIsClose(rotPivChan.defValue, scalePivChan.defValue, 1e-9)) {
            conformsToCommonAPI = false;
        }

        // If opType, usdType or precision are not the same, does not conformsToCommonAPI anymore
        if (rotPivChan.opType != scalePivChan.opType
            || rotPivChan.usdOpType != scalePivChan.usdOpType
            || rotPivChan.precision != scalePivChan.precision) {
            conformsToCommonAPI = false;
        }

        if (conformsToCommonAPI) {
            // To Merge, we first rename rotatePivot and the scalePivot inverse
            // to pivot. Then we remove the scalePivot and the inverse of the
            // rotatePivot.
            //
            // This means that pivot and its inverse will wrap rotate and scale
            // since no other ops have been found
            //
            // NOTE: scalePivotIdx > rotPivotINVIdx
            _animChannels[rotPivotIdx].suffix = UsdMayaXformStackTokens->pivot;
            _animChannels[scalePivotINVIdx].suffix = UsdMayaXformStackTokens->pivot;
            _animChannels.erase(_animChannels.begin() + scalePivotIdx);
            _animChannels.erase(_animChannels.begin() + rotPivotINVIdx);
        }
    }
}

void UsdMayaTransformWriter::_WriteChannelsXformOps(const UsdGeomXformable& usdXformable)
{
    _MakeAnimChannelsUnique(usdXformable);

    // Loop over anim channel vector and create corresponding XFormOps
    // including the inverse ones if needed
    for (_AnimChannel& animChan : _animChannels) {
        animChan.op = usdXformable.AddXformOp(
            animChan.usdOpType, animChan.precision, animChan.suffix, animChan.isInverse);
        if (!animChan.op) {
            TF_CODING_ERROR("Could not add xform op");
            animChan.op = UsdGeomXformOp();
        }
    }
}

static bool
needsWorldspaceTransform(const UsdMayaJobExportArgs& exportArgs, const MFnTransform& iTrans)
{
    if (!exportArgs.worldspace)
        return false;

    return exportArgs.dagPaths.count(iTrans.dagPath()) > 0;
}

UsdMayaTransformWriter::UsdMayaTransformWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    // Even though we define an Xform here, it's OK for subclasses to
    // re-define the prim as another type.
    UsdGeomXform primSchema = UsdGeomXform::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = primSchema.GetPrim();
    TF_VERIFY(_usdPrim);

    // There are special cases where you might subclass UsdMayaTransformWriter
    // without actually having a transform (e.g. the internal
    // UsdMaya_FunctorPrimWriter), so accommodate those here.
    if (GetMayaObject().hasFn(MFn::kTransform)) {
        const MFnTransform transFn(GetDagPath());
        // Create a vector of _AnimChannels based on the Maya transformation
        // ordering
        const bool worldspace = needsWorldspaceTransform(_writeJobCtx.GetArgs(), transFn);
        _PushTransformStack(
            GetDagPath(), transFn, primSchema, !_GetExportArgs().timeSamples.empty(), worldspace);
        _WriteChannelsXformOps(primSchema);
    }

    _distanceConversionScalar
        = UsdMayaUtil::GetExportDistanceConversionScalar(jobCtx.GetArgs().metersPerUnit);
}

/* virtual */
void UsdMayaTransformWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    // There are special cases where you might subclass UsdMayaTransformWriter
    // without actually having a transform (e.g. the internal
    // UsdMaya_FunctorPrimWriter), so accomodate those here.
    if (GetMayaObject().hasFn(MFn::kTransform)) {
        // There are valid cases where we have a transform in Maya but not one
        // in USD, e.g. typeless defs or other container prims in USD.
        if (UsdGeomXformable xformSchema = UsdGeomXformable(_usdPrim)) {
            _ComputeXformOps(
                _animChannels,
                usdTime,
                _GetExportArgs().eulerFilter,
                &_previousRotates,
                _GetSparseValueWriter(),
                _distanceConversionScalar);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE