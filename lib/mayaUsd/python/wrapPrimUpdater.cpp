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

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    PrimUpdaterWrapper() = default;

    PrimUpdaterWrapper(const MFnDependencyNode& node, const Ufe::Path& path)
        : UsdMayaPrimUpdater(node, path)
    {
    }

    virtual ~PrimUpdaterWrapper() = default;

    PushCopySpecs default_pushCopySpecs(
        UsdStageRefPtr srcStage,
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        UsdStageRefPtr dstStage,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath)
    {
        return base_t::pushCopySpecs(
            srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath);
    }
    PushCopySpecs pushCopySpecs(
        UsdStageRefPtr srcStage,
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        UsdStageRefPtr dstStage,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath) override
    {
        return this->CallVirtual<PushCopySpecs>("pushCopySpecs", &This::default_pushCopySpecs)(
            srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath);
    }

    bool default_shouldAutoEdit() const { return base_t::shouldAutoEdit(); }
    bool shouldAutoEdit() const override
    {
        return this->CallVirtual<bool>("shouldAutoEdit", &This::default_shouldAutoEdit)();
    }

    bool default_canEditAsMaya() const { return base_t::canEditAsMaya(); }
    bool canEditAsMaya() const override
    {
        return this->CallVirtual<bool>("canEditAsMaya", &This::default_canEditAsMaya)();
    }

    bool default_editAsMaya(const UsdMayaPrimUpdaterContext& context)
    {
        return base_t::editAsMaya(context);
    }
    bool editAsMaya(const UsdMayaPrimUpdaterContext& context) override
    {
        return this->CallVirtual<bool>("editAsMaya", &This::default_editAsMaya)(context);
    }

    bool default_discardEdits(const UsdMayaPrimUpdaterContext& context)
    {
        return base_t::discardEdits(context);
    }
    bool discardEdits(const UsdMayaPrimUpdaterContext& context) override
    {
        return this->CallVirtual<bool>("discardEdits", &This::default_discardEdits)(context);
    }

    bool default_pushEnd(const UsdMayaPrimUpdaterContext& context)
    {
        return base_t::pushEnd(context);
    }
    bool pushEnd(const UsdMayaPrimUpdaterContext& context) override
    {
        return this->CallVirtual<bool>("pushEnd", &This::default_pushEnd)(context);
    }

    //---------------------------------------------------------------------------------------------
    /// \brief  wraps a factory function that allows registering an updated Python class
    //---------------------------------------------------------------------------------------------
    class FactoryFnWrapper : public UsdMayaPythonObjectRegistry
    {
    public:
        // Instances of this class act as "function objects" that are fully compatible with the
        // std::function requested by UsdMayaSchemaApiAdaptorRegistry::Register. These will create
        // python wrappers based on the latest class registered.
        UsdMayaPrimUpdaterSharedPtr operator()(const MFnDependencyNode& node, const Ufe::Path& path)
        {
            boost::python::object pyClass = GetPythonObject(_classIndex);
            if (!pyClass) {
                // Prototype was unregistered
                return nullptr;
            }
            auto                  primUpdater = std::make_shared<This>(node, path);
            TfPyLock              pyLock;
            boost::python::object instance = pyClass((uintptr_t)&primUpdater);
            boost::python::incref(instance.ptr());
            initialize_wrapper(instance.ptr(), primUpdater.get());
            return primUpdater;
        }

        // Create a new wrapper for a Python class that is seen for the first time for a given
        // purpose. It we already have a registration for this purpose: update the class to
        // allow the previously issued factory function to use it.
        static UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn Register(
            boost::python::object cl,
            const std::string&    usdTypeName,
            const std::string&    mayaType,
            int                   sup)
        {
            size_t classIndex = RegisterPythonObject(cl, GetKey(cl, usdTypeName, mayaType, sup));
            if (classIndex != UsdMayaPythonObjectRegistry::UPDATED) {
                // Return a new factory function:
                return FactoryFnWrapper { classIndex };
            } else {
                // We already registered a factory function for this purpose:
                return nullptr;
            }
        }

        // Unregister a class for a given purpose. This will cause the associated factory
        // function to stop producing this Python class.
        static void Unregister(
            boost::python::object cl,
            const std::string&    usdTypeName,
            const std::string&    mayaType,
            int                   sup)
        {
            UnregisterPythonObject(cl, GetKey(cl, usdTypeName, mayaType, sup));
        }

    private:
        // Function object constructor. Requires only the index of the Python class to use.
        FactoryFnWrapper(size_t classIndex)
            : _classIndex(classIndex) {};

        size_t _classIndex;

        // Generates a unique key based on the name of the class, along with the class
        // purpose:
        static std::string GetKey(
            boost::python::object cl,
            const std::string&    usdTypeName,
            const std::string&    mayaType,
            int                   sup)
        {
            return ClassName(cl) + "," + usdTypeName + "," + mayaType + "," + std::to_string(sup)
                + ",PrimUpdater";
        }
    };

    static void Unregister(
        boost::python::object cl,
        const std::string&    usdTypeName,
        const std::string&    mayaType,
        int                   sup)
    {
        FactoryFnWrapper::Unregister(cl, usdTypeName, mayaType, sup);
    }

    static void Register(
        boost::python::object cl,
        const std::string&    usdTypeName,
        const std::string&    mayaType,
        int                   sup)
    {
        UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn fn
            = FactoryFnWrapper::Register(cl, usdTypeName, mayaType, sup);
        if (fn) {
            auto type = TfType::FindByName(usdTypeName);

            UsdMayaPrimUpdaterRegistry::Register(
                type, mayaType, UsdMayaPrimUpdater::Supports(sup), fn, true);
        }
    }
};

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimUpdaterArgs()
{
    boost::python::class_<UsdMayaPrimUpdaterArgs>("PrimUpdaterArgs", boost::python::no_init)
        .def("createFromDictionary", &UsdMayaPrimUpdaterArgs::createFromDictionary)
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
        .def("MapSdfPathToDagPath", &UsdMayaPrimUpdaterContext::MapSdfPathToDagPath);
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Invalid, "Invalid");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Push, "Push");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Pull, "Pull");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::Clear, "Clear");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::AutoPull, "AutoPull");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::Supports::All, "All");

    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::PushCopySpecs::Failed, "Failed");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::PushCopySpecs::Continue, "Continue");
    TF_ADD_ENUM_NAME(UsdMayaPrimUpdater::PushCopySpecs::Prune, "Prune");
}

//----------------------------------------------------------------------------------------------------------------------
void wrapPrimUpdater()
{
    typedef UsdMayaPrimUpdater This;

    boost::python::class_<PrimUpdaterWrapper, boost::noncopyable> c(
        "PrimUpdater", boost::python::no_init);

    boost::python::scope s(c);

    TfPyWrapEnum<UsdMayaPrimUpdater::Supports>();
    TfPyWrapEnum<UsdMayaPrimUpdater::PushCopySpecs>();

    c.def("__init__", make_constructor(&PrimUpdaterWrapper::New))
        .def("pushCopySpecs", &This::pushCopySpecs, &PrimUpdaterWrapper::default_pushCopySpecs)
        .def("shouldAutoEdit", &This::shouldAutoEdit, &PrimUpdaterWrapper::default_shouldAutoEdit)
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
        .staticmethod("Register")
        .def("Unregister", &PrimUpdaterWrapper::Unregister)
        .staticmethod("Unregister");
}
