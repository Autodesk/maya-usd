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

#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/pyConversions.h>

#include <maya/MObject.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

using namespace MAYAUSD_NS_DEF;

namespace {
constexpr auto find1
    = static_cast<const Converter* (*)(const SdfValueTypeName&, bool)>(&Converter::find);

static const Converter* find2(const std::string& attrName, const UsdAttribute& usdAttr)
{
    MPlug   plug;
    MStatus status = UsdMayaUtil::GetPlugByName(attrName, plug);
    CHECK_MSTATUS_AND_RETURN(status, nullptr);

    return Converter::find(plug, usdAttr);
}

static bool
validate(const Converter& self, const std::string& attrName, const UsdAttribute& usdAttr)
{
    MPlug plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess)
        return self.validate(plug, usdAttr);

    return false;
}

static void convertMPlugToUsdAttr(
    const Converter&     self,
    const std::string&   attrName,
    UsdAttribute&        usdAttr,
    const ConverterArgs& args)
{
    MPlug plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess)
        self.convert(plug, usdAttr, args);
}

static void convertUsdAttrToMPlug(
    const Converter&     self,
    const UsdAttribute&  usdAttr,
    const std::string&   attrName,
    const ConverterArgs& args)
{
    MPlug plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess)
        self.convert(usdAttr, plug, args);
}

static VtValue
convertMPlugToVtValue(const Converter& self, const std::string& attrName, const ConverterArgs& args)
{
    VtValue value;
    MPlug   plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess)
        self.convert(plug, value, args);

    return value;
}

static void convertVtValueToMPlug(
    const Converter&     self,
    const VtValue&       value,
    const std::string&   attrName,
    const ConverterArgs& args)
{
    MPlug plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess)
        self.convert(value, plug, args);
}

static void test_convertUsdAttrToMDGModifier(
    const Converter&     self,
    const UsdAttribute&  usdAttr,
    const std::string&   attrName,
    const ConverterArgs& args)
{
    // Testing function, no need for undo.
    OpUndoInfo undoInfo;
    MPlug      plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess) {
        MDGModifier& modifier = MDGModifierUndoItem::create("Test USD to DG conversion", undoInfo);
        self.convert(usdAttr, plug, modifier, args);

        modifier.doIt();
    }
}

static void test_convertVtValueToMDGModifier(
    const Converter&     self,
    const VtValue&       value,
    const std::string&   attrName,
    const ConverterArgs& args)
{
    MPlug plug;
    if (UsdMayaUtil::GetPlugByName(attrName, plug) == MS::kSuccess) {
        MDGModifier modifier;
        self.convert(value, plug, modifier, args);

        modifier.doIt();
    }
}
} // namespace

void wrapConverterArgs()
{
    using This = ConverterArgs;
    class_<This>("ConverterArgs")
        .def_readwrite("timeCode", &This::_timeCode)
        .def_readwrite("doGammaCorrection", &This::_doGammaCorrection);
}

void wrapConverter()
{
    using This = Converter;
    class_<This>("Converter", no_init)
        .def("find", find1, return_value_policy<reference_existing_object>())
        .def("find", find2, return_value_policy<reference_existing_object>())
        .staticmethod("find")
        .def("validate", validate)
        .def("convert", convertMPlugToUsdAttr)
        .def("convert", convertUsdAttrToMPlug)
        .def("convertVt", convertMPlugToVtValue)
        .def("convertVt", convertVtValueToMPlug)
        .def("test_convertAndSetWithModifier", test_convertUsdAttrToMDGModifier)
        .def("test_convertVtAndSetWithModifier", test_convertVtValueToMDGModifier);
}
