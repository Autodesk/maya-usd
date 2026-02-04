//
// Copyright 2016 Pixar
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
#include "jobArgs.h"

#include <mayaUsd/fileio/jobContextRegistry.h>
#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/utilDictionary.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/utils/diffPrims.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/schema.h>
#if PXR_VERSION < 2508
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#else
#include <pxr/usd/sdf/usdaFileFormat.h>
#include <pxr/usd/sdf/usdcFileFormat.h>
#endif
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MNodeClass.h>
#include <maya/MTypeId.h>

#include <ghc/filesystem.hpp>

#include <cstdlib>
#include <mutex>
#include <ostream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaTranslatorTokens, PXRUSDMAYA_TRANSLATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(UsdMayaJobExportArgsTokens, PXRUSDMAYA_JOB_EXPORT_ARGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(UsdMayaJobImportArgsTokens, PXRUSDMAYA_JOB_IMPORT_ARGS_TOKENS);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _usdExportInfoScope,

    (UsdMaya)
        (UsdExport)
);
// clang-format on

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _usdImportInfoScope,

    (UsdMaya)
        (UsdImport)
);
// clang-format on

namespace {

// Default material scope name as defined by USD Assets working group.
// See https://wiki.aswf.io/display/WGUSD/Guidelines+for+Structuring+USD+Assets
static constexpr auto kDefaultMaterialScopeName = "mtl";

using namespace MayaUsd::DictUtils;

// The chaser args are stored as vectors of vectors (since this is how you
// would need to pass them in the Maya Python command API). Convert this to a
// map of maps.
std::map<std::string, UsdMayaJobExportArgs::ChaserArgs>
_ChaserArgs(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> chaserArgs
        = extractVector<std::vector<VtValue>>(userArgs, key);

    std::map<std::string, UsdMayaJobExportArgs::ChaserArgs> result;
    for (const std::vector<VtValue>& argTriple : chaserArgs) {
        if (argTriple.size() != 3) {
            TF_CODING_ERROR("Each chaser arg must be a triple (chaser, arg, value)");
            return std::map<std::string, UsdMayaJobExportArgs::ChaserArgs>();
        }

        const std::string& chaser = argTriple[0].Get<std::string>();
        const std::string& arg = argTriple[1].Get<std::string>();
        const std::string& value = argTriple[2].Get<std::string>();
        result[chaser][arg] = value;
    }
    return result;
}

double _ExtractMetersPerUnit(const VtDictionary& userArgs)
{
    MDistance::Unit mayaInternalUnit = MDistance::internalUnit();
    auto metersPerUnit = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(mayaInternalUnit);

    auto value = extractDouble(userArgs, UsdMayaJobExportArgsTokens->metersPerUnit, 0.0);

    // Anything less than equal to -1 is treated as the UI unit
    if (value <= -1.0) {
        MDistance::Unit mayaUIUnit = MDistance::uiUnit();
        return UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(mayaUIUnit);
    }

    // Zero is treated as the default and returns the default internal unit
    if (value <= 0.0) {
        return metersPerUnit;
    }

    // Otherwise take the final value as is
    return value;
}

std::map<std::string, std::string> _UVSetRemaps(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> uvRemaps
        = extractVector<std::vector<VtValue>>(userArgs, key);

    std::map<std::string, std::string> result;
    for (const std::vector<VtValue>& remap : uvRemaps) {
        if (remap.size() != 2) {
            TF_CODING_ERROR("Failed to parse remapping, all items must be pairs (from, to)");
            return {};
        }

        const std::string& from = remap[0].Get<std::string>();
        const std::string& to = remap[1].Get<std::string>();
        result[from] = to;
    }
    return result;
}

bool _striequals(const std::string& a, const std::string& b)
{
    size_t aSize = a.size();
    if (b.size() != aSize) {
        return false;
    }
    for (size_t i = 0; i < aSize; ++i)
        if (std::tolower(a[i]) != std::tolower(b[i])) {
            return false;
        }
    return true;
}

// The Custom Layer Data is stored as a vector of vectors (as this is
// how a multi use , multi argument flag is passed in).
// This function converts it to a VtDictionary.
// Parsing failures skip the value instead of early returning.
static VtDictionary _CustomLayerData(const VtDictionary& userArgs, const TfToken& userArgKey)
{
    const std::vector<std::vector<VtValue>> keyValueTypes
        = extractVector<std::vector<VtValue>>(userArgs, userArgKey);

    VtDictionary data = VtDictionary(keyValueTypes.size());

    for (const std::vector<VtValue>& argTriple : keyValueTypes) {
        if (argTriple.size() != 3) {
            TF_WARN("Each customLayerData argument must be a triple (key, value, type)");
            continue;
        }

        const std::string& key = argTriple[0].Get<std::string>();
        const std::string& raw_value = argTriple[1].Get<std::string>();
        const std::string& type = argTriple[2].Get<std::string>();

        VtValue val = VtValue();
        if (type == "string") {
            val = raw_value;
        } else if (type == "int") {
            char* e = NULL;
            val = static_cast<int>(std::strtol(raw_value.c_str(), &e, 10));
            if (e != &raw_value[0] + raw_value.size()) {
                TF_WARN(
                    "Could not parse '%s' as an integer; the first invalid digit was: %s",
                    raw_value.c_str(),
                    e);
                continue;
            } else if (errno == ERANGE) {
                TF_WARN(
                    "Could not parse '%s' as an integer; it would have exceeded the valid range.",
                    raw_value.c_str());
                continue;
            }
        } else if (type == "float") {
            char* e = NULL;
            val = static_cast<float>(std::strtof(raw_value.c_str(), &e));
            if (e != &raw_value[0] + raw_value.size()) {
                TF_WARN(
                    "Could not parse '%s' as a float; the first invalid digit was: %s",
                    raw_value.c_str(),
                    e);
                errno = 0;
                continue;
            } else if (errno == ERANGE) {
                TF_WARN(
                    "Could not parse '%s' as a float; it would have exceeded the valid range.",
                    raw_value.c_str());
                errno = 0;
                continue;
            }
        } else if (type == "double") {
            char* e = NULL;
            val = static_cast<double>(std::strtod(raw_value.c_str(), &e));
            if (e != &raw_value[0] + raw_value.size()) {
                TF_WARN(
                    "Could not parse '%s' as a double; the first invalid digit was: %s",
                    raw_value.c_str(),
                    e);
                errno = 0;
                continue;
            } else if (errno == ERANGE) {
                TF_WARN(
                    "Could not parse '%s' as a double; it would have exceeded the valid range.",
                    raw_value.c_str());
                errno = 0;
                continue;
            }
        } else if (type == "bool") {
            if (raw_value == "1") {
                val = true;
            } else if (raw_value == "0") {
                val = false;
            } else if (_striequals(raw_value, "true")) {
                val = true;
            } else if (_striequals(raw_value, "false")) {
                val = false;
            } else {
                TF_WARN("Could not parse '%s' as bool", raw_value.c_str());
                continue;
            }
        } else {
            TF_WARN("Unsupported customLayerData type '%s' for '%s'", type.c_str(), key.c_str());
            continue;
        }

        data.SetValueAtPath(key, val);
    }

    return data;
}

// The shadingMode args are stored as vectors of vectors (since this is how you
// would need to pass them in the Maya Python command API).
UsdMayaJobImportArgs::ShadingModes
_shadingModesImportArgs(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> shadingModeArgs
        = extractVector<std::vector<VtValue>>(userArgs, key);

    const TfTokenVector modes = UsdMayaShadingModeRegistry::ListImporters();

    UsdMayaJobImportArgs::ShadingModes result;
    for (const std::vector<VtValue>& argTuple : shadingModeArgs) {
        if (argTuple.size() != 2) {
            TF_CODING_ERROR(
                "Each shadingMode arg must be a tuple (shadingMode, convertMaterialFrom)");
            return UsdMayaJobImportArgs::ShadingModes();
        }

        TfToken shadingMode = TfToken(argTuple[0].Get<std::string>().c_str());
        TfToken convertMaterialFrom = TfToken(argTuple[1].Get<std::string>().c_str());

        if (shadingMode == UsdMayaShadingModeTokens->none) {
            break;
        }

        if (std::find(modes.cbegin(), modes.cend(), shadingMode) == modes.cend()) {
            TF_CODING_ERROR("Unknown shading mode '%s'", shadingMode.GetText());
            return UsdMayaJobImportArgs::ShadingModes();
        }

        if (shadingMode == UsdMayaShadingModeTokens->useRegistry) {
            auto const& info
                = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(convertMaterialFrom);
            if (!info.hasImporter) {
                TF_CODING_ERROR("Unknown material conversion '%s'", convertMaterialFrom.GetText());
                return UsdMayaJobImportArgs::ShadingModes();
            }
            // Do not validate second parameter if not in a useRegistry scenario.
        }

        result.push_back(UsdMayaJobImportArgs::ShadingMode { shadingMode, convertMaterialFrom });
    }
    return result;
}

TfToken _GetMaterialsScopeName(const std::string& materialsScopeName)
{
    const TfToken defaultMaterialsScopeName = UsdUtilsGetMaterialsScopeName();

    if (TfGetEnvSetting(USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME)) {
        // If the env setting is set, make sure we don't allow the materials
        // scope name to be overridden by a parameter value.
        return defaultMaterialsScopeName;
    } else {
        const std::string mayaUsdDefaultMaterialsScopeName
            = TfGetenv("MAYAUSD_MATERIALS_SCOPE_NAME");
        if (!mayaUsdDefaultMaterialsScopeName.empty()) {
            return TfToken(mayaUsdDefaultMaterialsScopeName);
        }
    }

    if (SdfPath::IsValidIdentifier(materialsScopeName)) {
        return TfToken(materialsScopeName);
    }

    TF_CODING_ERROR(
        "'%s' value '%s' is not a valid identifier. Using default "
        "value of '%s' instead.",
        UsdMayaJobExportArgsTokens->materialsScopeName.GetText(),
        materialsScopeName.c_str(),
        defaultMaterialsScopeName.GetText());

    return defaultMaterialsScopeName;
}

PcpMapFunction::PathMap _ExportRootsMap(
    const VtDictionary&             userArgs,
    const TfToken&                  key,
    bool                            stripNamespaces,
    const UsdMayaUtil::MDagPathSet& dagPaths)
{
    PcpMapFunction::PathMap pathMap;

    auto addSdfPathToMapFn = [&pathMap](const SdfPath& rootSdfPath) {
        if (rootSdfPath.IsEmpty())
            return;

        SdfPath newRootSdfPath
            = rootSdfPath.ReplacePrefix(rootSdfPath.GetParentPath(), SdfPath::AbsoluteRootPath());

        pathMap[rootSdfPath] = newRootSdfPath;
    };

    auto addExportRootPathPairFn = [addSdfPathToMapFn,
                                    stripNamespaces](const MDagPath& rootDagPath) {
        if (!rootDagPath.isValid())
            return;

        SdfPath rootSdfPath = UsdMayaUtil::MDagPathToUsdPath(rootDagPath, false, stripNamespaces);
        addSdfPathToMapFn(rootSdfPath);
    };

    bool includeEntireSelection = false;

    const std::vector<std::string> exportRoots = extractVector<std::string>(userArgs, key);
    for (const std::string& rootPath : exportRoots) {
        if (!rootPath.empty()) {
            MDagPath rootDagPath = UsdMayaUtil::nameToDagPath(rootPath);
            addExportRootPathPairFn(rootDagPath);
        } else {
            includeEntireSelection = true;
        }
    }

    if (includeEntireSelection) {
        for (const MDagPath& dagPath : dagPaths) {
            addExportRootPathPairFn(dagPath);
        }
    }

    // If we export at least one root, then add the instance masters as root too.
    // Otherwise they would fail to map to anything and thus fail to be created.
    if (exportRoots.size() > 0) {
        addSdfPathToMapFn(UsdMayaWriteJobContext::GetInstanceMasterBasePath());
    }

    return pathMap;
}

void _AddFilteredTypeName(const MString& typeName, std::set<unsigned int>& filteredTypeIds)
{
    MNodeClass   cls(typeName);
    unsigned int id = cls.typeId().id();
    if (id == 0) {
        TF_WARN("Given excluded node type '%s' does not exist; ignoring", typeName.asChar());
        return;
    }
    filteredTypeIds.insert(id);
    // We also insert all inherited types - only way to query this is through mel,
    // which is slower, but this should be ok, as these queries are only done
    // "up front" when the export starts, not per-node
    MString queryCommand("nodeType -isTypeName -derived ");
    queryCommand += typeName;
    MStringArray inheritedTypes;
    MStatus      status = MGlobal::executeCommand(queryCommand, inheritedTypes, false, false);
    if (!status) {
        TF_WARN(
            "Error querying derived types for '%s': %s",
            typeName.asChar(),
            status.errorString().asChar());
        return;
    }

    for (unsigned int i = 0; i < inheritedTypes.length(); ++i) {
        if (inheritedTypes[i].length() == 0)
            continue;
        id = MNodeClass(inheritedTypes[i]).typeId().id();
        if (id == 0) {
            // Unfortunately, the returned list will often include weird garbage, like
            // "THconstraint" for "constraint", which cannot be converted to a MNodeClass,
            // so just ignore these...
            continue;
        }
        filteredTypeIds.insert(id);
    }
}

std::set<unsigned int> _FilteredTypeIds(const VtDictionary& userArgs)
{
    const std::vector<std::string> vec
        = extractVector<std::string>(userArgs, UsdMayaJobExportArgsTokens->filterTypes);
    std::set<unsigned int> result;
    for (const std::string& s : vec) {
        _AddFilteredTypeName(s.c_str(), result);
    }
    return result;
}

std::vector<VtValue>
_MergeVectors(const std::vector<VtValue>& existingValues, const std::vector<VtValue>& newValues)
{
    std::vector<VtValue> resultValues = existingValues;

    for (const VtValue& element : newValues) {
        if (element.IsHolding<std::vector<VtValue>>()) {
            // vector<vector<string>> is common for chaserArgs and shadingModes
            auto findElement = [&element](const VtValue& a) {
                return UsdUfe::compareValues(element, a) == UsdUfe::DiffResult::Same;
            };
            if (std::find_if(resultValues.begin(), resultValues.end(), findElement)
                == resultValues.end()) {
                resultValues.push_back(element);
            }
        } else {
            if (std::find(resultValues.begin(), resultValues.end(), element)
                == resultValues.end()) {
                resultValues.push_back(element);
            }
        }
    }

    return resultValues;
}

bool _MergeVectors(
    const VtValue& existingValue,
    const VtValue& newValue,
    VtValue&       resultValue,
    bool           allowConflicts)
{
    if (!newValue.IsHolding<std::vector<VtValue>>()) {
        return allowConflicts;
    }

    resultValue = _MergeVectors(
        existingValue.UncheckedGet<std::vector<VtValue>>(),
        newValue.UncheckedGet<std::vector<VtValue>>());

    return true;
}

// Merge into the existing dictionary but does *not* over-write existing values.
// Record the source name of new values in the given map.
// (It maps value names to the dictionary name where they were initially found.)
// Conflicting values will be reported and return false if conflicts are not allowed.
// Conflicting values will be ignored if conflicts are allowed.
bool _MergeDictionaries(
    VtDictionary&                       existingDict,
    std::map<std::string, std::string>& existingSourceNames,
    const VtDictionary&                 newDict,
    const std::string&                  newDictName,
    bool                                allowConflicts)
{
    bool allMergeOK = true;

    for (auto const& dictTuple : newDict) {
        const std::string& valueName = dictTuple.first;
        const VtValue&     newValue = dictTuple.second;

        auto existingValueIter = existingDict.find(valueName);
        if (existingValueIter == existingDict.end()) {
            // New value, no need to merge of manage conflicts.
            existingDict[valueName] = newValue;
            existingSourceNames[valueName] = newDictName;
            continue;
        }

        // We have already seen this argument from another jobContext. Look for conflicts:
        const VtValue& existingValue = existingValueIter->second;

        if (existingValue.IsHolding<std::vector<VtValue>>()) {
            VtValue mergedValue;
            if (_MergeVectors(existingValue, newValue, mergedValue, allowConflicts)) {
                existingDict[valueName] = mergedValue;
            } else if (!allowConflicts) {
                // We have both an array and a scalar under the same argument name.
                const std::string& existingDictName = existingSourceNames[valueName];
                TF_RUNTIME_ERROR(TfStringPrintf(
                    "Context '%s' and context '%s' do not agree on type of argument '%s'.",
                    newDictName.c_str(),
                    existingDictName.c_str(),
                    valueName.c_str()));
                allMergeOK = false;
            }
        } else {
            // Scalar value already exists. Check for value conflicts:
            if (existingValue != newValue) {
                if (!allowConflicts) {
                    const std::string& existingDictName = existingSourceNames[valueName];
                    TF_RUNTIME_ERROR(TfStringPrintf(
                        "Context '%s' and context '%s' do not agree on argument '%s'.",
                        newDictName.c_str(),
                        existingDictName.c_str(),
                        valueName.c_str()));
                    allMergeOK = false;
                }
            }
        }
    }

    return allMergeOK;
}

// Merges all the jobContext arguments dictionaries found while exploring the jobContexts into a
// single one. Also checks for conflicts and errors.
//
// Inputs:
// isExport: determines if we are calling the import or the export jobContext callback.
// userArgs: original user arguments, potentially containing jobContexts to merge.
//
// Outputs:
// allContextArgs: dictionary of all extra jobContext arguments merged together.
// return value: true if the merge was successful, false if a conflict or an error was detected.
bool _MergeJobContexts(
    bool                                isExport,
    const VtDictionary&                 userArgs,
    std::map<std::string, std::string>& argInitialSource,
    VtDictionary&                       allContextArgs)
{
    // List of all argument dictionaries found while exploring jobContexts
    std::vector<VtDictionary> contextArgs;

    bool canMergeContexts = true;

    // This first loop gathers all job context argument dictionaries found in the userArgs.
    // The job context provides their desired arguments through their callback.
    const TfToken& jcKey = UsdMayaJobExportArgsTokens->jobContext;
    if (VtDictionaryIsHolding<std::vector<VtValue>>(userArgs, jcKey)) {
        for (const VtValue& v : VtDictionaryGet<std::vector<VtValue>>(userArgs, jcKey)) {
            if (v.IsHolding<std::string>()) {
                const TfToken jobContext(v.UncheckedGet<std::string>());
                const UsdMayaJobContextRegistry::ContextInfo& ci
                    = UsdMayaJobContextRegistry::GetJobContextInfo(jobContext);
                auto enablerCallback
                    = isExport ? ci.exportEnablerCallback : ci.importEnablerCallback;
                if (enablerCallback) {
                    VtDictionary extraArgs = enablerCallback();
                    // Add the job context name to the args (for reference when merging):
                    VtDictionary::iterator jobContextNamesIt = extraArgs.find(jcKey);
                    if (jobContextNamesIt != extraArgs.end()) {
                        // We already have a vector. Ensure it is of size 1 and contains only the
                        // current context name:
                        const std::vector<VtValue>& currContextNames
                            = VtDictionaryGet<std::vector<VtValue>>(extraArgs, jcKey);
                        if ((currContextNames.size() == 1 && currContextNames.front() != v)
                            || currContextNames.size() > 1) {
                            TF_RUNTIME_ERROR(TfStringPrintf(
                                "Arguments for job context '%s' can not include extra contexts.",
                                jobContext.GetText()));
                            canMergeContexts = false;
                        }
                    }
                    std::vector<VtValue> jobContextNames;
                    jobContextNames.push_back(v);
                    extraArgs[jcKey] = jobContextNames;
                    contextArgs.push_back(extraArgs);
                } else {
                    MGlobal::displayWarning(
                        TfStringPrintf("Ignoring unknown job context '%s'.", jobContext.GetText())
                            .c_str());
                }
            }
        }
    }

    // Traverse argument dictionaries and look for merge conflicts while building the returned
    // allContextArgs.
    for (auto const& dict : contextArgs) {
        // We made sure the value exists in the above loop, so we can fetch without fear:
        const std::string& sourceName = VtDictionaryGet<std::vector<VtValue>>(dict, jcKey)
                                            .front()
                                            .UncheckedGet<std::string>();

        const bool allowConflicts = false;
        if (!_MergeDictionaries(allContextArgs, argInitialSource, dict, sourceName, allowConflicts))
            canMergeContexts = false;
    }
    return canMergeContexts;
}

VtDictionary
_MergeAllArguments(bool isExport, const VtDictionary& userArgs, const VtDictionary& defaultsArgs)
{
    // This will contain user argumentss, default arguments and job-context arguments
    // all merged together.
    VtDictionary allArgs;

    // Convenience map holding the job-context that first introduces an argument to the final
    // dictionary. Allows printing meaningful error messages.
    std::map<std::string, std::string> argSources;

    // First we merge all job context arguments for all job contexts that were given in
    // the "jobContext" entry of the user arguments dictionary.
    if (!_MergeJobContexts(isExport, userArgs, argSources, allArgs)) {
        MGlobal::displayWarning("Errors while processing job contexts. Using base options.");
        return VtDictionaryOver(userArgs, defaultsArgs);
    }

    // We now merge user argument with the default values over the job-context arguments.
    // The job-context values have priority and we allow conflicting values. (After all,
    // one of the *goal* of the job context is to provide specific, different defaults
    // for import/export options than the default options.)
    const VtDictionary withDefaults = VtDictionaryOver(userArgs, defaultsArgs);
    const std::string  userArgsName = "user arguments";

    const bool allowConflicts = true;
    if (!_MergeDictionaries(allArgs, argSources, withDefaults, userArgsName, allowConflicts)) {
        MGlobal::displayWarning(
            "Errors while merging job contexts with user arguments. Using base options.");
    }

    return allArgs;
}

bool _GetEncodedArg(const MString& option, std::string& argName, MString& argValue)
{
    MStringArray theOption;
    option.split('=', theOption);
    if (theOption.length() < 1)
        return false;

    argName = theOption[0].asChar();
    argValue = theOption.length() > 1 ? theOption[1] : MString();
    return true;
}

} // namespace
UsdMayaJobExportArgs::UsdMayaJobExportArgs(
    const VtDictionary&             userArgs,
    const UsdMayaUtil::MDagPathSet& dagPaths,
    const MSelectionList&           fullList,
    const std::vector<double>&      timeSamples)
    : compatibility(extractToken(
        userArgs,
        UsdMayaJobExportArgsTokens->compatibility,
        UsdMayaJobExportArgsTokens->none,
        { UsdMayaJobExportArgsTokens->appleArKit }))
    , defaultMeshScheme(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->defaultMeshScheme,
          UsdGeomTokens->catmullClark,
          { UsdGeomTokens->loop, UsdGeomTokens->bilinear, UsdGeomTokens->none }))
    , defaultUSDFormat(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->defaultUSDFormat,
#if PXR_VERSION < 2508
          UsdUsdcFileFormatTokens->Id,
          { UsdUsdaFileFormatTokens->Id }))
