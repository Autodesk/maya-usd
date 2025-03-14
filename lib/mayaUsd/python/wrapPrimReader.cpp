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

#include "pythonObjectRegistry.h"

#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>
#include <mayaUsd/fileio/shading/symmetricShaderReader.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr_python.h>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaPrimReader
//----------------------------------------------------------------------------------------------------------------------
template <typename T = UsdMayaPrimReader>
class PrimReaderWrapper
    : public T
    , public TfPyPolymorphic<T>
{
public:
    typedef PrimReaderWrapper This;
    typedef T                 base_t;

    PrimReaderWrapper(const UsdMayaPrimReaderArgs& args)
        : T(args)
    {
    }

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    virtual ~PrimReaderWrapper() = default;

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

    //---------------------------------------------------------------------------------------------
    /// \brief  wraps a factory function that allows registering an updated Python class
    //---------------------------------------------------------------------------------------------
    class FactoryFnWrapper : public UsdMayaPythonObjectRegistry
    {
    public:
        // Instances of this class act as "function objects" that are fully compatible with the
        // std::function requested by UsdMayaSchemaApiAdaptorRegistry::Register. These will create
        // python wrappers based on the latest class registered.
        UsdMayaPrimReaderSharedPtr operator()(const UsdMayaPrimReaderArgs& args)
        {
            PXR_BOOST_PYTHON_NAMESPACE::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return nullptr;
            }
            auto                               sptr = std::make_shared<This>(args);
            TfPyLock                           pyLock;
            PXR_BOOST_PYTHON_NAMESPACE::object instance = pyClass((uintptr_t)&sptr);
            PXR_BOOST_PYTHON_NAMESPACE::incref(instance.ptr());
            initialize_wrapper(instance.ptr(), sptr.get());
            return sptr;
        }

        UsdMayaPrimReader::ContextSupport
        operator()(const UsdMayaJobImportArgs& args, const UsdPrim& importPrim)
        {
            PXR_BOOST_PYTHON_NAMESPACE::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return UsdMayaPrimReader::ContextSupport::Unsupported;
            }
            TfPyLock pyLock;
            if (PyObject_HasAttrString(pyClass.ptr(), "CanImport")) {
                PXR_BOOST_PYTHON_NAMESPACE::object CanImport = pyClass.attr("CanImport");
                PyObject*                          callable = CanImport.ptr();
                auto res = PXR_BOOST_PYTHON_NAMESPACE::call<int>(callable, args, importPrim);
                return UsdMayaPrimReader::ContextSupport(res);
            } else {
                return UsdMayaPrimReader::CanImport(args, importPrim);
            }
        }

        // Create a new wrapper for a Python class that is seen for the first time for a given
        // purpose. If we already have a registration for this purpose: update the class to
        // allow the previously issued factory function to use it.
        static FactoryFnWrapper
        Register(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& typeName, bool& updated)
        {
            size_t classIndex = RegisterPythonObject(cl, GetKey(cl, typeName));
            updated = classIndex == UsdMayaPythonObjectRegistry::UPDATED;
            // Return a new factory function:
            return FactoryFnWrapper { classIndex };
        }

        // Unregister a class for a given purpose. This will cause the associated factory
        // function to stop producing this Python class.
        static void Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& typeName)
        {
            UnregisterPythonObject(cl, GetKey(cl, typeName));
        }

    private:
        // Function object constructor. Requires only the index of the Python class to use.
        FactoryFnWrapper(size_t classIndex)
            : _classIndex(classIndex) {};

        size_t _classIndex;

        // Generates a unique key based on the name of the class, along with the class
        // purpose:
        static std::string
        GetKey(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& typeName)
        {
            return ClassName(cl) + "," + typeName + "," + ",PrimReader";
        }
    };

    static void Register(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& typeName)
    {
        bool             updated = false;
        FactoryFnWrapper fn = FactoryFnWrapper::Register(cl, typeName, updated);
        if (!updated) {
            auto type = TfType::FindByName(typeName);
            UsdMayaPrimReaderRegistry::Register(type, fn, fn, true);
        }
    }

    static void Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& typeName)
    {
        FactoryFnWrapper::Unregister(cl, typeName);
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

    ShaderReaderWrapper(const UsdMayaPrimReaderArgs& args)
        : PrimReaderWrapper<UsdMayaShaderReader>(args)
    {
    }

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    virtual ~ShaderReaderWrapper() { _downstreamReader = nullptr; }

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

    boost::optional<IsConverterResult> default_IsConverter() { return base_t::IsConverter(); }
    boost::optional<IsConverterResult> IsConverter() override
    {
        if (Override o = GetOverride("IsConverter")) {
            auto res = std::function<PXR_BOOST_PYTHON_NAMESPACE::object()>(
                TfPyCall<PXR_BOOST_PYTHON_NAMESPACE::object>(o))();
            if (res) {
                TfPyLock pyLock;

                PXR_BOOST_PYTHON_NAMESPACE::tuple t(res);
                if (PXR_BOOST_PYTHON_NAMESPACE::len(t) == 2) {
                    PXR_BOOST_PYTHON_NAMESPACE::extract<UsdShadeShader> downstreamSchema(t[0]);
                    if (downstreamSchema.check()) {
                        PXR_BOOST_PYTHON_NAMESPACE::extract<TfToken> downstreamOutputName(t[1]);
                        if (downstreamOutputName.check()) {
                            PXR_BOOST_PYTHON_NAMESPACE::incref(t.ptr());
                            return IsConverterResult { downstreamSchema, downstreamOutputName };
                        } else {
                            TF_CODING_ERROR(
                                "ShaderReaderWrapper.IsConverter: TfToken key expected, not "
                                "found!");
                        }
                    } else {
                        TF_CODING_ERROR(
                            "ShaderReaderWrapper.IsConverter: UsdShadeShader key expected, not "
                            "found!");
                    }
                }
            }
        }
        return This::default_IsConverter();
    }

    void SetDownstreamReader(std::shared_ptr<UsdMayaShaderReader> downstreamReader) override
    {
        _downstreamReader = downstreamReader;
    }

    const UsdMayaPrimReaderArgs& _GetArgs() { return base_t::_GetArgs(); }

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

    std::shared_ptr<UsdMayaShaderReader> GetDownstreamReader() { return _downstreamReader; }

    //---------------------------------------------------------------------------------------------
    /// \brief  wraps a factory function that allows registering an updated Python class
    //---------------------------------------------------------------------------------------------
    class FactoryFnWrapper : public UsdMayaPythonObjectRegistry
    {
    public:
        // Instances of this class act as "function objects" that are fully compatible with the
        // std::function requested by UsdMayaSchemaApiAdaptorRegistry::Register. These will create
        // python wrappers based on the latest class registered.
        UsdMayaPrimReaderSharedPtr operator()(const UsdMayaPrimReaderArgs& args)
        {
            PXR_BOOST_PYTHON_NAMESPACE::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return nullptr;
            }
            auto                               sptr = std::make_shared<This>(args);
            TfPyLock                           pyLock;
            PXR_BOOST_PYTHON_NAMESPACE::object instance = pyClass((uintptr_t)&sptr);
            PXR_BOOST_PYTHON_NAMESPACE::incref(instance.ptr());
            initialize_wrapper(instance.ptr(), sptr.get());
            return sptr;
        }

        // We can have multiple function objects, this one adapts the CanImport function:
        UsdMayaPrimReader::ContextSupport operator()(const UsdMayaJobImportArgs& args)
        {
            PXR_BOOST_PYTHON_NAMESPACE::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return UsdMayaPrimReader::ContextSupport::Unsupported;
            }
            TfPyLock pyLock;
            if (PyObject_HasAttrString(pyClass.ptr(), "CanImport")) {
                PXR_BOOST_PYTHON_NAMESPACE::object CanImport = pyClass.attr("CanImport");
                PyObject*                          callable = CanImport.ptr();
                auto res = PXR_BOOST_PYTHON_NAMESPACE::call<int>(callable, args);
                return UsdMayaPrimReader::ContextSupport(res);
            } else {
                return UsdMayaShaderReader::CanImport(args);
            }
        }

        // Create a new wrapper for a Python class that is seen for the first time for a given
        // purpose. If we already have a registration for this purpose: update the class to
        // allow the previously issued factory function to use it.
        static FactoryFnWrapper Register(
            PXR_BOOST_PYTHON_NAMESPACE::object cl,
            const std::string&                 usdShaderId,
            bool&                              updated)
        {
            size_t classIndex = RegisterPythonObject(cl, GetKey(cl, usdShaderId));
            updated = classIndex == UsdMayaPythonObjectRegistry::UPDATED;
            // Return a new factory function:
            return FactoryFnWrapper { classIndex };
        }

        // Unregister a class for a given purpose. This will cause the associated factory
        // function to stop producing this Python class.
        static void
        Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& usdShaderId)
        {
            UnregisterPythonObject(cl, GetKey(cl, usdShaderId));
        }

    private:
        // Function object constructor. Requires only the index of the Python class to use.
        FactoryFnWrapper(size_t classIndex)
            : _classIndex(classIndex) {};

        size_t _classIndex;

        // Generates a unique key based on the name of the class, along with the class
        // purpose:
        static std::string
        GetKey(PXR_BOOST_PYTHON_NAMESPACE::object cl, const std::string& usdShaderId)
        {
            return ClassName(cl) + "," + usdShaderId + "," + ",ShaderReader";
        }
    };

    static void Register(PXR_BOOST_PYTHON_NAMESPACE::object cl, const TfToken& usdShaderId)
    {
        bool             updated = false;
        FactoryFnWrapper fn = FactoryFnWrapper::Register(cl, usdShaderId, updated);
        if (!updated) {
            UsdMayaShaderReaderRegistry::Register(usdShaderId, fn, fn, true);
        }
    }

    static void Unregister(PXR_BOOST_PYTHON_NAMESPACE::object cl, const TfToken& usdShaderId)
    {
        FactoryFnWrapper::Unregister(cl, usdShaderId);
    }

    static void RegisterSymmetric(
        PXR_BOOST_PYTHON_NAMESPACE::object cl,
        const TfToken&                     usdShaderId,
        const TfToken&                     mayaNodeTypeName,
        const TfToken&                     materialConversion)
    {
        UsdMayaSymmetricShaderReader::RegisterReader(
            usdShaderId, mayaNodeTypeName, materialConversion, true);
    }

    std::shared_ptr<UsdMayaShaderReader> _downstreamReader;
};

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimReaderContext()
{
    using namespace PXR_BOOST_PYTHON_NAMESPACE;

    class_<UsdMayaPrimReaderContext>("PrimReaderContext", no_init)
        .def(
            "GetMayaNode",
            &UsdMayaPrimReaderContext::GetMayaNode,
            return_value_policy<return_by_value>())
        .def("RegisterNewMayaNode", &UsdMayaPrimReaderContext::RegisterNewMayaNode)
        .def("GetPruneChildren", &UsdMayaPrimReaderContext::GetPruneChildren)
        .def("SetPruneChildren", &UsdMayaPrimReaderContext::SetPruneChildren)
        .def("GetTimeSampleMultiplier", &UsdMayaPrimReaderContext::GetTimeSampleMultiplier)
        .def("SetTimeSampleMultiplier", &UsdMayaPrimReaderContext::SetTimeSampleMultiplier);
}

