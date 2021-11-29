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

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>

#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaPrimWriter
//----------------------------------------------------------------------------------------------------------------------
template <typename T = UsdMayaPrimWriter>
class PrimWriterWrapper
    : public T
    , public TfPyPolymorphic<UsdMayaPrimWriter>
{
public:
    typedef PrimWriterWrapper This;
    typedef T                 base_t;

    PrimWriterWrapper(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : T(depNodeFn, usdPath, jobCtx)
    {
    }

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    virtual ~PrimWriterWrapper() { }

    const UsdStage& GetUsdStage() const { return *get_pointer(base_t::GetUsdStage()); }

    // This is the pattern inspired from USD/pxr/base/tf/wrapTestTfPython.cpp
    // It would have been simpler to call 'this->CallVirtual<>("Write",
    // &base_t::default_Write)(usdTime);' But it is not allowed Instead of having to create a
    // function 'default_Write(...)' to call the base class when there is no Python implementation.
    void default_Write(const UsdTimeCode& usdTime) { base_t::Write(usdTime); }
    void Write(const UsdTimeCode& usdTime) override
    {
        this->template CallVirtual<>("Write", &This::default_Write)(usdTime);
    }

    void default_PostExport() { base_t::PostExport(); }
    void PostExport() override
    {
        this->template CallVirtual<>("PostExport", &This::default_PostExport)();
    }

    bool default_ExportsGprims() const { return base_t::ExportsGprims(); }
    bool ExportsGprims() const override
    {
        return this->template CallVirtual<bool>("ExportsGprims", &This::default_ExportsGprims)();
    }

    bool default_ShouldPruneChildren() const { return base_t::ShouldPruneChildren(); };
    bool ShouldPruneChildren() const override
    {
        return this->template CallVirtual<bool>(
            "ShouldPruneChildren", &This::default_ShouldPruneChildren)();
    }

    bool default__HasAnimCurves() const { return base_t::_HasAnimCurves(); };
    bool _HasAnimCurves() const override
    {
        return this->template CallVirtual<bool>("_HasAnimCurves", &This::default__HasAnimCurves)();
    }

    void _SetUsdPrim(const UsdPrim& usdPrim) { base_t::_SetUsdPrim(usdPrim); }

    const UsdMayaJobExportArgs& _GetExportArgs() const { return base_t::_GetExportArgs(); }
    boost::python::object       _GetSparseValueWriter()
    {
        return boost::python::object(base_t::_GetSparseValueWriter());
    }

    const SdfPathVector& default_GetModelPaths() const { return base_t::GetModelPaths(); }
    const SdfPathVector& GetModelPaths() const override
    {
        if (Override o = GetOverride("GetModelPaths")) {
            auto res = std::function<boost::python::object()>(TfPyCall<boost::python::object>(o))();
            if (res) {
                const_cast<SdfPathVector*>(&_modelPaths)->clear();
                TfPyLock pyLock;

                boost::python::list keys(res);
                for (int i = 0; i < len(keys); ++i) {
                    boost::python::extract<SdfPath> extractedKey(keys[i]);
                    if (!extractedKey.check()) {
                        TF_CODING_ERROR(
                            "PrimWriterWrapper.GetModelPaths: SdfPath key expected, not "
                            "found!");
                        return _modelPaths;
                    }

                    SdfPath path = extractedKey;
                    const_cast<SdfPathVector*>(&_modelPaths)->push_back(path);
                }
                return _modelPaths;
            }
        }
        return This::default_GetModelPaths();
    }

    const UsdMayaUtil::MDagPathMap<SdfPath>& default_GetDagToUsdPathMapping() const
    {
        return base_t::GetDagToUsdPathMapping();
    }
    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagToUsdPathMapping() const override
    {
        if (Override o = GetOverride("GetDagToUsdPathMapping")) {
            auto res = std::function<boost::python::object()>(TfPyCall<boost::python::object>(o))();
            if (res) {
                const_cast<UsdMayaUtil::MDagPathMap<SdfPath>*>(&_dagPathMap)->clear();
                TfPyLock pyLock;

                boost::python::list tuples(res);
                for (int i = 0; i < len(tuples); ++i) {
                    boost::python::tuple t(tuples[i]);
                    if (boost::python::len(t) != 2) {
                        TF_CODING_ERROR("PrimWriterWrapper.GetDagToUsdPathMapping: list<tuples> "
                                        "key expected, not "
                                        "found!");
                        return _dagPathMap;
                    }
                    boost::python::extract<MDagPath> extractedDagPath(t[0]);
                    if (!extractedDagPath.check()) {
                        TF_CODING_ERROR(
                            "PrimWriterWrapper.GetDagToUsdPathMapping: MDagPath key expected, not "
                            "found!");
                        return _dagPathMap;
                    }
                    boost::python::extract<SdfPath> extractedSdfPath(t[1]);
                    if (!extractedSdfPath.check()) {
                        TF_CODING_ERROR(
                            "PrimWriterWrapper.GetDagToUsdPathMapping: SdfPath key expected, not "
                            "found!");
                        return _dagPathMap;
                    }

                    MDagPath dagPath = extractedDagPath;
                    SdfPath  path = extractedSdfPath;
                    const_cast<UsdMayaUtil::MDagPathMap<SdfPath>*>(&_dagPathMap)
                        ->
                        operator[](dagPath)
                        = path;
                }
                return _dagPathMap;
            }
        }
        return This::default_GetDagToUsdPathMapping();
    }

    static void Register(boost::python::object cl, const std::string& mayaTypeName)
    {
        UsdMayaPrimWriterRegistry::Register(
            mayaTypeName,
            [=](const MFnDependencyNode& depNodeFn,
                const SdfPath&           usdPath,
                UsdMayaWriteJobContext&  jobCtx) {
                auto                  sptr = std::make_shared<This>(depNodeFn, usdPath, jobCtx);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)&sptr);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            },
            true);
    }