#else
          SdfUsdcFileFormatTokens->Id,
          { SdfUsdaFileFormatTokens->Id }))
#endif
    , eulerFilter(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->eulerFilter))
    , excludeInvisible(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->renderableOnly))
    , exportCollectionBasedBindings(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportCollectionBasedBindings))
    , exportColorSets(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportColorSets))
    , exportMaterials(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportMaterials))
    , exportAssignedMaterials(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportAssignedMaterials))
    , legacyMaterialScope(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->legacyMaterialScope))
    , exportDefaultCameras(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->defaultCameras))
    , exportDisplayColor(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportDisplayColor))
    , exportDistanceUnit(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportDistanceUnit))
    , exportInstances(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportInstances))
    , exportMaterialCollections(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportMaterialCollections))
    , exportMeshUVs(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportUVs))
    , exportNurbsExplicitUV(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportUVs))
    , exportRelativeTextures(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->exportRelativeTextures,
          UsdMayaJobExportArgsTokens->automatic,
          { UsdMayaJobExportArgsTokens->automatic,
            UsdMayaJobExportArgsTokens->absolute,
            UsdMayaJobExportArgsTokens->relative }))
    , referenceObjectMode(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->referenceObjectMode,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->attributeOnly, UsdMayaJobExportArgsTokens->defaultToMesh }))
    , exportRefsAsInstanceable(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportRefsAsInstanceable))
    , exportSelected(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportSelected))
    , exportSkels(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->exportSkels,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->auto_, UsdMayaJobExportArgsTokens->explicit_ }))
    , exportSkin(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->exportSkin,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->auto_, UsdMayaJobExportArgsTokens->explicit_ }))
    , exportBlendShapes(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportBlendShapes))
    , exportVisibility(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportVisibility))
    , exportComponentTags(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportComponentTags))
    , exportStagesAsRefs(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->exportStagesAsRefs))
    , file(extractString(userArgs, UsdMayaJobExportArgsTokens->file))
    , ignoreWarnings(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->ignoreWarnings))
    , includeEmptyTransforms(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->includeEmptyTransforms))
    , isDuplicating(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->isDuplicating))
    , materialCollectionsPath(
          extractAbsolutePath(userArgs, UsdMayaJobExportArgsTokens->materialCollectionsPath))
    , materialsScopeName(_GetMaterialsScopeName(
          extractString(userArgs, UsdMayaJobExportArgsTokens->materialsScopeName)))
    , mergeTransformAndShape(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->mergeTransformAndShape))
    , normalizeNurbs(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->normalizeNurbs))
    , preserveUVSetNames(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->preserveUVSetNames))
    , stripNamespaces(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->stripNamespaces))
    , hideSourceData(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->hideSourceData))
    , worldspace(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->worldspace))
    , writeDefaults(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->writeDefaults))
    , parentScope(extractAbsolutePath(userArgs, UsdMayaJobExportArgsTokens->parentScope))
    , rootPrim(extractAbsolutePath(userArgs, UsdMayaJobExportArgsTokens->rootPrim))
    , rootPrimType(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->rootPrimType,
          UsdMayaJobExportArgsTokens->scope,
          { UsdMayaJobExportArgsTokens->xform }))
    , upAxis(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->upAxis,
          UsdMayaJobExportArgsTokens->mayaPrefs,
          { UsdMayaJobExportArgsTokens->none,
            UsdMayaJobExportArgsTokens->y,
            UsdMayaJobExportArgsTokens->z }))
    , unit(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->unit,
          UsdMayaJobExportArgsTokens->mayaPrefs,
          { UsdMayaJobExportArgsTokens->none,
            UsdMayaJobExportArgsTokens->nm,
            UsdMayaJobExportArgsTokens->um,
            UsdMayaJobExportArgsTokens->mm,
            UsdMayaJobExportArgsTokens->cm,
            UsdMayaJobExportArgsTokens->dm,
            UsdMayaJobExportArgsTokens->m,
            UsdMayaJobExportArgsTokens->km,
            UsdMayaJobExportArgsTokens->lightyear,
            UsdMayaJobExportArgsTokens->inch,
            UsdMayaJobExportArgsTokens->foot,
            UsdMayaJobExportArgsTokens->yard,
            UsdMayaJobExportArgsTokens->mile }))
    , renderLayerMode(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->renderLayerMode,
          UsdMayaJobExportArgsTokens->defaultLayer,
          { UsdMayaJobExportArgsTokens->currentLayer,
            UsdMayaJobExportArgsTokens->modelingVariant }))
    , rootKind(extractString(userArgs, UsdMayaJobExportArgsTokens->kind))
    , animationType(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->animationType,
          UsdMayaJobExportArgsTokens->timesamples,
          {
              UsdMayaJobExportArgsTokens->timesamples,
              UsdMayaJobExportArgsTokens->curves,
              UsdMayaJobExportArgsTokens->both,
          }))
    , disableModelKindProcessor(
          extractBoolean(userArgs, UsdMayaJobExportArgsTokens->disableModelKindProcessor))
    , shadingMode(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->shadingMode,
          UsdMayaShadingModeTokens->useRegistry,
          []() {
              auto exporters = UsdMayaShadingModeRegistry::ListExporters();
              exporters.emplace_back(UsdMayaShadingModeTokens->none);
              return exporters;
          }()))
    , allMaterialConversions(
          extractTokenSet(userArgs, UsdMayaJobExportArgsTokens->convertMaterialsTo))
    , verbose(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->verbose))
    , staticSingleSample(extractBoolean(userArgs, UsdMayaJobExportArgsTokens->staticSingleSample))
    , geomSidedness(extractToken(
          userArgs,
          UsdMayaJobExportArgsTokens->geomSidedness,
          UsdMayaJobExportArgsTokens->derived,
          { UsdMayaJobExportArgsTokens->single, UsdMayaJobExportArgsTokens->double_ }))
    , includeAPINames(extractTokenSet(userArgs, UsdMayaJobExportArgsTokens->apiSchema))
    , jobContextNames(extractTokenSet(userArgs, UsdMayaJobExportArgsTokens->jobContext))
    , excludeExportTypes(extractTokenSet(userArgs, UsdMayaJobExportArgsTokens->excludeExportTypes))
    , defaultPrim(extractString(userArgs, UsdMayaJobExportArgsTokens->defaultPrim))
    , chaserNames(extractVector<std::string>(userArgs, UsdMayaJobExportArgsTokens->chaser))
    , allChaserArgs(_ChaserArgs(userArgs, UsdMayaJobExportArgsTokens->chaserArgs))
    , customLayerData(_CustomLayerData(userArgs, UsdMayaJobExportArgsTokens->customLayerData))
    , metersPerUnit(_ExtractMetersPerUnit(userArgs))
    , remapUVSetsTo(_UVSetRemaps(userArgs, UsdMayaJobExportArgsTokens->remapUVSetsTo))
    , melPerFrameCallback(extractString(userArgs, UsdMayaJobExportArgsTokens->melPerFrameCallback))
    , melPostCallback(extractString(userArgs, UsdMayaJobExportArgsTokens->melPostCallback))
    , pythonPerFrameCallback(
          extractString(userArgs, UsdMayaJobExportArgsTokens->pythonPerFrameCallback))
    , pythonPostCallback(extractString(userArgs, UsdMayaJobExportArgsTokens->pythonPostCallback))
    , dagPaths(dagPaths)
    , fullObjectList(fullList)
    , timeSamples(timeSamples)
    , exportRoots(extractVector<std::string>(userArgs, UsdMayaJobExportArgsTokens->exportRoots))
    , rootMapFunction(PcpMapFunction::Create(
          _ExportRootsMap(
              userArgs,
              UsdMayaJobExportArgsTokens->exportRoots,
              stripNamespaces,
              dagPaths),
          SdfLayerOffset()))
    , filteredTypeIds(_FilteredTypeIds(userArgs))
{
}