namespace {

PXR_BOOST_PYTHON_NAMESPACE::object get_allChaserArgs(UsdMayaJobImportArgs& self)
{
    PXR_BOOST_PYTHON_NAMESPACE::dict allChaserArgs;
    for (auto&& perChaser : self.allChaserArgs) {
        auto perChaserDict = PXR_BOOST_PYTHON_NAMESPACE::dict();
        for (auto&& perItem : perChaser.second) {
            perChaserDict[perItem.first] = perItem.second;
        }
        allChaserArgs[perChaser.first] = perChaserDict;
    }
    return PXR_BOOST_PYTHON_NAMESPACE::object(allChaserArgs);
}

PXR_BOOST_PYTHON_NAMESPACE::object get_remapUVSetsTo(UsdMayaJobImportArgs& self)
{
    PXR_BOOST_PYTHON_NAMESPACE::dict uvSetRemaps;
    for (auto&& remap : self.remapUVSetsTo) {
        uvSetRemaps[remap.first] = remap.second;
    }
    return PXR_BOOST_PYTHON_NAMESPACE::object(uvSetRemaps);
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
void wrapJobImportArgs()
{
    using namespace PXR_BOOST_PYTHON_NAMESPACE;

    class_<UsdMayaJobImportArgs> c("JobImportArgs", no_init);

    scope s(c);

    class_<UsdMayaJobImportArgs::ShadingMode>("ShadingMode", no_init)
        .def_readonly("mode", &UsdMayaJobImportArgs::ShadingMode::mode)
        .def_readonly("materialConversion", &UsdMayaJobImportArgs::ShadingMode::materialConversion);

    c.add_property(
         "assemblyRep",
         make_getter(&UsdMayaJobImportArgs::assemblyRep, return_value_policy<return_by_value>()))
        .add_property("allChaserArgs", ::get_allChaserArgs)
        .add_property(
            "chaserNames",
            make_getter(
                &UsdMayaJobImportArgs::chaserNames, return_value_policy<TfPySequenceToSet>()))
        .add_property(
            "excludePrimvarNames",
            make_getter(
                &UsdMayaJobImportArgs::excludePrimvarNames,
                return_value_policy<TfPySequenceToSet>()))
        .add_property(
            "excludePrimvarNamespaces",
            make_getter(
                &UsdMayaJobImportArgs::excludePrimvarNamespaces,
                return_value_policy<TfPySequenceToSet>()))
        .add_property("remapUVSetsTo", ::get_remapUVSetsTo)
        .def_readonly("importInstances", &UsdMayaJobImportArgs::importInstances)
        .def_readonly("importUSDZTextures", &UsdMayaJobImportArgs::importUSDZTextures)
        .def_readonly(
            "importUSDZTexturesFilePath", &UsdMayaJobImportArgs::importUSDZTexturesFilePath)
        .def_readonly("importRelativeTextures", &UsdMayaJobImportArgs::importRelativeTextures)
        .def_readonly("axisAndUnitMethod", &UsdMayaJobImportArgs::axisAndUnitMethod)
        .def_readonly("upAxis", &UsdMayaJobImportArgs::upAxis)
        .def_readonly("unit", &UsdMayaJobImportArgs::unit)
        .def_readonly("importWithProxyShapes", &UsdMayaJobImportArgs::importWithProxyShapes)
        .add_property(
            "includeAPINames",
            make_getter(
                &UsdMayaJobImportArgs::includeAPINames, return_value_policy<TfPySequenceToSet>()))
        .add_property(
            "includeMetadataKeys",
            make_getter(
                &UsdMayaJobImportArgs::includeMetadataKeys,
                return_value_policy<TfPySequenceToSet>()))
        .add_property(
            "jobContextNames",
            make_getter(
                &UsdMayaJobImportArgs::jobContextNames, return_value_policy<TfPySequenceToSet>()))
        .add_property(
            "preferredMaterial",
            make_getter(
                &UsdMayaJobImportArgs::preferredMaterial, return_value_policy<return_by_value>()))
        .def_readonly("shadingModes", &UsdMayaJobImportArgs::shadingModes)
        .add_property(
            "timeInterval",
            make_getter(
                &UsdMayaJobImportArgs::timeInterval, return_value_policy<return_by_value>()))
        .def_readonly("useAsAnimationCache", &UsdMayaJobImportArgs::useAsAnimationCache)
        .def_readonly("preserveTimeline", &UsdMayaJobImportArgs::preserveTimeline)
        .def("GetMaterialConversion", &UsdMayaJobImportArgs::GetMaterialConversion);

    to_python_converter<
        std::vector<UsdMayaJobImportArgs::ShadingMode>,
        TfPySequenceToPython<std::vector<UsdMayaJobImportArgs::ShadingMode>>>();
}

void wrapPrimReaderArgs()
{
    using namespace PXR_BOOST_PYTHON_NAMESPACE;

    class_<UsdMayaPrimReaderArgs>("PrimReaderArgs", no_init)
        .def("GetUsdPrim", &UsdMayaPrimReaderArgs::GetUsdPrim, return_internal_reference<>())
        .def(
            "GetJobArguments",
            &UsdMayaPrimReaderArgs::GetJobArguments,
            return_internal_reference<>())
        .def("GetTimeInterval", &UsdMayaPrimReaderArgs::GetTimeInterval)
        .def(
            "GetIncludeMetadataKeys",
            &UsdMayaPrimReaderArgs::GetIncludeMetadataKeys,
            return_internal_reference<>())
        .def(
            "GetIncludeAPINames",
            &UsdMayaPrimReaderArgs::GetIncludeAPINames,
            return_internal_reference<>())
        .def(
            "GetExcludePrimvarNames",
            &UsdMayaPrimReaderArgs::GetExcludePrimvarNames,
            return_internal_reference<>())
        .def(
            "GetExcludePrimvarNamespaces",
            &UsdMayaPrimReaderArgs::GetExcludePrimvarNamespaces,
            return_internal_reference<>())
        .def("GetUseAsAnimationCache", &UsdMayaPrimReaderArgs::GetUseAsAnimationCache);
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdMayaPrimReader::ContextSupport::Supported, "Supported");
    TF_ADD_ENUM_NAME(UsdMayaPrimReader::ContextSupport::Fallback, "Fallback");
    TF_ADD_ENUM_NAME(UsdMayaPrimReader::ContextSupport::Unsupported, "Unsupported");
}

void wrapPrimReader()
{
    using namespace PXR_BOOST_PYTHON_NAMESPACE;

    typedef UsdMayaPrimReader This;

    class_<PrimReaderWrapper<>, PXR_BOOST_PYTHON_NAMESPACE::noncopyable> c("PrimReader", no_init);

    scope s(c);

    TfPyWrapEnum<UsdMayaPrimReader::ContextSupport>();

    c.def("__init__", make_constructor(&PrimReaderWrapper<>::New))
        .def("Read", pure_virtual(&UsdMayaPrimReader::Read))
        .def(
            "HasPostReadSubtree",
            &This::HasPostReadSubtree,
            &PrimReaderWrapper<>::default_HasPostReadSubtree)
        .def(
            "PostReadSubtree",
            &This::PostReadSubtree,
            &PrimReaderWrapper<>::default_PostReadSubtree)
        .def("_GetArgs", &PrimReaderWrapper<>::_GetArgs, return_internal_reference<>())
        .def("Register", &PrimReaderWrapper<>::Register)
        .staticmethod("Register")
        .def("Unregister", &PrimReaderWrapper<>::Unregister)
        .staticmethod("Unregister");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapShaderReader()
{
    using namespace PXR_BOOST_PYTHON_NAMESPACE;

    typedef UsdMayaShaderReader This;

    class_<ShaderReaderWrapper, bases<UsdMayaPrimReader>, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>
        c("ShaderReader", no_init);

    scope s(c);

    c.def("__init__", make_constructor(&ShaderReaderWrapper::New))
        .def("Read", pure_virtual(&UsdMayaPrimReader::Read))
        .def(
            "GetMayaPlugForUsdAttrName",
            &This::GetMayaPlugForUsdAttrName,
            &ShaderReaderWrapper::default_GetMayaPlugForUsdAttrName)
        .def(
            "GetMayaNameForUsdAttrName",
            &This::GetMayaNameForUsdAttrName,
            &ShaderReaderWrapper::default_GetMayaNameForUsdAttrName)
        .def(
            "PostConnectSubtree",
            &This::PostConnectSubtree,
            &ShaderReaderWrapper::default_PostConnectSubtree)
        .def("IsConverter", &This::IsConverter, &ShaderReaderWrapper::default_IsConverter)
        .def(
            "GetCreatedObject",
            &This::GetCreatedObject,
            &ShaderReaderWrapper::default_GetCreatedObject)
        .def("_GetArgs", &ShaderReaderWrapper::_GetArgs, return_internal_reference<>())
        .add_property("_downstreamReader", &ShaderReaderWrapper::GetDownstreamReader)

        .def("Register", &ShaderReaderWrapper::Register)
        .staticmethod("Register")
        .def("Unregister", &ShaderReaderWrapper::Unregister)
        .staticmethod("Unregister")
        .def("RegisterSymmetric", &ShaderReaderWrapper::RegisterSymmetric)
        .staticmethod("RegisterSymmetric");

    // For wrapping UsdMayaShaderReader created in c++
    class_<This, std::shared_ptr<This>, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "ShaderReaderWrapper", no_init)
        .def("Read", pure_virtual(&UsdMayaPrimReader::Read))
        .def("GetMayaPlugForUsdAttrName", &This::GetMayaPlugForUsdAttrName)
        .def("GetMayaNameForUsdAttrName", &This::GetMayaNameForUsdAttrName)
        .def("PostConnectSubtree", &This::PostConnectSubtree)
        .def("IsConverter", &This::IsConverter)
        .def("GetCreatedObject", &This::GetCreatedObject);
}
