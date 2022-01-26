//
// Copyright 2018 Pixar
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
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/utils/undoHelperCommand.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyPolymorphic.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/pyConversions.h>
#include <pxr/usd/usdGeom/primvar.h>

#include <maya/MObject.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/self.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

class SchemaAdaptorWrapper
    : public UsdMayaSchemaAdaptor
    , public TfPyPolymorphic<UsdMayaSchemaAdaptor>
{
public:
    typedef SchemaAdaptorWrapper This;
    typedef UsdMayaSchemaAdaptor base_t;

    SchemaAdaptorWrapper() { }

    SchemaAdaptorWrapper(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef)
        : base_t(object, schemaName, schemaPrimDef)
    {
    }

    static std::shared_ptr<This> New(uintptr_t createdWrapper)
    {
        return *((std::shared_ptr<This>*)createdWrapper);
    }

    virtual ~SchemaAdaptorWrapper() { }

    UsdMayaAttributeAdaptor default_GetAttribute(const TfToken& attrName) const
    {
        return base_t::GetAttribute(attrName);
    }
    UsdMayaAttributeAdaptor GetAttribute(const TfToken& attrName) const override
    {
        return this->CallVirtual("GetAttribute", &This::default_GetAttribute)(attrName);
    }

    TfTokenVector default_GetAuthoredAttributeNames() const
    {
        return base_t::GetAuthoredAttributeNames();
    }
    TfTokenVector GetAuthoredAttributeNames() const override
    {
        return this->CallVirtual(
            "GetAuthoredAttributeNames", &This::default_GetAuthoredAttributeNames)();
    }

    UsdMayaAttributeAdaptor
    default_UndoableCreateAttribute(const TfToken& attrName, MDGModifier& modifier)
    {
        return base_t::CreateAttribute(attrName, modifier);
    }
    UsdMayaAttributeAdaptor CreateAttribute(const TfToken& attrName, MDGModifier& modifier) override
    {
        // Not using TfPolymorphic::CallVirtual because MDGModifier is non-copyable
        TfPyLock pyLock;
        auto     pyOverride = this->GetOverride("UndoableCreateAttribute");
        if (pyOverride) {
            // Do *not* call through if there's an active python exception.
            if (!PyErr_Occurred()) {
                try {
                    return boost::python::call<UsdMayaAttributeAdaptor>(
                        pyOverride.ptr(), attrName, modifier);
                } catch (boost::python::error_already_set const&) {
                    // Convert any exception to TF_ERRORs.
                    TfPyConvertPythonExceptionToTfErrors();
                    PyErr_Clear();
                }
            }
        }
        return default_UndoableCreateAttribute(attrName, modifier);
    }

    void default_UndoableRemoveAttribute(const TfToken& attrName, MDGModifier& modifier)
    {
        base_t::RemoveAttribute(attrName, modifier);
    }
    void RemoveAttribute(const TfToken& attrName, MDGModifier& modifier) override
    {
        // Not using TfPolymorphic::CallVirtual because MDGModifier is non-copyable
        TfPyLock pyLock;
        auto     pyOverride = this->GetOverride("UndoableRemoveAttribute");
        if (pyOverride) {
            // Do *not* call through if there's an active python exception.
            if (!PyErr_Occurred()) {
                try {
                    return boost::python::call<void>(pyOverride.ptr(), attrName, modifier);
                } catch (boost::python::error_already_set const&) {
                    // Convert any exception to TF_ERRORs.
                    TfPyConvertPythonExceptionToTfErrors();
                    PyErr_Clear();
                }
            }
        }
        return default_UndoableRemoveAttribute(attrName, modifier);
    }
};
using SchemaAdaptorWrapperPtr = std::shared_ptr<SchemaAdaptorWrapper>;

static UsdMayaAdaptor* _Adaptor__init__(const std::string& dagPath)
{
    MObject object;
    MStatus status = UsdMayaUtil::GetMObjectByName(dagPath, object);
    if (!status) {
        return new UsdMayaAdaptor(MObject::kNullObj);
    }

    return new UsdMayaAdaptor(object);
}

static boost::python::object _Adaptor_GetMetadata(const UsdMayaAdaptor& self, const TfToken& key)
{
    VtValue value;
    if (self.GetMetadata(key, &value)) {
        return boost::python::object(value);
    }
    return boost::python::object();
}

static bool _Adaptor_SetMetadata(UsdMayaAdaptor& self, const TfToken& key, const VtValue& value)
{
    return UsdMayaUndoHelperCommand::ExecuteWithUndo<bool>(
        [&self, &key, &value](MDGModifier& modifier) {
            return self.SetMetadata(key, value, modifier);
        });
}

static void _Adaptor_ClearMetadata(UsdMayaAdaptor& self, const TfToken& key)
{
    UsdMayaUndoHelperCommand::ExecuteWithUndo(
        [&self, &key](MDGModifier& modifier) { self.ClearMetadata(key, modifier); });
}