std::ostream& operator<<(std::ostream& out, const UsdMayaJobExportArgs& exportArgs)
{
    out << "compatibility: " << exportArgs.compatibility << std::endl
        << "defaultMeshScheme: " << exportArgs.defaultMeshScheme << std::endl
        << "defaultUSDFormat: " << exportArgs.defaultUSDFormat << std::endl
        << "eulerFilter: " << TfStringify(exportArgs.eulerFilter) << std::endl
        << "excludeInvisible: " << TfStringify(exportArgs.excludeInvisible) << std::endl
        << "exportCollectionBasedBindings: "
        << TfStringify(exportArgs.exportCollectionBasedBindings) << std::endl
        << "exportColorSets: " << TfStringify(exportArgs.exportColorSets) << std::endl
        << "exportMaterials: " << TfStringify(exportArgs.exportMaterials) << std::endl
        << "exportAssignedMaterials: " << TfStringify(exportArgs.exportAssignedMaterials)
        << std::endl
        << "legacyMaterialScope: " << TfStringify(exportArgs.legacyMaterialScope) << std::endl
        << "exportDefaultCameras: " << TfStringify(exportArgs.exportDefaultCameras) << std::endl
        << "exportDisplayColor: " << TfStringify(exportArgs.exportDisplayColor) << std::endl
        << "exportDistanceUnit: " << TfStringify(exportArgs.exportDistanceUnit) << std::endl
        << "metersPerUnit: " << TfStringify(exportArgs.metersPerUnit) << std::endl
        << "exportInstances: " << TfStringify(exportArgs.exportInstances) << std::endl
        << "exportMaterialCollections: " << TfStringify(exportArgs.exportMaterialCollections)
        << std::endl
        << "exportMeshUVs: " << TfStringify(exportArgs.exportMeshUVs) << std::endl
        << "exportNurbsExplicitUV: " << TfStringify(exportArgs.exportNurbsExplicitUV) << std::endl
        << "exportRelativeTextures: " << TfStringify(exportArgs.exportRelativeTextures) << std::endl
        << "referenceObjectMode: " << exportArgs.referenceObjectMode << std::endl
        << "exportRefsAsInstanceable: " << TfStringify(exportArgs.exportRefsAsInstanceable)
        << std::endl
        << "exportSelected: " << TfStringify(exportArgs.exportSelected) << std::endl
        << "exportSkels: " << TfStringify(exportArgs.exportSkels) << std::endl
        << "exportSkin: " << TfStringify(exportArgs.exportSkin) << std::endl
        << "exportBlendShapes: " << TfStringify(exportArgs.exportBlendShapes) << std::endl
        << "exportVisibility: " << TfStringify(exportArgs.exportVisibility) << std::endl
        << "exportComponentTags: " << TfStringify(exportArgs.exportComponentTags) << std::endl
        << "exportStagesAsRefs: " << TfStringify(exportArgs.exportStagesAsRefs) << std::endl
        << "file: " << exportArgs.file << std::endl
        << "ignoreWarnings: " << TfStringify(exportArgs.ignoreWarnings) << std::endl
        << "includeEmptyTransforms: " << TfStringify(exportArgs.includeEmptyTransforms)
        << "isDuplicating: " << TfStringify(exportArgs.isDuplicating) << std::endl;
    out << "includeAPINames (" << exportArgs.includeAPINames.size() << ")" << std::endl;
    for (const std::string& includeAPIName : exportArgs.includeAPINames) {
        out << "    " << includeAPIName << std::endl;
    }
    out << "jobContextNames (" << exportArgs.jobContextNames.size() << ")" << std::endl;
    for (const std::string& jobContextName : exportArgs.jobContextNames) {
        out << "    " << jobContextName << std::endl;
    }
    out << "materialCollectionsPath: " << exportArgs.materialCollectionsPath << std::endl
        << "materialsScopeName: " << exportArgs.materialsScopeName << std::endl
        << "mergeTransformAndShape: " << TfStringify(exportArgs.mergeTransformAndShape) << std::endl
        << "normalizeNurbs: " << TfStringify(exportArgs.normalizeNurbs) << std::endl
        << "preserveUVSetNames: " << TfStringify(exportArgs.preserveUVSetNames) << std::endl
        << "writeDefaults: " << TfStringify(exportArgs.writeDefaults) << std::endl
        << "parentScope: " << exportArgs.parentScope << std::endl // Deprecated
        << "rootPrim: " << exportArgs.parentScope << std::endl
        << "defaultPrim: " << TfStringify(exportArgs.defaultPrim) << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl
        << "animationType: " << exportArgs.animationType << std::endl
        << "disableModelKindProcessor: " << exportArgs.disableModelKindProcessor << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "allMaterialConversions: " << std::endl;
    for (const auto& conv : exportArgs.allMaterialConversions) {
        out << "    " << conv << std::endl;
    }

    out << "stripNamespaces: " << TfStringify(exportArgs.stripNamespaces) << std::endl
        << "worldspace: " << TfStringify(exportArgs.worldspace) << std::endl
        << "hideSourceData: " << TfStringify(exportArgs.hideSourceData) << std::endl
        << "timeSamples: " << exportArgs.timeSamples.size() << " sample(s)" << std::endl
        << "staticSingleSample: " << TfStringify(exportArgs.staticSingleSample) << std::endl
        << "geomSidedness: " << TfStringify(exportArgs.geomSidedness) << std::endl
        << "usdModelRootOverridePath: " << exportArgs.usdModelRootOverridePath << std::endl;

    out << "melPerFrameCallback: " << exportArgs.melPerFrameCallback << std::endl
        << "melPostCallback: " << exportArgs.melPostCallback << std::endl
        << "pythonPerFrameCallback: " << exportArgs.pythonPerFrameCallback << std::endl
        << "pythonPostCallback: " << exportArgs.pythonPostCallback << std::endl;

    out << "dagPaths (" << exportArgs.dagPaths.size() << ")" << std::endl;
    for (const MDagPath& dagPath : exportArgs.dagPaths) {
        out << "    " << dagPath.fullPathName().asChar() << std::endl;
    }

    out << "fullObjectList (" << exportArgs.fullObjectList.length() << ")" << std::endl;
    {
        MStringArray names;
        exportArgs.fullObjectList.getSelectionStrings(names);
        for (unsigned i = 0; i < names.length(); ++i) {
            out << "    " << names[i].asChar() << std::endl;
        }
    }

    out << "filteredTypeIds (" << exportArgs.filteredTypeIds.size() << ")" << std::endl;
    for (unsigned int id : exportArgs.filteredTypeIds) {
        out << "    " << id << ": " << MNodeClass(MTypeId(id)).typeName() << std::endl;
    }

    out << "chaserNames (" << exportArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : exportArgs.chaserNames) {
        out << "    " << chaserName << std::endl;
    }

    out << "remapUVSetsTo (" << exportArgs.remapUVSetsTo.size() << ")" << std::endl;
    for (const auto& remapIt : exportArgs.remapUVSetsTo) {
        out << "    " << remapIt.first << " -> " << remapIt.second << std::endl;
    }

    out << "allChaserArgs (" << exportArgs.allChaserArgs.size() << ")" << std::endl;
    for (const auto& chaserIter : exportArgs.allChaserArgs) {
        // Chaser name.
        out << "    " << chaserIter.first << std::endl;

        for (const auto& argIter : chaserIter.second) {
            out << "        Arg Name: " << argIter.first << ", Value: " << argIter.second
                << std::endl;
        }
    }

    out << "exportRootMapFunction (" << exportArgs.rootMapFunction.GetString() << ")" << std::endl;

    return out;
}

