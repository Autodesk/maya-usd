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
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Metadata.h"

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/usd/schemaBase.h>

#include <maya/MProfiler.h>
namespace {
const int _translatorProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "TranslatorManufacture",
    "TranslatorManufacture"
#else
    "TranslatorManufacture"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

std::vector<TranslatorRefPtr> TranslatorManufacture::m_pythonTranslators;
TranslatorContextPtrStack     TranslatorManufacture::m_contextPtrStack;
std::unordered_map<std::string, TranslatorRefPtr>
    TranslatorManufacture::m_assetTypeToPythonTranslatorsMap;

TfToken TranslatorManufacture::TranslatorPrefixAssetType("assettype:");
TfToken TranslatorManufacture::TranslatorPrefixSchemaType("schematype:");

//----------------------------------------------------------------------------------------------------------------------
TranslatorManufacture::TranslatorManufacture(TranslatorContextPtr context)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Initialise TranslatorManufacture");

    std::set<TfType> loadedTypes;
    std::set<TfType> derivedTypes;

    bool keepGoing = true;
    while (keepGoing) {
        keepGoing = false;
        derivedTypes.clear();
        PlugRegistry::GetAllDerivedTypes<TranslatorBase>(&derivedTypes);
        for (const TfType& t : derivedTypes) {
            const auto insertResult = loadedTypes.insert(t);
            if (insertResult.second) {
                // TfType::GetFactory may cause additional plugins to be loaded
                // may means potentially more translator types. We need to re-iterate
                // over the derived types just to be sure...
                keepGoing = true;
                if (auto* factory = t.GetFactory<TranslatorFactoryBase>()) {
                    if (TranslatorRefPtr ptr = factory->create(context)) {
                        auto it = m_translatorsMap.find(ptr->getTranslatedType().GetTypeName());
                        if (it == m_translatorsMap.end()) {
                            m_translatorsMap.emplace(ptr->getTranslatedType().GetTypeName(), ptr);
                            ptr->setRegistrationType(
                                TranslatorManufacture::TranslatorPrefixSchemaType);
                        } else {
                            // located two translators for the same type
                            auto& previous = it->second;
                            if (previous->canBeOverridden() && !ptr->canBeOverridden()) {
                                m_translatorsMap[ptr->getTranslatedType().GetTypeName()] = ptr;
                                ptr->setRegistrationType(
                                    TranslatorManufacture::TranslatorPrefixSchemaType);
                            }
                        }
                    }
                }
            }
        }
    }

    derivedTypes.clear();
    PlugRegistry::GetAllDerivedTypes<ExtraDataPluginBase>(&derivedTypes);
    for (const TfType& t : derivedTypes) {
        // TfType::GetFactory may cause additional plugins to be loaded
        // may means potentially more translator types. We need to re-iterate
        // over the derived types just to be sure...
        if (auto* factory = t.GetFactory<ExtraDataPluginFactoryBase>()) {
            if (auto ptr = factory->create(context)) {
                m_extraDataPlugins.push_back(ptr);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

TranslatorRefPtr TranslatorManufacture::get(const UsdPrim& prim)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get translator from prim");

    TranslatorRefPtr translator = TfNullPtr;

    // Try metadata first
    std::string assetType;
    prim.GetMetadata(Metadata::assetType, &assetType);
    if (!assetType.empty()) {
        translator = getTranslatorByAssetTypeMetadata(assetType);
    }

    // Then try schema - which tries C++ then python
    if (!translator) {
        translator = getTranslatorBySchemaType(prim.GetTypeName());
    }
    return translator;
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorManufacture::RefPtr
TranslatorManufacture::getTranslatorByAssetTypeMetadata(const std::string& assetTypeValue)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get translator by assettype metadata");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorManufacture::getTranslatorByAssetTypeMetadata:: looking for type %s\n",
            assetTypeValue.c_str());

    // Look it up in our map of translators
    auto it = m_assetTypeToPythonTranslatorsMap.find(assetTypeValue);
    if (it != m_assetTypeToPythonTranslatorsMap.end()) {
        if (it->second->active()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorManufacture::getTranslatorByAssetTypeMetadata:: found python "
                    "translator for type %s\n",
                    assetTypeValue.c_str());
            return it->second;
        }
    }
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorManufacture::getTranslatorByAssetTypeMetadata:: no translator found for "
            "%s\n",
            assetTypeValue.c_str());
    return TfNullPtr;
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorRefPtr TranslatorManufacture::getTranslatorBySchemaType(const TfToken type_name)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get translator by schema type");

    if (auto py = getPythonTranslatorBySchemaType(type_name)) {
        return py;
    }