static UsdMayaSchemaAdaptorPtr _Adaptor_ApplySchema(UsdMayaAdaptor& self, const TfType& ty)
{
    typedef UsdMayaSchemaAdaptorPtr Result;
    return UsdMayaUndoHelperCommand::ExecuteWithUndo<Result>(
        [&self, &ty](MDGModifier& modifier) { return self.ApplySchema(ty, modifier); });
}

static UsdMayaSchemaAdaptorPtr
_Adaptor_ApplySchemaByName(UsdMayaAdaptor& self, const TfToken& schemaName)
{
    typedef UsdMayaSchemaAdaptorPtr Result;
    return UsdMayaUndoHelperCommand::ExecuteWithUndo<Result>(
        [&self, &schemaName](MDGModifier& modifier) {
            return self.ApplySchemaByName(schemaName, modifier);
        });
}

static void _Adaptor_UnapplySchema(UsdMayaAdaptor& self, const TfType& ty)
{
    UsdMayaUndoHelperCommand::ExecuteWithUndo(
        [&self, &ty](MDGModifier& modifier) { self.UnapplySchema(ty, modifier); });
}

static void _Adaptor_UnapplySchemaByName(UsdMayaAdaptor& self, const TfToken& schemaName)
{
    UsdMayaUndoHelperCommand::ExecuteWithUndo([&self, &schemaName](MDGModifier& modifier) {
        self.UnapplySchemaByName(schemaName, modifier);
    });
}

static std::string _Adaptor__repr__(const UsdMayaAdaptor& self)
{
    if (self) {
        return TfStringPrintf(
            "%sAdaptor('%s')", TF_PY_REPR_PREFIX.c_str(), self.GetMayaNodeName().c_str());
    } else {
        return "invalid adaptor";
    }
}

static UsdMayaAttributeAdaptor
_SchemaAdaptor_CreateAttribute(UsdMayaSchemaAdaptorPtr& self, const TfToken& attrName)
{
    typedef UsdMayaAttributeAdaptor Result;
    return UsdMayaUndoHelperCommand::ExecuteWithUndo<Result>(
        [&self, &attrName](MDGModifier& modifier) {
            return self->CreateAttribute(attrName, modifier);
        });
}

static void _SchemaAdaptor_RemoveAttribute(UsdMayaSchemaAdaptorPtr& self, const TfToken& attrName)
{
    UsdMayaUndoHelperCommand::ExecuteWithUndo([&self, &attrName](MDGModifier& modifier) {
        return self->RemoveAttribute(attrName, modifier);
    });
}

static std::string _SchemaAdaptor__repr__(const UsdMayaSchemaAdaptorPtr& self)
{
    if (self) {
        return TfStringPrintf("UsdMayaSchemaAdaptor<%s>", self->GetName().GetText());
    } else {
        return "invalid schema adaptor";
    }
}

static boost::python::object _AttributeAdaptor_Get(const UsdMayaAttributeAdaptor& self)
{
    VtValue value;
    if (self.Get(&value)) {
        return boost::python::object(value);
    }
    return boost::python::object();
}

static bool _AttributeAdaptor_Set(UsdMayaAttributeAdaptor& self, const VtValue& value)
{
    return UsdMayaUndoHelperCommand::ExecuteWithUndo<bool>(
        [&self, &value](MDGModifier& modifier) { return self.Set(value, modifier); });
}

static std::string _AttributeAdaptor__repr__(const UsdMayaAttributeAdaptor& self)
{
    std::string                  schemaName;
    const SdfAttributeSpecHandle attrDef = self.GetAttributeDefinition();
    if (TF_VERIFY(attrDef)) {
        const SdfPrimSpecHandle schemaDef
            = TfDynamic_cast<const SdfPrimSpecHandle>(attrDef->GetOwner());
        if (TF_VERIFY(schemaDef)) {
            schemaName = schemaDef->GetName();
        }
    }

    if (self) {
        return TfStringPrintf(
            "UsdMayaAttributeAdaptor<%s:%s>", schemaName.c_str(), self.GetName().GetText());
    } else {
        return "invalid attribute adaptor";
    }
}

static void RegisterTypedSchemaConversion(const std::string& nodeTypeName, const TfType& usdType)
{
    UsdMayaAdaptor::RegisterTypedSchemaConversion(nodeTypeName, usdType, true);
}

static void RegisterAttributeAlias(const TfToken& attributeName, const std::string& alias)
{
    UsdMayaAdaptor::RegisterAttributeAlias(attributeName, alias, true);
}