private:
    SdfPathVector                     _modelPaths;
    UsdMayaUtil::MDagPathMap<SdfPath> _dagPathMap;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaShaderWriter
//----------------------------------------------------------------------------------------------------------------------
class ShaderWriterWrapper : public PrimWriterWrapper<UsdMayaShaderWriter>
{
public:
    typedef ShaderWriterWrapper This;
    typedef UsdMayaShaderWriter base_t;

    ShaderWriterWrapper(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : PrimWriterWrapper<UsdMayaShaderWriter>(depNodeFn, usdPath, jobCtx)
    {
    }

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    virtual ~ShaderWriterWrapper() { }

    TfToken default_GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName)
    {
        return base_t::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }
    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override
    {
        return this->CallVirtual<TfToken>(
            "GetShadingAttributeNameForMayaAttrName",
            &This::default_GetShadingAttributeNameForMayaAttrName)(mayaAttrName);
    }

    UsdAttribute default_GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName)
    {
        return base_t::GetShadingAttributeForMayaAttrName(mayaAttrName, typeName);
    }
    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override
    {
        return this->CallVirtual<UsdAttribute>(
            "GetShadingAttributeForMayaAttrName",
            &This::default_GetShadingAttributeForMayaAttrName)(mayaAttrName, typeName);
    }

    static void Register(boost::python::object cl, const TfToken& mayaNodeTypeName)
    {
        UsdMayaShaderWriterRegistry::Register(
            mayaNodeTypeName,
            [=](const UsdMayaJobExportArgs& exportArgs) {
                TfPyLock              pyLock;
                boost::python::object CanExport = cl.attr("CanExport");
                PyObject*             callable = CanExport.ptr();
                auto                  res = boost::python::call<int>(callable, exportArgs);
                return UsdMayaShaderWriter::ContextSupport(res);
            },
            [=](const MFnDependencyNode& depNodeFn,
                const SdfPath&           usdPath,
                UsdMayaWriteJobContext&  jobCtx) {
                auto                  sptr = std::make_shared<This>(depNodeFn, usdPath, jobCtx);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)&sptr);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            },
            true);
    }

    static void RegisterSymmetric(
        boost::python::object cl,
        const TfToken&        mayaNodeTypeName,
        const TfToken&        usdShaderId,
        const TfToken&        materialConversionName)
    {
        UsdMayaSymmetricShaderWriter::RegisterWriter(
            mayaNodeTypeName, usdShaderId, materialConversionName, true);
    }
};

