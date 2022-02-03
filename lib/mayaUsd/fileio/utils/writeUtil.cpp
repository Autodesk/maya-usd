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
#include "writeUtil.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/userTaggedAttribute.h>
#include <mayaUsd/utils/colorSpace.h>
#include <mayaUsd/utils/converter.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/inherits.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdRi/statementsAPI.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

#include <maya/MAnimControl.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include <string>
#include <vector>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    PIXMAYA_WRITE_UV_AS_FLOAT2,
    false,
    "Set to true to write uv sets as Float2Array types and set to false to "
    "write Texture Coordinate value types (TexCoord2h, TexCoord2f, "
    "TexCoord2d, TexCoord3h, TexCoord3f, TexCoord3d and their associated "
    "Array types)");

static bool _GetMayaAttributeNumericTypedAndUnitDataTypes(
    const MPlug&            attrPlug,
    MFnNumericData::Type&   numericDataType,
    MFnData::Type&          typedDataType,
    MFnUnitAttribute::Type& unitDataType)
{
    numericDataType = MFnNumericData::kInvalid;
    typedDataType = MFnData::kInvalid;
    unitDataType = MFnUnitAttribute::kInvalid;

    MObject attrObj(attrPlug.attribute());
    if (attrObj.isNull()) {
        return false;
    }

    if (attrObj.hasFn(MFn::kNumericAttribute)) {
        MFnNumericAttribute numericAttrFn(attrObj);
        numericDataType = numericAttrFn.unitType();
    } else if (attrObj.hasFn(MFn::kTypedAttribute)) {
        MFnTypedAttribute typedAttrFn(attrObj);
        typedDataType = typedAttrFn.attrType();

        if (typedDataType == MFnData::kNumeric) {
            // Inspect the type of the data itself to find the actual type.
            MObject plugObj = attrPlug.asMObject();
            if (plugObj.hasFn(MFn::kNumericData)) {
                MFnNumericData numericDataFn(plugObj);
                numericDataType = numericDataFn.numericType();
            }
        }
    } else if (attrObj.hasFn(MFn::kUnitAttribute)) {
        MFnUnitAttribute unitAttrFn(attrObj);
        unitDataType = unitAttrFn.unitType();
    }

    return true;
}

bool UsdMayaWriteUtil::WriteUVAsFloat2()
{
    static const bool writeUVAsFloat2 = TfGetEnvSetting(PIXMAYA_WRITE_UV_AS_FLOAT2);
    return writeUVAsFloat2;
}

