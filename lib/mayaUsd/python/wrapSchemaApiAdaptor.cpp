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

#include "wrapSparseValueWriter.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>

#include <pxr/base/tf/pyPolymorphic.h>

#include <maya/MTypes.h>

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
        const UsdPrim&              prim,
        const UsdTimeCode&          usdTime,
        MayaUsdLibSparseValueWriter valueWriter) const
    {
        return base_t::CopyToPrim(prim, usdTime, valueWriter.Get());
    }
    bool CopyToPrim(
        const UsdPrim&             prim,
        const UsdTimeCode&         usdTime,
        UsdUtilsSparseValueWriter* valueWriter) const override
    {
        return python_CopyToPrim(prim, usdTime, MayaUsdLibSparseValueWriter(valueWriter));
    }
    virtual bool python_CopyToPrim(
        const UsdPrim&              prim,
        const UsdTimeCode&          usdTime,
        MayaUsdLibSparseValueWriter valueWriter) const
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

    bool default_CanAdaptForImport(const UsdMayaJobImportArgs& args) const
    {
        return base_t::CanAdaptForImport(args);
    }
    bool CanAdaptForImport(const UsdMayaJobImportArgs& args) const override
    {
        return this->template CallVirtual<bool>(
            "CanAdaptForImport", &This::default_CanAdaptForImport)(args);
    }

    bool default_ApplySchema(MDGModifier& modifier) { return base_t::ApplySchema(modifier); }
    bool ApplySchema(MDGModifier& modifier) override
    {
        // The TfPolymorphic CallVirtual mechanism uses variadic arguments with parameter packs. A
        // major flaw of this technique is that references and other cv qualifiers are lost in the
        // parameter pack deduction phase, leading to parameter being passed by copy instead of by
        // reference. This fails with MDGModifier because the copy semantics are private. Use the
        // internals of TfPolymorphic::CallVirtual() as a way to preserve the reference on the
        // "modifier" parameter.
        TfPyLock pyLock;
        auto     pyOverride = this->GetOverride("ApplySchema");
        if (pyOverride) {
            // Do *not* call through if there's an active python exception.
            if (!PyErr_Occurred()) {
                try {
                    return boost::python::call<bool>(pyOverride.ptr(), modifier);
                } catch (boost::python::error_already_set const&) {
                    // Convert any exception to TF_ERRORs.
                    TfPyConvertPythonExceptionToTfErrors();
                    PyErr_Clear();
                }
            }
        }
        return default_ApplySchema(modifier);
    }

    bool default_ApplySchemaForImport(
        const UsdMayaPrimReaderArgs& primReaderArgs,
        UsdMayaPrimReaderContext&    context)
    {
        return base_t::ApplySchema(primReaderArgs, context);
    }
    bool ApplySchema(const UsdMayaPrimReaderArgs& primReaderArgs, UsdMayaPrimReaderContext& context)
        override
    {
        // Note the different function name Python-side. Python does not do overload resolution
        // based on argument types because every argument is a PyObject.
        return this->CallVirtual("ApplySchemaForImport", &This::default_ApplySchemaForImport)(
            primReaderArgs, context);
    }

    bool default_UnapplySchema(MDGModifier& modifier) { return base_t::UnapplySchema(modifier); }
    bool UnapplySchema(MDGModifier& modifier) override
    {
        // Not using TfPolymorphic::CallVirtual. See ApplySchema(MDGModifier&) above for details.
        TfPyLock pyLock;
        auto     pyOverride = this->GetOverride("UnapplySchema");
        if (pyOverride) {
            // Do *not* call through if there's an active python exception.
            if (!PyErr_Occurred()) {
                try {
                    return boost::python::call<bool>(pyOverride.ptr(), modifier);
                } catch (boost::python::error_already_set const&) {
                    // Convert any exception to TF_ERRORs.
                    TfPyConvertPythonExceptionToTfErrors();
                    PyErr_Clear();
                }
            }
        }
        return default_UnapplySchema(modifier);
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
            "GetAdaptedAttributeNames", &This::default_GetAdaptedAttributeNames)();
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

    MObject _GetMayaObject() const { return _handle.object(); }
};

void wrapSchemaApiAdaptor()
{
    typedef UsdMayaSchemaApiAdaptor This;

    boost::python::class_<
        SchemaApiAdaptorWrapper,
        boost::python::bases<UsdMayaSchemaAdaptor>,
        boost::noncopyable>("SchemaApiAdaptor", boost::python::no_init)
        .def("__init__", make_constructor(&SchemaApiAdaptorWrapper::New))

        .def("CanAdapt", &This::CanAdapt, &SchemaApiAdaptorWrapper::default_CanAdapt)
        .def(
            "CanAdaptForExport",
            &This::CanAdaptForExport,
            &SchemaApiAdaptorWrapper::default_CanAdaptForExport)
        .def(
            "CanAdaptForImport",
            &This::CanAdaptForImport,
            &SchemaApiAdaptorWrapper::default_CanAdaptForImport)
        .def(
            "ApplySchema",
            (bool (This::*)(MDGModifier&)) & This::ApplySchema,
            (bool (This::*)(MDGModifier&)) & SchemaApiAdaptorWrapper::default_ApplySchema)
        .def(
            "ApplySchemaForImport",
            (bool (This::*)(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&))
                & This::ApplySchema,
            &SchemaApiAdaptorWrapper::default_ApplySchemaForImport)
        .def("UnapplySchema", &This::UnapplySchema, &SchemaApiAdaptorWrapper::default_UnapplySchema)
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
        .def(
            "CopyToPrim",
            &SchemaApiAdaptorWrapper::python_CopyToPrim,
            &SchemaApiAdaptorWrapper::default_CopyToPrim)
        .def("CopyFromPrim", &This::CopyFromPrim, &SchemaApiAdaptorWrapper::default_CopyFromPrim)
        .def("Register", &SchemaApiAdaptorWrapper::Register)
        .staticmethod("Register")
        .add_property("mayaObject", &SchemaApiAdaptorWrapper::_GetMayaObject);
}
