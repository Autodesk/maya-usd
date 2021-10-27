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
#include "adaptor.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/apiSchemaBase.h>
#include <pxr/usd/usd/schemaBase.h>
#include <pxr/usd/usd/tokens.h>

#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

PXR_NAMESPACE_OPEN_SCOPE

const UsdPrimDefinition*
_GetPrimDefinition(const UsdMayaAdaptor& adaptor, const TfToken& schemaName)
{
    const UsdSchemaRegistry& schemaReg = UsdSchemaRegistry::GetInstance();
    if (const UsdPrimDefinition* primDef = schemaReg.FindAppliedAPIPrimDefinition(schemaName)) {
        return primDef;
    }
    // Is this a typed schema?
    else if (const UsdPrimDefinition* primDef = schemaReg.FindConcretePrimDefinition(schemaName)) {
        // XXX
        // We currently require an exact type match instead of the polymorphic
        // behavior that actual USD schema classes implement. This is because
        // we can't currently get the prim definition from the schema registry
        // for non-concrete schemas like Imageable (see bug 160436). Ideally,
        // once that's resolved, we would cache a mapping of Maya types to all
        // compatible USD type names based on schema inheritance.
        // (In that future world, we'll also want to special case some schemas
        // like UsdGeomImageable to be "compatible" with all Maya nodes.)
        const TfToken objectTypeName = adaptor.GetUsdTypeName();
        if (schemaName == objectTypeName) {
            // There's an exact MFn::Type match? Easy-peasy.
            return primDef;
        }
    }
    return nullptr;
}

static std::string _GetMayaAttrNameForMetadataKey(const TfToken& key)
{
    return TfStringPrintf("USD_%s", TfMakeValidIdentifier(key.GetString()).c_str());
}

static std::string _GetMayaAttrNameForAttrName(const TfToken& attrName)
{
    return TfStringPrintf("USD_ATTR_%s", TfMakeValidIdentifier(attrName.GetString()).c_str());
}

static VtValue _GetListOpForTokenVector(const TfTokenVector& vector)
{
    SdfTokenListOp op;
    op.SetPrependedItems(vector);
    return VtValue(op);
}

std::map<std::string, TfType>               UsdMayaAdaptor::_schemaLookup;
std::map<TfToken, std::vector<std::string>> UsdMayaAdaptor::_attributeAliases;

UsdMayaAdaptor::UsdMayaAdaptor(const MObject& obj)
    : _handle(obj)
{
}

UsdMayaAdaptor::UsdMayaAdaptor(const MObject& obj, const UsdMayaJobExportArgs* jobExportArgs)
    : _handle(obj)
    , _jobExportArgs(jobExportArgs)
{
}

UsdMayaAdaptor::UsdMayaAdaptor(const MObject& obj, const UsdMayaJobImportArgs* jobImportArgs)
    : _handle(obj)
    , _jobImportArgs(jobImportArgs)
{
}

UsdMayaAdaptor::operator bool() const
{
    if (!_handle.isValid()) {
        return false;
    }

    MStatus           status;
    MFnDependencyNode node(_handle.object(), &status);
    return status;
}

std::string UsdMayaAdaptor::GetMayaNodeName() const
{
    if (!*this) {
        return std::string();
    }

    if (_handle.object().hasFn(MFn::kDagNode)) {
        MFnDagNode dagNode(_handle.object());
        return dagNode.fullPathName().asChar();
    } else {
        MFnDependencyNode depNode(_handle.object());
        return depNode.name().asChar();
    }
}

TfToken UsdMayaAdaptor::GetUsdTypeName() const
{
    if (!*this) {
        return TfToken();
    }

    const TfType ty = GetUsdType();
    return UsdSchemaRegistry::GetInstance().GetSchemaTypeName(ty);
}

TfType UsdMayaAdaptor::GetUsdType() const
{
    if (!*this) {
        return TfType();
    }

    MObject           object = _handle.object();
    MFnDependencyNode depNode(object);

    // The adaptor type mapping might be registered externally in a prim writer
    // plugin. This simply pokes the prim writer registry to load the prim
    // writer plugin in order to pull in the adaptor mapping.
    UsdMayaPrimWriterRegistry::Find(depNode.typeName().asChar());
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    const auto iter = _schemaLookup.find(depNode.typeName().asChar());
    if (iter != _schemaLookup.end()) {
        return iter->second;
    } else {
        return TfType();
    }
}

