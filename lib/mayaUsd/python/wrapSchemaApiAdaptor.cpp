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

#include <maya/MTypes.h>

// Hack because MDGModifier assign operator is not public.
// For 2019, the hack is different see MDGModifier2019 in this file
#if MAYA_API_VERSION >= 20200000
#undef OPENMAYA_PRIVATE
#define OPENMAYA_PRIVATE public
#endif

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>

#include <pxr/base/tf/pyPolymorphic.h>

#include <maya/MDGModifier.h>

#include <boost/python/args.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  boost python binding for the UsdMayaSchemaApiAdaptor
//----------------------------------------------------------------------------------------------------------------------
class SchemaApiAdaptorWrapper
    : public UsdMayaSchemaApiAdaptor
    , public TfPyPolymorphic<UsdMayaSchemaApiAdaptor>
{
public:
    typedef SchemaApiAdaptorWrapper This;
    typedef UsdMayaSchemaApiAdaptor base_t;

    SchemaApiAdaptorWrapper() { }

    SchemaApiAdaptorWrapper(
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

    virtual ~SchemaApiAdaptorWrapper() { }

    bool default_CopyFromPrim(
        const UsdPrim&               prim,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext&    context)
    {
        return base_t::CopyFromPrim(prim, args, context);
    }
    bool CopyFromPrim(
        const UsdPrim&               prim,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext&    context) override
    {
        return this->CallVirtual("CopyFromPrim", &This::default_CopyFromPrim)(prim, args, context);
    }

    bool default_CopyToPrim(
        const UsdPrim&             prim,
        const UsdTimeCode&         usdTime,
        UsdUtilsSparseValueWriter* valueWriter) const
    {
        return base_t::CopyToPrim(prim, usdTime, valueWriter);
    }
    bool CopyToPrim(
        const UsdPrim&             prim,
        const UsdTimeCode&         usdTime,
        UsdUtilsSparseValueWriter* valueWriter) const override
    {
        return this->template CallVirtual<bool>("CopyToPrim", &This::default_CopyToPrim)(
            prim, usdTime, valueWriter);
    }

    bool default_CanAdapt() const { return base_t::CanAdapt(); }
    bool CanAdapt() const override
    {
        return this->template CallVirtual<bool>("CanAdapt", &This::default_CanAdapt)();
    }

    bool default_CanAdaptForExport(const UsdMayaJobExportArgs& args) const
    {
        return base_t::CanAdaptForExport(args);
    }
    bool CanAdaptForExport(const UsdMayaJobExportArgs& args) const override
    {
        return this->template CallVirtual<bool>(
            "CanAdaptForExport", &This::default_CanAdaptForExport)(args);
    }

    bool default_ApplySchema(MDGModifier& modifier) { return base_t::ApplySchema(modifier); }
    bool ApplySchema(MDGModifier& modifier) override
    {
        return this->CallVirtual(
            "ApplySchema", (bool (This::*)(MDGModifier&)) & This::default_ApplySchema)(modifier);
    }

    bool default_ApplySchema(
        const UsdMayaPrimReaderArgs& primReaderArgs,
        UsdMayaPrimReaderContext&    context)
    {
        return base_t::ApplySchema(primReaderArgs, context);
    }
    bool ApplySchema(const UsdMayaPrimReaderArgs& primReaderArgs, UsdMayaPrimReaderContext& context)
        override
    {
        return this->CallVirtual(
            "ApplySchema",
            (bool (This::*)(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&))
                & This::default_ApplySchema)(primReaderArgs, context);
    }

    bool default_UnapplySchema(MDGModifier& modifier) { return base_t::UnapplySchema(modifier); }
    bool UnapplySchema(MDGModifier& modifier) override
    {
        return this->template CallVirtual<bool>("UnapplySchema", &This::default_UnapplySchema)(
            modifier);
    }

    TfTokenVector default_GetAuthoredAttributeNames() const
    {
        return base_t::GetAuthoredAttributeNames();
    }
    TfTokenVector GetAuthoredAttributeNames() const override
    {
        return this->template CallVirtual<TfTokenVector>(
            "GetAuthoredAttributeNames", &This::default_GetAuthoredAttributeNames)();
    }

    UsdMayaAttributeAdaptor default_GetAttribute(const TfToken& attrName) const
    {
        return base_t::GetAttribute(attrName);
    }
    UsdMayaAttributeAdaptor GetAttribute(const TfToken& attrName) const override
    {
        return this->template CallVirtual<UsdMayaAttributeAdaptor>(
            "GetAttribute", &This::default_GetAttribute)(attrName);
    }

    UsdMayaAttributeAdaptor default_CreateAttribute(const TfToken& attrName, MDGModifier& modifier)
    {
        return base_t::CreateAttribute(attrName, modifier);
    }
    UsdMayaAttributeAdaptor CreateAttribute(const TfToken& attrName, MDGModifier& modifier) override
    {
        return this->template CallVirtual<UsdMayaAttributeAdaptor>(
            "CreateAttribute", &This::default_CreateAttribute)(attrName, modifier);
    }

    void default_RemoveAttribute(const TfToken& attrName, MDGModifier& modifier)
    {
        base_t::RemoveAttribute(attrName, modifier);
    }
    void RemoveAttribute(const TfToken& attrName, MDGModifier& modifier) override
    {
        this->template CallVirtual<>("RemoveAttribute", &This::default_RemoveAttribute)(
            attrName, modifier);
    }

    MObject default_GetMayaObjectForSchema() const { return base_t::GetMayaObjectForSchema(); }
    MObject GetMayaObjectForSchema() const override
    {
        return this->template CallVirtual<MObject>(
            "GetMayaObjectForSchema", &This::default_GetMayaObjectForSchema)();
    }

    TfToken default_GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
    {
        return base_t::GetMayaNameForUsdAttrName(usdAttrName);
    }
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override
    {
        return this->template CallVirtual<TfToken>(
            "GetMayaNameForUsdAttrName", &This::default_GetMayaNameForUsdAttrName)(usdAttrName);
    }

    TfTokenVector default_GetAdaptedAttributeNames() const
    {
        return base_t::GetAdaptedAttributeNames();
    }
    TfTokenVector GetAdaptedAttributeNames() const override
    {
        return this->template CallVirtual<TfTokenVector>(
            "TfTokenVector", &This::default_GetAdaptedAttributeNames)();
    }

    static void Register(
        boost::python::object cl,
        const std::string&    mayaType,
        const std::string&    schemaApiName)
    {
        UsdMayaSchemaApiAdaptorRegistry::Register(
            mayaType,
            schemaApiName,
            [=](const MObjectHandle&     object,
                const TfToken&           schemaName,
                const UsdPrimDefinition* schemaPrimDef) {
                auto     sptr = std::make_shared<This>(object, schemaName, schemaPrimDef);
                TfPyLock pyLock;
                boost::python::object instance = cl((uintptr_t)&sptr);
                boost::python::incref(instance.ptr());
                initialize_wrapper(instance.ptr(), sptr.get());
                return sptr;
            },
            true);
    }
};

void wrapSchemaApiAdaptor()
{
    typedef UsdMayaSchemaApiAdaptor This;

    boost::python::class_<SchemaApiAdaptorWrapper, boost::noncopyable>(
        "SchemaApiAdaptor", boost::python::no_init)
        .def("__init__", make_constructor(&SchemaApiAdaptorWrapper::New))

        .def("CopyFromPrim", &This::CopyFromPrim, &SchemaApiAdaptorWrapper::default_CopyFromPrim)
        .def("CopyToPrim", &This::CopyToPrim, &SchemaApiAdaptorWrapper::default_CopyToPrim)

        .def("CanAdapt", &This::CanAdapt, &SchemaApiAdaptorWrapper::default_CanAdapt)
        .def(
            "CanAdaptForExport",
            &This::CanAdaptForExport,
            &SchemaApiAdaptorWrapper::default_CanAdaptForExport)
        .def(
            "ApplySchema",
            (bool (This::*)(MDGModifier&)) & This::ApplySchema,
            (bool (This::*)(MDGModifier&)) & SchemaApiAdaptorWrapper::default_ApplySchema)
        .def(
            "ApplySchema",
            (bool (This::*)(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&))
                & This::ApplySchema,
            (bool (This::*)(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&))
                & SchemaApiAdaptorWrapper::default_ApplySchema)
        .def("UnapplySchema", &This::UnapplySchema, &SchemaApiAdaptorWrapper::default_UnapplySchema)
        .def(
            "GetAuthoredAttributeNames",
            &This::GetAuthoredAttributeNames,
            &SchemaApiAdaptorWrapper::default_GetAuthoredAttributeNames)
        .def("GetAttribute", &This::GetAttribute, &SchemaApiAdaptorWrapper::default_GetAttribute)
        .def(
            "CreateAttribute",
            &This::CreateAttribute,
            &SchemaApiAdaptorWrapper::default_CreateAttribute)
        .def(
            "RemoveAttribute",
            &This::RemoveAttribute,
            &SchemaApiAdaptorWrapper::default_RemoveAttribute)
        .def(
            "GetMayaObjectForSchema",
            &This::GetMayaObjectForSchema,
            &SchemaApiAdaptorWrapper::default_GetMayaObjectForSchema)
        .def(
            "GetMayaNameForUsdAttrName",
            &This::GetMayaNameForUsdAttrName,
            &SchemaApiAdaptorWrapper::default_GetMayaNameForUsdAttrName)
        .def(
            "GetAdaptedAttributeNames",
            &This::GetAdaptedAttributeNames,
            &SchemaApiAdaptorWrapper::default_GetAdaptedAttributeNames)
        .def("Register", &SchemaApiAdaptorWrapper::Register)
        .staticmethod("Register");
}
