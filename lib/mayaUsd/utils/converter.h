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
#ifndef MAYAUSD_CONVERTER_H
#define MAYAUSD_CONVERTER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/utils/userTaggedAttribute.h>

#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MDGModifier.h>
#include <maya/MDataHandle.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

/*! \brief  Converter arguments are passed together using this convenient struct.

    \note   Not all arguments will be applicable to all conversions.
 */
struct MAYAUSD_CORE_PUBLIC ConverterArgs final
{
    //! When conversion needs to read / write to time, following argument will be used.
    UsdTimeCode _timeCode { UsdTimeCode::Default() };
    //! For types supporting gamma correction, following argument controls if it should be
    //! applied
    bool _doGammaCorrection { true };
};

/*! \brief Performant converter, providing several interfaces to translate between Maya and Usd
   data types.

    Instances of converter class are initialized once for all supported types. To use the
   converter, use static find methods to validate if the required converter is available.
   Pointer to converter class is safe to cache in acceleration structures to reduce runtime cost
   when the same conversion happens often (e.g. every frame).

    The converter will avoid using expensive or intermediate conversion like for example
   type-erased Get method on Usd attribute. This means VtValue is not used as an intermediate
   type unless it's the source or destination type.

    The converter takes destination containers as a reference to allow fetching the data to the
   same container.

    Type checking should be performed when retrieving data converter, no extra penalty at
   runtime should be necessary.

    Usage example:
    const Converter* myConverter = Converter::find(mayaPlug, usdAttribute);
    if(myConverter) {
        // ... some code to fetch data handle, or usd attribute, or plugs...
        // ...

        // Configure if dealing with time, gamma correction, etc
        ConverterArgs args;

        // Convert data handle value directly to usd attribute
        myConverter->convert(dataHandle, usdAttribute, args);

        // OR, convert usd attribute value directly to plug
        myConverter->convert(usdAttribute, plug, args);

        // OR, convert plug value to VtValue
        myConverter->convert(plug, vtValue, args);
    }

    \note See the list of currently implemented data converters in converter.cpp file
*/
class MAYAUSD_CORE_PUBLIC Converter final
{
    using MPlugToUsdAttrFn
        = std::add_pointer<void(const MPlug&, UsdAttribute&, const ConverterArgs&)>::type;
    using UsdAttrToMPlugFn
        = std::add_pointer<void(const UsdAttribute&, MPlug&, const ConverterArgs&)>::type;
    using UsdAttrToMDGModifierFn = std::add_pointer<
        void(const UsdAttribute&, MPlug&, MDGModifier&, const ConverterArgs&)>::type;
    using MPlugToVtValueFn
        = std::add_pointer<void(const MPlug&, VtValue&, const ConverterArgs&)>::type;
    using VtValueToMPlugFn
        = std::add_pointer<void(const VtValue&, MPlug&, const ConverterArgs&)>::type;
    using VtValueToMDGModifierFn
        = std::add_pointer<void(const VtValue&, MPlug&, MDGModifier&, const ConverterArgs&)>::type;

    using MDataHandleToUsdAttrFn
        = std::add_pointer<void(const MDataHandle&, UsdAttribute&, const ConverterArgs&)>::type;
    using UsdAttrToMDataHandleFn
        = std::add_pointer<void(const UsdAttribute&, MDataHandle&, const ConverterArgs&)>::type;
    using MDataHandleToVtValueFn
        = std::add_pointer<void(const MDataHandle&, VtValue&, const ConverterArgs&)>::type;
    using VtValueToMDataHandleFn
        = std::add_pointer<void(const VtValue&, MDataHandle&, const ConverterArgs&)>::type;