TfTokenVector UsdMayaAdaptor::GetAppliedSchemas() const
{
    if (!*this) {
        return TfTokenVector();
    }

    TfTokenVector result;

    // See if we have any plugin adaptors we could use:
    const MFnDependencyNode depNodeFn(_handle.object());
    const std::string       mayaTypeName(depNodeFn.typeName().asChar());
    for (auto&& adaptorEntry : UsdMayaSchemaApiAdaptorRegistry::Find(mayaTypeName)) {
        TfToken schemaName(adaptorEntry.first);
        if (_jobExportArgs
            && _jobExportArgs->includeAPINames.find(schemaName)
                == _jobExportArgs->includeAPINames.end()) {
            continue;
        }

        if (_jobImportArgs
            && _jobImportArgs->includeAPINames.find(schemaName)
                == _jobImportArgs->includeAPINames.end()) {
            continue;
        }

        const UsdPrimDefinition* primDef = _GetPrimDefinition(*this, schemaName);
        if (!primDef) {
            continue;
        }

        // Take the first one that can adapt:
        for (auto&& factoryFn : adaptorEntry.second) {
            if (!factoryFn) {
                // factories get nulled when their associated plugin unloads.
                continue;
            }
            UsdMayaSchemaApiAdaptorPtr schemaAdaptor(factoryFn(_handle, schemaName, primDef));
            if (schemaAdaptor) {
                if (_jobExportArgs) {
                    if (schemaAdaptor->CanAdaptForExport(*_jobExportArgs)) {
                        result.push_back(schemaName);
                        break;
                    }
                } else if (_jobImportArgs) {
                    if (schemaAdaptor->CanAdaptForImport(*_jobImportArgs)) {
                        result.push_back(schemaName);
                        break;
                    }
                } else {
                    if (schemaAdaptor->CanAdapt()) {
                        result.push_back(schemaName);
                        break;
                    }
                }
            }
        }
    }

    VtValue appliedSchemas;
    if (GetMetadata(UsdTokens->apiSchemas, &appliedSchemas)) {
        appliedSchemas.Get<SdfTokenListOp>().ApplyOperations(&result);
    }

    return result;
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::GetSchema(const TfType& ty) const
{
    const TfToken usdTypeName = UsdSchemaRegistry::GetInstance().GetSchemaTypeName(ty);
    if (usdTypeName.IsEmpty()) {
        return nullptr;
    }

    return GetSchemaByName(usdTypeName);
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::GetSchemaByName(const TfToken& schemaName) const
{
    if (!*this) {
        return nullptr;
    }

    // If exporting, see if we have any plugin adaptors we could use:
    const MFnDependencyNode  depNodeFn(_handle.object());
    const std::string        mayaTypeName(depNodeFn.typeName().asChar());
    const UsdPrimDefinition* primDef = _GetPrimDefinition(*this, schemaName);

    for (auto&& schemaFn :
         UsdMayaSchemaApiAdaptorRegistry::Find(mayaTypeName, schemaName.GetString())) {
        if (!schemaFn) {
            // factories get nulled when their associated plugin unloads.
            continue;
        }
        UsdMayaSchemaApiAdaptorPtr schemaAdaptor(schemaFn(_handle, schemaName, primDef));
        if (schemaAdaptor) {
            if (_jobExportArgs) {
                if (schemaAdaptor->CanAdaptForExport(*_jobExportArgs)) {
                    return schemaAdaptor;
                }
            } else if (_jobImportArgs) {
                if (schemaAdaptor->CanAdaptForImport(*_jobImportArgs)) {
                    return schemaAdaptor;
                }
            } else {
                if (schemaAdaptor->CanAdapt()) {
                    return schemaAdaptor;
                }
            }
        }

        /*
           We need to come back afterwards for "relationships" since they require USD SdfPath
           <-> Maya MObject mapping. This will require registering for a final postGlobalExport pass
           to be done only on registered exporters and importers in order to rebuild the
           relationships and/or DG connections.
        */
    }

    // Is this an API schema?
    if (primDef) {
        return UsdMayaSchemaAdaptorPtr(
            new UsdMayaSchemaAdaptor(_handle.object(), schemaName, primDef));
    }

    return nullptr;
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::GetSchemaOrInheritedSchema(const TfType& ty) const
{
    if (!*this) {
        return nullptr;
    }

    if (ty.IsA<UsdAPISchemaBase>()) {
        // No "promotion" for API schemas.
        return GetSchema(ty);
    } else if (ty.IsA<UsdSchemaBase>()) {
        // Can "promote" typed schemas based on inheritance.
        const TfType objectType = GetUsdType();
        if (objectType.IsA(ty)) {
            return GetSchema(objectType);
        }
    }

    return nullptr;
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::ApplySchema(const TfType& ty)
{
    MDGModifier modifier;
    return ApplySchema(ty, modifier);
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::ApplySchema(const TfType& ty, MDGModifier& modifier)
{
    const TfToken usdTypeName = UsdSchemaRegistry::GetInstance().GetSchemaTypeName(ty);
    if (usdTypeName.IsEmpty()) {
        TF_CODING_ERROR("Can't find schema definition for type '%s'", ty.GetTypeName().c_str());
        return nullptr;
    }

    return ApplySchemaByName(usdTypeName, modifier);
}

UsdMayaSchemaAdaptorPtr UsdMayaAdaptor::ApplySchemaByName(const TfToken& schemaName)
{
    MDGModifier modifier;
    return ApplySchemaByName(schemaName, modifier);
}

UsdMayaSchemaAdaptorPtr
UsdMayaAdaptor::ApplySchemaByName(const TfToken& schemaName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return nullptr;
    }

    // Get the "apply" schema definition. If it's registered, there should be a
    // def.
    const UsdPrimDefinition* primDef
        = UsdSchemaRegistry::GetInstance().FindAppliedAPIPrimDefinition(schemaName);
    if (!primDef) {
        TF_CODING_ERROR("'%s' is not an applied API schema", schemaName.GetText());
        return nullptr;
    }

    // Do we have a plugin adapter for that schema?
    const MFnDependencyNode depNodeFn(_handle.object());
    const std::string       mayaTypeName(depNodeFn.typeName().asChar());
    for (auto&& schemaFn :
         UsdMayaSchemaApiAdaptorRegistry::Find(mayaTypeName, schemaName.GetString())) {
        if (!schemaFn) {
            // factories get nulled when their associated plugin unloads.
            continue;
        }
        UsdMayaSchemaApiAdaptorPtr schemaAdaptor(schemaFn(_handle, schemaName, primDef));
        if (schemaAdaptor) {
            if (_jobImportArgs && !schemaAdaptor->CanAdaptForImport(*_jobImportArgs)) {
                continue;
            }
            if (schemaAdaptor->ApplySchema(modifier)) {
                return schemaAdaptor;
            }
        }
    }

    // Add to schema list (if not yet present).
    TfTokenVector currentSchemas = GetAppliedSchemas();
    if (std::find(currentSchemas.begin(), currentSchemas.end(), schemaName)
        == currentSchemas.end()) {
        currentSchemas.push_back(schemaName);
        SetMetadata(UsdTokens->apiSchemas, _GetListOpForTokenVector(currentSchemas), modifier);
    }

    return UsdMayaSchemaAdaptorPtr(new UsdMayaSchemaAdaptor(_handle.object(), schemaName, primDef));
}

void UsdMayaAdaptor::UnapplySchema(const TfType& ty)
{
    MDGModifier modifier;
    UnapplySchema(ty, modifier);
}

void UsdMayaAdaptor::UnapplySchema(const TfType& ty, MDGModifier& modifier)
{
    const TfToken usdTypeName = UsdSchemaRegistry::GetInstance().GetSchemaTypeName(ty);
    if (usdTypeName.IsEmpty()) {
        TF_CODING_ERROR("Can't find schema definition for type '%s'", ty.GetTypeName().c_str());
        return;
    }

    UnapplySchemaByName(usdTypeName, modifier);
}

void UsdMayaAdaptor::UnapplySchemaByName(const TfToken& schemaName)
{
    MDGModifier modifier;
    UnapplySchemaByName(schemaName, modifier);
}

void UsdMayaAdaptor::UnapplySchemaByName(const TfToken& schemaName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return;
    }

    // Do we have a plugin adapter for that schema?
    const MFnDependencyNode depNodeFn(_handle.object());
    const std::string       mayaTypeName(depNodeFn.typeName().asChar());
    for (auto&& schemaFn :
         UsdMayaSchemaApiAdaptorRegistry::Find(mayaTypeName, schemaName.GetString())) {
        if (!schemaFn) {
            // factories get nulled when their associated plugin unloads.
            continue;
        }
        UsdMayaSchemaApiAdaptorPtr schemaAdaptor(schemaFn(_handle, schemaName, nullptr));
        if (schemaAdaptor) {
            if (schemaAdaptor->UnapplySchema(modifier)) {
                return;
            }
        }
    }

    // Remove from schema list.
    TfTokenVector currentSchemas = GetAppliedSchemas();
    currentSchemas.erase(
        std::remove(currentSchemas.begin(), currentSchemas.end(), schemaName),
        currentSchemas.end());
    if (currentSchemas.empty()) {
        ClearMetadata(UsdTokens->apiSchemas, modifier);
    } else {
        SetMetadata(UsdTokens->apiSchemas, _GetListOpForTokenVector(currentSchemas), modifier);
    }
}

static bool _GetMetadataUnchecked(const MFnDependencyNode& node, const TfToken& key, VtValue* value)
{
    VtValue fallback = SdfSchema::GetInstance().GetFallback(key);
    if (fallback.IsEmpty()) {
        return false;
    }

    std::string mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    MPlug       plug = node.findPlug(mayaAttrName.c_str());
    if (plug.isNull()) {
        return false;
    }

    TfType  ty = fallback.GetType();
    VtValue result = UsdMayaWriteUtil::GetVtValue(plug, ty, TfToken());
    if (result.IsEmpty()) {
        TF_RUNTIME_ERROR(
            "Cannot convert plug '%s' into metadata '%s' (%s)",
            plug.name().asChar(),
            key.GetText(),
            ty.GetTypeName().c_str());
        return false;
    }

    *value = result;
    return true;
}

UsdMetadataValueMap UsdMayaAdaptor::GetAllAuthoredMetadata() const
{
    if (!*this) {
        return UsdMetadataValueMap();
    }

    MFnDependencyNode   node(_handle.object());
    UsdMetadataValueMap metaMap;
    for (const TfToken& key : GetPrimMetadataFields()) {
        VtValue value;
        if (_GetMetadataUnchecked(node, key, &value)) {
            metaMap[key] = value;
        }
    }

    return metaMap;
}

bool UsdMayaAdaptor::GetMetadata(const TfToken& key, VtValue* value) const
{
    if (!*this) {
        return false;
    }

    if (!SdfSchema::GetInstance().IsRegistered(key)) {
        TF_CODING_ERROR("Metadata key '%s' is not registered", key.GetText());
        return false;
    }

    MFnDependencyNode node(_handle.object());
    return _GetMetadataUnchecked(node, key, value);
}

bool UsdMayaAdaptor::SetMetadata(const TfToken& key, const VtValue& value)
{
    MDGModifier modifier;
    return SetMetadata(key, value, modifier);
}

bool UsdMayaAdaptor::SetMetadata(const TfToken& key, const VtValue& value, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return false;
    }

    VtValue fallback;
    if (!SdfSchema::GetInstance().IsRegistered(key, &fallback)) {
        TF_CODING_ERROR("Metadata key '%s' is not registered", key.GetText());
        return false;
    }

    if (fallback.IsEmpty()) {
        return false;
    }

    VtValue castValue = VtValue::CastToTypeOf(value, fallback);
    if (castValue.IsEmpty()) {
        TF_CODING_ERROR("Can't cast value to type '%s'", fallback.GetTypeName().c_str());
        return false;
    }

    std::string       mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    std::string       mayaNiceAttrName = key.GetText();
    MFnDependencyNode node(_handle.object());
    TfType            ty = fallback.GetType();
    MObject           attrObj = UsdMayaReadUtil::FindOrCreateMayaAttr(
        ty, TfToken(), SdfVariabilityUniform, node, mayaAttrName, mayaNiceAttrName, modifier);
    if (attrObj.isNull()) {
        return false;
    }

    MPlug plug = node.findPlug(attrObj);
    if (!UsdMayaReadUtil::SetMayaAttr(plug, castValue, modifier)) {
        return false;
    }

    return true;
}

void UsdMayaAdaptor::ClearMetadata(const TfToken& key)
{
    MDGModifier modifier;
    ClearMetadata(key, modifier);
}

void UsdMayaAdaptor::ClearMetadata(const TfToken& key, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Adaptor is not valid");
        return;
    }

    MFnDependencyNode node(_handle.object());
    std::string       mayaAttrName = _GetMayaAttrNameForMetadataKey(key);
    if (node.hasAttribute(mayaAttrName.c_str())) {
        MObject attr = node.attribute(mayaAttrName.c_str());
        modifier.removeAttribute(_handle.object(), attr);
        modifier.doIt();
    }
}

/* static */
TfTokenVector UsdMayaAdaptor::GetPrimMetadataFields()
{
    return SdfSchema::GetInstance().GetMetadataFields(SdfSpecTypePrim);
}

template <typename T> static TfToken::Set _GetRegisteredSchemas()
{
    TfToken::Set     schemas;
    std::set<TfType> derivedTypes;
    TfType::Find<T>().GetAllDerivedTypes(&derivedTypes);

    const UsdSchemaRegistry& registry = UsdSchemaRegistry::GetInstance();
    for (const TfType& ty : derivedTypes) {
        const TfToken usdTypeName = registry.GetSchemaTypeName(ty);
        if (usdTypeName.IsEmpty()) {
            continue;
        }

        schemas.insert(usdTypeName);
    }

    return schemas;
}

/* static */
TfToken::Set UsdMayaAdaptor::GetRegisteredAPISchemas()
{
    return _GetRegisteredSchemas<UsdAPISchemaBase>();
}

/* static */
TfToken::Set UsdMayaAdaptor::GetRegisteredTypedSchemas()
{
    return _GetRegisteredSchemas<UsdSchemaBase>();
}

/* static */
void UsdMayaAdaptor::RegisterTypedSchemaConversion(
    const std::string& nodeTypeName,
    const TfType&      usdType,
    bool               fromPython)
{
    const auto iterAndInserted = _schemaLookup.insert(std::make_pair(nodeTypeName, usdType));
    if (iterAndInserted.second) {
        UsdMaya_RegistryHelper::AddUnloader(
            [nodeTypeName]() { _schemaLookup.erase(nodeTypeName); }, fromPython);
    } else {
        TF_CODING_ERROR(
            "Typed schema conversion already registered for Maya "
            "type %s",
            nodeTypeName.c_str());
    }
}

/* static */
void UsdMayaAdaptor::RegisterAttributeAlias(
    const TfToken&     attributeName,
    const std::string& alias,
    bool               fromPython)
{
    std::vector<std::string>& aliases = _attributeAliases[attributeName];
    if (std::find(aliases.begin(), aliases.end(), alias) == aliases.end()) {
        aliases.push_back(alias);
        UsdMaya_RegistryHelper::AddUnloader(
            [attributeName, alias]() {
                std::vector<std::string>& aliases = _attributeAliases[attributeName];
                aliases.erase(std::remove(aliases.begin(), aliases.end(), alias), aliases.end());
            },
            fromPython);
    } else {
        TF_CODING_ERROR(
            "Attribute alias '%s' (='%s') already registered",
            alias.c_str(),
            attributeName.GetText());
    }
}

/* static */
std::vector<std::string> UsdMayaAdaptor::GetAttributeAliases(const TfToken& attributeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    std::vector<std::string> result;
    result.push_back(_GetMayaAttrNameForAttrName(attributeName));

    auto iter = _attributeAliases.find(attributeName);
    if (iter != _attributeAliases.end()) {
        const std::vector<std::string>& aliases = iter->second;
        result.insert(result.end(), aliases.begin(), aliases.end());
    }

    return result;
}

UsdMayaSchemaAdaptor::UsdMayaSchemaAdaptor()
    : _handle()
    , _schemaDef(nullptr)
{
}

UsdMayaSchemaAdaptor::UsdMayaSchemaAdaptor(
    const MObjectHandle&     handle,
    const TfToken&           schemaName,
    const UsdPrimDefinition* schemaDef)
    : _handle(handle)
    , _schemaDef(schemaDef)
    , _schemaName(schemaName)
{
}

UsdMayaSchemaAdaptor::~UsdMayaSchemaAdaptor() { }

UsdMayaSchemaAdaptor::operator bool() const
{
    if (!_handle.isValid() || !_schemaDef) {
        return false;
    }

    MStatus           status;
    MFnDependencyNode node(_handle.object(), &status);
    return status;
}

std::string UsdMayaSchemaAdaptor::_GetMayaAttrNameOrAlias(const TfToken& name) const
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return std::string();
    }

    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaAdaptor>();

    const MObject     thisObject = _handle.object();
    MFnDependencyNode depNode(thisObject);

    // If the generated name exists, it is the most preferred name,
    const std::string genName = _GetMayaAttrNameForAttrName(name);
    if (depNode.hasAttribute(genName.c_str())) {
        return genName;
    }

    // Otherwise, search for any aliases that may already exist.
    for (const std::string& alias : UsdMayaAdaptor::GetAttributeAliases(name)) {
        if (depNode.hasAttribute(alias.c_str())) {
            return alias;
        }
    }

    // No attribute exists for this USD attribute. When creating, always use
    // the generated name.
    return genName;
}