/* static */
UsdMayaJobExportArgs UsdMayaJobExportArgs::CreateFromDictionary(
    const VtDictionary&             userArgs,
    const UsdMayaUtil::MDagPathSet& dagPaths,
    const MSelectionList&           fullList,
    const std::vector<double>&      timeSamples)
{
    const bool         isExport = true;
    const VtDictionary allArgs = _MergeAllArguments(isExport, userArgs, GetDefaultDictionary());

    return UsdMayaJobExportArgs(allArgs, dagPaths, fullList, timeSamples);
}

/* static */
MStatus UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
    const MString& optionsString,
    VtDictionary*  toFill)
{
    if (!toFill)
        return MS::kFailure;

    VtDictionary& userArgs = *toFill;

    // Get the options
    if (optionsString.length() > 0) {
        MStringArray optionList;
        std::string  argName;
        MString      argValue;
        optionsString.split(';', optionList);
        for (unsigned int i = 0; i < optionList.length(); ++i) {
            if (!_GetEncodedArg(optionList[i], argName, argValue))
                continue;

            // We allow an empty string to be passed to exportRoots. We must process it here.
            if (argName == UsdMayaJobExportArgsTokens->exportRoots.GetText()) {
                if (argValue.length() == 0) {
                    std::vector<VtValue> userArgVals;
                    userArgVals.push_back(VtValue(""));
                    userArgs[UsdMayaJobExportArgsTokens->exportRoots] = userArgVals;
                    continue;
                }
            }

            if (argName == "filterTypes") {
                std::vector<VtValue> userArgVals;
                MStringArray         filteredTypes;
                argValue.split(',', filteredTypes);
                unsigned int nbTypes = filteredTypes.length();
                for (unsigned int idxType = 0; idxType < nbTypes; ++idxType) {
                    const std::string filteredType = filteredTypes[idxType].asChar();
                    userArgVals.emplace_back(filteredType);
                }
                userArgs[UsdMayaJobExportArgsTokens->filterTypes] = userArgVals;
            } else if (argName == "frameSample") {
                std::vector<double> samples;
                MStringArray        samplesStrings;
                argValue.split(' ', samplesStrings);
                unsigned int nbSams = samplesStrings.length();
                for (unsigned int sam = 0; sam < nbSams; ++sam) {
                    if (samplesStrings[sam].isDouble()) {
                        const double value = samplesStrings[sam].asDouble();
                        samples.emplace_back(value);
                    }
                }
                userArgs[argName] = samples;
            } else if (argName == UsdMayaJobExportArgsTokens->exportRoots.GetText()) {
                MStringArray exportRootStrings;
                argValue.split(',', exportRootStrings);

                std::vector<VtValue> userArgVals;

                unsigned int nbRoots = exportRootStrings.length();
                for (unsigned int idxRoot = 0; idxRoot < nbRoots; ++idxRoot) {
                    const std::string exportRootPath = exportRootStrings[idxRoot].asChar();

                    if (!exportRootPath.empty()) {
                        MDagPath rootDagPath = UsdMayaUtil::nameToDagPath(exportRootPath);
                        if (!rootDagPath.isValid()) {
                            MGlobal::displayError(
                                MString("Invalid dag path provided for export root: ")
                                + exportRootStrings[idxRoot]);
                            return MS::kFailure;
                        }
                        userArgVals.push_back(VtValue(exportRootPath));
                    } else {
                        userArgVals.push_back(VtValue(""));
                    }
                }
                userArgs[argName] = userArgVals;
            } else {
                if (argName == "shadingMode") {
                    TfToken shadingMode(argValue.asChar());
                    if (!shadingMode.IsEmpty()
                        && UsdMayaShadingModeRegistry::GetInstance().GetExporter(shadingMode)
                            == nullptr
                        && shadingMode != UsdMayaShadingModeTokens->none) {

                        MGlobal::displayError(
                            TfStringPrintf("No shadingMode '%s' found.", shadingMode.GetText())
                                .c_str());
                        return MS::kFailure;
                    }
                }

                // Note: when round-tripping settings, some extra settings are not part
                //       of the guiding dictionary. Ignore them.
                const static bool reportError = false;
                userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                    argName,
                    argValue.asChar(),
                    UsdMayaJobExportArgs::GetGuideDictionary(),
                    reportError);
            }
        }
    }

    return MS::kSuccess;
}

