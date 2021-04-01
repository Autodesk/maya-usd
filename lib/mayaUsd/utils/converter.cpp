//
// Copyright 2020 Autodesk
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
#include "converter.h"

#include <mayaUsd/utils/colorSpace.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDataBlock.h>
#include <maya/MFnData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMatrixArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnUnitAttribute.h>

#include <unordered_map>

// clang-format off
/* Converter supported types

| SdfValueTypeName | USD C++ Type          | Maya Type                                                      | Maya C++ Type           | Helper TypeTrait   |
|:-----------------|:----------------------|:---------------------------------------------------------------|:------------------------|:-------------------|
| Bool             | bool                  | MFnNumericData::kBoolean                                       | bool                    | MakeMayaSimpleData |
| Int              | int                   | MFnNumericData::kInt                                           | int                     | MakeMayaSimpleData |
||
| String           | std::string           | MFnData::kString, MFn::kStringData                             | MString, MFnStringData  | MakeMayaFnData     |
||
| Float3           | GfVec3f               | MFnData::kNumeric, MFn::kNumericData, MFnNumericData::k3Float  | float3,  MFnNumericData | MakeMayaFnData     |
| Double3          | GfVec3d               | MFnData::kNumeric, MFn::kNumericData, MFnNumericData::k3Double | double3, MFnNumericData | MakeMayaFnData     |
| Color3f          | GfVec3f               | MFnData::kNumeric, MFn::kNumericData, MFnNumericData::k3Float  | float3,  MFnNumericData | MakeMayaFnData     |
| Color3d          | GfVec3d               | MFnData::kNumeric, MFn::kNumericData, MFnNumericData::k3Double | double3, MFnNumericData | MakeMayaFnData     |
||
| Matrix4d         | GfMatrix4d            | MFnData::kMatrix,  MFn::kMatrixData                            | MMatrix, MFnMatrixData  | MakeMayaFnData     |
||
| IntArray         | VtArray< int >        | MFnData::kIntArray, MFn::kIntArrayData                | MIntArray, MFnIntArrayData       | MakeMayaFnData     |
| Point3fArray     | VtArray< GfVec3f >    | MFnData::kPointArray, MFn::kPointArrayData            | MPointArray, MFnPointArrayData   | MakeMayaFnData     |
| Matrix4dArray    | VtArray< GfMatrix4d > | MFnData::kMatrixArray, MFn::kMatrixArrayData          | MMatrixArray, MFnMatrixArrayData | MakeMayaFnData     |

This table lists currently supported types for array attributes

| USD C++ Item Type | Maya C++ Item Type | Helper TypeTrait            |
|-------------------|--------------------|-----------------------------|
| GfMatrix4d        | MMatrix            | MakeUsdArray, MakeMayaArray |

Useful links when adding new types to the converter:
 https://github.com/PixarAnimationStudios/USD/blob/be1a80f8cb91133ac75e1fc2a2e1832cd10d91c8/pxr/usd/usd/doxygen/datatypes.dox#L35
 https://github.com/PixarAnimationStudios/USD/blob/be1a80f8cb91133ac75e1fc2a2e1832cd10d91c8/pxr/usd/sdf/types.h#L479
*/
// clang-format on

