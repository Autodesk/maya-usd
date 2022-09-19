//
// Copyright 2022 Pixar
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

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>
#include <mayaUsd/fileio/translators/translatorRfMLight.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/usd/usdLux/meshLightAPI.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>

// PxrMeshLight gets exported specially.  Rather than resulting in it's own
// prim, the PxrMeshLight is instead exported as a MeshLightAPI on the mesh
// prim.  We use the schema API adaptors to handle this.
//
// The current implementation only supports exporting.

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (PxrMeshLight));

// helper function to get the schema name for a given APISchemaType.
// _GetSchemaName<UsdLuxMeshLightAPI>() -> "MeshLightAPI"
template <typename AppliedAPISchemaType> TfToken _GetSchemaName()
{
    return UsdSchemaRegistry::GetSchemaTypeName(TfType::Find<AppliedAPISchemaType>());
}

// This returns a (const-ref to a) map that maps UsdAttribute names to the
// corresponding maya names.  For each AppliedAPISchemaType, this is computed
// once when first requested.
//
// We use Sdr to compute all the attributes for PxrMeshLight.  We then consult
// the UsdSchemaRegistry to filter out attributes that do not belong to
// AppliedAPISchemaType.
template <typename AppliedAPISchemaType> const std::map<TfToken, TfToken>& _GetUsdToMayaNames()
{
    static std::map<TfToken, TfToken> ret;
    static std::once_flag             flag;
    std::call_once(flag, []() {
        const TfToken schemaName
            = UsdSchemaRegistry::GetSchemaTypeName(TfType::Find<AppliedAPISchemaType>());
        const UsdPrimDefinition* primDef
            = UsdSchemaRegistry::GetInstance().FindAppliedAPIPrimDefinition(
                _GetSchemaName<AppliedAPISchemaType>());
        if (!primDef) {
            TF_CODING_ERROR(
                "Could not find Applied API Prim Definition for '%s'.", schemaName.GetText());
            return;
        }
        const auto&             propertyNames = primDef->GetPropertyNames();
        const std::set<TfToken> propertyNamesSet(propertyNames.begin(), propertyNames.end());

        // Start off with all of the PxrMeshLight attributes
        ret = UsdMayaTranslatorUtil::ComputeUsdAttributeToMayaAttributeNamesForShader(
            _tokens->PxrMeshLight);

        // Now, we remove ones that do not belong to the
        // AppliedAPISchemaType (propertyNamesSet).
        for (auto iter = ret.begin(); iter != ret.end();) {
            const TfToken& usdAttrName = iter->first;
            const bool     erase = propertyNamesSet.count(usdAttrName) == 0;
            if (erase) {
                iter = ret.erase(iter);
            } else {
                ++iter;
            }
        }
    });
    return ret;
}

// This adaptor gets instantiated for each appliedAPISchema type that are
// relevant to the PxrMeshLight (UsdLuxMeshLightAPI, UsdLuxShadowAPI, UsdLuxShapingAPI).
// Each template instantiation will handle the properties that belong to that
// specific API schema.
template <typename AppliedAPISchemaType>
class _SchemaApiAdaptorForMeshLight : public UsdMayaSchemaApiAdaptor
{
public:
    _SchemaApiAdaptorForMeshLight(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef)
        : UsdMayaSchemaApiAdaptor(object, schemaName, schemaPrimDef)
    {
    }

    ~_SchemaApiAdaptorForMeshLight() override = default;

public: // UsdMayaSchemaApiAdaptor overrides
    bool CanAdapt() const override
    {
        // Since we have to register the schema API adaptor for all shapes
        // (due to https://github.com/Autodesk/maya-usd/issues/2605), we filter
        // here to make sure we only run on "mesh".
        const MFnDependencyNode depNodeFn(_handle.object());
        if (depNodeFn.typeName() != "mesh") {
            return false;
        }

        return !UsdMayaTranslatorRfMLight::GetAttachedPxrMeshLight(_handle.object()).isNull();
    }

    bool CanAdaptForImport(const UsdMayaJobImportArgs& jobArgs) const override
    {
        // XXX import not supported
        return false;
    }

    bool CanAdaptForExport(const UsdMayaJobExportArgs& jobArgs) const override
    {
        if (jobArgs.includeAPINames.find(_GetSchemaName<AppliedAPISchemaType>())
            != jobArgs.includeAPINames.end()) {
            return CanAdapt();
        }
        return false;
    }

    bool ApplySchema(const UsdMayaPrimReaderArgs& primReaderArgs, UsdMayaPrimReaderContext& context)
        override
    {
        // XXX import not supported
        return false;
    }

    bool ApplySchema(MDGModifier&) override
    {
        // XXX import not supported
        return false;
    }

    bool UnapplySchema(MDGModifier&) override
    {
        // XXX import not supported
        return false;
    }

    MObject GetMayaObjectForSchema() const override
    {
        return UsdMayaTranslatorRfMLight::GetAttachedPxrMeshLight(_handle.object());
    }

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override
    {
        const auto& usdToMayaName = _GetUsdToMayaNames<AppliedAPISchemaType>();
        auto        found = usdToMayaName.find(usdAttrName);
        if (found == usdToMayaName.end()) {
            return usdAttrName;
        }

        return found->second;
    }

    TfTokenVector GetAdaptedAttributeNames() const override
    {
        const auto&             usdToMayaName = _GetUsdToMayaNames<AppliedAPISchemaType>();
        const MFnDependencyNode depFn(GetMayaObjectForSchema());
        TfTokenVector           ret;
        for (const auto& usdNameAndMayaName : usdToMayaName) {
            const TfToken& usdName = usdNameAndMayaName.first;
            const TfToken& mayaName = usdNameAndMayaName.second;
            MPlug          plug = depFn.findPlug(mayaName.GetText(), true);
            if (!plug.isNull()) {
                ret.push_back(usdName);
            }
        }
        return ret;
    }
};

using _MeshLightAdaptor_MeshLightAPI = _SchemaApiAdaptorForMeshLight<UsdLuxMeshLightAPI>;
using _MeshLightAdaptor_ShadowAPI = _SchemaApiAdaptorForMeshLight<UsdLuxShadowAPI>;
using _MeshLightAdaptor_ShapingAPI = _SchemaApiAdaptorForMeshLight<UsdLuxShapingAPI>;

// These really want to be registered with "mesh" as this is a mesh light;
// however, it seems like doing so will remove anything that's registered on
// "shape" (like in the testSchemaApiAdaptor.py test).  So, we just do "shape"
// here until that's fixed.
//
// https://github.com/Autodesk/maya-usd/issues/2605
PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(shape, MeshLightAPI, _MeshLightAdaptor_MeshLightAPI);
PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(shape, ShadowAPI, _MeshLightAdaptor_ShadowAPI);
PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(shape, ShapingAPI, _MeshLightAdaptor_ShapingAPI);

TF_REGISTRY_FUNCTION(UsdMayaPrimWriterRegistry)
{
    // To prevent the exporter from processing PxrMeshLight, we register this as
    // a "primless" type.
    UsdMayaPrimWriterRegistry::RegisterPrimless(_tokens->PxrMeshLight);
}

PXR_NAMESPACE_CLOSE_SCOPE