TfToken UsdMayaSchemaAdaptor::GetName() const
{
    if (!*this) {
        return TfToken();
    }

    return _schemaName;
}

static SdfAttributeSpecHandle
_GetAttributeSpec(const UsdPrimDefinition* primDef, const TfToken& attrName)
{
    return primDef->GetSchemaAttributeSpec(attrName);
}

UsdMayaAttributeAdaptor UsdMayaSchemaAdaptor::GetAttribute(const TfToken& attrName) const
{
    if (!*this) {
        return UsdMayaAttributeAdaptor();
    }

    SdfAttributeSpecHandle attrDef = _GetAttributeSpec(_schemaDef, attrName);
    if (!attrDef) {
        TF_CODING_ERROR(
            "Attribute '%s' doesn't exist on schema '%s'",
            attrName.GetText(),
            _schemaName.GetText());
        return UsdMayaAttributeAdaptor();
    }

    std::string       mayaAttrName = _GetMayaAttrNameOrAlias(attrName);
    MFnDependencyNode node(_handle.object());
    MPlug             plug = node.findPlug(mayaAttrName.c_str());
    if (plug.isNull()) {
        return UsdMayaAttributeAdaptor();
    }

    return UsdMayaAttributeAdaptor(plug, attrDef);
}

UsdMayaAttributeAdaptor UsdMayaSchemaAdaptor::CreateAttribute(const TfToken& attrName)
{
    MDGModifier modifier;
    return CreateAttribute(attrName, modifier);
}