    //! \brief  Construct new converter. This should never be called directly other than during
    //! generation of all supported converters.
    Converter(
        const SdfValueTypeName& usdTypeName,
        MPlugToUsdAttrFn        plugToAttr,
        UsdAttrToMPlugFn        attrToPlug,
        UsdAttrToMDGModifierFn  attrToModifier,
        MPlugToVtValueFn        plugToVtValue,
        VtValueToMPlugFn        vtValueToPlug,
        VtValueToMDGModifierFn  vtValueToModifier,
        MDataHandleToUsdAttrFn  handleToAttr,
        UsdAttrToMDataHandleFn  attrToHandle,
        MDataHandleToVtValueFn  handleToVtValue,
        VtValueToMDataHandleFn  vtValueoHandle)
        : _usdTypeName(usdTypeName)
        , _plugToAttr(plugToAttr)
        , _attrToPlug(attrToPlug)
        , _attrToModifier(attrToModifier)
        , _plugToVtValue { plugToVtValue }
        , _vtValueToPlug { vtValueToPlug }
        , _vtValueToModifier(vtValueToModifier)
        , _handleToAttr(handleToAttr)
        , _attrToHandle(attrToHandle)
        , _handleToVtValue { handleToVtValue }
        , _vtValueToHandle { vtValueoHandle }
    {
    }

    //! \brief  Default constructor makes no sense for this class
    Converter() = delete;

public:
    //! \brief  Nothing special to do in the destructor.
    ~Converter() = default;

    //! \brief  Look for converter which allows translation for given \p typeName
    //! \return Valid pointer if conversion is supported.
    static const Converter* find(const SdfValueTypeName& typeName, bool isArrayPlug);
    //! \brief  Look for converter which allows translation for given pair of Maya's plug and
    //! Usd attribute. \return Valid pointer if conversion is supported.
    static const Converter* find(const MPlug& plug, const UsdAttribute& attr);

    //! \brief  Return sdf value type name this converter is registered for.
    const SdfValueTypeName& usdType() const { return _usdTypeName; }

    //! \brief  Validate if given pair of Maya's plug and Usd attribute is supported by this
    //! converter.
    bool validate(const MPlug& plug, const UsdAttribute& usdAttr) const
    {
        return (usdAttr.GetTypeName() == _usdTypeName)
            && (getUsdTypeName(plug, false) == _usdTypeName);
    }

    //! \brief  Read current value from given plug and set it on destination attribute. Use
    //! arguments to control converter behavior, like the time for setting value on attribute.
    void convert(const MPlug& src, UsdAttribute& dst, const ConverterArgs& args) const
    {
        _plugToAttr(src, dst, args);
    }
    //! \brief  Read current value from given attribute and set it on destination plug. Use
    //! arguments to control converter behavior.
    void convert(const UsdAttribute& src, MPlug& dst, const ConverterArgs& args) const
    {
        _attrToPlug(src, dst, args);
    }
    //! \brief  Read current value from given attribute and set it on destination plug using
    //! provided DG modifier. Use arguments to control converter behavior.
    void
    convert(const UsdAttribute& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args) const
    {
        _attrToModifier(src, plug, dst, args);
    }

    //! \brief  Read current value from given plug and set it on destination VtValue. Use
    //! arguments to control converter behavior.
    void convert(const MPlug& src, VtValue& dst, const ConverterArgs& args) const
    {
        _plugToVtValue(src, dst, args);
    }
    //! \brief  Read current value from given VtValue and set it on destination plug. Use
    //! arguments to control converter behavior.
    void convert(const VtValue& src, MPlug& dst, const ConverterArgs& args) const
    {
        _vtValueToPlug(src, dst, args);
    }
    //! \brief  Read current value from given VtValue and set it on destination plug using
    //! provided DG modifier. Use arguments to control converter behavior.
    void convert(const VtValue& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args) const
    {
        _vtValueToModifier(src, plug, dst, args);
    }

    //! \brief  Read current value from given data handle and set it on destination attribute.
    //! Use arguments to control converter behavior, like the time for setting value on
    //! attribute.
    void convert(const MDataHandle& src, UsdAttribute& dst, const ConverterArgs& args) const
    {
        _handleToAttr(src, dst, args);
    }
    //! \brief  Read current value from given attribute and set it on destination data handle.
    //! Use arguments to control converter behavior, like the time for reading value from
    //! attribute.
    void convert(const UsdAttribute& src, MDataHandle& dst, const ConverterArgs& args) const
    {
        _attrToHandle(src, dst, args);
    }