/* static */
void UsdMayaJobExportArgs::GetDictionaryTimeSamples(
    const VtDictionary&  userArgs,
    std::vector<double>& timeSamples)
{
    const bool   exportAnimation = extractBoolean(userArgs, UsdMayaJobExportArgsTokens->animation);
    const double startTime = extractDouble(userArgs, UsdMayaJobExportArgsTokens->startTime, 1.0);
    const double endTime = extractDouble(userArgs, UsdMayaJobExportArgsTokens->endTime, 1.0);
    const double frameStride
        = extractDouble(userArgs, UsdMayaJobExportArgsTokens->frameStride, 1.0);
    const std::vector<double> samples
        = extractVector<double>(userArgs, UsdMayaJobExportArgsTokens->frameSample);

    std::set<double> frameSamples(samples.begin(), samples.end());

    GfInterval timeInterval(1.0, 1.0);
    timeInterval.SetMin(startTime);
    timeInterval.SetMax(endTime);

    // Now resync start and end frame based on export time interval.
    if (exportAnimation) {
        if (timeInterval.IsEmpty()) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            timeInterval = GfInterval(timeInterval.GetMin());
        }
    } else {
        // No animation, so empty interval.
        timeInterval = GfInterval();
    }

    timeSamples = UsdMayaWriteUtil::GetTimeSamples(timeInterval, frameSamples, frameStride);
}

/* static */
const VtDictionary& UsdMayaJobExportArgs::GetDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Base defaults.
        d[UsdMayaJobExportArgsTokens->animation] = false;
        d[UsdMayaJobExportArgsTokens->animationType]
            = UsdMayaJobExportArgsTokens->timesamples.GetString();
        d[UsdMayaJobExportArgsTokens->startTime] = 1.0;
        d[UsdMayaJobExportArgsTokens->endTime] = 1.0;
        d[UsdMayaJobExportArgsTokens->frameStride] = 1.0;
        d[UsdMayaJobExportArgsTokens->frameSample] = std::vector<double>();
        d[UsdMayaJobExportArgsTokens->chaser] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->chaserArgs] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->remapUVSetsTo] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->compatibility] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->defaultCameras] = false;
        d[UsdMayaJobExportArgsTokens->defaultMeshScheme] = UsdGeomTokens->catmullClark.GetString();
#if PXR_VERSION < 2508
        d[UsdMayaJobExportArgsTokens->defaultUSDFormat] = UsdUsdcFileFormatTokens->Id.GetString();
#else
        d[UsdMayaJobExportArgsTokens->defaultUSDFormat] = SdfUsdcFileFormatTokens->Id.GetString();
