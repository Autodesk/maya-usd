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

#include <pxr/pxr.h>

#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <maya/MDataHandle.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MMatrix.h>
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>
#include <maya/MMatrixArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDGModifier.h>

#include <mayaUsd/base/api.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
    
struct MAYAUSD_CORE_PUBLIC ConverterArgs final
{
    UsdTimeCode _timeCode{UsdTimeCode::Default()};
    bool        _doGammaCorrection{true};
};
    
class MAYAUSD_CORE_PUBLIC Converter final
{
public:
    using MPlugToUsdAttrFn
        = std::add_pointer<void(const MPlug&, UsdAttribute&, const ConverterArgs&)>::type;
    using UsdAttrToMPlugFn
        = std::add_pointer<void(const UsdAttribute&, MPlug&, const ConverterArgs&)>::type;
    using UsdAttrToMDGModifierFn
        = std::add_pointer<void(const UsdAttribute&, MPlug&, MDGModifier&, const ConverterArgs&)>::type;
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
    
    Converter(const SdfValueTypeName& typeName, MPlugToUsdAttrFn plugToAttr,
        UsdAttrToMPlugFn attrToPlug, UsdAttrToMDGModifierFn attrToModifier,
        MPlugToVtValueFn plugToVtValue, VtValueToMPlugFn vtValueToPlug,
        VtValueToMDGModifierFn vtValueToModifier,
        MDataHandleToUsdAttrFn handleToAttr,
        UsdAttrToMDataHandleFn attrToHandle,
        MDataHandleToVtValueFn handleToVtValue,
        VtValueToMDataHandleFn vtValueoHandle) : _typeName(typeName),
        _plugToAttr(plugToAttr), _attrToPlug(attrToPlug),
        _attrToModifier(attrToModifier), _plugToVtValue { plugToVtValue },
        _vtValueToPlug { vtValueToPlug }, _vtValueToModifier(vtValueToModifier),
        _handleToAttr(handleToAttr), _attrToHandle(attrToHandle),
        _handleToVtValue { handleToVtValue }, _vtValueoHandle { vtValueoHandle }
    {}
    
    Converter() = delete;
    ~Converter() = default;
    
    static const Converter* find(const SdfValueTypeName& typeName, bool isArrayPlug);
    static const Converter* find(const MPlug& plug, const UsdAttribute& attr);
    
    const SdfValueTypeName& type() const
    { return _typeName; }
    
    bool validate(const MPlug& plug, const UsdAttribute& usdAttr, const bool translateMayaDoubleToUsdSinglePrecision) const
    { return (usdAttr.GetTypeName() == _typeName) && (getUsdTypeName(plug,translateMayaDoubleToUsdSinglePrecision) == _typeName); }
    
    void convert(const MPlug& src, UsdAttribute& dst, const ConverterArgs& args) const
    { _plugToAttr(src,dst,args); }
    void convert(const UsdAttribute& src, MPlug& dst, const ConverterArgs& args) const
    { _attrToPlug(src,dst,args); }
    void convert(const UsdAttribute& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args) const
    { _attrToModifier(src,plug,dst,args); }
    
    void convert(const MPlug& src, VtValue& dst, const ConverterArgs& args) const
    { _plugToVtValue(src, dst, args); }
    void convert(const VtValue& src, MPlug& dst, const ConverterArgs& args) const
    { _vtValueToPlug(src, dst, args); }
    void convert(const VtValue& src, MPlug& plug, MDGModifier& dst, const ConverterArgs& args) const
    { _vtValueToModifier(src, plug, dst, args); }
    
    void convert(const MDataHandle& src, UsdAttribute& dst, const ConverterArgs& args) const
    { _handleToAttr(src,dst,args); }
    void convert(const UsdAttribute& src, MDataHandle& dst, const ConverterArgs& args) const
    { _attrToHandle(src,dst,args); }
    
    void convert(const MDataHandle& src, VtValue& dst, const ConverterArgs& args) const
    { _handleToVtValue(src, dst, args); }
    void convert(const VtValue& src, MDataHandle& dst, const ConverterArgs& args) const
    { _vtValueoHandle(src, dst, args); }
    
