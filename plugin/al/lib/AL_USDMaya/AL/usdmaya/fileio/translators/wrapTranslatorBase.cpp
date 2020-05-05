//
// Copyright 2017 Animal Logic
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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyPtrHelpers.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/refPtr.h>

#include <maya/MBoundingBox.h>
#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>

#include <functional>
#include <memory>

using namespace AL::usdmaya::fileio::translators;
using namespace AL::usdmaya::nodes;

namespace {

struct MStatusFromPythonBool
{
    static void Register()
    {
        // from-python
        boost::python::converter::registry::push_back(
            &_convertible, &_construct, boost::python::type_id<MStatus>());
    }

private:
    // from-python
    static void* _convertible(PyObject* p) { return PyBool_Check(p) ? p : nullptr; }
    static void
    _construct(PyObject* p, boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        // Turn the python float into a C++ double, make a GfHalf from that
        // double, and store it where boost.python expects it.
        void* storage
            = ((boost::python::converter::rvalue_from_python_storage<MStatus>*)data)->storage.bytes;
        MStatus status = PyObject_IsTrue(p) ? MStatus::kSuccess : MStatus::kFailure;
        new (storage) MStatus(status);
        data->convertible = storage;
    }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the PluginTranslator
//----------------------------------------------------------------------------------------------------------------------
class TranslatorBaseWrapper
    : public TranslatorBase
    , public TfPyPolymorphic<TranslatorBase>
{
public:
    typedef TranslatorBaseWrapper This;
    typedef TranslatorBase        base_t;
    typedef TranslatorRefPtr      refptr_t;
    typedef TranslatorManufacture manufacture_t;

    static TfRefPtr<TranslatorBaseWrapper> New()
    {
        return TfCreateRefPtr(new TranslatorBaseWrapper);
    }

    virtual ~TranslatorBaseWrapper();

    TfType getTranslatedType() const override
    {
        return this->CallPureVirtual<TfType>("getTranslatedType")();
    }

    std::size_t generateUniqueKey(const UsdPrim& prim) const override
    {
        if (Override o = GetOverride("generateUniqueKey")) {
            auto res = std::function<boost::python::object(const UsdPrim&)>(
                TfPyCall<boost::python::object>(o))(prim);
            if (!res) {
                return 0;
            }
            TfPyLock                            pyLock;
            boost::python::str                  strObj(res);
            boost::python::extract<std::string> strValue(strObj);
            if (strValue.check()) {
                return std::hash<std::string> {}(strValue);
            }
        }
        return 0;
    }

    bool needsTransformParent() const override
    {
        return this->CallVirtual<bool>("needsTransformParent", &This::needsTransformParent)();
    }

    bool supportsUpdate() const override
    {
        return this->CallVirtual<bool>("supportsUpdate", &This::supportsUpdate)();
    }

    bool importableByDefault() const override
    {
        return this->CallVirtual<bool>("importableByDefault", &This::importableByDefault)();
    }

    MStatus initialize() override
    {
        return this->CallVirtual<MStatus>("initialize", &This::initialize)();
    }

    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override
    {
        // "import" is a python keyword so python's override will be called
        // "importObject" instead
        if (Override o = GetOverride("importObject")) {
            MDagPath path;
            MDagPath::getAPathTo(parent, path);

            auto res = std::function<bool(const UsdPrim&, const char*)>(TfPyCall<bool>(o))(
                prim, path.fullPathName().asChar());
            return res ? MS::kSuccess : MS::kFailure;
        }
        return MS::kSuccess;
    }

    MStatus postImport(const UsdPrim& prim) override
    {
        if (Override o = GetOverride("postImport")) {
            auto res = std::function<bool(const UsdPrim&)>(TfPyCall<bool>(o))(prim);
            return res ? MS::kSuccess : MS::kFailure;
        }
        return MS::kSuccess;
    }

    MStatus preTearDown(UsdPrim& prim) override
    {
        return this->CallVirtual<MStatus>("preTearDown", &This::preTearDown)(prim);
    }

    MStatus tearDown(const SdfPath& path) override
    {
        return this->CallVirtual<MStatus>("tearDown", &This::tearDown)(path);
    }

    MStatus update(const UsdPrim& prim) override
    {
        return this->CallVirtual<MStatus>("update", &This::update)(prim);
    }

    ExportFlag canExport(const MObject& obj) override
    {
        MFnDependencyNode fn(obj);
        std::string       name(fn.name().asChar());

        // pass a string to the python method instead of a dependency node
        if (Override o = GetOverride("canExport")) {
            return std::function<ExportFlag(const char*)>(TfPyCall<ExportFlag>(o))(name.c_str());
        }
        return ExportFlag::kNotSupported;
    }

    UsdPrim exportObject(
        UsdStageRefPtr                             stage,
        MDagPath                                   dagPath,
        const SdfPath&                             usdPath,
        const AL::usdmaya::fileio::ExporterParams& params) override
    {
        // note: an incomplete list, add as needed
        boost::python::dict pyParams;
        pyParams["dynamicAttributes"] = params.m_dynamicAttributes;
        pyParams["m_minFrame"] = params.m_minFrame;
        pyParams["m_maxFrame"] = params.m_maxFrame;

        std::string name(dagPath.fullPathName().asChar());
        // pass a dagPath name and dictionary of params to the python method
        if (Override o = GetOverride("exportObject")) {
            return std::function<UsdPrim(
                UsdStageRefPtr, const char*, SdfPath, boost::python::dict)>(TfPyCall<UsdPrim>(o))(
                stage, name.c_str(), usdPath, pyParams);
        }
        return UsdPrim();
    }

    static void registerTranslator(refptr_t plugin, const TfToken& assetType = TfToken())
    {
        if (!manufacture_t::addPythonTranslator(plugin, assetType)) {
            MGlobal::displayWarning("Cannot register python translator because of unknown type");
            return;
        }

        MFnDependencyNode  fn;
        MItDependencyNodes iter(MFn::kPluginShape);
        for (; !iter.isDone(); iter.next()) {
            auto mobj = iter.thisNode();
            fn.setObject(mobj);
            if (fn.typeId() != ProxyShape::kTypeId)
                continue;
            if (auto proxyShape = static_cast<ProxyShape*>(fn.userNode())) {
                auto manufacture = proxyShape->translatorManufacture();
                auto context = proxyShape->context();
                manufacture.updatePythonTranslators(context);
            }
        }
    }

    static bool unregisterTranslator(const std::string& typeName)
    {
        auto type = TfType::FindByName(typeName);
        return manufacture_t::deletePythonTranslator(type);
    }

    static void clearTranslators() { manufacture_t::clearPythonTranslators(); }

    UsdStageRefPtr stage() const { return context()->getUsdStage(); }

    void insertItem(
        const boost::shared_ptr<UsdPrim>& primBeingImported,
        const std::string&                nodeNameOrPath)
    {
        MSelectionList sl;
        MObject        object;
        sl.add(nodeNameOrPath.c_str());
        sl.getDependNode(0, object);
        auto ctx = context();
        ctx->insertItem(*primBeingImported, object);
    }

    void removeItems(const boost::shared_ptr<SdfPath>& primPathBeingRemoved)
    {
        auto ctx = context();
        ctx->removeItems(*primPathBeingRemoved);
    }

    std::vector<std::string> getMObjects(boost::shared_ptr<UsdPrim> prim)
    {
        MObjectHandleArray returned;
        context()->getMObjects(*prim, returned);
        std::vector<std::string> names(returned.size());
        MFnDependencyNode        fn;
        for (size_t i = 0, n = returned.size(); i != n; ++i) {
            fn.setObject(returned[i].object());
            names[i] = fn.name().asChar();
        }
        return names;
    }
};

TranslatorBaseWrapper::~TranslatorBaseWrapper()
{
    //
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
void wrapTranslatorBase()
{
    typedef TfWeakPtr<TranslatorBaseWrapper>                           TranslatorBaseWrapperPtr;
    typedef TfRefPtr<AL::usdmaya::fileio::translators::TranslatorBase> TranslatorBasePtr;

    boost::python::import("maya");

    boost::python::enum_<ExportFlag>("ExportFlag")
        .value("kNotSupported", ExportFlag::kNotSupported)
        .value("kFallbackSupport", ExportFlag::kFallbackSupport)
        .value("kSupported", ExportFlag::kSupported);

    boost::python::class_<TranslatorBaseWrapper, TranslatorBaseWrapperPtr, boost::noncopyable>(
        "TranslatorBase", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&TranslatorBaseWrapper::New))
        .def("initialize", &TranslatorBase::initialize, &TranslatorBaseWrapper::initialize)
        .def("getTranslatedType", boost::python::pure_virtual(&TranslatorBase::getTranslatedType))
        .def(
            "generateUniqueKey",
            &TranslatorBase::generateUniqueKey,
            &TranslatorBaseWrapper::generateUniqueKey)
        .def("context", &TranslatorBase::context)
        .def(
            "needsTransformParent",
            &TranslatorBase::needsTransformParent,
            &TranslatorBaseWrapper::needsTransformParent)
        .def(
            "importableByDefault",
            &TranslatorBase::importableByDefault,
            &TranslatorBaseWrapper::importableByDefault)
        .def("importObject", &TranslatorBase::import, &TranslatorBaseWrapper::import)
        .def("exportObject", &TranslatorBase::exportObject, &TranslatorBaseWrapper::exportObject)
        .def("postImport", &TranslatorBase::postImport, &TranslatorBaseWrapper::postImport)
        .def("preTearDown", &TranslatorBase::preTearDown, &TranslatorBaseWrapper::preTearDown)
        .def("tearDown", &TranslatorBase::tearDown, &TranslatorBaseWrapper::tearDown)
        .def("canExport", &TranslatorBase::canExport, &TranslatorBaseWrapper::canExport)
        .def("stage", &TranslatorBaseWrapper::stage)
        .def("getMObjects", &TranslatorBaseWrapper::getMObjects)
        .def(
            "registerTranslator",
            &TranslatorBaseWrapper::registerTranslator,
            (boost::python::arg("translator"), boost::python::arg("assetType") = TfToken()))
        .staticmethod("registerTranslator")
        .def("clearTranslators", &TranslatorBaseWrapper::clearTranslators)
        .staticmethod("clearTranslators")
        .def("unregisterTranslator", &TranslatorBaseWrapper::unregisterTranslator)
        .staticmethod("unregisterTranslator")
        .def("insertItem", &TranslatorBaseWrapper::insertItem)
        .def("removeItems", &TranslatorBaseWrapper::removeItems)
        .def("getPythonTranslators", &TranslatorManufacture::getPythonTranslators)
        .staticmethod("getPythonTranslators");

    boost::python::to_python_converter<
        std::vector<TranslatorBasePtr>,
        TfPySequenceToPython<std::vector<TranslatorBasePtr>>>();

    MStatusFromPythonBool::Register();
}

TF_REFPTR_CONST_VOLATILE_GET(TranslatorBaseWrapper)
TF_REFPTR_CONST_VOLATILE_GET(TranslatorBase)