#endif
        d[UsdMayaJobExportArgsTokens->eulerFilter] = false;
        d[UsdMayaJobExportArgsTokens->exportCollectionBasedBindings] = false;
        d[UsdMayaJobExportArgsTokens->exportColorSets] = true;
        d[UsdMayaJobExportArgsTokens->exportMaterials] = true;
        d[UsdMayaJobExportArgsTokens->exportAssignedMaterials] = true;
        d[UsdMayaJobExportArgsTokens->legacyMaterialScope] = false;
        d[UsdMayaJobExportArgsTokens->exportDisplayColor] = false;
        d[UsdMayaJobExportArgsTokens->exportDistanceUnit] = false;
        d[UsdMayaJobExportArgsTokens->exportInstances] = true;
        d[UsdMayaJobExportArgsTokens->exportMaterialCollections] = false;
        d[UsdMayaJobExportArgsTokens->referenceObjectMode]
            = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportRefsAsInstanceable] = false;
        d[UsdMayaJobExportArgsTokens->exportRoots] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->exportSelected] = false;
        d[UsdMayaJobExportArgsTokens->exportSkin] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportSkels] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportBlendShapes] = false;
        d[UsdMayaJobExportArgsTokens->exportUVs] = true;
        d[UsdMayaJobExportArgsTokens->exportRelativeTextures]
            = UsdMayaJobExportArgsTokens->automatic.GetString();
        d[UsdMayaJobExportArgsTokens->exportVisibility] = true;
        d[UsdMayaJobExportArgsTokens->exportComponentTags] = true;
        d[UsdMayaJobExportArgsTokens->exportStagesAsRefs] = true;
        d[UsdMayaJobExportArgsTokens->file] = std::string();
        d[UsdMayaJobExportArgsTokens->filterTypes] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->ignoreWarnings] = false;
        d[UsdMayaJobExportArgsTokens->includeEmptyTransforms] = true;
        d[UsdMayaJobExportArgsTokens->isDuplicating] = false;
        d[UsdMayaJobExportArgsTokens->kind] = std::string();
        d[UsdMayaJobExportArgsTokens->disableModelKindProcessor] = false;
        d[UsdMayaJobExportArgsTokens->materialCollectionsPath] = std::string();
        d[UsdMayaJobExportArgsTokens->materialsScopeName] = kDefaultMaterialScopeName;
        d[UsdMayaJobExportArgsTokens->melPerFrameCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->melPostCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->mergeTransformAndShape] = true;
        d[UsdMayaJobExportArgsTokens->normalizeNurbs] = false;
        d[UsdMayaJobExportArgsTokens->preserveUVSetNames] = false;
        d[UsdMayaJobExportArgsTokens->writeDefaults] = false;
        d[UsdMayaJobExportArgsTokens->parentScope] = std::string(); // Deprecated
        d[UsdMayaJobExportArgsTokens->rootPrim] = std::string();
        d[UsdMayaJobExportArgsTokens->rootPrimType] = UsdMayaJobExportArgsTokens->scope.GetString();
        d[UsdMayaJobExportArgsTokens->upAxis] = UsdMayaJobExportArgsTokens->mayaPrefs.GetString();
        d[UsdMayaJobExportArgsTokens->unit] = UsdMayaJobExportArgsTokens->mayaPrefs.GetString();
        d[UsdMayaJobExportArgsTokens->pythonPerFrameCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->pythonPostCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->renderableOnly] = false;
        d[UsdMayaJobExportArgsTokens->renderLayerMode]
            = UsdMayaJobExportArgsTokens->defaultLayer.GetString();
        d[UsdMayaJobExportArgsTokens->shadingMode]
            = UsdMayaShadingModeTokens->useRegistry.GetString();
        // The default convertMaterialsTo string matches shadingTokens.h:
        // TrMtlxTokens->conversionName
        d[UsdMayaJobExportArgsTokens->convertMaterialsTo]
            = std::vector<VtValue> { VtValue(UsdMayaTranslatorTokens->materialX.GetText()) };
        d[UsdMayaJobExportArgsTokens->apiSchema] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->jobContext] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->stripNamespaces] = false;
        d[UsdMayaJobExportArgsTokens->hideSourceData] = false;
        d[UsdMayaJobExportArgsTokens->worldspace] = false;
        d[UsdMayaJobExportArgsTokens->verbose] = false;
        d[UsdMayaJobExportArgsTokens->staticSingleSample] = false;
        d[UsdMayaJobExportArgsTokens->geomSidedness]
            = UsdMayaJobExportArgsTokens->derived.GetString();
        d[UsdMayaJobExportArgsTokens->customLayerData] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->metersPerUnit] = 0.0;
        d[UsdMayaJobExportArgsTokens->excludeExportTypes] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->defaultPrim] = std::string();

        // plugInfo.json site defaults.
        // The defaults dict should be correctly-typed, so enable
        // coerceToWeakerOpinionType.
        const VtDictionary site
            = UsdMaya_RegistryHelper::GetComposedInfoDictionary(_usdExportInfoScope->allTokens);
        VtDictionaryOver(site, &d, /*coerceToWeakerOpinionType*/ true);
    });

    return d;
}

const VtDictionary& UsdMayaJobExportArgs::GetGuideDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;

    std::call_once(once, []() {
        // Common types:
        const auto _boolean = VtValue(false);
        const auto _double = VtValue(0.0);
        const auto _string = VtValue(std::string());
        const auto _doubleVector = VtValue(std::vector<double>());
        const auto _stringVector = VtValue(std::vector<VtValue>({ _string }));
        const auto _stringPair = VtValue(std::vector<VtValue>({ _string, _string }));
        const auto _stringPairVector = VtValue(std::vector<VtValue>({ _stringPair }));
        const auto _stringTriplet = VtValue(std::vector<VtValue>({ _string, _string, _string }));
        const auto _stringTripletVector = VtValue(std::vector<VtValue>({ _stringTriplet }));

        // Provide guide types for the parser:
        d[UsdMayaJobExportArgsTokens->animation] = _boolean;
        d[UsdMayaJobExportArgsTokens->animationType] = _string;
        d[UsdMayaJobExportArgsTokens->startTime] = _double;
        d[UsdMayaJobExportArgsTokens->endTime] = _double;
        d[UsdMayaJobExportArgsTokens->frameStride] = _double;
        d[UsdMayaJobExportArgsTokens->frameSample] = _doubleVector;
        d[UsdMayaJobExportArgsTokens->chaser] = _stringVector;
        d[UsdMayaJobExportArgsTokens->chaserArgs] = _stringTripletVector;
        d[UsdMayaJobExportArgsTokens->remapUVSetsTo] = _stringPairVector;
        d[UsdMayaJobExportArgsTokens->customLayerData] = _stringTripletVector;
        d[UsdMayaJobExportArgsTokens->metersPerUnit] = _double;
        d[UsdMayaJobExportArgsTokens->compatibility] = _string;
        d[UsdMayaJobExportArgsTokens->defaultCameras] = _boolean;
        d[UsdMayaJobExportArgsTokens->defaultMeshScheme] = _string;
        d[UsdMayaJobExportArgsTokens->defaultUSDFormat] = _string;
        d[UsdMayaJobExportArgsTokens->eulerFilter] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportCollectionBasedBindings] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportColorSets] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportMaterials] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportAssignedMaterials] = _boolean;
        d[UsdMayaJobExportArgsTokens->legacyMaterialScope] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportDisplayColor] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportDistanceUnit] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportInstances] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportMaterialCollections] = _boolean;
        d[UsdMayaJobExportArgsTokens->referenceObjectMode] = _string;
        d[UsdMayaJobExportArgsTokens->exportRefsAsInstanceable] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportRoots] = _stringVector;
        d[UsdMayaJobExportArgsTokens->exportSkin] = _string;
        d[UsdMayaJobExportArgsTokens->exportSelected] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportSkels] = _string;
        d[UsdMayaJobExportArgsTokens->exportBlendShapes] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportUVs] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportRelativeTextures] = _string;
        d[UsdMayaJobExportArgsTokens->exportVisibility] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportComponentTags] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportStagesAsRefs] = _boolean;
        d[UsdMayaJobExportArgsTokens->file] = _string;
        d[UsdMayaJobExportArgsTokens->filterTypes] = _stringVector;
        d[UsdMayaJobExportArgsTokens->ignoreWarnings] = _boolean;
        d[UsdMayaJobExportArgsTokens->includeEmptyTransforms] = _boolean;
        d[UsdMayaJobExportArgsTokens->isDuplicating] = _boolean;
        d[UsdMayaJobExportArgsTokens->kind] = _string;
        d[UsdMayaJobExportArgsTokens->disableModelKindProcessor] = _boolean;
        d[UsdMayaJobExportArgsTokens->materialCollectionsPath] = _string;
        d[UsdMayaJobExportArgsTokens->materialsScopeName] = _string;
        d[UsdMayaJobExportArgsTokens->melPerFrameCallback] = _string;
        d[UsdMayaJobExportArgsTokens->melPostCallback] = _string;
        d[UsdMayaJobExportArgsTokens->mergeTransformAndShape] = _boolean;
        d[UsdMayaJobExportArgsTokens->normalizeNurbs] = _boolean;
        d[UsdMayaJobExportArgsTokens->preserveUVSetNames] = _boolean;
        d[UsdMayaJobExportArgsTokens->writeDefaults] = _boolean;
        d[UsdMayaJobExportArgsTokens->parentScope] = _string; // Deprecated
        d[UsdMayaJobExportArgsTokens->rootPrim] = _string;
        d[UsdMayaJobExportArgsTokens->rootPrimType] = _string;
        d[UsdMayaJobExportArgsTokens->upAxis] = _string;
        d[UsdMayaJobExportArgsTokens->unit] = _string;
        d[UsdMayaJobExportArgsTokens->pythonPerFrameCallback] = _string;
        d[UsdMayaJobExportArgsTokens->pythonPostCallback] = _string;
        d[UsdMayaJobExportArgsTokens->renderableOnly] = _boolean;
        d[UsdMayaJobExportArgsTokens->renderLayerMode] = _string;
        d[UsdMayaJobExportArgsTokens->shadingMode] = _string;
        d[UsdMayaJobExportArgsTokens->convertMaterialsTo] = _stringVector;
        d[UsdMayaJobExportArgsTokens->apiSchema] = _stringVector;
        d[UsdMayaJobExportArgsTokens->jobContext] = _stringVector;
        d[UsdMayaJobExportArgsTokens->stripNamespaces] = _boolean;
        d[UsdMayaJobExportArgsTokens->hideSourceData] = _boolean;
        d[UsdMayaJobExportArgsTokens->worldspace] = _boolean;
        d[UsdMayaJobExportArgsTokens->verbose] = _boolean;
        d[UsdMayaJobExportArgsTokens->staticSingleSample] = _boolean;
        d[UsdMayaJobExportArgsTokens->geomSidedness] = _string;
        d[UsdMayaJobExportArgsTokens->excludeExportTypes] = _stringVector;
        d[UsdMayaJobExportArgsTokens->defaultPrim] = _string;
    });

    return d;
}