UsdMayaAttributeAdaptor
UsdMayaSchemaAdaptor::CreateAttribute(const TfToken& attrName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return UsdMayaAttributeAdaptor();
    }

    SdfAttributeSpecHandle attrDef = _GetAttributeSpec(_schemaDef, attrName);
    if (!attrDef) {
        TF_CODING_ERROR(
            "Attribute '%s' doesn't exist on schema '%s'",
            attrName.GetText(),
            _schemaName.GetText());
        return UsdMayaAttributeAdaptor();
    }

    std::string       mayaAttrName = _GetMayaAttrNameOrAlias(attrName);
    std::string       mayaNiceAttrName = attrDef->GetName();
    MFnDependencyNode node(_handle.object());

    bool    newAttr = !node.hasAttribute(mayaAttrName.c_str());
    MObject attrObj = UsdMayaReadUtil::FindOrCreateMayaAttr(
        attrDef->GetTypeName(),
        attrDef->GetVariability(),
        node,
        mayaAttrName,
        mayaNiceAttrName,
        modifier);
    if (attrObj.isNull()) {
        return UsdMayaAttributeAdaptor();
    }

    MPlug plug = node.findPlug(attrObj);
    if (newAttr && attrDef->HasDefaultValue()) {
        // Set the fallback value as the initial value of the attribute, if
        // it exists. (There's not much point in setting the "default" value in
        // Maya, because it won't behave like the fallback value in USD.)
        UsdMayaReadUtil::SetMayaAttr(plug, attrDef->GetDefaultValue(), modifier);
    }

    return UsdMayaAttributeAdaptor(plug, attrDef);
}