    TfType      type = TfType::FindDerivedByName<UsdSchemaBase>(type_name);
    std::string typeName(type.GetTypeName());
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorManufacture::getTranslatorBySchemaType:: found schema %s\n",
            typeName.c_str());

    // Look it up in our map of translators
    auto it = m_translatorsMap.find(typeName);
    if (it != m_translatorsMap.end()) {
        if (it->second->active()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorManufacture::getTranslatorBySchemaType:: found active C++ "
                    "translator for schema %s\n",
                    typeName.c_str());
            return it->second;
        }
    }
    return TranslatorRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorRefPtr TranslatorManufacture::get(const MObject& mayaObject)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get translator from Maya object");

    TranslatorRefPtr              base;
    TranslatorManufacture::RefPtr derived;

    if (auto py = TranslatorManufacture::getPythonTranslator(mayaObject)) {
        return py;
    }

    for (auto& it : m_translatorsMap) {
        if (it.second->active()) {
            ExportFlag mode = it.second->canExport(mayaObject);
            switch (mode) {
            case ExportFlag::kNotSupported: break;
            case ExportFlag::kFallbackSupport: base = it.second; break;
            case ExportFlag::kSupported: derived = it.second; break;
            default: break;
            }
        }
    }
    if (derived) {
        return derived;
    }
    return base;
}

TranslatorRefPtr TranslatorManufacture::getTranslatorFromId(const std::string& translatorId)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get translator from id");

    TranslatorRefPtr translator;

    if (translatorId.find(TranslatorManufacture::TranslatorPrefixAssetType.GetString(), 0) == 0) {
        // cover the assettype use case
        translator = getTranslatorByAssetTypeMetadata(translatorId.substr(10));
    } else if (
        translatorId.find(TranslatorManufacture::TranslatorPrefixSchemaType.GetString(), 0) == 0) {
        // cover the schema type use case,
        translator = getTranslatorBySchemaType(TfToken(translatorId.substr(11)));
    } else {
        // support backward compatibility (where the schema type was stored with no prefix
        translator = getTranslatorBySchemaType(TfToken(translatorId));
    }
    return translator;
}

