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

#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaPrimReader
//----------------------------------------------------------------------------------------------------------------------
template <typename T = UsdMayaPrimReader>
class PrimReaderWrapper
    : public T
    , public TfPyPolymorphic<UsdMayaPrimReader>
{
public:
    typedef PrimReaderWrapper This;
    typedef T                 base_t;

    PrimReaderWrapper(const UsdMayaPrimReaderArgs& args)
        : T(args)
    {
    }

    static PrimReaderWrapper* New(uintptr_t createdWrapper)
    {
        return (PrimReaderWrapper*)createdWrapper;
    }

    virtual ~PrimReaderWrapper() { }

    bool Read(UsdMayaPrimReaderContext& context) override
    {
        return this->template CallPureVirtual<bool>("Read")(context);
    }

    bool default_HasPostReadSubtree() const { return base_t::HasPostReadSubtree(); }
    bool HasPostReadSubtree() const override
    {
        return this->template CallVirtual<bool>(
            "HasPostReadSubtree", &This::default_HasPostReadSubtree)();
    }

    void default_PostReadSubtree(UsdMayaPrimReaderContext& context)
    {
        base_t::PostReadSubtree(context);
    }
    void PostReadSubtree(UsdMayaPrimReaderContext& context) override
    {
        this->template CallVirtual<>("PostReadSubtree", &This::default_PostReadSubtree)(context);
    }

    // Must be declared for python to be able to call the protected function _GetArgs in
    // UsdMayaPrimReader
    const UsdMayaPrimReaderArgs& _GetArgs() { return base_t::_GetArgs(); }

    static void Register(boost::python::object cl, const std::string& typeName)
    {
        auto type = TfType::FindByName(typeName);
        UsdMayaPrimReaderRegistry::Register(
            type,
            [=](const UsdMayaPrimReaderArgs& args) {
                auto                  sptr = std::make_shared<PrimReaderWrapper>(args);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)(PrimReaderWrapper*)sptr.get());
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            },
            true);
    }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaShaderReader
//----------------------------------------------------------------------------------------------------------------------
class ShaderReaderWrapper : public PrimReaderWrapper<UsdMayaShaderReader>
{
public:
    typedef ShaderReaderWrapper This;
    typedef UsdMayaShaderReader base_t;

    const TfToken& _mayaNodeTypeName;

    ShaderReaderWrapper(const UsdMayaPrimReaderArgs& args, const TfToken& mayaNodeTypeName)
        : PrimReaderWrapper<UsdMayaShaderReader>(args)
        , _mayaNodeTypeName(mayaNodeTypeName)
    {
    }

    static ShaderReaderWrapper* New(uintptr_t createdWrapper)
    {
        return (ShaderReaderWrapper*)createdWrapper;
    }

    virtual ~ShaderReaderWrapper() { }

    MPlug
    default_GetMayaPlugForUsdAttrName(const TfToken& usdAttrName, const MObject& mayaObject) const
    {
        return base_t::GetMayaPlugForUsdAttrName(usdAttrName, mayaObject);
    }
    MPlug
    GetMayaPlugForUsdAttrName(const TfToken& usdAttrName, const MObject& mayaObject) const override
    {
        return this->CallVirtual<MPlug>(
            "GetMayaPlugForUsdAttrName",
            &This::default_GetMayaPlugForUsdAttrName)(usdAttrName, mayaObject);
    }

    TfToken default_GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
    {
        return base_t::GetMayaNameForUsdAttrName(usdAttrName);
    }
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override
    {
        return this->CallVirtual<TfToken>(
            "GetMayaNameForUsdAttrName", &This::default_GetMayaNameForUsdAttrName)(usdAttrName);
    }

    void default_PostConnectSubtree(UsdMayaPrimReaderContext* context)
    {
        base_t::PostConnectSubtree(context);
    }
    void PostConnectSubtree(UsdMayaPrimReaderContext* context) override
    {
        this->CallVirtual<>("PostConnectSubtree", &This::default_PostConnectSubtree)(context);
    }

    bool default_IsConverter(UsdShadeShader& downstreamSchema, TfToken& downstreamOutputName)
    {
        return base_t::IsConverter(downstreamSchema, downstreamOutputName);
    }
    bool IsConverter(UsdShadeShader& downstreamSchema, TfToken& downstreamOutputName) override
    {
        return this->CallVirtual<bool>("IsConverter", &This::default_IsConverter)(
            downstreamSchema, downstreamOutputName);
    }