void UsdMayaSchemaAdaptor::RemoveAttribute(const TfToken& attrName)
{
    MDGModifier modifier;
    RemoveAttribute(attrName, modifier);
}

void UsdMayaSchemaAdaptor::RemoveAttribute(const TfToken& attrName, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Schema adaptor is not valid");
        return;
    }

    SdfAttributeSpecHandle attrDef = _GetAttributeSpec(_schemaDef, attrName);
    if (!attrDef) {
        TF_CODING_ERROR(
            "Attribute '%s' doesn't exist on schema '%s'",
            attrName.GetText(),
            _schemaName.GetText());
        return;
    }

    std::string       mayaAttrName = _GetMayaAttrNameOrAlias(attrName);
    MFnDependencyNode node(_handle.object());
    if (node.hasAttribute(mayaAttrName.c_str())) {
        MObject attr = node.attribute(mayaAttrName.c_str());
        modifier.removeAttribute(_handle.object(), attr);
        modifier.doIt();
    }
}

TfTokenVector UsdMayaSchemaAdaptor::GetAuthoredAttributeNames() const
{
    if (!*this) {
        return TfTokenVector();
    }

    MFnDependencyNode node(_handle.object());
    TfTokenVector     result;
    for (const TfToken& propName : _schemaDef->GetPropertyNames()) {
        if (_schemaDef->GetSpecType(propName) == SdfSpecTypeAttribute) {
            std::string mayaAttrName = _GetMayaAttrNameOrAlias(propName);
            if (node.hasAttribute(mayaAttrName.c_str())) {
                result.push_back(propName);
            }
        }
    }

    return result;
}