std::string TranslatorManufacture::generateTranslatorId(const UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorManufacture::generateTranslatorId for prim %s\n", prim.GetPath().GetText());
    std::string      translatorId;
    TranslatorRefPtr translator = TfNullPtr;

    // Try metadata first
    std::string assetType;
    prim.GetMetadata(Metadata::assetType, &assetType);
    if (!assetType.empty()) {
        translator = getTranslatorByAssetTypeMetadata(assetType);
        if (translator) {
            translatorId = TranslatorManufacture::TranslatorPrefixAssetType.GetString() + assetType;
        }
    }
    // Then try schema - which tries C++ then python
    if (!translator) {
        translator = getTranslatorBySchemaType(prim.GetTypeName());
        if (translator) {
            translatorId = TranslatorManufacture::TranslatorPrefixSchemaType.GetString()
                + prim.GetTypeName().GetString();
        }
    }
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorManufacture::generateTranslatorId generated ID %s\n", translatorId.c_str());
    return translatorId;
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<TranslatorManufacture::ExtraDataPluginPtr>
TranslatorManufacture::getExtraDataPlugins(const MObject& mayaObject)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory,
        MProfiler::kColorE_L3,
        "Get extraDataPlugins from Maya object");

    std::vector<TranslatorManufacture::ExtraDataPluginPtr> ptrs;
    for (auto plugin : m_extraDataPlugins) {
        MFn::Type type = plugin->getFnType();
        if (mayaObject.hasFn(type)) {
            switch (type) {
            case MFn::kPluginMotionPathNode:
            case MFn::kPluginDependNode:
            case MFn::kPluginLocatorNode:
            case MFn::kPluginDeformerNode:
            case MFn::kPluginShape:
            case MFn::kPluginFieldNode:
            case MFn::kPluginEmitterNode:
            case MFn::kPluginSpringNode:
            case MFn::kPluginIkSolver:
            case MFn::kPluginHardwareShader:
            case MFn::kPluginHwShaderNode:
            case MFn::kPluginTransformNode:
            case MFn::kPluginObjectSet:
            case MFn::kPluginImagePlaneNode:
            case MFn::kPluginConstraintNode:
            case MFn::kPluginManipulatorNode:
            case MFn::kPluginSkinCluster:
            case MFn::kPluginGeometryFilter:
            case MFn::kPluginBlendShape: {
                const MString     typeName = plugin->getPluginTypeName();
                MFnDependencyNode fn(mayaObject);
                if (fn.typeName() != typeName) {
                    continue;
                }
            } break;
            default: break;
            }
            ptrs.push_back(plugin);
        }
    }
    return ptrs;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::activate(const TfTokenVector& types)
{
    for (auto& type : types) {
        auto it = m_translatorsMap.find(type.GetString());
        if (it != m_translatorsMap.end())
            it->second->activate();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::deactivate(const TfTokenVector& types)
{
    for (auto& type : types) {
        auto it = m_translatorsMap.find(type.GetString());
        if (it != m_translatorsMap.end())
            it->second->deactivate();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::activateAll()
{
    for (auto& it : m_translatorsMap) {
        it.second->activate();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::deactivateAll()
{
    for (auto& it : m_translatorsMap) {
        it.second->deactivate();
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorManufacture::addPythonTranslator(TranslatorRefPtr tb, const TfToken& assetType)
{
    if (tb->getTranslatedType().IsUnknown() && assetType.IsEmpty()) {
        return false;
    }
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorManufacture::addPythonTranslator\n");
    tb->initialize();
    m_pythonTranslators.push_back(tb);
    if (!assetType.IsEmpty()) {
        m_assetTypeToPythonTranslatorsMap.emplace(assetType.GetString(), tb);
        tb->setRegistrationType(TranslatorManufacture::TranslatorPrefixAssetType);
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorManufacture::addPythonTranslator added by asset type %s\n",
                assetType.GetText());
    } else {
        tb->setRegistrationType(TranslatorManufacture::TranslatorPrefixSchemaType);
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorManufacture::addPythonTranslator added by schema type %s\n",
                tb->getTranslatedType().GetTypeName().c_str());
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::clearPythonTranslators()
{
    m_pythonTranslators.clear();
    m_assetTypeToPythonTranslatorsMap.clear();
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorRefPtr TranslatorManufacture::getPythonTranslatorBySchemaType(const TfToken type_name)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Get Python translator by schema type");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorManufacture::getPythonTranslatorBySchemaType looking for translator for "
            "type %s\n",
            type_name.GetText());
    TfType type = TfType::FindDerivedByName<UsdSchemaBase>(type_name);

    for (auto it : TranslatorManufacture::m_pythonTranslators) {
        if ((it->getRegistrationType() == TranslatorManufacture::TranslatorPrefixSchemaType)
            && (type == it->getTranslatedType())) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorManufacture::getPythonTranslatorBySchemaType:: found a translator "
                    "for type %s\n",
                    type_name.GetText());
            return it;
        }
    }
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorManufacture::getPythonTranslatorBySchemaType:: :didn't find any "
             "translator::returning nothing");
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorRefPtr TranslatorManufacture::getPythonTranslator(const MObject& mayaObject)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory,
        MProfiler::kColorE_L3,
        "Get Python translator from Maya object");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorManufacture::getPythonTranslator %s\n", mayaObject.apiTypeStr());
    TranslatorRefPtr base;
    for (auto it : TranslatorManufacture::m_pythonTranslators) {
        auto canExport = it->canExport(mayaObject);
        switch (canExport) {
        case ExportFlag::kSupported: return it;

        case ExportFlag::kFallbackSupport: base = it; break;

        default: break;
        }
    }
    return base;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorManufacture::deletePythonTranslator(const TfType type_name)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Delete Python translator");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorManufacture::deletePythonTranslator\n");
    for (auto it = TranslatorManufacture::m_pythonTranslators.begin(),
              end = TranslatorManufacture::m_pythonTranslators.end();
         it != end;
         ++it) {
        auto state_ = PyGILState_Ensure();
        auto thetype = (*it)->getTranslatedType();
        PyGILState_Release(state_);
        if (type_name == thetype) {
            TranslatorManufacture::m_pythonTranslators.erase(it);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::setPythonTranslatorContexts(TranslatorContext::RefPtr context)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Prepare Python translators");

    for (auto it : TranslatorManufacture::m_pythonTranslators) {
        it->setContext(context);
    }
    for (auto it : TranslatorManufacture::m_assetTypeToPythonTranslatorsMap) {
        (it.second)->setContext(context);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::preparePythonTranslators(TranslatorContext::RefPtr context)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorManufacture::preparePythonTranslators\n");
    setPythonTranslatorContexts(context);
    m_contextPtrStack.push(context);
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::updatePythonTranslators(TranslatorContext::RefPtr context)
{
    MProfilingScope profilerScope(
        _translatorProfilerCategory, MProfiler::kColorE_L3, "Update Python translators");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorManufacture::updatePythonTranslators\n");
    m_contextualisedPythonTranslators.clear();
    for (const auto& tr : m_pythonTranslators) {
        m_contextualisedPythonTranslators.push_back(tr);
        m_contextualisedPythonTranslators.back()->setContext(context);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorManufacture::popPythonTranslatorContexts()
{
    if (m_contextPtrStack.empty()) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg("TranslatorManufacture::popPythonTranslatorContexts(): No contextPtr left in the "
                 "stack\n");
        return;
    }
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorManufacture::popPythonTranslatorContexts()\n");
    m_contextPtrStack.pop();

    if (!m_contextPtrStack.empty()) {
        setPythonTranslatorContexts(m_contextPtrStack.top());
    }
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<TranslatorRefPtr> TranslatorManufacture::getPythonTranslators()
{
    return m_pythonTranslators;
}

//----------------------------------------------------------------------------------------------------------------------
TF_REGISTRY_FUNCTION(TfType) { TfType::Define<TranslatorBase>(); }

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