    //! \brief  Read current value from given data handle and set it on destination VtValue. Use
    //! arguments to control converter behavior.
    void convert(const MDataHandle& src, VtValue& dst, const ConverterArgs& args) const
    {
        _handleToVtValue(src, dst, args);
    }
    //! \brief  Read current value from given VtValue and set it on destination data handle. Use
    //! arguments to control converter behavior.
    void convert(const VtValue& src, MDataHandle& dst, const ConverterArgs& args) const
    {
        _vtValueToHandle(src, dst, args);
    }

    //! \brief  Return whether a plug's attribute is a typed attribute with given type.
    static bool hasAttrType(const MPlug& plug, MFnData::Type type)
    {
        MObject object = plug.attribute();
        if (!object.hasFn(MFn::kTypedAttribute)) {
            return false;
        }

        MFnTypedAttribute attr(object);
        return attr.attrType() == type;
    }

    //! \brief  Return whether a plug's attribute is a numeric attribute with given type.
    static bool hasNumericType(const MPlug& plug, MFnNumericData::Type type)
    {
        MObject object = plug.attribute();
        if (!object.hasFn(MFn::kNumericAttribute)) {
            return false;
        }

        MFnNumericAttribute attr(object);
        return attr.unitType() == type;
    }

    //! \brief  Return whether a plug's attribute is an enum type.
    static bool hasEnumType(const MPlug& plug)
    {
        MObject object = plug.attribute();
        return object.hasFn(MFn::kEnumAttribute);
    }