TfTokenVector UsdMayaSchemaAdaptor::GetAttributeNames() const
{
    if (!*this) {
        return TfTokenVector();
    }

    TfTokenVector attrNames;
    for (const TfToken& propName : _schemaDef->GetPropertyNames()) {
        if (_schemaDef->GetSpecType(propName) == SdfSpecTypeAttribute) {
            attrNames.push_back(propName);
        }
    }

    return attrNames;
}

const UsdPrimDefinition* UsdMayaSchemaAdaptor::GetSchemaDefinition() const { return _schemaDef; }

bool UsdMayaSchemaAdaptor::CopyToPrim(
    const UsdPrim&,
    const UsdTimeCode&,
    UsdUtilsSparseValueWriter*) const
{
    return false;
}

bool UsdMayaSchemaAdaptor::CopyFromPrim(const UsdPrim&) { return false; }

UsdMayaAttributeAdaptor::UsdMayaAttributeAdaptor()
    : _plug()
    , _node()
    , _attr()
    , _attrDef(nullptr)
{
}

UsdMayaAttributeAdaptor::UsdMayaAttributeAdaptor(const MPlug& plug, SdfAttributeSpecHandle attrDef)
    : _plug(plug)
    , _node(plug.node())
    , _attr(plug.attribute())
    , _attrDef(attrDef)
{
}