namespace {
std::vector<std::string> getChaserNames(UsdMayaJobExportArgs& self) { return self.chaserNames; }

boost::python::object getChaserArgs(UsdMayaJobExportArgs& self, const std::string& chaser)
{
    boost::python::dict editDict;

    std::map<std::string, std::string> myArgs;
    TfMapLookup(self.allChaserArgs, chaser, &myArgs);

    for (const auto& item : myArgs) {
        editDict[item.first] = item.second;
    }
    return editDict;
}
} // namespace
//----------------------------------------------------------------------------------------------------------------------
void wrapJobExportArgs()
{
    boost::python::class_<UsdMayaJobExportArgs>("JobExportArgs", boost::python::no_init)
        .def("getChaserNames", &::getChaserNames)
        .def("getChaserArgs", &::getChaserArgs)
        .def("GetResolvedFileName", &UsdMayaJobExportArgs::GetResolvedFileName);
}

void wrapPrimWriter()
{
    boost::python::class_<PrimWriterWrapper<>, boost::noncopyable>(
        "PrimWriter", boost::python::no_init)
        .def("__init__", make_constructor(&PrimWriterWrapper<>::New))
        .def(
            "PostExport",
            &PrimWriterWrapper<>::PostExport,
            &PrimWriterWrapper<>::default_PostExport)
        .def("Write", &PrimWriterWrapper<>::Write, &PrimWriterWrapper<>::default_Write)

        .def("GetExportVisibility", &PrimWriterWrapper<>::GetExportVisibility)
        .def("SetExportVisibility", &PrimWriterWrapper<>::SetExportVisibility)
        .def(
            "GetMayaObject",
            &PrimWriterWrapper<>::GetMayaObject,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetUsdPrim",
            &PrimWriterWrapper<>::GetUsdPrim,
            boost::python::return_internal_reference<>())
        .def("_SetUsdPrim", &PrimWriterWrapper<>::_SetUsdPrim)
        .def(
            "MakeSingleSamplesStatic",
            static_cast<void (PrimWriterWrapper<>::*)()>(
                &PrimWriterWrapper<>::MakeSingleSamplesStatic))
        .def(
            "MakeSingleSamplesStatic",
            static_cast<void (PrimWriterWrapper<>::*)(UsdAttribute attr)>(
                &PrimWriterWrapper<>::MakeSingleSamplesStatic))

        .def(
            "_HasAnimCurves",
            &PrimWriterWrapper<>::_HasAnimCurves,
            &PrimWriterWrapper<>::default__HasAnimCurves)
        .def(
            "_GetExportArgs",
            &PrimWriterWrapper<>::_GetExportArgs,
            boost::python::return_internal_reference<>())
        .def("_GetSparseValueWriter", &PrimWriterWrapper<>::_GetSparseValueWriter)

        .def(
            "GetDagPath",
            &PrimWriterWrapper<>::GetDagPath,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetUsdStage",
            &UsdMayaPrimWriter::GetUsdStage,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetUsdPath",
            &UsdMayaPrimWriter::GetUsdPath,
            boost::python::return_value_policy<boost::python::return_by_value>())

        .def("Register", &PrimWriterWrapper<>::Register)
        .staticmethod("Register");
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdMayaShaderWriter::ContextSupport::Supported, "Supported");
    TF_ADD_ENUM_NAME(UsdMayaShaderWriter::ContextSupport::Fallback, "Fallback");
    TF_ADD_ENUM_NAME(UsdMayaShaderWriter::ContextSupport::Unsupported, "Unsupported");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapShaderWriter()
{
    boost::python::
        class_<ShaderWriterWrapper, boost::python::bases<PrimWriterWrapper<>>, boost::noncopyable>
            c("ShaderWriter", boost::python::no_init);

    boost::python::scope s(c);

    TfPyWrapEnum<UsdMayaShaderWriter::ContextSupport>();

    c.def("__init__", make_constructor(&ShaderWriterWrapper::New))
        .def(
            "GetShadingAttributeNameForMayaAttrName",
            &ShaderWriterWrapper::GetShadingAttributeNameForMayaAttrName,
            &ShaderWriterWrapper::default_GetShadingAttributeNameForMayaAttrName)
        .def(
            "GetShadingAttributeForMayaAttrName",
            &ShaderWriterWrapper::GetShadingAttributeForMayaAttrName,
            &ShaderWriterWrapper::default_GetShadingAttributeForMayaAttrName)

        .def("Register", &ShaderWriterWrapper::Register)
        .staticmethod("Register")
        .def("RegisterSymmetric", &ShaderWriterWrapper::RegisterSymmetric)
        .staticmethod("RegisterSymmetric");
}