/* static */
UsdAttribute UsdMayaWriteUtil::GetOrCreateUsdAttr(
    const MPlug&       attrPlug,
    const UsdPrim&     usdPrim,
    const std::string& attrName,
    const bool         custom,
    const bool         translateMayaDoubleToUsdSinglePrecision)
{
    UsdAttribute usdAttr;

    if (!usdPrim) {
        return usdAttr;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken usdAttrNameToken(attrName);
    if (usdAttrNameToken.IsEmpty()) {
        TF_RUNTIME_ERROR(
            "Invalid USD attribute name '%s' for Maya plug '%s'",
            attrName.c_str(),
            attrPlug.name().asChar());
        return usdAttr;
    }

    // See if the USD attribute already exists. If so, return it.
    usdAttr = usdPrim.GetAttribute(usdAttrNameToken);
    if (usdAttr) {
        return usdAttr;
    }

    const SdfValueTypeName& typeName
        = Converter::getUsdTypeName(attrPlug, translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        usdAttr = usdPrim.CreateAttribute(usdAttrNameToken, typeName, custom);
    }

    return usdAttr;
}

/* static */
UsdGeomPrimvar UsdMayaWriteUtil::GetOrCreatePrimvar(
    const MPlug&       attrPlug,
    UsdGeomImageable&  imageable,
    const std::string& primvarName,
    const TfToken&     interpolation,
    const int          elementSize,
    const bool         translateMayaDoubleToUsdSinglePrecision)
{
    UsdGeomPrimvar primvar;

    if (!imageable) {
        return primvar;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken primvarNameToken(primvarName);
    if (primvarNameToken.IsEmpty()) {
        TF_RUNTIME_ERROR(
            "Invalid primvar name '%s' for Maya plug '%s'",
            primvarName.c_str(),
            attrPlug.name().asChar());
        return primvar;
    }

    // See if the primvar already exists. If so, return it.
    primvar = imageable.GetPrimvar(primvarNameToken);
    if (primvar) {
        return primvar;
    }

    const SdfValueTypeName& typeName
        = Converter::getUsdTypeName(attrPlug, translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        primvar = imageable.CreatePrimvar(primvarNameToken, typeName, interpolation, elementSize);
    }

    return primvar;
}

/* static */
UsdAttribute UsdMayaWriteUtil::GetOrCreateUsdRiAttribute(
    const MPlug&       attrPlug,
    const UsdPrim&     usdPrim,
    const std::string& attrName,
    const std::string& nameSpace,
    const bool         translateMayaDoubleToUsdSinglePrecision)
{
    UsdAttribute usdAttr;

    if (!usdPrim) {
        return usdAttr;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken riAttrNameToken(attrName);
    if (riAttrNameToken.IsEmpty()) {
        TF_RUNTIME_ERROR(
            "Invalid UsdRi attribute name '%s' for Maya plug '%s'",
            attrName.c_str(),
            attrPlug.name().asChar());
        return usdAttr;
    }

    UsdRiStatementsAPI riStatements(usdPrim);

    // See if a UsdRi attribute with this name already exists. If so, return it.
    // XXX: There isn't currently API for looking for a specific UsdRi attribute
    // by name, so we have to get them all and then see if one matches.
    const std::vector<UsdProperty>& riAttrs = riStatements.GetRiAttributes(nameSpace);
    TF_FOR_ALL(iter, riAttrs)
    {
        if (iter->GetBaseName() == riAttrNameToken) {
            // Re-get the attribute from the prim so we can return it as a
            // UsdAttribute rather than a UsdProperty.
            return usdPrim.GetAttribute(iter->GetName());
        }
    }

    const SdfValueTypeName& typeName
        = Converter::getUsdTypeName(attrPlug, translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        riStatements = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdRiStatementsAPI>(usdPrim);
        usdAttr = riStatements.CreateRiAttribute(riAttrNameToken, typeName.GetType(), nameSpace);
    }

    return usdAttr;
}

/// Converts a vec from display to linear color if its role is color.
template <typename T>
static VtValue _ConvertVec(const T& val, const TfToken& role, const bool linearizeColors)
{
    return VtValue(
        ((role == SdfValueRoleNames->Color) && linearizeColors)
            ? MayaUsd::utils::ConvertMayaToLinear(val)
            : val);
}

VtValue UsdMayaWriteUtil::GetVtValue(
    const MPlug&            attrPlug,
    const SdfValueTypeName& typeName,
    const bool              linearizeColors)
{
    const TfType  type = typeName.GetType();
    const TfToken role = typeName.GetRole();
    ;
    return GetVtValue(attrPlug, type, role, linearizeColors);
}
VtValue UsdMayaWriteUtil::GetVtValue(
    const MPlug&   attrPlug,
    const TfType&  type,
    const TfToken& role,
    const bool     linearizeColors)
{
    // We perform a similar set of type-infererence acrobatics here as we do
    // in Converter::getUsdTypeName(). See the comments there for more detail on a
    // few type-related oddities.

    MObject attrObj(attrPlug.attribute());

    if (attrObj.hasFn(MFn::kEnumAttribute)) {
        return VtValue(attrPlug.asInt());
    }

    MFnNumericData::Type   numericDataType;
    MFnData::Type          typedDataType;
    MFnUnitAttribute::Type unitDataType;

    _GetMayaAttributeNumericTypedAndUnitDataTypes(
        attrPlug, numericDataType, typedDataType, unitDataType);

    if (attrObj.hasFn(MFn::kMatrixAttribute)) {
        typedDataType = MFnData::kMatrix;
    }

    // For the majority of things, we don't care about the role, just about
    // the type, e.g. we import normal3f/vector3f/float3 the same.
    // We do care about colors and points because those can be specially-marked
    // in Maya.
    switch (typedDataType) {
    case MFnData::kString: {
        MFnStringData     stringDataFn(attrPlug.asMObject());
        const std::string usdVal(stringDataFn.string().asChar());
        if (type.IsA<SdfAssetPath>()) {
            return VtValue(SdfAssetPath(usdVal));
        } else if (type.IsA<std::string>()) {
            return VtValue(usdVal);
        } else if (type.IsA<TfToken>()) {
            return VtValue(TfToken(usdVal));
        }
        break;
    }
    case MFnData::kMatrix: {
        if (type.IsA<GfMatrix4d>()) {
            MFnMatrixData    matrixDataFn(attrPlug.asMObject());
            const GfMatrix4d usdVal(matrixDataFn.matrix().matrix);
            return VtValue(usdVal);
        }
        break;
    }
    case MFnData::kStringArray: {
        if (type.IsA<VtStringArray>()) {
            MFnStringArrayData stringArrayDataFn(attrPlug.asMObject());
            VtStringArray      usdVal(stringArrayDataFn.length());
            for (unsigned int i = 0; i < stringArrayDataFn.length(); ++i) {
                usdVal[i] = std::string(stringArrayDataFn[i].asChar());
            }
            return VtValue(usdVal);
        } else if (type.IsA<VtTokenArray>()) {
            MFnStringArrayData stringArrayDataFn(attrPlug.asMObject());
            VtTokenArray       usdVal(stringArrayDataFn.length());
            for (unsigned int i = 0; i < stringArrayDataFn.length(); ++i) {
                usdVal[i] = TfToken(stringArrayDataFn[i].asChar());
            }
            return VtValue(usdVal);
        } else if (type.IsA<SdfStringListOp>()) {
            MFnStringArrayData       stringArrayDataFn(attrPlug.asMObject());
            std::vector<std::string> prepended(stringArrayDataFn.length());
            for (unsigned int i = 0; i < stringArrayDataFn.length(); ++i) {
                prepended[i] = std::string(stringArrayDataFn[i].asChar());
            }
            SdfStringListOp listOp;
            listOp.SetPrependedItems(prepended);
            return VtValue(listOp);
        } else if (type.IsA<SdfTokenListOp>()) {
            MFnStringArrayData stringArrayDataFn(attrPlug.asMObject());
            TfTokenVector      prepended(stringArrayDataFn.length());
            for (unsigned int i = 0; i < stringArrayDataFn.length(); ++i) {
                prepended[i] = TfToken(stringArrayDataFn[i].asChar());
            }
            SdfTokenListOp listOp;
            listOp.SetPrependedItems(prepended);
            return VtValue(listOp);
        }
        break;
    }
    case MFnData::kDoubleArray: {
        MFnDoubleArrayData doubleArrayDataFn(attrPlug.asMObject());
        if (type.IsA<VtFloatArray>()) {
            VtFloatArray usdVal(doubleArrayDataFn.length());
            for (unsigned int i = 0; i < doubleArrayDataFn.length(); ++i) {
                usdVal[i] = (float)doubleArrayDataFn[i];
            }
            return VtValue(usdVal);
        } else if (type.IsA<VtDoubleArray>()) {
            VtDoubleArray usdVal(doubleArrayDataFn.length());
            for (unsigned int i = 0; i < doubleArrayDataFn.length(); ++i) {
                usdVal[i] = doubleArrayDataFn[i];
            }
            return VtValue(usdVal);
        }
        break;
    }
    case MFnData::kFloatArray: {
        if (type.IsA<VtFloatArray>()) {
            MFnFloatArrayData floatArrayDataFn(attrPlug.asMObject());
            VtFloatArray      usdVal(floatArrayDataFn.length());
            for (unsigned int i = 0; i < floatArrayDataFn.length(); ++i) {
                usdVal[i] = floatArrayDataFn[i];
            }
            return VtValue(usdVal);
        }
        break;
    }
    case MFnData::kIntArray: {
        if (type.IsA<VtIntArray>()) {
            MFnIntArrayData intArrayDataFn(attrPlug.asMObject());
            VtIntArray      usdVal(intArrayDataFn.length());
            for (unsigned int i = 0; i < intArrayDataFn.length(); ++i) {
                usdVal[i] = intArrayDataFn[i];
            }
            return VtValue(usdVal);
        }
        break;
    }
    case MFnData::kPointArray: {
        MFnPointArrayData pointArrayDataFn(attrPlug.asMObject());
        if (type.IsA<VtVec3fArray>()) {
            VtVec3fArray usdVal(pointArrayDataFn.length());
            for (unsigned int i = 0; i < pointArrayDataFn.length(); ++i) {
                MPoint tmpMayaVal = pointArrayDataFn[i];
                if (tmpMayaVal.w != 0) {
                    tmpMayaVal.cartesianize();
                }
                usdVal[i]
                    = GfVec3f((float)tmpMayaVal[0], (float)tmpMayaVal[1], (float)tmpMayaVal[2]);
            }
            return VtValue(usdVal);
        } else if (type.IsA<VtVec3dArray>()) {
            VtVec3dArray usdVal(pointArrayDataFn.length());
            for (unsigned int i = 0; i < pointArrayDataFn.length(); ++i) {
                MPoint tmpMayaVal = pointArrayDataFn[i];
                if (tmpMayaVal.w != 0) {
                    tmpMayaVal.cartesianize();
                }
                usdVal[i] = GfVec3d(tmpMayaVal[0], tmpMayaVal[1], tmpMayaVal[2]);
            }
            return VtValue(usdVal);
        }
        break;
    }
    case MFnData::kVectorArray: {
        MFnVectorArrayData vectorArrayDataFn(attrPlug.asMObject());
        if (type.IsA<VtVec3fArray>()) {
            VtVec3fArray usdVal(vectorArrayDataFn.length());
            for (unsigned int i = 0; i < vectorArrayDataFn.length(); ++i) {
                MVector tmpMayaVal = vectorArrayDataFn[i];
                usdVal[i]
                    = GfVec3f((float)tmpMayaVal[0], (float)tmpMayaVal[1], (float)tmpMayaVal[2]);
            }
            return VtValue(usdVal);
        } else if (type.IsA<VtVec3dArray>()) {
            VtVec3dArray usdVal(vectorArrayDataFn.length());
            for (unsigned int i = 0; i < vectorArrayDataFn.length(); ++i) {
                MVector tmpMayaVal = vectorArrayDataFn[i];
                usdVal[i] = GfVec3d(tmpMayaVal[0], tmpMayaVal[1], tmpMayaVal[2]);
            }
            return VtValue(usdVal);
        }
        break;
    }
    default: break;
    }

    switch (numericDataType) {
    case MFnNumericData::kBoolean: {
        if (type.IsA<bool>()) {
            const bool usdVal(attrPlug.asBool());
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::kByte:
    case MFnNumericData::kChar: {
        if (type.IsA<int>()) {
            const int usdVal(attrPlug.asChar());
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::kShort: {
        if (type.IsA<int>()) {
            const int usdVal(attrPlug.asShort());
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::kInt: {
        if (type.IsA<int>()) {
            const int usdVal(attrPlug.asInt());
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::k2Short: {
        if (type.IsA<GfVec2i>()) {
            short          tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return VtValue(GfVec2i(tmp1, tmp2));
        }
        break;
    }
    case MFnNumericData::k2Int: {
        if (type.IsA<GfVec2i>()) {
            int            tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return VtValue(GfVec2i(tmp1, tmp2));
        }
        break;
    }
    case MFnNumericData::k3Short: {
        if (type.IsA<GfVec3i>()) {
            short          tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return VtValue(GfVec3i(tmp1, tmp2, tmp3));
        }
        break;
    }
    case MFnNumericData::k3Int: {
        if (type.IsA<GfVec3i>()) {
            int            tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return VtValue(GfVec3i(tmp1, tmp2, tmp3));
        }
        break;
    }
    case MFnNumericData::kFloat: {
        if (type.IsA<float>()) {
            const float usdVal(attrPlug.asFloat());
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::k2Float: {
        if (type.IsA<GfVec2f>()) {
            float          tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return VtValue(GfVec2f(tmp1, tmp2));
        }
        break;
    }
    case MFnNumericData::k3Float: {
        if (type.IsA<GfVec3f>()) {
            float          tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return _ConvertVec(GfVec3f(tmp1, tmp2, tmp3), role, linearizeColors);
        }
        break;
    }
    case MFnNumericData::kDouble: {
        const double usdVal(attrPlug.asDouble());
        if (type.IsA<float>()) {
            return VtValue((float)usdVal);
        } else if (type.IsA<double>()) {
            return VtValue(usdVal);
        }
        break;
    }
    case MFnNumericData::k2Double: {
        double         tmp1, tmp2;
        MFnNumericData numericDataFn(attrPlug.asMObject());
        numericDataFn.getData(tmp1, tmp2);
        if (type.IsA<GfVec2f>()) {
            return VtValue(GfVec2f((float)tmp1, (float)tmp2));
        } else if (type.IsA<GfVec2d>()) {
            return VtValue(GfVec2d(tmp1, tmp2));
        }
        break;
    }
    case MFnNumericData::k3Double: {
        double         tmp1, tmp2, tmp3;
        MFnNumericData numericDataFn(attrPlug.asMObject());
        numericDataFn.getData(tmp1, tmp2, tmp3);
        if (type.IsA<GfVec3f>()) {
            return _ConvertVec(
                GfVec3f((float)tmp1, (float)tmp2, (float)tmp3), role, linearizeColors);
        } else if (type.IsA<GfVec3d>()) {
            return _ConvertVec(GfVec3d(tmp1, tmp2, tmp3), role, linearizeColors);
        }
        break;
    }
    case MFnNumericData::k4Double: {
        double         tmp1, tmp2, tmp3, tmp4;
        MFnNumericData numericDataFn(attrPlug.asMObject());
        numericDataFn.getData(tmp1, tmp2, tmp3, tmp4);
        if (type.IsA<GfVec4f>()) {
            return _ConvertVec(
                GfVec4f((float)tmp1, (float)tmp2, (float)tmp3, (float)tmp4), role, linearizeColors);
        } else if (type.IsA<GfVec4d>()) {
            return _ConvertVec(GfVec4d(tmp1, tmp2, tmp3, tmp4), role, linearizeColors);
        } else if (type.IsA<GfQuatf>()) {
            float   re = tmp1;
            GfVec3f im(tmp2, tmp3, tmp4);
            return VtValue(GfQuatf(re, im));
        } else if (type.IsA<GfQuatd>()) {
            double  re = tmp1;
            GfVec3d im(tmp2, tmp3, tmp4);
            return VtValue(GfQuatd(re, im));
        }
        break;
    }
    default: break;
    }

    switch (unitDataType) {
    case MFnUnitAttribute::kAngle:
    case MFnUnitAttribute::kDistance:
        if (type.IsA<float>()) {
            const float usdVal(attrPlug.asFloat());
            return VtValue(usdVal);
        } else if (type.IsA<double>()) {
            const double usdVal(attrPlug.asDouble());
            return VtValue(usdVal);
        }
        break;
    default: break;
    }

    return VtValue();
}

namespace {
bool _IsAnimated(const MPlug& p)
{
    if (p.isDestination()) {
        return true;
    }
    if (p.isCompound()) {
        const unsigned int numChildren = p.numChildren();
        for (unsigned int i = 0; i < numChildren; ++i) {
            if (_IsAnimated(p.child(i))) {
                return true;
            }
        }
    }
    return false;
}
} // namespace

bool UsdMayaWriteUtil::SetUsdAttr(
    const MPlug&               attrPlug,
    const UsdAttribute&        usdAttr,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    if (!usdAttr || attrPlug.isNull()) {
        return false;
    }

    if (!(usdTime.IsDefault() || _IsAnimated(attrPlug))) {
        return true;
    }

    MDGContext      dgContext(MAnimControl::currentTime());
    MDGContextGuard contextGuard(dgContext);
    VtValue         val = GetVtValue(attrPlug, usdAttr.GetTypeName());
    if (val.IsEmpty()) {
        return false;
    }

    return SetAttribute(usdAttr, val, usdTime, valueWriter);
}

// This method inspects the JSON blob stored in the
// 'USD_UserExportedAttributesJson' attribute on the Maya node mayaNode and
// exports any attributes specified there onto usdPrim at time usdTime.
// The JSON should contain an object that maps Maya attribute names to other
// JSON objects that contain metadata about how to export the attribute into
// USD. For example:
//
//    {
//        "myMayaAttributeOne": {
//        },
//        "myMayaAttributeTwo": {
//            "usdAttrName": "my:namespace:attributeTwo"
//        },
//        "attributeAsPrimvar": {
//            "usdAttrType": "primvar"
//        },
//        "attributeAsVertexInterpPrimvar": {
//            "usdAttrType": "primvar",
//            "interpolation": "vertex"
//        },
//        "attributeAsRibAttribute": {
//            "usdAttrType": "usdRi"
//        },
//        "doubleAttributeAsFloatAttribute": {
//            "translateMayaDoubleToUsdSinglePrecision": true
//        }
//    }
//
// If the attribute metadata contains a value for "usdAttrName", the attribute
// will be given that name in USD. Otherwise, the Maya attribute name will be
// used for primvars and UsdRi attributes, or the Maya attribute name prepended
// with the "userProperties" namespace will be used for regular USD attributes.
// Maya attributes in the JSON will be processed in sorted order, and any
// USD attribute name collisions will be resolved by using the first attribute
// visited and warning about subsequent attribute tags.
//
bool UsdMayaWriteUtil::WriteUserExportedAttributes(
    const MObject&             mayaNode,
    const UsdPrim&             usdPrim,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    std::vector<UsdMayaUserTaggedAttribute> exportedAttributes
        = UsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(mayaNode);
    for (const UsdMayaUserTaggedAttribute& attr : exportedAttributes) {
        const std::string& usdAttrName = attr.GetUsdName();
        const TfToken&     usdAttrType = attr.GetUsdType();
        const TfToken&     interpolation = attr.GetUsdInterpolation();
        const bool         translateMayaDoubleToUsdSinglePrecision
            = attr.GetTranslateMayaDoubleToUsdSinglePrecision();
        const MPlug& attrPlug = attr.GetMayaPlug();
        UsdAttribute usdAttr;

        if (usdAttrType == UsdMayaUserTaggedAttributeTokens->USDAttrTypePrimvar) {
            UsdGeomImageable imageable(usdPrim);
            if (!imageable) {
                TF_RUNTIME_ERROR(
                    "Cannot create primvar for non-UsdGeomImageable USD "
                    "prim <%s>",
                    usdPrim.GetPath().GetText());
                continue;
            }
            UsdGeomPrimvar primvar = UsdMayaWriteUtil::GetOrCreatePrimvar(
                attrPlug,
                imageable,
                usdAttrName,
                interpolation,
                -1,
                translateMayaDoubleToUsdSinglePrecision);
            if (primvar) {
                usdAttr = primvar.GetAttr();
            }
        } else if (usdAttrType == UsdMayaUserTaggedAttributeTokens->USDAttrTypeUsdRi) {
            usdAttr = UsdMayaWriteUtil::GetOrCreateUsdRiAttribute(
                attrPlug, usdPrim, usdAttrName, "user", translateMayaDoubleToUsdSinglePrecision);
        } else {
            usdAttr = UsdMayaWriteUtil::GetOrCreateUsdAttr(
                attrPlug, usdPrim, usdAttrName, true, translateMayaDoubleToUsdSinglePrecision);
        }

        if (usdAttr) {
            if (!UsdMayaWriteUtil::SetUsdAttr(attrPlug, usdAttr, usdTime, valueWriter)) {
                TF_RUNTIME_ERROR(
                    "Could not set value for attribute <%s>", usdAttr.GetPath().GetText());
                continue;
            }
        } else {
            TF_RUNTIME_ERROR(
                "Could not create attribute '%s' for USD prim <%s>",
                usdAttrName.c_str(),
                usdPrim.GetPath().GetText());
            continue;
        }
    }

    return true;
}

/* static */
bool UsdMayaWriteUtil::WriteMetadataToPrim(const MObject& mayaObject, const UsdPrim& prim)
{
    UsdMayaAdaptor adaptor(mayaObject);
    if (!adaptor) {
        return false;
    }

    for (const auto& keyValue : adaptor.GetAllAuthoredMetadata()) {
        prim.SetMetadata(keyValue.first, keyValue.second);
    }
    return true;
}

/* static */
bool UsdMayaWriteUtil::WriteAPISchemaAttributesToPrim(
    const MObject&              mayaObject,
    const UsdPrim&              prim,
    const UsdMayaJobExportArgs& jobExportArgs,
    const UsdTimeCode&          usdTime,
    UsdUtilsSparseValueWriter*  valueWriter)
{
    UsdMayaAdaptor adaptor(mayaObject, jobExportArgs);
    if (!adaptor) {
        return false;
    }

    for (const TfToken& schemaName : adaptor.GetAppliedSchemas()) {
        if (const UsdMayaSchemaAdaptorPtr schemaAdaptor = adaptor.GetSchemaByName(schemaName)) {
#if PXR_VERSION > 2005
            prim.AddAppliedSchema(schemaName);
#endif
            if (schemaAdaptor->CopyToPrim(prim, usdTime, valueWriter)) {
                continue;
            }
            for (const TfToken& attrName : schemaAdaptor->GetAuthoredAttributeNames()) {
                if (const UsdMayaAttributeAdaptor attrAdaptor
                    = schemaAdaptor->GetAttribute(attrName)) {
                    VtValue value;
                    if (attrAdaptor.Get(&value)) {
                        const SdfAttributeSpecHandle attrDef = attrAdaptor.GetAttributeDefinition();
                        UsdAttribute                 attr = prim.CreateAttribute(
                            attrDef->GetNameToken(),
                            attrDef->GetTypeName(),
                            /*custom*/ false,
                            attrDef->GetVariability());
                        SetAttribute(attr, value, usdTime, valueWriter);
                    }
                }
            }
        }
    }
    return true;
}

/* static */
size_t UsdMayaWriteUtil::WriteSchemaAttributesToPrim(
    const MObject&              object,
    const UsdPrim&              prim,
    const TfType&               schemaType,
    const std::vector<TfToken>& attributeNames,
    const UsdTimeCode&          usdTime,
    UsdUtilsSparseValueWriter*  valueWriter)
{
    UsdMayaSchemaAdaptorPtr schema;
    if (UsdMayaAdaptor adaptor = UsdMayaAdaptor(object)) {
        schema = adaptor.GetSchemaOrInheritedSchema(schemaType);
    }
    if (!schema) {
        return 0;
    }

    size_t count = 0;
    for (const TfToken& attrName : attributeNames) {
        VtValue                value;
        SdfAttributeSpecHandle attrDef;
        if (UsdMayaAttributeAdaptor attr = schema->GetAttribute(attrName)) {
            attr.Get(&value);
            attrDef = attr.GetAttributeDefinition();
        }

        if (!value.IsEmpty() && attrDef) {
            UsdAttribute attr = prim.CreateAttribute(
                attrDef->GetNameToken(),
                attrDef->GetTypeName(),
                /*custom*/ false,
                attrDef->GetVariability());
            if (SetAttribute(attr, value, usdTime, valueWriter)) {
                count++;
            }
        }
    }

    return count;
}

// static
bool UsdMayaWriteUtil::WriteClassInherits(
    const UsdPrim&                  prim,
    const std::vector<std::string>& classNamesToInherit)
{
    if (classNamesToInherit.empty()) {
        return true;
    }

    for (const auto& className : classNamesToInherit) {
        if (!TfIsValidIdentifier(className)) {
            return false;
        }
    }

    UsdStagePtr stage = prim.GetStage();

    auto inherits = prim.GetInherits();
    for (const auto& className : classNamesToInherit) {
        const SdfPath inheritPath = SdfPath::AbsoluteRootPath().AppendChild(TfToken(className));
        UsdPrim       classPrim = stage->CreateClassPrim(inheritPath);
        inherits.AddInherit(classPrim.GetPath());
    }
    return true;
}

template <typename MArrayType, typename M, typename V>
static VtArray<V>
_MapMayaToVtArray(const MArrayType& mayaArray, const std::function<V(const M)> mapper)
{
    VtArray<V> vtArray(mayaArray.length());
    for (unsigned int i = 0; i < mayaArray.length(); ++i) {
        vtArray[i] = mapper(mayaArray[i]);
    }
    return vtArray;
}

// static
bool UsdMayaWriteUtil::WriteArrayAttrsToInstancer(
    MFnArrayAttrsData&           inputPointsData,
    const UsdGeomPointInstancer& instancer,
    const size_t                 numPrototypes,
    const UsdTimeCode&           usdTime,
    UsdUtilsSparseValueWriter*   valueWriter)
{
    MStatus status;

    // We need to figure out how many instances there are. Some arrays are
    // sparse (contain less values than there are instances), so just loop
    // through all the arrays and assume that there are as many instances as the
    // size of the largest array.
    unsigned int       numInstances = 0;
    const MStringArray channels = inputPointsData.list();
    for (unsigned int i = 0; i < channels.length(); ++i) {
        MFnArrayAttrsData::Type type;
        if (inputPointsData.checkArrayExist(channels[i], type)) {
            switch (type) {
            case MFnArrayAttrsData::kVectorArray: {
                MVectorArray arr = inputPointsData.vectorArray(channels[i]);
                numInstances = std::max(numInstances, arr.length());
            } break;
            case MFnArrayAttrsData::kDoubleArray: {
                MDoubleArray arr = inputPointsData.doubleArray(channels[i]);
                numInstances = std::max(numInstances, arr.length());
            } break;
            case MFnArrayAttrsData::kIntArray: {
                MIntArray arr = inputPointsData.intArray(channels[i]);
                numInstances = std::max(numInstances, arr.length());
            } break;
            case MFnArrayAttrsData::kStringArray: {
                MStringArray arr = inputPointsData.stringArray(channels[i]);
                numInstances = std::max(numInstances, arr.length());
            } break;
            default: break;
            }
        }
    }

    // Most Maya instancer data sources provide id's. If this once doesn't, then
    // just skip the id's attr because it's optional in USD, and we don't have
    // a good way to generate sane id's.
    // Note that we need to populate indicesOrIds in either case; the schema
    // interprets some attributes (e.g. visibility) as referring to id's if
    // present or indices otherwise.
    VtInt64Array            indicesOrIds;
    MFnArrayAttrsData::Type type;
    if (inputPointsData.checkArrayExist("id", type) && type == MFnArrayAttrsData::kDoubleArray) {
        const MDoubleArray id = inputPointsData.doubleArray("id", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        indicesOrIds = _MapMayaToVtArray<MDoubleArray, double, int64_t>(
            id, [](double x) { return (int64_t)x; });
        SetAttribute(instancer.CreateIdsAttr(), indicesOrIds, usdTime, valueWriter);
    } else {
        // Skip writing the id's, but still generate the indicesOrIds array.
        for (size_t i = 0; i < numInstances; ++i) {
            indicesOrIds.push_back(i);
        }
    }

    // Export the rest of the per-instance array attrs.
    // Some attributes might be missing elements; pad the array according to
    // Maya's fallback behavior up to the numInstances.
    if (inputPointsData.checkArrayExist("objectIndex", type)
        && type == MFnArrayAttrsData::kDoubleArray) {
        const MDoubleArray objectIndex = inputPointsData.doubleArray("objectIndex", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        VtIntArray vtArray
            = _MapMayaToVtArray<MDoubleArray, double, int>(objectIndex, [numPrototypes](double x) {
                  if (x < numPrototypes) {
                      return (int)x;
                  } else {
                      // Return the *last* prototype if out of bounds.
                      return (int)numPrototypes - 1;
                  }
              });
        SetAttribute(instancer.CreateProtoIndicesAttr(), vtArray, usdTime, valueWriter);
    } else {
        VtIntArray vtArray;
        vtArray.assign(numInstances, 0);
        SetAttribute(instancer.CreateProtoIndicesAttr(), vtArray, usdTime, valueWriter);
    }

    if (inputPointsData.checkArrayExist("position", type)
        && type == MFnArrayAttrsData::kVectorArray) {
        const MVectorArray position = inputPointsData.vectorArray("position", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        VtVec3fArray vtArray = _MapMayaToVtArray<MVectorArray, const MVector&, GfVec3f>(
            position, [](const MVector& v) { return GfVec3f(v.x, v.y, v.z); });
        SetAttribute(instancer.CreatePositionsAttr(), vtArray, usdTime, valueWriter);
    } else {
        VtVec3fArray vtArray;
        vtArray.assign(numInstances, GfVec3f(0.0f));
        SetAttribute(instancer.CreatePositionsAttr(), vtArray, usdTime, valueWriter);
    }

    if (inputPointsData.checkArrayExist("rotation", type)
        && type == MFnArrayAttrsData::kVectorArray) {
        const MVectorArray rotation = inputPointsData.vectorArray("rotation", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        VtQuathArray vtArray = _MapMayaToVtArray<MVectorArray, const MVector&, GfQuath>(
            rotation, [](const MVector& v) {
                GfRotation rot = GfRotation(GfVec3d::XAxis(), v.x)
                    * GfRotation(GfVec3d::YAxis(), v.y) * GfRotation(GfVec3d::ZAxis(), v.z);
                return GfQuath(rot.GetQuat());
            });
        SetAttribute(instancer.CreateOrientationsAttr(), vtArray, usdTime, valueWriter);
    } else {
        VtQuathArray vtArray;
        vtArray.assign(numInstances, GfQuath(0.0f));
        SetAttribute(instancer.CreateOrientationsAttr(), vtArray, usdTime, valueWriter);
    }

    if (inputPointsData.checkArrayExist("scale", type) && type == MFnArrayAttrsData::kVectorArray) {
        const MVectorArray scale = inputPointsData.vectorArray("scale", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        VtVec3fArray vtArray = _MapMayaToVtArray<MVectorArray, const MVector&, GfVec3f>(
            scale, [](const MVector& v) { return GfVec3f(v.x, v.y, v.z); });
        SetAttribute(instancer.CreateScalesAttr(), vtArray, usdTime, valueWriter);
    } else {
        VtVec3fArray vtArray;
        vtArray.assign(numInstances, GfVec3f(1.0));
        SetAttribute(instancer.CreateScalesAttr(), vtArray, usdTime, valueWriter);
    }

    // Note: Maya stores visibility as an array of doubles, one corresponding
    // to each instance. USD stores visibility as a sparse array of only the
    // particular id's (or indices) to be invis'ed.
    // Visibility isn't required, so skip authoring if it doesn't exist.
    if (inputPointsData.checkArrayExist("visibility", type)
        && type == MFnArrayAttrsData::kDoubleArray) {
        const MDoubleArray visibility = inputPointsData.doubleArray("visibility", &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        VtInt64Array invisibleIds;
        for (size_t i = 0; i < visibility.length(); ++i) {
            if (visibility[i] == 0.0) {
                if (i < indicesOrIds.size()) {
                    invisibleIds.push_back(indicesOrIds[i]);
                }
            }
        }
        SetAttribute(instancer.CreateInvisibleIdsAttr(), invisibleIds, usdTime, valueWriter);
    }

    return true;
}

bool UsdMayaWriteUtil::ReadMayaAttribute(
    const MFnDependencyNode& depNode,
    const MString&           name,
    std::string*             val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug   plug = depNode.findPlug(name);
        MObject dataObj;

        if ((plug.getValue(dataObj) == MS::kSuccess) && (dataObj.hasFn(MFn::kStringData))) {

            (*val) = std::string(plug.asString().asChar());
            return true;
        }
    }

    return false;
}

bool UsdMayaWriteUtil::ReadMayaAttribute(
    const MFnDependencyNode&  depNode,
    const MString&            name,
    std::vector<std::string>* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug   plug = depNode.findPlug(name);
        MObject dataObj;

        if ((plug.getValue(dataObj) == MS::kSuccess) && (dataObj.hasFn(MFn::kStringArrayData))) {

            MFnStringArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MStringArray arrayValues = dData.array();
                size_t       numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = std::string(arrayValues[i].asChar());
                }
                return true;
            }
        }
    }

    return false;
}

bool UsdMayaWriteUtil::ReadMayaAttribute(
    const MFnDependencyNode& depNode,
    const MString&           name,
    VtIntArray*              val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug   plug = depNode.findPlug(name);
        MObject dataObj;

        if ((plug.getValue(dataObj) == MS::kSuccess) && (dataObj.hasFn(MFn::kIntArrayData))) {

            MFnIntArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MIntArray arrayValues = dData.array();
                size_t    numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = arrayValues[i];
                }
                return true;
            }
        }
    }

    return false;
}

bool UsdMayaWriteUtil::ReadMayaAttribute(
    const MFnDependencyNode& depNode,
    const MString&           name,
    VtFloatArray*            val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug   plug = depNode.findPlug(name);
        MObject dataObj;

        if ((plug.getValue(dataObj) == MS::kSuccess) && (dataObj.hasFn(MFn::kDoubleArrayData))) {

            MFnDoubleArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MDoubleArray arrayValues = dData.array();
                size_t       numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = arrayValues[i];
                }
                return true;
            }
        }
    }

    return false;
}

bool UsdMayaWriteUtil::ReadMayaAttribute(
    const MFnDependencyNode& depNode,
    const MString&           name,
    VtVec3fArray*            val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug   plug = depNode.findPlug(name);
        MObject dataObj;

        if ((plug.getValue(dataObj) == MS::kSuccess) && (dataObj.hasFn(MFn::kVectorArrayData))) {

            MFnVectorArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MVectorArray arrayValues = dData.array();
                size_t       numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i].Set(arrayValues[i][0], arrayValues[i][1], arrayValues[i][2]);
                }
                return true;
            }
        }
    }

    return false;
}

std::vector<double> UsdMayaWriteUtil::GetTimeSamples(
    const GfInterval&       frameRange,
    const std::set<double>& subframeOffsets,
    const double            stride)
{
    std::vector<double> samples;

    // Error if stride is <= 0.0.
    if (stride <= 0.0) {
        TF_RUNTIME_ERROR("stride (%f) is not greater than 0", stride);
        return samples;
    }

    // Only warn if subframe offsets are outside the stride. Resulting time
    // samples are still sane.
    for (const double t : subframeOffsets) {
        if (t <= -stride) {
            TF_WARN("subframe offset (%f) <= -stride (-%f)", t, stride);
        } else if (t >= stride) {
            TF_WARN("subframe offset (%f) >= stride (%f)", t, stride);
        }
    }

    // Early-out if this is an empty range.
    if (frameRange.IsEmpty()) {
        return samples;
    }

    // Iterate over all possible times and sample offsets.
    static const std::set<double> zeroOffset = { 0.0 };
    const std::set<double>& actualOffsets = subframeOffsets.empty() ? zeroOffset : subframeOffsets;
    double                  currentTime = frameRange.GetMin();
    while (frameRange.Contains(currentTime)) {
        for (const double offset : actualOffsets) {
            samples.push_back(currentTime + offset);
        }
        currentTime += stride;
    }

    // Need to sort list before returning to make sure it's in time order.
    // This is mainly important for if there's a subframe offset outside the
    // interval (-stride, stride).
    std::sort(samples.begin(), samples.end());
    return samples;
}

PXR_NAMESPACE_CLOSE_SCOPE
