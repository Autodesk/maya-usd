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

#include <mayaUsd/fileio/primUpdater.h>
#include <mayaUsd/fileio/primUpdaterContext.h>
#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaPrimUpdater
//----------------------------------------------------------------------------------------------------------------------
class PrimUpdaterWrapper
    : public UsdMayaPrimUpdater
    , public TfPyPolymorphic<UsdMayaPrimUpdater>
{
public:
    typedef PrimUpdaterWrapper This;
    typedef UsdMayaPrimUpdater base_t;

    static PrimUpdaterWrapper* New(uintptr_t createdWrapper)
    {
        return (PrimUpdaterWrapper*)createdWrapper;
    }

    PrimUpdaterWrapper() = default;

    PrimUpdaterWrapper(const MFnDependencyNode& node, const Ufe::Path& path)
        : UsdMayaPrimUpdater(node, path)
    {}

    virtual ~PrimUpdaterWrapper() = default;

    bool default_pushCopySpecs(
        UsdStageRefPtr srcStage,
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        UsdStageRefPtr dstStage,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath)
    {
        return base_t::pushCopySpecs(srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath);
    }
    bool pushCopySpecs(
        UsdStageRefPtr srcStage,
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        UsdStageRefPtr dstStage,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath)
    {
        return this->CallVirtual<bool>("pushCopySpecs", &This::default_pushCopySpecs)(
            srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath);
    }

    bool default_canEditAsMaya() const
    {
        return base_t::canEditAsMaya();
    }
    bool canEditAsMaya() const
    {
        return this->CallVirtual<bool>("canEditAsMaya", &This::default_canEditAsMaya)();
    }

    bool default_editAsMaya(const UsdMayaPrimUpdaterContext& context) { return base_t::editAsMaya(context); }
    bool editAsMaya(const UsdMayaPrimUpdaterContext& context)
    {
        return this->CallVirtual<bool>("editAsMaya", &This::default_editAsMaya)(context);
    }

    bool default_discardEdits(const UsdMayaPrimUpdaterContext& context)
    {
        return base_t::discardEdits(context);
    }
    bool discardEdits(const UsdMayaPrimUpdaterContext& context)
    {
        return this->CallVirtual<bool>("discardEdits", &This::default_discardEdits)(context);
    }

    bool default_pushEnd(const UsdMayaPrimUpdaterContext& context)
    {
        return base_t::pushEnd(context);
    }
    bool pushEnd(const UsdMayaPrimUpdaterContext& context) {
        return this->CallVirtual<bool>("pushEnd", &This::default_pushEnd)(context);
    }

    static void Register(
        boost::python::object        cl,
        const std::string&           typeName,
        const std::string&           mayaType,
        int                          sup)
    {
        auto type = TfType::FindByName(typeName);

        UsdMayaPrimUpdaterRegistry::Register(
            type,
            mayaType,
            UsdMayaPrimUpdater::Supports(sup),
            [=](const MFnDependencyNode& node, const Ufe::Path& path) {
                auto                  primUpdater = std::make_shared<This>(node, path);
                TfPyLock              pyLock;
                boost::python::object instance = cl((uintptr_t)&primUpdater);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), primUpdater);
                return primUpdater;
            },
            true);
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimUpdaterArgs() {
    boost::python::class_<UsdMayaPrimUpdaterArgs>("PrimUpdaterArgs", boost::python::no_init)
        .def(
            "createFromDictionary",
            &UsdMayaPrimUpdaterArgs::createFromDictionary)
        .staticmethod("createFromDictionary")
        .def(
            "getDefaultDictionary",
            &UsdMayaPrimUpdaterArgs::getDefaultDictionary,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("getDefaultDictionary");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimUpdaterContext()
{
    boost::python::class_<UsdMayaPrimUpdaterContext>("PrimUpdaterContext", boost::python::no_init)
        .def(
            "GetTimeCode",
            &UsdMayaPrimUpdaterContext::GetTimeCode,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetUsdStage",
            &UsdMayaPrimUpdaterContext::GetUsdStage,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetUserArgs",
            &UsdMayaPrimUpdaterContext::GetUserArgs,
            boost::python::return_value_policy<boost::python::return_by_value>())
        .def(
            "GetArgs",
            &UsdMayaPrimUpdaterContext::GetArgs,
            boost::python::return_internal_reference<>())
        .def(
            "MapSdfPathToDagPath",
            &UsdMayaPrimUpdaterContext::MapSdfPathToDagPath);
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Invalid, "Invalid");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Push, "Push");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Pull, "Pull");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Clear, "Clear");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::AutoPull, "AutoPull");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::All, "All");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimUpdater()
{
    typedef UsdMayaPrimUpdater This;

    boost::python::class_<PrimUpdaterWrapper, boost::noncopyable>
        c("PrimUpdater", boost::python::no_init);

        boost::python::scope s(c);

        TfPyWrapEnum<UsdMayaPrimUpdater::Supports>();

        c.def("__init__", make_constructor(&PrimUpdaterWrapper::New))
            .def("pushCopySpecs", &This::pushCopySpecs, &PrimUpdaterWrapper::default_pushCopySpecs)
            .def("canEditAsMaya", &This::canEditAsMaya, &PrimUpdaterWrapper::default_canEditAsMaya)
            .def("editAsMaya", &This::editAsMaya, &PrimUpdaterWrapper::default_editAsMaya)
            .def("discardEdits", &This::discardEdits, &PrimUpdaterWrapper::default_discardEdits)
            .def("pushEnd", &This::pushEnd, &PrimUpdaterWrapper::default_pushEnd)
            .def(
                "getMayaObject",
                &This::getMayaObject,
                boost::python::return_value_policy<boost::python::return_by_value>())
            .def("getUfePath", &This::getUfePath, boost::python::return_internal_reference<>())
            .def("getUsdPrim", &This::getUsdPrim)
            .def("isAnimated", &This::isAnimated)
            .staticmethod("isAnimated")
            .def("Register", &PrimUpdaterWrapper::Register)
            .staticmethod("Register");
}