const std::string UsdMayaJobExportArgs::GetDefaultMaterialsScopeName()
{
    return _GetMaterialsScopeName(kDefaultMaterialScopeName);
}

std::string UsdMayaJobExportArgs::GetResolvedFileName() const
{
    MFileObject fileObj;
    fileObj.setRawFullName(file.c_str());

    // Make sure it's an absolute path
    fileObj.setRawFullName(fileObj.resolvedFullName());
    const std::string resolvedFileName = fileObj.resolvedFullName().asChar();

    if (!resolvedFileName.empty()) {
        return resolvedFileName;
    }

    return file;
}

static bool isExcluded(const UsdMayaJobExportArgs& args, const TfToken* tokens, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (args.excludeExportTypes.count(tokens[i]) > 0)
            return true;
    return false;
}

bool UsdMayaJobExportArgs::isExportingMeshes() const
{
    static const TfToken meshTokens[]
        = { TfToken("Meshes"), TfToken("meshes"), TfToken("Mesh"), TfToken("mesh") };
    return !isExcluded(*this, meshTokens, sizeof(meshTokens) / sizeof(meshTokens[0]));
}

bool UsdMayaJobExportArgs::isExportingCameras() const
{
    static const TfToken meshTokens[]
        = { TfToken("Cameras"), TfToken("cameras"), TfToken("Camera"), TfToken("camera") };
    return !isExcluded(*this, meshTokens, sizeof(meshTokens) / sizeof(meshTokens[0]));
}

bool UsdMayaJobExportArgs::isExportingLights() const
{
    static const TfToken meshTokens[]
        = { TfToken("Lights"), TfToken("lights"), TfToken("Light"), TfToken("light") };
    return !isExcluded(*this, meshTokens, sizeof(meshTokens) / sizeof(meshTokens[0]));
}

UsdMayaJobImportArgs::UsdMayaJobImportArgs(
    const VtDictionary& userArgs,
    const bool          importWithProxyShapes,
    const GfInterval&   timeInterval)
    : assemblyRep(extractToken(
        userArgs,
        UsdMayaJobImportArgsTokens->assemblyRep,
        UsdMayaJobImportArgsTokens->Collapsed,
        { UsdMayaJobImportArgsTokens->Full,
          UsdMayaJobImportArgsTokens->Import,
          UsdMayaJobImportArgsTokens->Unloaded }))
    , excludePrimvarNames(extractTokenSet(userArgs, UsdMayaJobImportArgsTokens->excludePrimvar))
    , excludePrimvarNamespaces(
          extractTokenSet(userArgs, UsdMayaJobImportArgsTokens->excludePrimvarNamespace))
    , includeAPINames(extractTokenSet(userArgs, UsdMayaJobImportArgsTokens->apiSchema))
    , jobContextNames(extractTokenSet(userArgs, UsdMayaJobImportArgsTokens->jobContext))
    , includeMetadataKeys(extractTokenSet(userArgs, UsdMayaJobImportArgsTokens->metadata))
    , shadingModes(_shadingModesImportArgs(userArgs, UsdMayaJobImportArgsTokens->shadingMode))
    , preferredMaterial(extractToken(
          userArgs,
          UsdMayaJobImportArgsTokens->preferredMaterial,
          UsdMayaPreferredMaterialTokens->none,
          UsdMayaPreferredMaterialTokens->allTokens))
    , importUSDZTexturesFilePath(UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(userArgs))
    , importUSDZTextures(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->importUSDZTextures))
    , importRelativeTextures(extractToken(
          userArgs,
          UsdMayaJobImportArgsTokens->importRelativeTextures,
          UsdMayaJobImportArgsTokens->none,
          { UsdMayaJobImportArgsTokens->automatic,
            UsdMayaJobImportArgsTokens->absolute,
            UsdMayaJobImportArgsTokens->relative,
            UsdMayaJobImportArgsTokens->none }))
    , axisAndUnitMethod(extractToken(
          userArgs,
          UsdMayaJobImportArgsTokens->axisAndUnitMethod,
          UsdMayaJobImportArgsTokens->rotateScale,
          { UsdMayaJobImportArgsTokens->rotateScale,
            UsdMayaJobImportArgsTokens->addTransform,
            UsdMayaJobImportArgsTokens->overwritePrefs }))
    , upAxis(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->upAxis))
    , unit(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->unit))
    , importInstances(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->importInstances))
    , useAsAnimationCache(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->useAsAnimationCache))
    , importWithProxyShapes(importWithProxyShapes)
    , preserveTimeline(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->preserveTimeline))
    , applyEulerFilter(extractBoolean(userArgs, UsdMayaJobImportArgsTokens->applyEulerFilter))
    , pullImportStage(extractUsdStageRefPtr(userArgs, UsdMayaJobImportArgsTokens->pullImportStage))
    , timeInterval(timeInterval)
    , chaserNames(extractVector<std::string>(userArgs, UsdMayaJobImportArgsTokens->chaser))
    , allChaserArgs(_ChaserArgs(userArgs, UsdMayaJobImportArgsTokens->chaserArgs))
    , remapUVSetsTo(_UVSetRemaps(userArgs, UsdMayaJobImportArgsTokens->remapUVSetsTo))
{
}

TfToken UsdMayaJobImportArgs::GetMaterialConversion() const
{
    return shadingModes.empty() ? TfToken() : shadingModes.front().materialConversion;
}

/* static */
UsdMayaJobImportArgs UsdMayaJobImportArgs::CreateFromDictionary(
    const VtDictionary& userArgs,
    const bool          importWithProxyShapes,
    const GfInterval&   timeInterval)
{
    const bool         isExport = false;
    const VtDictionary allArgs = _MergeAllArguments(isExport, userArgs, GetDefaultDictionary());

    return UsdMayaJobImportArgs(allArgs, importWithProxyShapes, timeInterval);
}

/* static */
MStatus UsdMayaJobImportArgs::GetDictionaryFromEncodedOptions(
    const MString& optionsString,
    VtDictionary*  toFill)
{
    if (!toFill)
        return MS::kFailure;

    VtDictionary& userArgs = *toFill;

    // Get the options
    if (optionsString.length() > 0) {
        MStringArray optionList;
        std::string  argName;
        MString      argValue;
        optionsString.split(';', optionList);
        for (unsigned int i = 0; i < optionList.length(); ++i) {
            if (!_GetEncodedArg(optionList[i], argName, argValue))
                continue;

            // Note: when round-tripping settings, some extra settings are not part
            //       of the guiding dictionary. Ignore them.
            const static bool reportError = false;
            userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                argName,
                argValue.asChar(),
                UsdMayaJobImportArgs::GetGuideDictionary(),
                reportError);
        }
    }

    return MS::kSuccess;
}

/* static */
const VtDictionary& UsdMayaJobImportArgs::GetDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Base defaults.
        d[UsdMayaJobImportArgsTokens->assemblyRep]
            = UsdMayaJobImportArgsTokens->Collapsed.GetString();
        d[UsdMayaJobImportArgsTokens->apiSchema] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->excludePrimvar] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->excludePrimvarNamespace] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->jobContext] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->metadata]
            = std::vector<VtValue>({ VtValue(SdfFieldKeys->Hidden.GetString()),
                                     VtValue(SdfFieldKeys->Instanceable.GetString()),
                                     VtValue(SdfFieldKeys->Kind.GetString()) });
        d[UsdMayaJobImportArgsTokens->preferredMaterial]
            = UsdMayaPreferredMaterialTokens->none.GetString();
        d[UsdMayaJobImportArgsTokens->importInstances] = true;
        d[UsdMayaJobImportArgsTokens->importUSDZTextures] = false;
        d[UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath] = "";
        d[UsdMayaJobImportArgsTokens->importRelativeTextures]
            = UsdMayaJobImportArgsTokens->none.GetString();
        d[UsdMayaJobImportArgsTokens->axisAndUnitMethod]
            = UsdMayaJobImportArgsTokens->rotateScale.GetString();
        d[UsdMayaJobImportArgsTokens->upAxis] = true;
        d[UsdMayaJobImportArgsTokens->unit] = true;
        d[UsdMayaJobImportArgsTokens->pullImportStage] = UsdStageRefPtr();
        d[UsdMayaJobImportArgsTokens->useAsAnimationCache] = false;
        d[UsdMayaJobImportArgsTokens->preserveTimeline] = false;
        d[UsdMayaJobImportArgsTokens->chaser] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->chaserArgs] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->remapUVSetsTo] = std::vector<VtValue>();
        d[UsdMayaJobImportArgsTokens->applyEulerFilter] = false;

        // plugInfo.json site defaults.
        // The defaults dict should be correctly-typed, so enable
        // coerceToWeakerOpinionType.
        const VtDictionary site
            = UsdMaya_RegistryHelper::GetComposedInfoDictionary(_usdImportInfoScope->allTokens);
        VtDictionaryOver(site, &d, /*coerceToWeakerOpinionType*/ true);
    });

    // Shading options default value is variable and depends on loaded plugins.
    // Default priorities for searching for materials, as found in
    //  lib\mayaUsd\commands\baseListShadingModesCommand.cpp:
    //    - Specialized importers using registry based import.
    //    - Specialized importers, non-registry based.
    //    - UsdPreviewSurface importer.
    //    - Display colors as last resort
    std::vector<VtValue> shadingModes;
    for (const auto& c : UsdMayaShadingModeRegistry::ListMaterialConversions()) {
        if (c != UsdImagingTokens->UsdPreviewSurface) {
            auto const& info = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(c);
            if (info.hasImporter) {
                shadingModes.emplace_back(std::vector<VtValue> {
                    VtValue(UsdMayaShadingModeTokens->useRegistry.GetString()),
                    VtValue(c.GetString()) });
            }
        }
    }
    for (const auto& s : UsdMayaShadingModeRegistry::ListImporters()) {
        if (s != UsdMayaShadingModeTokens->useRegistry
            && s != UsdMayaShadingModeTokens->displayColor) {
            shadingModes.emplace_back(std::vector<VtValue> {
                VtValue(s.GetString()), VtValue(UsdMayaShadingModeTokens->none.GetString()) });
        }
    }
    shadingModes.emplace_back(
        std::vector<VtValue> { VtValue(UsdMayaShadingModeTokens->useRegistry.GetString()),
                               VtValue(UsdImagingTokens->UsdPreviewSurface.GetString()) });
    shadingModes.emplace_back(
        std::vector<VtValue> { VtValue(UsdMayaShadingModeTokens->displayColor.GetString()),
                               VtValue(UsdMayaShadingModeTokens->none.GetString()) });

    d[UsdMayaJobImportArgsTokens->shadingMode] = { VtValue(shadingModes) };

    return d;
}