UsdMayaAttributeAdaptor::~UsdMayaAttributeAdaptor() { }

UsdMayaAttributeAdaptor::operator bool() const
{
    if (_plug.isNull() || !_node.isValid() || !_attr.isValid() || !_attrDef) {
        return false;
    }

    MStatus           status;
    MFnDependencyNode depNode(_node.object(), &status);
    if (!status) {
        return false;
    }

    MFnAttribute attr(_attr.object(), &status);
    if (!status) {
        return false;
    }

    return depNode.hasAttribute(attr.name());
}

TfToken UsdMayaAttributeAdaptor::GetName() const
{
    if (!*this) {
        return TfToken();
    }

    return _attrDef->GetNameToken();
}

bool UsdMayaAttributeAdaptor::Get(VtValue* value) const
{
    if (!*this) {
        return false;
    }

    VtValue result = UsdMayaWriteUtil::GetVtValue(_plug, _attrDef->GetTypeName());
    if (result.IsEmpty()) {
        return false;
    }

    *value = result;
    return true;
}

bool UsdMayaAttributeAdaptor::Set(const VtValue& newValue)
{
    MDGModifier modifier;
    return Set(newValue, modifier);
}

bool UsdMayaAttributeAdaptor::Set(const VtValue& newValue, MDGModifier& modifier)
{
    if (!*this) {
        TF_CODING_ERROR("Attribute adaptor is not valid");
        return false;
    }

    return UsdMayaReadUtil::SetMayaAttr(_plug, newValue, modifier);
}

const SdfAttributeSpecHandle UsdMayaAttributeAdaptor::GetAttributeDefinition() const
{
    return _attrDef;
}

PXR_NAMESPACE_CLOSE_SCOPE