    static bool hasAttrType(const MPlug& plug, MFnData::Type type)
    {
        MObject object = plug.attribute();
        if (!object.hasFn(MFn::kTypedAttribute)) {
            return false;
        }

        MFnTypedAttribute attr(object);
        return attr.attrType() == type;
    }

    static bool hasNumericType(const MPlug& plug, MFnNumericData::Type type)
    {
        MObject object = plug.attribute();
        if (!object.hasFn(MFn::kNumericAttribute)) {
            return false;
        }

        MFnNumericAttribute attr(object);
        return attr.unitType() == type;
    }
    
    static SdfValueTypeName getUsdTypeName(
        const MPlug& attrPlug,
        const bool translateMayaDoubleToUsdSinglePrecision);

private:
    const SdfValueTypeName& _typeName;
    
    // MPlug <--> UsdAttribute
    MPlugToUsdAttrFn _plugToAttr{nullptr};
    UsdAttrToMPlugFn _attrToPlug{nullptr};
    UsdAttrToMDGModifierFn _attrToModifier{nullptr};
    // MPlug <--> VtValue
    MPlugToVtValueFn _plugToVtValue{nullptr};
    VtValueToMPlugFn _vtValueToPlug{nullptr};
    VtValueToMDGModifierFn _vtValueToModifier{nullptr};
    
    // MDataBlock <--> UsdAttribute
    MDataHandleToUsdAttrFn _handleToAttr{nullptr};
    UsdAttrToMDataHandleFn _attrToHandle{nullptr};
    // MDataBlock <--> VtValue
    MDataHandleToVtValueFn _handleToVtValue{nullptr};
    VtValueToMDataHandleFn _vtValueoHandle{nullptr};
};

template <class MAYA_Type, class USD_Type, class Enable=void> struct TypedConverter {};

template <class MAYA_Type, class USD_Type>
struct TypedConverter<MAYA_Type,USD_Type, typename std::enable_if<std::is_same<MAYA_Type, USD_Type>::value>::type>
{
    static void convert(const USD_Type& src, MAYA_Type& dst)
    {
        dst = src;
    }
};

template <>
struct TypedConverter<float3,GfVec3f>
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

template <>
struct TypedConverter<double3,GfVec3d>
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

template <>
struct TypedConverter<MPoint,GfVec3f>
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
    
template <>
struct TypedConverter<MString,std::string>
{
    static void convert(const std::string& src, MString& dst)
    {
        dst = src.c_str();
    }
    static void convert(const MString& src, std::string& dst)
    {
        dst = src.asChar();
    }
};

template <>
struct TypedConverter<MMatrix,GfMatrix4d>
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
    
template <>
struct TypedConverter<MIntArray, VtArray<int>>
{
    static void convert(const VtArray<int>& src, MIntArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            dst[i]=src[i];
        }
    }
    static void convert(const MIntArray& src, VtArray<int>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            dst[i]=src[i];
        }
    }
};
    
template <>
struct TypedConverter<MPointArray, VtArray<GfVec3f>>
{
    static void convert(const VtArray<GfVec3f>& src, MPointArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            TypedConverter<MPoint,GfVec3f>::convert(src[i], dst[i]);
        }
    }
    static void convert(const MPointArray& src, VtArray<GfVec3f>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            TypedConverter<MPoint,GfVec3f>::convert(src[i], dst[i]);
        }
    }
};

template <>
struct TypedConverter<MMatrixArray, VtArray<GfMatrix4d>>
{
    static void convert(const VtArray<GfMatrix4d>& src, MMatrixArray& dst)
    {
        const size_t srcSize = src.size();
        dst.setLength(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            TypedConverter<MMatrix,GfMatrix4d>::convert(src[i], dst[i]);
        }
    }
    static void convert(const MMatrixArray& src, VtArray<GfMatrix4d>& dst)
    {
        const size_t srcSize = src.length();
        dst.resize(srcSize);
        for(size_t i = 0; i < srcSize; i++)
        {
            TypedConverter<MMatrix,GfMatrix4d>::convert(src[i], dst[i]);
        }
    }
};
    
} // namespace MayaUsd

#endif