    void default_SetDownstreamReader(std::shared_ptr<UsdMayaShaderReader> downstreamReader)
    {
        base_t::SetDownstreamReader(downstreamReader);
    }
    void SetDownstreamReader(std::shared_ptr<UsdMayaShaderReader> downstreamReader) override
    {
        this->CallVirtual<>("SetDownstreamReader", &This::default_SetDownstreamReader)(
            downstreamReader);
    }

    MObject default_GetCreatedObject(
        const UsdMayaShadingModeImportContext& context,
        const UsdPrim&                         prim) const
    {
        return base_t::GetCreatedObject(context, prim);
    }
    MObject GetCreatedObject(const UsdMayaShadingModeImportContext& context, const UsdPrim& prim)
        const override
    {
        return this->CallVirtual<MObject>("GetCreatedObject", &This::default_GetCreatedObject)(
            context, prim);
    }

    const TfToken& GetmayaNodeTypeName() { return _mayaNodeTypeName; }

    static void Register(
        boost::python::object cl,
        const TfToken&        usdShaderId,
        const TfToken&        mayaNodeTypeName,
        const TfToken&        materialConversion)
    {
        UsdMayaShaderReaderRegistry::Register(
            usdShaderId,
            [=](const UsdMayaJobImportArgs& args) {
                TfPyLock              pyLock;
                boost::python::object CanImport = cl.attr("CanImport");
                PyObject*             callable = CanImport.ptr();
                auto res = boost::python::call<int>(callable, args, materialConversion);
                return UsdMayaShaderReader::ContextSupport(res);
            },
            [=](const UsdMayaPrimReaderArgs& args) {
                auto     sptr = std::make_shared<ShaderReaderWrapper>(args, mayaNodeTypeName);
                TfPyLock pyLock;
                boost::python::object instance = cl((uintptr_t)(ShaderReaderWrapper*)sptr.get());
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            },
            true);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimReaderContext()
{
    boost::python::class_<UsdMayaPrimReaderContext>("PrimReaderContext", boost::python::no_init)
        .def(
            "GetMayaNode",
            &UsdMayaPrimReaderContext::GetMayaNode,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def("RegisterNewMayaNode", &UsdMayaPrimReaderContext::RegisterNewMayaNode)
        .def("GetPruneChildren", &UsdMayaPrimReaderContext::GetPruneChildren)
        .def("SetPruneChildren", &UsdMayaPrimReaderContext::SetPruneChildren)
        .def("GetTimeSampleMultiplier", &UsdMayaPrimReaderContext::GetTimeSampleMultiplier)
        .def("SetTimeSampleMultiplier", &UsdMayaPrimReaderContext::SetTimeSampleMultiplier);
}

//----------------------------------------------------------------------------------------------------------------------
void wrapJobImportArgs()
{
    boost::python::class_<UsdMayaJobImportArgs>("JobImportArgs", boost::python::no_init);
}

void wrapPrimReaderArgs()
{
    boost::python::class_<UsdMayaPrimReaderArgs>("PrimReaderArgs", boost::python::no_init)
        .def(
            "GetUsdPrim",
            &UsdMayaPrimReaderArgs::GetUsdPrim,
            boost::python::return_internal_reference<>())
        .def(
            "GetJobArguments",
            &UsdMayaPrimReaderArgs::GetJobArguments,
            boost::python::return_internal_reference<>())
        .def("GetTimeInterval", &UsdMayaPrimReaderArgs::GetTimeInterval)
        .def(
            "GetIncludeMetadataKeys",
            &UsdMayaPrimReaderArgs::GetIncludeMetadataKeys,
            boost::python::return_internal_reference<>())
        .def(
            "GetIncludeAPINames",
            &UsdMayaPrimReaderArgs::GetIncludeAPINames,
            boost::python::return_internal_reference<>())
        .def(
            "GetExcludePrimvarNames",
            &UsdMayaPrimReaderArgs::GetExcludePrimvarNames,
            boost::python::return_internal_reference<>())
        .def("GetUseAsAnimationCache", &UsdMayaPrimReaderArgs::GetUseAsAnimationCache);
}

void wrapPrimReader()
{
    boost::python::class_<PrimReaderWrapper<>, boost::noncopyable>(
        "PrimReader", boost::python::no_init)
        .def("__init__", make_constructor(&PrimReaderWrapper<>::New))
        .def("Read", boost::python::pure_virtual(&UsdMayaPrimReader::Read))
        .def(
            "HasPostReadSubtree",
            &PrimReaderWrapper<>::HasPostReadSubtree,
            &PrimReaderWrapper<>::default_HasPostReadSubtree)
        .def(
            "PostReadSubtree",
            &PrimReaderWrapper<>::PostReadSubtree,
            &PrimReaderWrapper<>::default_PostReadSubtree)
        .def(
            "_GetArgs",
            &PrimReaderWrapper<>::_GetArgs,
            boost::python::return_internal_reference<>())
        .def(
            "Register",
            &PrimReaderWrapper<>::Register,
            (boost::python::arg("class"), boost::python::arg("type")))
        .staticmethod("Register");
}

void wrapShadingModeImportContext()
{
    boost::python::class_<UsdMayaShadingModeImportContext>(
        "ShadingModeImportContext", boost::python::no_init)
        .def("GetCreatedObject", &UsdMayaShadingModeImportContext::GetCreatedObject)
        //        .def("AddCreatedObject",&UsdMayaShadingModeImportContext::AddCreatedObject) //
        //        overloads
        .def("CreateShadingEngine", &UsdMayaShadingModeImportContext::CreateShadingEngine)
        .def("GetShadingEngineName", &UsdMayaShadingModeImportContext::GetShadingEngineName)
        .def("GetSurfaceShaderPlugName", &UsdMayaShadingModeImportContext::GetSurfaceShaderPlugName)
        .def("GetVolumeShaderPlugName", &UsdMayaShadingModeImportContext::GetVolumeShaderPlugName)
        .def(
            "GetDisplacementShaderPlugName",
            &UsdMayaShadingModeImportContext::GetDisplacementShaderPlugName)
        .def("SetSurfaceShaderPlugName", &UsdMayaShadingModeImportContext::SetSurfaceShaderPlugName)
        .def("SetVolumeShaderPlugName", &UsdMayaShadingModeImportContext::SetVolumeShaderPlugName)
        .def(
            "SetDisplacementShaderPlugName",
            &UsdMayaShadingModeImportContext::SetDisplacementShaderPlugName)
        .def(
            "GetPrimReaderContext",
            &UsdMayaShadingModeImportContext::GetPrimReaderContext,
            boost::python::return_value_policy<boost::python::return_by_value>());
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdMayaShaderReader::ContextSupport::Supported, "Supported");
    TF_ADD_ENUM_NAME(UsdMayaShaderReader::ContextSupport::Fallback, "Fallback");
    TF_ADD_ENUM_NAME(UsdMayaShaderReader::ContextSupport::Unsupported, "Unsupported");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapShaderReader()
{
    boost::python::
        class_<ShaderReaderWrapper, boost::python::bases<PrimReaderWrapper<>>, boost::noncopyable>
            c("ShaderReader", boost::python::no_init);

    boost::python::scope s(c);

    TfPyWrapEnum<UsdMayaShaderReader::ContextSupport>();

    c.def("__init__", make_constructor(&ShaderReaderWrapper::New))
        .def("Read", boost::python::pure_virtual(&UsdMayaPrimReader::Read))
        .def(
            "GetMayaPlugForUsdAttrName",
            &ShaderReaderWrapper::GetMayaPlugForUsdAttrName,
            &ShaderReaderWrapper::default_GetMayaPlugForUsdAttrName)
        .def(
            "GetMayaNameForUsdAttrName",
            &ShaderReaderWrapper::GetMayaNameForUsdAttrName,
            &ShaderReaderWrapper::default_GetMayaNameForUsdAttrName)
        .def(
            "PostConnectSubtree",
            &ShaderReaderWrapper::PostConnectSubtree,
            &ShaderReaderWrapper::default_PostConnectSubtree)
        .def(
            "IsConverter",
            &ShaderReaderWrapper::IsConverter,
            &ShaderReaderWrapper::default_IsConverter)
        .def(
            "SetDownstreamReader",
            &ShaderReaderWrapper::SetDownstreamReader,
            &ShaderReaderWrapper::default_SetDownstreamReader)
        .def(
            "GetCreatedObject",
            &ShaderReaderWrapper::GetCreatedObject,
            &ShaderReaderWrapper::default_GetCreatedObject)
        .add_property(
            "_mayaNodeTypeName",
            make_function(
                &ShaderReaderWrapper::GetmayaNodeTypeName,
                boost::python::return_value_policy<boost::python::return_by_value>()))

        .def(
            "Register",
            &ShaderReaderWrapper::Register,
            (boost::python::arg("class"), boost::python::arg("mayaTypeName")))
        .staticmethod("Register");
}
