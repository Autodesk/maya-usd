//
// Copyright 2021 Autodesk
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
#ifndef PXRUSDMAYA_SCHEMA_API_ADAPTOR_H
#define PXRUSDMAYA_SCHEMA_API_ADAPTOR_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/utils/adaptor.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>

PXR_NAMESPACE_OPEN_SCOPE

struct UsdMayaJobExportArgs;

/// Base class for plugin schema API adaptors. Allows transparent USD API use on Maya data.
///
class UsdMayaSchemaApiAdaptor : public UsdMayaSchemaAdaptor
{
public:
    /// Constructs a schema API adaptor
    MAYAUSD_CORE_PUBLIC
    UsdMayaSchemaApiAdaptor();

    MAYAUSD_CORE_PUBLIC
    UsdMayaSchemaApiAdaptor(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef);

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaSchemaApiAdaptor();

    //
    // Callbacks for discovery at the UsdAdaptor level
    //

    /// Can this plugin adapt in a context-free environment
    ///
    /// This will be used to answer the GetAppliedSchemas question in a global context.
    MAYAUSD_CORE_PUBLIC
    virtual bool CanAdapt() const;

    /// Can this plugin adapt the provided MObject for export
    ///
    /// In this case, the answer is true only if there is sufficient Maya data to export the
    /// requested API.
    ///
    MAYAUSD_CORE_PUBLIC
    virtual bool CanAdaptForExport(const UsdMayaJobExportArgs&) const;

    /// Can this plugin adapt the provided MObject for import
    ///
    /// In this case, the answer is true only if the Maya data can be adapted to the requested API.
    ///
    MAYAUSD_CORE_PUBLIC
    virtual bool CanAdaptForImport(const UsdMayaJobImportArgs&) const;

    /// Modify the Maya scene so it supports this schema.
    ///
    /// Returns true on success, false if schema can not be applied or the application failed.
    MAYAUSD_CORE_PUBLIC
    virtual bool ApplySchema(MDGModifier& modifier);

    /// Modify the Maya scene so the wrapped Maya object does not support the wrapped schema anymore
    ///
    /// Returns true on success, false if schema can not be unapplied or unapplication failed.
    MAYAUSD_CORE_PUBLIC
    virtual bool UnapplySchema(MDGModifier& modifier);

    //
    // UsdMayaSchemaAdaptor overloads:
    //

    MAYAUSD_CORE_PUBLIC
    TfTokenVector GetAuthoredAttributeNames() const override;

    MAYAUSD_CORE_PUBLIC
    UsdMayaAttributeAdaptor GetAttribute(const TfToken& attrName) const override;

    MAYAUSD_CORE_PUBLIC
    UsdMayaAttributeAdaptor
    CreateAttribute(const TfToken& attrName, MDGModifier& modifier) override;

    MAYAUSD_CORE_PUBLIC
    void RemoveAttribute(const TfToken& attrName, MDGModifier& modifier) override;

    //
    // Adapter specific API to handle simple 1-to-1 adaptations:
    //
    // In some cases, like a bullet simulation, the information can be found on the bullet shape
    // node found under the same transform as the mesh primitive. In that case, we provide services
    // to adapt Maya attributes found on this remote object.
    //

    /// Return the target object for attribute adaptors
    ///
    /// Can be the same object wrapped by the UsdMayaAdaptor or can be on a separate one.
    MAYAUSD_CORE_PUBLIC
    virtual MObject GetMayaObjectForSchema() const;

    /// Get the name of the Maya attribute that corresponds to the USD attribute named \p
    /// usdAttrName.
    ///
    /// The default implementation always returns an empty string, which triggers the use of dynamic
    /// attributes on the object returned by GetMayaObjectForSchema().
    MAYAUSD_CORE_PUBLIC
    virtual TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const;

    /// Returns the USD attribute names that are natively handled by the Maya object
    ///
    MAYAUSD_CORE_PUBLIC
    virtual TfTokenVector GetAdaptedAttributeNames() const;

private:
    /// Look for any dynamically authored USD attributes on the provided \p mayaObject
    ///
    /// For adaptors that have only partial coverage between USD and Maya attributes and allow
    /// creating dynamic attributes for the missing Maya attributes.
    MAYAUSD_CORE_PUBLIC
    TfTokenVector GetAuthoredAttributeNamesOnMayaObject(MObject mayaObject) const;

    /// Return a 1-to-1 attribute adaptor for the \p mayaAttribute of \p mayaObject
    ///
    /// Allows remapping a USD attribute request to a different object if necessary
    MAYAUSD_CORE_PUBLIC
    UsdMayaAttributeAdaptor GetConvertibleAttribute(
        MObject                       mayaObject,
        const MString&                mayaAttribute,
        const SdfAttributeSpecHandle& attrDef) const;
};

using UsdMayaSchemaApiAdaptorPtr = std::shared_ptr<UsdMayaSchemaApiAdaptor>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