    /*! \brief  Get the SdfValueTypeName that corresponds to the given plug \p attrPlug.

     If \p translateMayaDoubleToUsdSinglePrecision is true, Maya plugs that
     contain double data will return the appropriate float-based type.
     Otherwise, the type returned will be the appropriate double-based type.
    */
    static SdfValueTypeName getUsdTypeName(
        const MPlug& attrPlug,
        const bool   translateMayaDoubleToUsdSinglePrecision
        = UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    struct GenerateConverters;
    struct GenerateArrayPlugConverters;

private:
    //! Usd type this converter is operating with.
    const SdfValueTypeName& _usdTypeName;

    // MPlug <--> UsdAttribute
    //! Pointer to a function responsible for converting plug value to given usd attribute.
    MPlugToUsdAttrFn _plugToAttr { nullptr };
    //! Pointer to a function responsible for converting usd attribute value to a given plug.
    UsdAttrToMPlugFn _attrToPlug { nullptr };
    //! Pointer to a function responsible for converting usd attribute value to a given plug
    //! using provided DG modifier.
    UsdAttrToMDGModifierFn _attrToModifier { nullptr };

    // MPlug <--> VtValue
    //! Pointer to a function responsible for converting plug value to VtValue.
    MPlugToVtValueFn _plugToVtValue { nullptr };
    //! Pointer to a function responsible for converting VtValue to a given plug.
    VtValueToMPlugFn _vtValueToPlug { nullptr };
    //! Pointer to a function responsible for converting VtValue to a given plug using provided
    //! DG modifier.
    VtValueToMDGModifierFn _vtValueToModifier { nullptr };

    // MDataBlock <--> UsdAttribute
    //! Pointer to a function responsible for converting data handle value to given usd
    //! attribute.
    MDataHandleToUsdAttrFn _handleToAttr { nullptr };
    //! Pointer to a function responsible for converting usd attribute value to a given data
    //! handle.
    UsdAttrToMDataHandleFn _attrToHandle { nullptr };

    // MDataBlock <--> VtValue
    //! Pointer to a function responsible for converting data handle value to VtValue.
    MDataHandleToVtValueFn _handleToVtValue { nullptr };
    //! Pointer to a function responsible for converting VtValue to a given data handle.
    VtValueToMDataHandleFn _vtValueToHandle { nullptr };
};

//! \brief  Declaration of base typed converter struct. Each supported type will specialize this
//! struct and provide
//!         two conversion methods. All specializations will be available in converter header
//!         file to allow reuse when both types are already known.
template <class MAYA_Type, class USD_Type, class Enable = void> struct TypedConverter
{
};

//! \brief  Specialization of TypedConverter for case where MAYA_Type and USD_Type are exactly
//! the same type.
template <class MAYA_Type, class USD_Type>
struct TypedConverter<
    MAYA_Type,
    USD_Type,
    typename std::enable_if<std::is_same<MAYA_Type, USD_Type>::value>::type>
{
    static void convert(const USD_Type& src, MAYA_Type& dst) { dst = src; }
};

//! \brief  Specialization of TypedConverter for float3 <--> GfVec3f
template <> struct TypedConverter<float3, GfVec3f>
{
    static void convert(const GfVec3f& src, float3& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    static void convert(const float3& src, GfVec3f& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
};

//! \brief  Specialization of TypedConverter for double3 <--> GfVec3d
template <> struct TypedConverter<double3, GfVec3d>
{
    static void convert(const GfVec3d& src, double3& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    static void convert(const double3& src, GfVec3d& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
};

//! \brief  Specialization of TypedConverter for MPoint <--> GfVec3f
template <> struct TypedConverter<MPoint, GfVec3f>
{
    static void convert(const GfVec3f& src, MPoint& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    static void convert(const MPoint& src, GfVec3f& dst)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
};

//! \brief  Specialization of TypedConverter for MString <--> std::string
template <> struct TypedConverter<MString, std::string>
{
    static void convert(const std::string& src, MString& dst) { dst = src.c_str(); }
    static void convert(const MString& src, std::string& dst) { dst = src.asChar(); }
};

//! \brief  Specialization of TypedConverter for MMatrix <--> GfMatrix4d
template <> struct TypedConverter<MMatrix, GfMatrix4d>
{
    static void convert(const GfMatrix4d& src, MMatrix& dst)
    {
        memcpy(dst[0], src.GetArray(), sizeof(double) * 16);
    }
    static void convert(const MMatrix& src, GfMatrix4d& dst)
    {
        memcpy(dst.GetArray(), src[0], sizeof(double) * 16);
    }
};

//! \brief  Specialization of TypedConverter for MIntArray <--> VtArray<int>
template <> struct TypedConverter<MIntArray, VtArray<int>>
{
    static void convert(const VtArray<int>& src, MIntArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            dst[i] = src[i];
        }
    }
    static void convert(const MIntArray& src, VtArray<int>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            dst[i] = src[i];
        }
    }
};

//! \brief  Specialization of TypedConverter for MPointArray <--> VtArray<GfVec3f>
template <> struct TypedConverter<MPointArray, VtArray<GfVec3f>>
{
    static void convert(const VtArray<GfVec3f>& src, MPointArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            TypedConverter<MPoint, GfVec3f>::convert(src[i], dst[i]);
        }
    }
    static void convert(const MPointArray& src, VtArray<GfVec3f>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            TypedConverter<MPoint, GfVec3f>::convert(src[i], dst[i]);
        }
    }
};

//! \brief  Specialization of TypedConverter for MMatrixArray <--> VtArray<GfMatrix4d>
template <> struct TypedConverter<MMatrixArray, VtArray<GfMatrix4d>>
{
    static void convert(const VtArray<GfMatrix4d>& src, MMatrixArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            TypedConverter<MMatrix, GfMatrix4d>::convert(src[i], dst[i]);
        }
    }
    static void convert(const MMatrixArray& src, VtArray<GfMatrix4d>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for (size_t i = 0; i < srcSize; i++) {
            TypedConverter<MMatrix, GfMatrix4d>::convert(src[i], dst[i]);
        }
    }
};

} // namespace MAYAUSD_NS_DEF

#endif