void wrapAdaptor()
{
    typedef UsdMayaAdaptor This;
    scope                  Adaptor
        = class_<This>("Adaptor", no_init)
              .def(!self)
              .def("__init__", make_constructor(_Adaptor__init__))
              .def("__repr__", _Adaptor__repr__)
              .def("GetMayaNodeName", &This::GetMayaNodeName)
              .def("GetUsdTypeName", &This::GetUsdTypeName)
              .def("GetUsdType", &This::GetUsdType)
              .def("GetAppliedSchemas", &This::GetAppliedSchemas)
              .def("GetSchema", &This::GetSchema)
              .def(
                  "GetSchemaByName",
                  (UsdMayaSchemaAdaptorPtr(This::*)(const TfToken&) const) & This::GetSchemaByName)
              .def(
                  "GetSchemaOrInheritedSchema",
                  (UsdMayaSchemaAdaptorPtr(This::*)(const TfType&) const)
                      & This::GetSchemaOrInheritedSchema)
              .def("ApplySchema", _Adaptor_ApplySchema)
              .def("ApplySchemaByName", _Adaptor_ApplySchemaByName)
              .def("UnapplySchema", _Adaptor_UnapplySchema)
              .def("UnapplySchemaByName", _Adaptor_UnapplySchemaByName)
              .def(
                  "GetAllAuthoredMetadata",
                  &This::GetAllAuthoredMetadata,
                  return_value_policy<TfPyMapToDictionary>())
              .def("GetMetadata", _Adaptor_GetMetadata)
              .def("SetMetadata", _Adaptor_SetMetadata)
              .def("ClearMetadata", _Adaptor_ClearMetadata)
              .def("GetPrimMetadataFields", &This::GetPrimMetadataFields)
              .staticmethod("GetPrimMetadataFields")
              .def(
                  "GetRegisteredAPISchemas",
                  &This::GetRegisteredAPISchemas,
                  return_value_policy<TfPySequenceToList>())
              .staticmethod("GetRegisteredAPISchemas")
              .def(
                  "GetRegisteredTypedSchemas",
                  &This::GetRegisteredTypedSchemas,
                  return_value_policy<TfPySequenceToList>())
              .staticmethod("GetRegisteredTypedSchemas")
              .def("RegisterAttributeAlias", &::RegisterAttributeAlias)
              .staticmethod("RegisterAttributeAlias")
              .def("GetAttributeAliases", &This::GetAttributeAliases)
              .staticmethod("GetAttributeAliases")
              .def("RegisterTypedSchemaConversion", &::RegisterTypedSchemaConversion)
              .staticmethod("RegisterTypedSchemaConversion");

    class_<UsdMayaAttributeAdaptor>("AttributeAdaptor")
        .def(!self)
        .def("__repr__", _AttributeAdaptor__repr__)
        .def("GetName", &UsdMayaAttributeAdaptor::GetName)
        .def("Get", _AttributeAdaptor_Get)
        .def("Set", _AttributeAdaptor_Set)
        .def("GetAttributeDefinition", &UsdMayaAttributeAdaptor::GetAttributeDefinition);

    class_<SchemaAdaptorWrapper, boost::noncopyable> c("SchemaAdaptor", boost::python::no_init);
    boost::python::scope                             s(c);

    c.def(!self)
        .def("__init__", make_constructor(&SchemaAdaptorWrapper::New))
        .def("__repr__", _SchemaAdaptor__repr__)
        .def("GetName", &UsdMayaSchemaAdaptor::GetName)
        .def(
            "GetAttribute",
            &SchemaAdaptorWrapper::GetAttribute,
            &SchemaAdaptorWrapper::default_GetAttribute)
        .def("CreateAttribute", _SchemaAdaptor_CreateAttribute)
        .def("RemoveAttribute", _SchemaAdaptor_RemoveAttribute)
        .def(
            "GetAuthoredAttributeNames",
            &SchemaAdaptorWrapper::GetAuthoredAttributeNames,
            &SchemaAdaptorWrapper::default_GetAuthoredAttributeNames)
        .def("GetAttributeNames", &UsdMayaSchemaAdaptor::GetAttributeNames);

    // For wrapping UsdMayaSchemaAdaptor created in c++
    boost::python::class_<UsdMayaSchemaAdaptor, UsdMayaSchemaAdaptorPtr, boost::noncopyable>(
        "SchemaAdaptor", boost::python::no_init)
        .def(!self)
        .def("GetName", &UsdMayaSchemaAdaptor::GetName)
        .def("GetAttribute", &UsdMayaSchemaAdaptor::GetAttribute)
        .def("CreateAttribute", _SchemaAdaptor_CreateAttribute)
        .def("RemoveAttribute", _SchemaAdaptor_RemoveAttribute)
        .def("GetAuthoredAttributeNames", &UsdMayaSchemaAdaptor::GetAuthoredAttributeNames)
        .def("GetAttributeNames", &UsdMayaSchemaAdaptor::GetAttributeNames);
}