/* static */
const VtDictionary& UsdMayaJobImportArgs::GetGuideDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Common types:
        const auto _boolean = VtValue(false);
        const auto _usdStageRefPtr = VtValue(nullptr);
        const auto _string = VtValue(std::string());
        const auto _stringVector = VtValue(std::vector<VtValue>({ _string }));
        const auto _stringPair = VtValue(std::vector<VtValue>({ _string, _string }));
        const auto _stringPairVector = VtValue(std::vector<VtValue>({ _stringPair }));
        const auto _stringTriplet = VtValue(std::vector<VtValue>({ _string, _string, _string }));
        const auto _stringTripletVector = VtValue(std::vector<VtValue>({ _stringTriplet }));

        // Provide guide types for the parser:
        d[UsdMayaJobImportArgsTokens->assemblyRep] = _string;
        d[UsdMayaJobImportArgsTokens->apiSchema] = _stringVector;
        d[UsdMayaJobImportArgsTokens->excludePrimvar] = _stringVector;
        d[UsdMayaJobImportArgsTokens->excludePrimvarNamespace] = _stringVector;
        d[UsdMayaJobImportArgsTokens->jobContext] = _stringVector;
        d[UsdMayaJobImportArgsTokens->metadata] = _stringVector;
        d[UsdMayaJobImportArgsTokens->shadingMode] = _stringTripletVector;
        d[UsdMayaJobImportArgsTokens->preferredMaterial] = _string;
        d[UsdMayaJobImportArgsTokens->importInstances] = _boolean;
        d[UsdMayaJobImportArgsTokens->importUSDZTextures] = _boolean;
        d[UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath] = _string;
        d[UsdMayaJobImportArgsTokens->importRelativeTextures] = _string;
        d[UsdMayaJobImportArgsTokens->axisAndUnitMethod] = _string;
        d[UsdMayaJobImportArgsTokens->upAxis] = _boolean;
        d[UsdMayaJobImportArgsTokens->unit] = _boolean;
        d[UsdMayaJobImportArgsTokens->pullImportStage] = _usdStageRefPtr;
        d[UsdMayaJobImportArgsTokens->useAsAnimationCache] = _boolean;
        d[UsdMayaJobImportArgsTokens->preserveTimeline] = _boolean;
        d[UsdMayaJobImportArgsTokens->chaser] = _stringVector;
        d[UsdMayaJobImportArgsTokens->chaserArgs] = _stringTripletVector;
        d[UsdMayaJobImportArgsTokens->remapUVSetsTo] = _stringPairVector;
        d[UsdMayaJobImportArgsTokens->applyEulerFilter] = _boolean;
    });

    return d;
}

const std::string UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(const VtDictionary& userArgs)
{
    if (!extractBoolean(userArgs, UsdMayaJobImportArgsTokens->importUSDZTextures))
        return ""; // Not importing textures. File path stays empty.

    const std::string pathArg
        = extractString(userArgs, UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath);
    std::string importTexturesRootDirPath;
    if (pathArg.size() == 0) { // NOTE: (yliangsiew) If the user gives an empty argument, we'll try
                               // to determine the best directory to write to instead.
        MString currentMayaWorkspacePath = UsdMayaUtil::GetCurrentMayaWorkspacePath();
        MString currentMayaSceneFilePath = UsdMayaUtil::GetCurrentSceneFilePath();
        if (currentMayaSceneFilePath.length() != 0
            && strstr(currentMayaSceneFilePath.asChar(), currentMayaWorkspacePath.asChar())
                == NULL) {
            TF_RUNTIME_ERROR(
                "The current scene does not seem to be part of the current Maya project set. "
                "Could not automatically determine a path to write out USDZ texture imports.");
            return "";
        }
        if (currentMayaWorkspacePath.length() == 0
            || !ghc::filesystem::is_directory(currentMayaWorkspacePath.asChar())) {
            TF_RUNTIME_ERROR(
                "Could not automatically determine a path to write out USDZ texture imports. "
                "Please specify a location using the -importUSDZTexturesFilePath argument, or "
                "set the Maya project appropriately.");
            return "";
        } else {
            // NOTE: (yliangsiew) Textures are, by convention, supposed to be located in the
            // `sourceimages` folder under a Maya project root folder.
            importTexturesRootDirPath.assign(
                currentMayaWorkspacePath.asChar(), currentMayaWorkspacePath.length());
            MString sourceImagesDirBaseName
                = MGlobal::executeCommandStringResult("workspace -fre \"sourceImages\"");
            if (sourceImagesDirBaseName.length() == 0) {
                TF_RUNTIME_ERROR(
                    "Unable to determine the sourceImages fileRule for the Maya project: %s.",
                    currentMayaWorkspacePath.asChar());
                return "";
            }
            bool bStat = UsdMayaUtilFileSystem::pathAppendPath(
                importTexturesRootDirPath, sourceImagesDirBaseName.asChar());
            if (!bStat) {
                TF_RUNTIME_ERROR(
                    "Unable to determine the texture directory for the Maya project: %s.",
                    currentMayaWorkspacePath.asChar());
                return "";
            }
            // Make sure the sourceimage folder is created in the project:
            TfMakeDirs(importTexturesRootDirPath);
        }
    } else {
        importTexturesRootDirPath.assign(pathArg);
    }

    if (!ghc::filesystem::is_directory(importTexturesRootDirPath)) {
        TF_RUNTIME_ERROR(
            "The directory specified for USDZ texture imports: %s is not valid.",
            importTexturesRootDirPath.c_str());
        return "";
    }

    return importTexturesRootDirPath;
}

std::ostream& operator<<(std::ostream& out, const UsdMayaJobImportArgs& importArgs)
{
    out << "shadingModes (" << importArgs.shadingModes.size() << ")" << std::endl;
    for (const auto& shadingMode : importArgs.shadingModes) {
        out << "    " << TfStringify(shadingMode.mode) << ", "
            << TfStringify(shadingMode.materialConversion) << std::endl;
    }
    out << "preferredMaterial: " << importArgs.preferredMaterial << std::endl
        << "assemblyRep: " << importArgs.assemblyRep << std::endl
        << "importInstances: " << TfStringify(importArgs.importInstances) << std::endl
        << "importUSDZTextures: " << TfStringify(importArgs.importUSDZTextures) << std::endl
        << "importUSDZTexturesFilePath: " << TfStringify(importArgs.importUSDZTexturesFilePath)
        << "importRelativeTextures: " << TfStringify(importArgs.importRelativeTextures) << std::endl
        << "axisAndUnitMethod: " << TfStringify(importArgs.axisAndUnitMethod) << std::endl
        << "upAxis: " << TfStringify(importArgs.upAxis) << std::endl
        << "unit: " << TfStringify(importArgs.unit) << std::endl
        << "pullImportStage: " << TfStringify(importArgs.pullImportStage) << std::endl
        << std::endl
        << "timeInterval: " << importArgs.timeInterval << std::endl
        << "useAsAnimationCache: " << TfStringify(importArgs.useAsAnimationCache) << std::endl
        << "preserveTimeline: " << TfStringify(importArgs.preserveTimeline) << std::endl
        << "importWithProxyShapes: " << TfStringify(importArgs.importWithProxyShapes) << std::endl
        << "applyEulerFilter: " << TfStringify(importArgs.applyEulerFilter) << std::endl;

    out << "jobContextNames (" << importArgs.jobContextNames.size() << ")" << std::endl;
    for (const std::string& jobContextName : importArgs.jobContextNames) {
        out << "    " << jobContextName << std::endl;
    }

    out << "chaserNames (" << importArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : importArgs.chaserNames) {
        out << "    " << chaserName << std::endl;
    }

    out << "allChaserArgs (" << importArgs.allChaserArgs.size() << ")" << std::endl;
    for (const auto& chaserIter : importArgs.allChaserArgs) {
        // Chaser name.
        out << "    " << chaserIter.first << std::endl;

        for (const auto& argIter : chaserIter.second) {
            out << "        Arg Name: " << argIter.first << ", Value: " << argIter.second
                << std::endl;
        }
    }

    out << "remapUVSetsTo (" << importArgs.remapUVSetsTo.size() << ")" << std::endl;
    for (const auto& remapIt : importArgs.remapUVSetsTo) {
        out << "    " << remapIt.first << " -> " << remapIt.second << std::endl;
    }

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