namespace MAYAUSD_NS_DEF {

namespace {
//---------------------------------------------------------------------------------
//! \brief Retrieve Maya's attribute numeric and unit data types
//! \return True on success, False otherwise.
bool getMayaAttributeNumericTypedAndUnitDataTypes(
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

//---------------------------------------------------------------------------------
//! \brief  Type tranformation class to obtain at compile time array type for a given Maya
//! templated type.
template <class T> struct MakeMayaArray
{
};

//! \brief  Type tranformation class to obtain at compile time array type for a given Maya
//! templated type.
template <> struct MakeMayaArray<MMatrix>
{
    using Type = MMatrixArray;
};

//! \brief  Type tranformation class to obtain at compile time array type for a given Maya
//! templated type.
template <> struct MakeMayaArray<MString>
{
    using Type = MStringArray;
};

//! \brief  Helper type to save on typing.
template <class T> using MakeMayaArrayT = typename MakeMayaArray<T>::Type;

//---------------------------------------------------------------------------------
//! \brief  Type tranformation class to obtain at compile time array type for a given Usd
//! templated type.
template <class T> struct MakeUsdArray
{
    using Type = VtArray<T>;
};

//! \brief  Helper type to save on typing.
template <class T> using MakeUsdArrayT = typename MakeUsdArray<T>::Type;

//---------------------------------------------------------------------------------
//! \brief  Type trait declaration for simple data types. Each specialized type will inherit
//! from std::true_type
//!         to enable compile time code generation.
template <class MAYA_Type> struct MakeMayaSimpleData : public std::false_type
{
};

//! \brief  Type trait for Maya's bool type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaSimpleData<bool> : public std::true_type
{
    using Type = bool;
    enum
    {
        kNumericType = MFnNumericData::kBoolean
    };

    static void get(const MDataHandle& handle, Type& value) { value = handle.asBool(); }

    static void set(MDataHandle& handle, const Type& value) { handle.setBool(value); }

    static void set(const MPlug& plug, MDGModifier& dst, const Type& value)
    {
        dst.newPlugValueBool(plug, value);
    }
};

//! \brief  Type trait for Maya's int type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaSimpleData<int> : public std::true_type
{
    using Type = int;
    enum
    {
        kNumericType = MFnNumericData::kInt
    };

    static void get(const MDataHandle& handle, Type& value) { value = handle.asInt(); }

    static void set(MDataHandle& handle, const Type& value) { handle.setInt(value); }

    static void set(const MPlug& plug, MDGModifier& dst, const Type& value)
    {
        dst.newPlugValueInt(plug, value);
    }
};

//---------------------------------------------------------------------------------
//! \brief  Type trait declaration for complex data types. Each specialized type will inherit
//! from std::true_type
//!         to enable compile time code generation.
template <class MAYA_Type> struct MakeMayaFnData : public std::false_type
{
};

//! \brief  Type trait for Maya's MMatrixArray type providing get and set methods for data
//! handle and plugs.
template <> struct MakeMayaFnData<MMatrixArray> : public std::true_type
{
    using Type = MMatrixArray;
    using FnType = MFnMatrixArrayData;
    enum
    {
        kDataType = MFnData::kMatrixArray
    };
    enum
    {
        kApiType = MFn::kMatrixArrayData
    };

    static MObject create(FnType& data) { return data.create(); }

    static void get(const FnType& data, Type& value) { data.copyTo(value); }

    static void set(FnType& data, const Type& value) { data.set(value); }

    static void get(const MDataHandle& handle, Type& value)
    {
        MObject dataObj = const_cast<MDataHandle&>(handle).data();
        FnType  dataFn(dataObj);
        get(dataFn, value);
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        FnType  dataFn;
        MObject dataObj = create(dataFn);
        set(dataFn, value);

        handle.setMObject(dataObj);
    }
};

//! \brief  Type trait for Maya's MIntArray type providing get and set methods for data handle
//! and plugs.
template <> struct MakeMayaFnData<MIntArray> : public std::true_type
{
    using Type = MIntArray;
    using FnType = MFnIntArrayData;
    enum
    {
        kDataType = MFnData::kIntArray
    };
    enum
    {
        kApiType = MFn::kIntArrayData
    };

    static MObject create(FnType& data) { return data.create(); }

    static void get(const FnType& data, Type& value) { data.copyTo(value); }

    static void set(FnType& data, const Type& value) { data.set(value); }

    static void get(const MDataHandle& handle, Type& value)
    {
        MObject dataObj = const_cast<MDataHandle&>(handle).data();
        FnType  dataFn(dataObj);
        get(dataFn, value);
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        FnType  dataFn;
        MObject dataObj = create(dataFn);
        set(dataFn, value);

        handle.setMObject(dataObj);
    }
};

//! \brief  Type trait for Maya's MPointArray type providing get and set methods for data handle
//! and plugs.
template <> struct MakeMayaFnData<MPointArray> : public std::true_type
{
    using Type = MPointArray;
    using FnType = MFnPointArrayData;
    enum
    {
        kDataType = MFnData::kPointArray
    };
    enum
    {
        kApiType = MFn::kPointArrayData
    };

    static MObject create(FnType& data) { return data.create(); }

    static void get(const FnType& data, Type& value) { data.copyTo(value); }

    static void set(FnType& data, const Type& value) { data.set(value); }

    static void get(const MDataHandle& handle, Type& value)
    {
        MObject dataObj = const_cast<MDataHandle&>(handle).data();
        FnType  dataFn(dataObj);
        get(dataFn, value);
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        FnType  dataFn;
        MObject dataObj = create(dataFn);
        set(dataFn, value);

        handle.setMObject(dataObj);
    }
};

//! \brief  Type trait for Maya's MMatrix type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaFnData<MMatrix> : public std::true_type
{
    using Type = MMatrix;
    using FnType = MFnMatrixData;
    enum
    {
        kDataType = MFnData::kMatrix
    };
    enum
    {
        kApiType = MFn::kMatrixData
    };

    static MObject create(FnType& data) { return data.create(); }

    static void get(const FnType& data, Type& value) { value = data.matrix(); }

    static void set(FnType& data, const Type& value) { data.set(value); }

    static void get(const MDataHandle& handle, Type& value)
    {
        MObject dataObj = const_cast<MDataHandle&>(handle).data();
        FnType  dataFn(dataObj);
        get(dataFn, value);
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        FnType  dataFn;
        MObject dataObj = create(dataFn);
        set(dataFn, value);

        handle.setMObject(dataObj);
    }
};

//! \brief  Type trait for Maya's float3 type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaFnData<float3> : public std::true_type
{
    using Type = float3;
    using FnType = MFnNumericData;
    enum
    {
        kDataType = MFnData::kNumeric
    };
    enum
    {
        kApiType = MFn::kNumericData
    };

    static MObject create(FnType& data) { return data.create(MFnNumericData::k3Float); }

    static void get(FnType& data, Type& value) { data.getData(value[0], value[1], value[2]); }

    static void set(FnType& data, const Type& value) { data.setData(value[0], value[1], value[2]); }

    static void get(const MDataHandle& handle, Type& value)
    {
        memcpy(&value, &handle.asFloat3(), sizeof(float3));
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        handle.set3Float(value[0], value[1], value[2]);
    }
};

//! \brief  Type trait for Maya's double3 type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaFnData<double3> : public std::true_type
{
    using Type = double3;
    using FnType = MFnNumericData;
    enum
    {
        kDataType = MFnData::kNumeric
    };
    enum
    {
        kApiType = MFn::kNumericData
    };

    static MObject create(FnType& data) { return data.create(MFnNumericData::k3Double); }

    static void get(FnType& data, Type& value) { data.getData(value[0], value[1], value[2]); }

    static void set(FnType& data, const Type& value) { data.setData(value[0], value[1], value[2]); }

    static void get(const MDataHandle& handle, Type& value)
    {
        memcpy(&value, &handle.asDouble3(), sizeof(double3));
    }

    static void set(MDataHandle& handle, const Type& value)
    {
        handle.set3Double(value[0], value[1], value[2]);
    }
};

//! \brief  Type trait for Maya's MString type providing get and set methods for data handle and
//! plugs.
template <> struct MakeMayaFnData<MString> : public std::true_type
{
    using Type = MString;
    using FnType = MFnStringData;
    enum
    {
        kDataType = MFnData::kString
    };
    enum
    {
        kApiType = MFn::kStringData
    };

    static MObject create(FnType& data) { return data.create(); }

    static void get(FnType& data, Type& value) { value = data.string(); }

    static void set(FnType& data, const Type& value) { data.set(value); }

    static void get(const MDataHandle& handle, Type& value) { value = handle.asString(); }

    static void set(MDataHandle& handle, const Type& value) { handle.setString(value); }
};

//---------------------------------------------------------------------------------
//! \brief  Helper classe to provide single a interface for data handles of simple and complex
//! types.
template <class MAYA_Type, class Enable = void> struct MDataHandleUtils
{
};

//! \brief  Specialization of helper classe to provide a single interface for data handles of
//! simple types.
template <class MAYA_Type>
struct MDataHandleUtils<
    MAYA_Type,
    typename std::enable_if<MakeMayaSimpleData<MAYA_Type>::value>::type>
{
    using SimpleTypeHelper = MakeMayaSimpleData<MAYA_Type>;

    static bool valid(const MDataHandle& handle)
    {
        return (handle.isNumeric() && handle.numericType() == SimpleTypeHelper::kNumericType);
    }

    static void get(const MDataHandle& handle, MAYA_Type& value)
    {
        SimpleTypeHelper::get(handle, value);
    }

    static void set(MDataHandle& handle, const MAYA_Type& value)
    {
        SimpleTypeHelper::set(handle, value);
    }
};

//! \brief  Specialization of helper classe to provide a single interface for data handles of
//! complex types.
template <class MAYA_Type>
struct MDataHandleUtils<MAYA_Type, typename std::enable_if<MakeMayaFnData<MAYA_Type>::value>::type>
{
    using FnTypeHelper = MakeMayaFnData<MAYA_Type>;

    static bool valid(const MDataHandle& handle)
    {
        return (handle.type() == FnTypeHelper::kDataType);
    }

    static void get(const MDataHandle& handle, MAYA_Type& value)
    {
        FnTypeHelper::get(handle, value);
    }

    static void set(MDataHandle& handle, const MAYA_Type& value)
    {
        FnTypeHelper::set(handle, value);
    }
};

//---------------------------------------------------------------------------------
//! \brief  Helper classe to provide single a interface for plugs of simple and complex types.
template <class MAYA_Type, class Enable = void> struct MPlugUtils
{
};

//! \brief  Specialization of helper classe to provide a single interface for plugs of simple
//! types.
template <class MAYA_Type>
struct MPlugUtils<MAYA_Type, typename std::enable_if<MakeMayaSimpleData<MAYA_Type>::value>::type>
{
    using SimpleTypeHelper = MakeMayaSimpleData<MAYA_Type>;

    static bool valid(const MPlug& plug)
    {
        return Converter::hasNumericType(plug, SimpleTypeHelper::kNumericType);
    }

    static void get(const MPlug& plug, MAYA_Type& value) { plug.getValue(value); }

    static void set(MPlug& plug, const MAYA_Type& value) { plug.setValue(value); }
};

//! \brief  Specialization of helper classe to provide a single interface for plugs of complex
//! types.
template <class MAYA_Type>
struct MPlugUtils<MAYA_Type, typename std::enable_if<MakeMayaFnData<MAYA_Type>::value>::type>
{
    using FnTypeHelper = MakeMayaFnData<MAYA_Type>;

    static bool valid(const MPlug& plug) { return Converter::hasAttrType(FnTypeHelper::kDataType); }

    static void get(const MPlug& plug, MAYA_Type& value)
    {
        typename FnTypeHelper::FnType dataFn(plug.asMObject());
        FnTypeHelper::get(dataFn, value);
    }

    static void set(MPlug& plug, const MAYA_Type& value)
    {
        typename FnTypeHelper::FnType dataFn;
        MObject                       dataObj = FnTypeHelper::create(dataFn);
        FnTypeHelper::set(dataFn, value);

        plug.setMObject(dataObj);
    }
};

//---------------------------------------------------------------------------------
//! \brief  Helper classe to provide single a interface for modifying plugs of simple and
//! complex types with DG modifier.
template <class MAYA_Type, class Enable = void> struct MDGModifierUtils
{
};

//! \brief  Specialization of helper classe to provide a single interface for modifying plugs of
//! simple types with DG modifier.
template <class MAYA_Type>
struct MDGModifierUtils<
    MAYA_Type,
    typename std::enable_if<MakeMayaSimpleData<MAYA_Type>::value>::type>
{
    using SimpleTypeHelper = MakeMayaSimpleData<MAYA_Type>;

    static void set(const MPlug& plug, MDGModifier& dst, const MAYA_Type& value)
    {
        SimpleTypeHelper::set(plug, dst, value);
    }
};

//! \brief  Specialization of helper classe to provide a single interface for modifying plugs of
//! complex types with DG modifier.
template <class MAYA_Type>
struct MDGModifierUtils<MAYA_Type, typename std::enable_if<MakeMayaFnData<MAYA_Type>::value>::type>
{
    using FnTypeHelper = MakeMayaFnData<MAYA_Type>;

    static void set(const MPlug& plug, MDGModifier& dst, const MAYA_Type& value)
    {
        typename FnTypeHelper::FnType dataFn;
        MObject                       dataObj = FnTypeHelper::create(dataFn);
        FnTypeHelper::set(dataFn, value);

        dst.newPlugValue(plug, dataObj);
    }
};

//! \brief  Specialization for MString class which has a different way of setting new string
//! value on a plug.
template <> struct MDGModifierUtils<MString>
{
    static void set(const MPlug& plug, MDGModifier& dst, const MString& value)
    {
        dst.newPlugValueString(plug, value);
    }
};

//---------------------------------------------------------------------------------
//! \brief  Switch used to conditionally enable or disable gamma correction support at compile
//! time for types
//!         supporting it.
enum class NeedsGammaCorrection
{
    kYes,
    kNo
};

//! \brief  Utility class for conversion between data handles and Usd.
template <class MAYA_Type, class USD_Type, NeedsGammaCorrection ColorCorrection>
struct MDataHandleConvert
{
    // MDataHandle <--> USD_Type
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kNo, void>::type
    convert(const USD_Type& src, MDataHandle& dst, const ConverterArgs&)
    {
        MAYA_Type tmpDst;
        TypedConverter<MAYA_Type, USD_Type>::convert(src, tmpDst);
        MDataHandleUtils<MAYA_Type>::set(dst, tmpDst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kNo, void>::type
    convert(const MDataHandle& src, USD_Type& dst, const ConverterArgs&)
    {
        MAYA_Type tmpSrc;
        MDataHandleUtils<MAYA_Type>::get(src, tmpSrc);
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, dst);
    }

    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kYes, void>::type
    convert(const USD_Type& src, MDataHandle& dst, const ConverterArgs& args)
    {
        MAYA_Type tmpDst;
        USD_Type  tmpSrc = args._doGammaCorrection ? MayaUsd::utils::ConvertLinearToMaya(src) : src;
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, tmpDst);
        MDataHandleUtils<MAYA_Type>::set(dst, tmpDst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kYes, void>::type
    convert(const MDataHandle& src, USD_Type& dst, const ConverterArgs& args)
    {
        MAYA_Type tmpSrc;
        MDataHandleUtils<MAYA_Type>::get(src, tmpSrc);
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, dst);

        if (args._doGammaCorrection)
            dst = MayaUsd::utils::ConvertMayaToLinear(dst);
    }

    // MDataHandle <--> UsdAttribute
    static void convert(const MDataHandle& src, UsdAttribute& dst, const ConverterArgs& args)
    {
        USD_Type tmpDst;
        convert(src, tmpDst, args);
        dst.Set<USD_Type>(tmpDst, args._timeCode);
    }
    static void convert(const UsdAttribute& src, MDataHandle& dst, const ConverterArgs& args)
    {
        USD_Type tmpSrc;
        src.Get<USD_Type>(&tmpSrc, args._timeCode);
        convert(tmpSrc, dst, args);
    }

    // MDataHandle <--> VtValue
    static void convert(const MDataHandle& src, VtValue& dst, const ConverterArgs& args)
    {
        USD_Type tmpDst;
        convert(src, tmpDst, args);
        dst = tmpDst;
    }
    static void convert(const VtValue& src, MDataHandle& dst, const ConverterArgs& args)
    {
        const USD_Type& tmpSrc = src.Get<USD_Type>();
        convert(tmpSrc, dst, args);
    }
};

//! \brief  Utility class for conversion between plugs and Usd.
template <class MAYA_Type, class USD_Type, NeedsGammaCorrection ColorCorrection> struct MPlugConvert
{
    // MPlug <--> USD_Type
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kNo, void>::type
    convert(const USD_Type& src, MPlug& dst, const ConverterArgs&)
    {
        MAYA_Type tmpDst;
        TypedConverter<MAYA_Type, USD_Type>::convert(src, tmpDst);
        MPlugUtils<MAYA_Type>::set(dst, tmpDst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kNo, void>::type
    convert(const MPlug& src, USD_Type& dst, const ConverterArgs&)
    {
        MAYA_Type tmpSrc;
        MPlugUtils<MAYA_Type>::get(src, tmpSrc);
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, dst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kNo, void>::type
    convert(const USD_Type& src, const MPlug& plug, MDGModifier& dst, const ConverterArgs&)
    {
        MAYA_Type tmpDst;
        TypedConverter<MAYA_Type, USD_Type>::convert(src, tmpDst);
        MDGModifierUtils<MAYA_Type>::set(plug, dst, tmpDst);
    }

    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kYes, void>::type
    convert(const USD_Type& src, MPlug& dst, const ConverterArgs& args)
    {
        MAYA_Type tmpDst;
        USD_Type  tmpSrc = args._doGammaCorrection ? MayaUsd::utils::ConvertLinearToMaya(src) : src;
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, tmpDst);

        MPlugUtils<MAYA_Type>::set(dst, tmpDst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kYes, void>::type
    convert(const MPlug& src, USD_Type& dst, const ConverterArgs& args)
    {
        MAYA_Type tmpSrc;
        MPlugUtils<MAYA_Type>::get(src, tmpSrc);
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, dst);

        if (args._doGammaCorrection)
            dst = MayaUsd::utils::ConvertMayaToLinear(dst);
    }
    template <NeedsGammaCorrection C = ColorCorrection>
    static typename std::enable_if<C == NeedsGammaCorrection::kYes, void>::type
    convert(const USD_Type& src, const MPlug& plug, MDGModifier& dst, const ConverterArgs& args)
    {
        MAYA_Type tmpDst;
        USD_Type  tmpSrc = args._doGammaCorrection ? MayaUsd::utils::ConvertLinearToMaya(src) : src;
        TypedConverter<MAYA_Type, USD_Type>::convert(tmpSrc, tmpDst);

        MDGModifierUtils<MAYA_Type>::set(plug, dst, tmpDst);
    }

    // MPlug <--> UsdAttribute
    static void convert(const MPlug& src, UsdAttribute& dst, const ConverterArgs& args)
    {
        USD_Type tmpDst;
        convert(src, tmpDst, args);
        dst.Set<USD_Type>(tmpDst, args._timeCode);
    }
    static void convert(const UsdAttribute& src, MPlug& dst, const ConverterArgs& args)
    {
        USD_Type tmpSrc;
        src.Get<USD_Type>(&tmpSrc, args._timeCode);
        convert(tmpSrc, dst, args);
    }
    static void
    convert(const UsdAttribute& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args)
    {
        USD_Type tmpSrc;
        src.Get<USD_Type>(&tmpSrc, args._timeCode);
        convert(tmpSrc, plug, dst, args);
    }

    // MPlug <--> VtValue
    static void convert(const MPlug& src, VtValue& dst, const ConverterArgs& args)
    {
        USD_Type tmpDst;
        convert(src, tmpDst, args);
        dst = tmpDst;
    }
    static void convert(const VtValue& src, MPlug& dst, const ConverterArgs& args)
    {
        const USD_Type& tmpSrc = src.Get<USD_Type>();
        convert(tmpSrc, dst, args);
    }
    static void
    convert(const VtValue& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args)
    {
        const USD_Type& tmpSrc = src.Get<USD_Type>();
        convert(tmpSrc, plug, dst, args);
    }
};

// Missing attribute method for MDataHandle prior to Maya 2020. For now disable all
// array converters to avoid complicating interface and error handling. If needed,
// we should be able to back-port the change to an update.
#if MAYA_API_VERSION >= 20200000
//! \brief  Utility class for conversion between array data handles and Usd.
template <class MAYA_Type, class USD_Type, NeedsGammaCorrection ColorCorrection>
struct MArrayDataHandleConvert
{
    using MAYA_TypeArray = MakeMayaArrayT<MAYA_Type>;
    using USD_TypeArray = MakeUsdArrayT<USD_Type>;
    using ElementConverter = MDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>;

    // MDataHandle <--> UsdAttribute
    static void convert(const MDataHandle& src, UsdAttribute& dst, const ConverterArgs& args)
    {
        MArrayDataHandle   srcArray(src);
        const unsigned int srcSize = srcArray.elementCount();

        USD_TypeArray tmpDst;
        tmpDst.resize(srcSize);

        srcArray.jumpToElement(0);
        for (unsigned int i = 0; i < srcSize; i++) {
            MDataHandle srcHandle = srcArray.inputValue();
            ElementConverter::convert(srcHandle, tmpDst[i], args);

            srcArray.next();
        }

        dst.Set<USD_TypeArray>(tmpDst, args._timeCode);
    }
    static void convert(const UsdAttribute& src, MDataHandle& dst, const ConverterArgs& args)
    {
        USD_TypeArray tmpSrc;
        src.Get<USD_TypeArray>(&tmpSrc, args._timeCode);
        const size_t srcSize = tmpSrc.size();

        MArrayDataHandle dstArray(dst);

        MDataBlock        dstDataBlock = dst.datablock();
        MObject           dstAttribute = dst.attribute();
        MArrayDataBuilder dstArrayBuilder(&dstDataBlock, dstAttribute, srcSize);

        for (size_t i = 0; i < srcSize; i++) {
            MDataHandle dstElement = dstArrayBuilder.addElement(i);
            ElementConverter::convert(tmpSrc[i], dstElement, args);
        }

        dstArray.set(dstArrayBuilder);
    }

    // MDataHandle <--> VtValue
    static void convert(const MDataHandle& src, VtValue& dst, const ConverterArgs& args)
    {
        MArrayDataHandle   srcArray(src);
        const unsigned int srcSize = srcArray.elementCount();

        USD_TypeArray tmpDst;
        tmpDst.resize(srcSize);

        srcArray.jumpToElement(0);
        for (unsigned int i = 0; i < srcSize; i++) {
            MDataHandle srcHandle = srcArray.inputValue();
            ElementConverter::convert(srcHandle, tmpDst[i], args);

            srcArray.next();
        }

        dst = tmpDst;
    }
    static void convert(const VtValue& src, MDataHandle& dst, const ConverterArgs& args)
    {
        const USD_TypeArray& tmpSrc = src.Get<USD_TypeArray>();
        const size_t         srcSize = tmpSrc.size();

        MArrayDataHandle dstArray(dst);

        MDataBlock        dstDataBlock = dst.datablock();
        MObject           dstAttribute = dst.attribute();
        MArrayDataBuilder dstArrayBuilder(&dstDataBlock, dstAttribute, srcSize);

        for (size_t i = 0; i < srcSize; i++) {
            MDataHandle dstElement = dstArrayBuilder.addElement(i);
            ElementConverter::convert(tmpSrc[i], dstElement, args);
        }

        dstArray.set(dstArrayBuilder);
    }
};

//! \brief  Utility class for conversion between array attribute plugs and Usd.
template <class MAYA_Type, class USD_Type, NeedsGammaCorrection ColorCorrection>
struct MArrayPlugConvert
{
    using MAYA_TypeArray = MakeMayaArrayT<MAYA_Type>;
    using USD_TypeArray = MakeUsdArrayT<USD_Type>;
    using ElementConverter = MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>;

    // MPlug <--> UsdAttribute
    static void convert(const MPlug& src, UsdAttribute& dst, const ConverterArgs& args)
    {
        const unsigned int srcSize = src.numElements();

        USD_TypeArray tmpDst;
        tmpDst.resize(srcSize);

        for (unsigned int i = 0; i < srcSize; i++) {
            MPlug srcElement = src.elementByPhysicalIndex(i);
            ElementConverter::convert(srcElement, tmpDst[i], args);
        }

        dst.Set<USD_TypeArray>(tmpDst, args._timeCode);
    }
    static void convert(const UsdAttribute& src, MPlug& dst, const ConverterArgs& args)
    {
        USD_TypeArray tmpSrc;
        src.Get<USD_TypeArray>(&tmpSrc, args._timeCode);

        const size_t srcSize = tmpSrc.size();
        dst.setNumElements(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            MPlug srcElement = dst.elementByLogicalIndex(i);
            ElementConverter::convert(tmpSrc[i], srcElement, args);
        }
    }
    static void
    convert(const UsdAttribute& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args)
    {
        USD_TypeArray tmpSrc;
        src.Get<USD_TypeArray>(&tmpSrc, args._timeCode);

        const size_t srcSize = tmpSrc.size();
        plug.setNumElements(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            MPlug srcElement = plug.elementByLogicalIndex(i);
            ElementConverter::convert(tmpSrc[i], srcElement, dst, args);
        }
    }

    // MPlug <--> VtValue
    static void convert(const MPlug& src, VtValue& dst, const ConverterArgs& args)
    {
        const unsigned int srcSize = src.numElements();

        USD_TypeArray tmpDst;
        tmpDst.resize(srcSize);

        for (unsigned int i = 0; i < srcSize; i++) {
            MPlug srcElement = src.elementByPhysicalIndex(i);
            ElementConverter::convert(srcElement, tmpDst[i], args);
        }

        dst = tmpDst;
    }
    static void convert(const VtValue& src, MPlug& dst, const ConverterArgs& args)
    {
        const USD_TypeArray& tmpSrc = src.Get<USD_TypeArray>();

        const size_t srcSize = tmpSrc.size();
        dst.setNumElements(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            MPlug srcElement = dst.elementByLogicalIndex(i);
            ElementConverter::convert(tmpSrc[i], srcElement, args);
        }
    }
    static void
    convert(const VtValue& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args)
    {
        const USD_TypeArray& tmpSrc = src.Get<USD_TypeArray>();

        const size_t srcSize = tmpSrc.size();
        plug.setNumElements(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            MPlug srcElement = plug.elementByLogicalIndex(i);
            ElementConverter::convert(tmpSrc[i], srcElement, dst, args);
        }
    }
};
#endif

//---------------------------------------------------------------------------------
//! \brief  Storage for generated instances of converters.
using ConvertStorage = std::unordered_map<SdfValueTypeName, const Converter, SdfValueTypeNameHash>;
} // namespace

//! \brief  Utility class responsible for generating all supported converters. This list
//! excludes all array attribute types.
struct Converter::GenerateConverters
{
    template <
        class MAYA_Type,
        class USD_Type,
        NeedsGammaCorrection ColorCorrection = NeedsGammaCorrection::kNo>
    static void createConverter(ConvertStorage& converters, const SdfValueTypeName& typeName)
    {
        converters.emplace(
            typeName,
            Converter(
                typeName,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::convert // MPlugToUsdAttrFn
                ,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::convert // UsdAttrToMPlugFn
                ,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // UsdAttrToMDGModifierFn
                ,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::convert // MPlugToVtValueFn
                ,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::convert // VtValueToMPlugFn
                ,
                &MPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // VtValueToMDGModifierFn
                ,
                &MDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MDataHandleToUsdAttrFn
                ,
                &MDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // UsdAttrToMDataHandleFn
                ,
                &MDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MDataHandleToVtValueFn
                ,
                &MDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // VtValueToMDataHandleFn
                ));
    }

    static ConvertStorage generate()
    {
        ConvertStorage converters;

        createConverter<bool, bool>(converters, SdfValueTypeNames->Bool);
        createConverter<int, int32_t>(converters, SdfValueTypeNames->Int);

        createConverter<MString, std::string>(converters, SdfValueTypeNames->String);
        createConverter<float3, GfVec3f>(converters, SdfValueTypeNames->Float3);
        createConverter<double3, GfVec3d>(converters, SdfValueTypeNames->Double3);
        createConverter<MMatrix, GfMatrix4d>(converters, SdfValueTypeNames->Matrix4d);

        createConverter<float3, GfVec3f, NeedsGammaCorrection::kYes>(
            converters, SdfValueTypeNames->Color3f);
        createConverter<double3, GfVec3d, NeedsGammaCorrection::kYes>(
            converters, SdfValueTypeNames->Color3d);

        createConverter<MIntArray, VtArray<int>>(converters, SdfValueTypeNames->IntArray);
        createConverter<MPointArray, VtArray<GfVec3f>>(converters, SdfValueTypeNames->Point3fArray);
        createConverter<MMatrixArray, VtArray<GfMatrix4d>>(
            converters, SdfValueTypeNames->Matrix4dArray);

        return converters;
    }
};

//! \brief  Utility class responsible for generating all supported array attribute converters.
struct Converter::GenerateArrayPlugConverters
{
#if MAYA_API_VERSION >= 20200000
    template <
        class MAYA_Type,
        class USD_Type,
        NeedsGammaCorrection ColorCorrection = NeedsGammaCorrection::kNo>
    static void createArrayConverter(ConvertStorage& converters, const SdfValueTypeName& typeName)
    {
        converters.emplace(
            typeName,
            Converter(
                typeName,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MPlugToUsdAttrFn
                ,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // UsdAttrToMPlugFn
                ,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // UsdAttrToMDGModifierFn
                ,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MPlugToVtValueFn
                ,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // VtValueToMPlugFn
                ,
                &MArrayPlugConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // VtValueToMDGModifierFn
                ,
                &MArrayDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MDataHandleToUsdAttrFn
                ,
                &MArrayDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // UsdAttrToMDataHandleFn
                ,
                &MArrayDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // MDataHandleToVtValueFn
                ,
                &MArrayDataHandleConvert<MAYA_Type, USD_Type, ColorCorrection>::
                    convert // VtValueToMDataHandleFn
                ));
    }

    static ConvertStorage generate()
    {
        ConvertStorage converters;
        createArrayConverter<MMatrix, GfMatrix4d>(converters, SdfValueTypeNames->Matrix4dArray);
        return converters;
    }
#else
    static ConvertStorage generate()
    {
        ConvertStorage converters;
        return converters;
    }
#endif
};

namespace {
//! \brief  Global storage for non-array attribute converters
ConvertStorage _converters = Converter::GenerateConverters::generate();
//! \brief  Global storage for array attribute converters
ConvertStorage _convertersForArrayPlug = Converter::GenerateArrayPlugConverters::generate();
} // namespace

const Converter* Converter::find(const SdfValueTypeName& typeName, bool isArrayPlug)
{
    auto& container = isArrayPlug ? _convertersForArrayPlug : _converters;

    auto findIt = container.find(typeName);
    if (findIt != container.end())
        return &findIt->second;
    else
        return nullptr;
}

const Converter* Converter::find(const MPlug& plug, const UsdAttribute& attr)
{
    auto valueTypeName = getUsdTypeName(plug, false);

    if (attr.GetTypeName() != valueTypeName)
        return nullptr;

    return find(valueTypeName, plug.isArray());
}

SdfValueTypeName
Converter::getUsdTypeName(const MPlug& attrPlug, const bool translateMayaDoubleToUsdSinglePrecision)
{
    // The various types of Maya attributes that can be created are spread
    // across a handful of MFn function sets. Some are a straightforward
    // translation such as MFnEnumAttributes or MFnMatrixAttributes, but others
    // are interesting mixes of function sets. For example, an attribute created
    // with addAttr and 'double' as the type results in an MFnNumericAttribute
    // while 'double2' as the type results in an MFnTypedAttribute that has
    // MFnData::Type kNumeric.

    MObject attrObj(attrPlug.attribute());
    if (attrObj.isNull()) {
        return SdfValueTypeName();
    }

    if (attrObj.hasFn(MFn::kEnumAttribute)) {
        return SdfValueTypeNames->Int;
    }

    MFnNumericData::Type   numericDataType;
    MFnData::Type          typedDataType;
    MFnUnitAttribute::Type unitDataType;

    getMayaAttributeNumericTypedAndUnitDataTypes(
        attrPlug, numericDataType, typedDataType, unitDataType);

    if (attrObj.hasFn(MFn::kMatrixAttribute)) {
        // Using type "fltMatrix" with addAttr results in an MFnMatrixAttribute
        // while using type "matrix" results in an MFnTypedAttribute with type
        // kMatrix, but the data is extracted the same way for both.
        typedDataType = MFnData::kMatrix;
    }

    // Deal with the MFnTypedAttribute attributes first. If it is numeric, it
    // will fall through to the numericDataType switch below.
    switch (typedDataType) {
    case MFnData::kString:
        // If the attribute is marked as a filename, then return Asset
        if (MFnAttribute(attrObj).isUsedAsFilename()) {
            return SdfValueTypeNames->Asset;
        } else {
            return SdfValueTypeNames->String;
        }
        break;
    case MFnData::kMatrix:
        // This must be a Matrix4d even if
        // translateMayaDoubleToUsdSinglePrecision is true, since Matrix4f
        // is not supported in Sdf.
        return SdfValueTypeNames->Matrix4d;
        break;
    case MFnData::kMatrixArray: return SdfValueTypeNames->Matrix4dArray; break;
    case MFnData::kStringArray: return SdfValueTypeNames->StringArray; break;
    case MFnData::kDoubleArray:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->FloatArray;
        } else {
            return SdfValueTypeNames->DoubleArray;
        }
        break;
    case MFnData::kFloatArray: return SdfValueTypeNames->FloatArray; break;
    case MFnData::kIntArray: return SdfValueTypeNames->IntArray; break;
    case MFnData::kPointArray:
        // Sdf does not have a 4-float point type, so we'll divide out W
        // and export the points as 3 floats.
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Point3fArray;
        } else {
            return SdfValueTypeNames->Point3dArray;
        }
        break;
    case MFnData::kVectorArray:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Vector3fArray;
        } else {
            return SdfValueTypeNames->Vector3dArray;
        }
        break;
    default: break;
    }

    switch (numericDataType) {
    case MFnNumericData::kBoolean: return SdfValueTypeNames->Bool; break;
    case MFnNumericData::kByte:
    case MFnNumericData::kChar:
    case MFnNumericData::kShort:
    // Maya treats longs the same as ints, since long is not
    // platform-consistent. The Maya constants MFnNumericData::kInt and
    // MFnNumericData::kLong have the same value. The same is true of
    // k2Int/k2Long and k3Int/k3Long.
    case MFnNumericData::kInt: return SdfValueTypeNames->Int; break;
    case MFnNumericData::k2Short:
    case MFnNumericData::k2Int: return SdfValueTypeNames->Int2; break;
    case MFnNumericData::k3Short:
    case MFnNumericData::k3Int: return SdfValueTypeNames->Int3; break;
    case MFnNumericData::kFloat: return SdfValueTypeNames->Float; break;
    case MFnNumericData::k2Float: return SdfValueTypeNames->Float2; break;
    case MFnNumericData::k3Float:
        if (MFnAttribute(attrObj).isUsedAsColor()) {
            return SdfValueTypeNames->Color3f;
        } else {
            return SdfValueTypeNames->Float3;
        }
        break;
    case MFnNumericData::kDouble:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Float;
        } else {
            return SdfValueTypeNames->Double;
        }
        break;
    case MFnNumericData::k2Double:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Float2;
        } else {
            return SdfValueTypeNames->Double2;
        }
        break;
    case MFnNumericData::k3Double:
        if (MFnAttribute(attrObj).isUsedAsColor()) {
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Color3f;
            } else {
                return SdfValueTypeNames->Color3d;
            }
        } else {
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Float3;
            } else {
                return SdfValueTypeNames->Double3;
            }
        }
        break;
    case MFnNumericData::k4Double:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Float4;
        } else {
            return SdfValueTypeNames->Double4;
        }
        break;
    default: break;
    }

    switch (unitDataType) {
    case MFnUnitAttribute::kAngle:
    case MFnUnitAttribute::kDistance:
        if (translateMayaDoubleToUsdSinglePrecision) {
            return SdfValueTypeNames->Float;
        } else {
            return SdfValueTypeNames->Double;
        }
        break;
    default: break;
    }

    return SdfValueTypeName();
}

} // namespace MAYAUSD_NS_DEF
