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

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>

#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <pxr/usd/usdGeom/camera.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

void ArchTest(void);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaPrimWriter
//----------------------------------------------------------------------------------------------------------------------
template< typename T = UsdMayaPrimWriter>
class PrimWriterWrapper
    : public T
    , public TfPyPolymorphic<UsdMayaPrimWriter>
{
public:
    typedef PrimWriterWrapper This;
    typedef T base_t;

    PrimWriterWrapper(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : T(depNodeFn, usdPath, jobCtx)
    {
    }

    static PrimWriterWrapper* New(uintptr_t createdWrapper)
    {
        return (PrimWriterWrapper*)createdWrapper;
    }

    virtual ~PrimWriterWrapper() { }

    const UsdStage& GetUsdStage() const 
    { 
        return *get_pointer(base_t::GetUsdStage()); 
    }

    void default_Write(const UsdTimeCode& usdTime) { base_t::Write(usdTime); }
    void Write(const UsdTimeCode& usdTime) override
    {
        this->CallVirtual<>("Write", &This::default_Write)(usdTime);
    }

    void default_PostExport() { base_t::PostExport(); }
    void PostExport() override
    { 
        this->CallVirtual<>("PostExport", &This::default_PostExport)(); 
    }

    bool default_ExportsGprims() const { return base_t::ExportsGprims(); } 
    bool ExportsGprims() const override
    {
        return this->CallVirtual<bool>("ExportsGprims", &This::default_ExportsGprims)(); 
    }

    bool default_ShouldPruneChildren() const { return base_t::ShouldPruneChildren(); } ;
    bool ShouldPruneChildren() const override
    {
        return this->CallVirtual<bool>("ShouldPruneChildren", &This::default_ShouldPruneChildren)(); 
    }

    bool default__HasAnimCurves() const { return base_t::_HasAnimCurves(); } ;
    bool _HasAnimCurves() const override
    {
        return this->CallVirtual<bool>("_HasAnimCurves", &This::default__HasAnimCurves)(); 
    }

    const UsdMayaJobExportArgs& _GetExportArgs() const { return base_t::_GetExportArgs(); }
    UsdUtilsSparseValueWriter* _GetSparseValueWriter() { return base_t::_GetSparseValueWriter(); }

    const SdfPathVector& default_GetModelPaths() const { return base_t::GetModelPaths(); }
    const SdfPathVector& GetModelPaths() const override
    {
        if (Override o = GetOverride("GetModelPaths")) {
            auto res = std::function<boost::python::object()>(
                TfPyCall<boost::python::object>(o))();
            if (res) {
                const_cast<SdfPathVector*>(&_modelPaths)->clear();
                TfPyLock pyLock;

                boost::python::list keys(res);
                for (int i = 0; i < len(keys); ++i) {
                    boost::python::extract<SdfPath> extractedKey(keys[i]);
                    if (!extractedKey.check()) {
                        TF_CODING_ERROR("PrimWriterWrapper.GetModelPaths: SdfPath key expected, not "
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

    const UsdMayaUtil::MDagPathMap<SdfPath>& default_GetDagToUsdPathMapping() const { return base_t::GetDagToUsdPathMapping(); }
    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagToUsdPathMapping() const override
    {
        if (Override o = GetOverride("GetDagToUsdPathMapping")) {
            auto res = std::function<boost::python::object()>(
                TfPyCall<boost::python::object>(o))();
            if (res) {
                const_cast<UsdMayaUtil::MDagPathMap<SdfPath>*>(&_dagPathMap)->clear();
                TfPyLock pyLock;

                boost::python::list tuples(res);
                for (int i = 0; i < len(tuples); ++i) {
                    boost::python::tuple t(tuples[i]);
                    if (boost::python::len(t) != 2) 
                    {
                        TF_CODING_ERROR("PrimWriterWrapper.GetDagToUsdPathMapping: list<tuples> key expected, not "
                                        "found!");
                        return _dagPathMap;
                    }
                    boost::python::extract<MDagPath> extractedDagPath(t[0]);
                    if (!extractedDagPath.check()) {
                        TF_CODING_ERROR("PrimWriterWrapper.GetDagToUsdPathMapping: MDagPath key expected, not "
                                        "found!");
                        return _dagPathMap;
                    }
                    boost::python::extract<SdfPath> extractedSdfPath(t[1]);
                    if (!extractedSdfPath.check()) {
                        TF_CODING_ERROR("PrimWriterWrapper.GetDagToUsdPathMapping: SdfPath key expected, not "
                                        "found!");
                        return _dagPathMap;
                    }

                    MDagPath dagPath = extractedDagPath;
                    SdfPath path = extractedSdfPath;
                    const_cast<UsdMayaUtil::MDagPathMap<SdfPath>*>(&_dagPathMap)->operator[](dagPath) = path;
                }
                return _dagPathMap;
            }
        }
        return This::default_GetDagToUsdPathMapping(); 
    }

    static void Register(boost::python::object cl, const std::string& mayaTypeName)
    {
        UsdMaya_RegistryHelper::g_pythonRegistry = true;
        UsdMayaPrimWriterRegistry::Register(
            mayaTypeName,
            [=](const MFnDependencyNode& depNodeFn,
                const SdfPath&           usdPath,
                UsdMayaWriteJobContext&  jobCtx) {
                auto sptr = std::make_shared<PrimWriterWrapper>(depNodeFn, usdPath, jobCtx);
                TfPyLock pyLock;
                boost::python::object instance = cl((uintptr_t)(PrimWriterWrapper*)sptr.get());
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            });
        UsdMaya_RegistryHelper::g_pythonRegistry = false;
    }

private:
    SdfPathVector _modelPaths;
    UsdMayaUtil::MDagPathMap<SdfPath> _dagPathMap;

};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaShaderWriter
//----------------------------------------------------------------------------------------------------------------------
class ShaderWriterWrapper
    : public PrimWriterWrapper<UsdMayaShaderWriter>
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

    static ShaderWriterWrapper* New(uintptr_t createdWrapper)
    {
        return (ShaderWriterWrapper*)createdWrapper;
    }

    virtual ~ShaderWriterWrapper() { }

    TfToken default_GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) { return base_t::GetShadingAttributeNameForMayaAttrName(mayaAttrName); }
    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override
    { 
        return this->CallVirtual<TfToken>("GetShadingAttributeNameForMayaAttrName", &This::default_GetShadingAttributeNameForMayaAttrName)(mayaAttrName); 
    }

    UsdAttribute default_GetShadingAttributeForMayaAttrName(const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) { return base_t::GetShadingAttributeForMayaAttrName(mayaAttrName,typeName); }
    UsdAttribute GetShadingAttributeForMayaAttrName(const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override
    { 
        return this->CallVirtual<UsdAttribute>("GetShadingAttributeForMayaAttrName", &This::default_GetShadingAttributeForMayaAttrName)(mayaAttrName,typeName); 
    }

    static void Register(boost::python::object cl, const TfToken& mayaType)
    {
        UsdMaya_RegistryHelper::g_pythonRegistry = true;
        UsdMayaShaderWriterRegistry::Register(
            mayaType,
            [=](const UsdMayaJobExportArgs& args) {
                return UsdMayaShaderWriter::ContextSupport(0);
            },
            [=](const MFnDependencyNode& depNodeFn,
                const SdfPath&           usdPath,
                UsdMayaWriteJobContext&  jobCtx) {
                auto sptr = std::make_shared<ShaderWriterWrapper>(depNodeFn, usdPath, jobCtx);
                TfPyLock pyLock;
                boost::python::object instance = cl((uintptr_t)(ShaderWriterWrapper*)sptr.get());
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            });
        UsdMaya_RegistryHelper::g_pythonRegistry = false;
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapJobExportArgs()
{
    boost::python::class_<UsdMayaJobExportArgs>(
        "JobExportArgs", boost::python::no_init)
        ;
}

void wrapPrimWriter()
{
    boost::python::class_<PrimWriterWrapper<>, boost::noncopyable>(
        "PrimWriter", boost::python::no_init)
        .def("__init__", make_constructor(&PrimWriterWrapper<>::New))
        .def("PostExport", &PrimWriterWrapper<>::PostExport, &PrimWriterWrapper<>::default_PostExport)
        .def("Write", &PrimWriterWrapper<>::Write, &PrimWriterWrapper<>::default_Write)

        .def("GetExportVisibility", &PrimWriterWrapper<>::GetExportVisibility)
        .def("SetExportVisibility", &PrimWriterWrapper<>::SetExportVisibility)
        .def("GetMayaObject", &PrimWriterWrapper<>::GetMayaObject, boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetUsdPrim", &PrimWriterWrapper<>::GetUsdPrim, boost::python::return_internal_reference<>())
        .def("SetUsdPrim", &UsdMayaPrimWriter::SetUsdPrim)
        .def("MakeSingleSamplesStatic", static_cast<void (PrimWriterWrapper<>::*)()>(&PrimWriterWrapper<>::MakeSingleSamplesStatic))
        .def("MakeSingleSamplesStatic", static_cast<void (PrimWriterWrapper<>::*)(UsdAttribute attr)>(&PrimWriterWrapper<>::MakeSingleSamplesStatic))

        .def("_HasAnimCurves", &PrimWriterWrapper<>::_HasAnimCurves, &PrimWriterWrapper<>::default__HasAnimCurves)
        .def("_GetExportArgs", &PrimWriterWrapper<>::_GetExportArgs, boost::python::return_internal_reference<>())
//        .def("_GetSparseValueWriter", &PrimWriterWrapper<>::_GetSparseValueWriter)

        .def("GetUsdPrim", &PrimWriterWrapper<>::GetUsdPrim, boost::python::return_internal_reference<>())
        .def("GetDagPath", &PrimWriterWrapper<>::GetDagPath, boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetUsdStage", &UsdMayaPrimWriter::GetUsdStage, boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetUsdPath", &UsdMayaPrimWriter::GetUsdPath, boost::python::return_value_policy<boost::python::return_by_value>())

        .def(
            "Register",
            &PrimWriterWrapper<>::Register,
            (boost::python::arg("class"), boost::python::arg("mayaTypeName")))
        .staticmethod("Register")
        ;
}

//----------------------------------------------------------------------------------------------------------------------
void wrapShaderWriter()
{
    typedef ShaderWriterWrapper* ShaderWriterWrapperPtr;

    boost::python::class_<ShaderWriterWrapper, boost::python::bases<PrimWriterWrapper<>>, boost::noncopyable>(
        "ShaderWriter", boost::python::no_init)
        .def("__init__", make_constructor(&ShaderWriterWrapper::New))
        .def("GetShadingAttributeNameForMayaAttrName", &UsdMayaShaderWriter::GetShadingAttributeNameForMayaAttrName, &ShaderWriterWrapper::default_GetShadingAttributeNameForMayaAttrName)
        .def("GetShadingAttributeForMayaAttrName", &UsdMayaShaderWriter::GetShadingAttributeForMayaAttrName, &ShaderWriterWrapper::default_GetShadingAttributeForMayaAttrName)
        .def(
            "Register",
            &ShaderWriterWrapper::Register,
            (boost::python::arg("class"), boost::python::arg("mayaTypeName")))
        .staticmethod("Register")
        ;
}
