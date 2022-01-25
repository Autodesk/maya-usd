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
#include "schemaApiAdaptor.h"

#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaSchemaApiAdaptor::UsdMayaSchemaApiAdaptor() { }

UsdMayaSchemaApiAdaptor::UsdMayaSchemaApiAdaptor(
    const MObjectHandle&     object,
    const TfToken&           schemaName,
    const UsdPrimDefinition* schemaPrimDef)
    : UsdMayaSchemaAdaptor(object, schemaName, schemaPrimDef)
{
}

UsdMayaSchemaApiAdaptor::~UsdMayaSchemaApiAdaptor() { }

bool UsdMayaSchemaApiAdaptor::CanAdapt() const { return false; }

bool UsdMayaSchemaApiAdaptor::CanAdaptForExport(const UsdMayaJobExportArgs&) const { return false; }

bool UsdMayaSchemaApiAdaptor::CanAdaptForImport(const UsdMayaJobImportArgs&) const { return false; }

bool UsdMayaSchemaApiAdaptor::ApplySchema(
    const UsdMayaPrimReaderArgs& primReaderArgs,
    UsdMayaPrimReaderContext&    context)
{
    return false;
}

bool UsdMayaSchemaApiAdaptor::ApplySchema(MDGModifier& modifier) { return false; }

bool UsdMayaSchemaApiAdaptor::UnapplySchema(MDGModifier& modifier) { return true; }

TfTokenVector UsdMayaSchemaApiAdaptor::GetAuthoredAttributeNames() const
{
    TfTokenVector retVal = GetAuthoredAttributeNamesOnMayaObject(GetMayaObjectForSchema());

    // Append the always translated ones:
    TfTokenVector adapted = GetAdaptedAttributeNames();
    retVal.insert(retVal.end(), adapted.begin(), adapted.end());

    return retVal;
}

UsdMayaAttributeAdaptor UsdMayaSchemaApiAdaptor::GetAttribute(const TfToken& attrName) const
{
    TfToken mayaAttribute = GetMayaNameForUsdAttrName(attrName);
    if (!mayaAttribute.IsEmpty()) {
        return GetConvertibleAttribute(
            GetMayaObjectForSchema(),
            mayaAttribute.GetText(),
            _schemaDef->GetSchemaAttributeSpec(attrName));
    } else {
        // Untranslatable attributes are handled with dynamic attributes.
        MObjectHandle objectHandle(GetMayaObjectForSchema());
        if (!objectHandle.isValid()) {
            // It is possible that the object got removed with RemoveSchema, making this call
            // impossible.
            TF_CODING_ERROR(
                "Could not find object referenced in schema '%s'", _schemaName.GetText());
            return UsdMayaAttributeAdaptor();
        }
        UsdMayaSchemaAdaptor genericAdaptor { objectHandle, _schemaName, _schemaDef };
        return genericAdaptor.GetAttribute(attrName);
    }
}

UsdMayaAttributeAdaptor
UsdMayaSchemaApiAdaptor::CreateAttribute(const TfToken& attrName, MDGModifier& modifier)
{
    TfToken mayaAttribute = GetMayaNameForUsdAttrName(attrName);
    if (!mayaAttribute.IsEmpty()) {
        // Transatable attribute always exists:
        return GetAttribute(attrName);
    } else {
        // Untranslatable attributes are handled with dynamic attributes.
        MObjectHandle objectHandle(GetMayaObjectForSchema());
        if (!objectHandle.isValid()) {
            // It is possible that the referenced object got removed, making this call impossible.
            TF_CODING_ERROR(
                "Could not find object referenced in schema '%s'", _schemaName.GetText());
            return UsdMayaAttributeAdaptor();
        }
        UsdMayaSchemaAdaptor genericAdaptor { objectHandle, _schemaName, _schemaDef };
        return genericAdaptor.CreateAttribute(attrName, modifier);
    }
}

void UsdMayaSchemaApiAdaptor::RemoveAttribute(const TfToken& attrName, MDGModifier& modifier)
{
    TfToken mayaAttribute = GetMayaNameForUsdAttrName(attrName);
    if (mayaAttribute.IsEmpty()) {
        // Untranslatable attributes are handled with dynamic attributes.
        MObjectHandle objectHandle(GetMayaObjectForSchema());
        if (!objectHandle.isValid()) {
            // It is possible that the referenced object got removed, making this call impossible.
            TF_CODING_ERROR(
                "Could not find object referenced in schema '%s'", _schemaName.GetText());
            return;
        }
        UsdMayaSchemaAdaptor genericAdaptor { objectHandle, _schemaName, _schemaDef };
        genericAdaptor.RemoveAttribute(attrName, modifier);
    }
}

MObject UsdMayaSchemaApiAdaptor::GetMayaObjectForSchema() const { return _handle.object(); };

TfToken UsdMayaSchemaApiAdaptor::GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const
{
    return {};
};

TfTokenVector UsdMayaSchemaApiAdaptor::GetAdaptedAttributeNames() const { return {}; }

TfTokenVector
UsdMayaSchemaApiAdaptor::GetAuthoredAttributeNamesOnMayaObject(MObject mayaObject) const
{
    if (!mayaObject.isNull()) {
        UsdMayaSchemaAdaptor genericAdaptor { MObjectHandle(mayaObject), _schemaName, _schemaDef };
        return genericAdaptor.GetAuthoredAttributeNames();
    }
    return {};
}

UsdMayaAttributeAdaptor UsdMayaSchemaApiAdaptor::GetConvertibleAttribute(
    MObject                       mayaObject,
    const MString&                mayaAttribute,
    const SdfAttributeSpecHandle& attrDef) const
{
    if (mayaObject.isNull()) {
        // It is possible that the object got removed with RemoveSchema, making this call
        // impossible.
        TF_CODING_ERROR("Could not find object referenced in schema '%s'", _schemaName.GetText());
        return UsdMayaAttributeAdaptor();
    }
    if (!attrDef) {
        TF_CODING_ERROR("Attribute doesn't exist on schema '%s'", _schemaName.GetText());
        return UsdMayaAttributeAdaptor();
    }
    MFnDependencyNode node(mayaObject);
    MPlug             plug = node.findPlug(mayaAttribute);
    if (plug.isNull()) {
        return UsdMayaAttributeAdaptor();
    }

    return UsdMayaAttributeAdaptor(plug, attrDef);
}

PXR_NAMESPACE_CLOSE_SCOPE
