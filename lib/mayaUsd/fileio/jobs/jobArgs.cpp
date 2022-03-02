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
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MNodeClass.h>
#include <maya/MTypeId.h>

#include <ghc/filesystem.hpp>

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
/// Extracts a bool at \p key from \p userArgs, or false if it can't extract.
bool _Boolean(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<bool>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not bool type",
            key.GetText());
        return false;
    }
    return VtDictionaryGet<bool>(userArgs, key);
}

/// Extracts a string at \p key from \p userArgs, or "" if it can't extract.
std::string _String(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<std::string>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not string type",
            key.GetText());
        return std::string();
    }
    return VtDictionaryGet<std::string>(userArgs, key);
}

/// Extracts a token at \p key from \p userArgs.
/// If the token value is not either \p defaultToken or one of the
/// \p otherTokens, then returns \p defaultToken instead.
TfToken _Token(
    const VtDictionary&         userArgs,
    const TfToken&              key,
    const TfToken&              defaultToken,
    const std::vector<TfToken>& otherTokens)
{
    const TfToken tok(_String(userArgs, key));
    for (const TfToken& allowedTok : otherTokens) {
        if (tok == allowedTok) {
            return tok;
        }
    }

    // Empty token will silently be promoted to default value.
    // Warning for non-empty tokens that don't match.
    if (tok != defaultToken && !tok.IsEmpty()) {
        TF_WARN(
            "Value '%s' is not allowed for flag '%s'; using fallback '%s' "
            "instead",
            tok.GetText(),
            key.GetText(),
            defaultToken.GetText());
    }
    return defaultToken;
}

/// Extracts an absolute path at \p key from \p userArgs, or the empty path if
/// it can't extract.
SdfPath _AbsolutePath(const VtDictionary& userArgs, const TfToken& key)
{
    const std::string s = _String(userArgs, key);
    // Assume that empty strings are empty paths. (This might be an error case.)
    if (s.empty()) {
        return SdfPath();
    }
    // Make all relative paths into absolute paths.
    SdfPath path(s);
    if (path.IsAbsolutePath()) {
        return path;
    } else {
        return SdfPath::AbsoluteRootPath().AppendPath(path);
    }
}

/// Extracts an vector<T> from the vector<VtValue> at \p key in \p userArgs.
/// Returns an empty vector if it can't convert the entire value at \p key into
/// a vector<T>.
template <typename T> std::vector<T> _Vector(const VtDictionary& userArgs, const TfToken& key)
{
    // Check that vector exists.
    if (!VtDictionaryIsHolding<std::vector<VtValue>>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not vector type",
            key.GetText());
        return std::vector<T>();
    }

    // Check that vector is correctly-typed.
    std::vector<VtValue> vals = VtDictionaryGet<std::vector<VtValue>>(userArgs, key);
    if (!std::all_of(vals.begin(), vals.end(), [](const VtValue& v) { return v.IsHolding<T>(); })) {
        TF_CODING_ERROR(
            "Vector at dictionary key '%s' contains elements of "
            "the wrong type",
            key.GetText());
        return std::vector<T>();
    }

    // Extract values.
    std::vector<T> result;
    for (const VtValue& v : vals) {
        result.push_back(v.UncheckedGet<T>());
    }
    return result;
}

/// Convenience function that takes the result of _Vector and converts it to a
/// TfToken::Set.
TfToken::Set _TokenSet(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::string> vec = _Vector<std::string>(userArgs, key);
    TfToken::Set                   result;
    for (const std::string& s : vec) {
        result.insert(TfToken(s));
    }
    return result;
}

// The chaser args are stored as vectors of vectors (since this is how you
// would need to pass them in the Maya Python command API). Convert this to a
// map of maps.
std::map<std::string, UsdMayaJobExportArgs::ChaserArgs>
_ChaserArgs(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> chaserArgs
        = _Vector<std::vector<VtValue>>(userArgs, key);

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

// The shadingMode args are stored as vectors of vectors (since this is how you
// would need to pass them in the Maya Python command API).
UsdMayaJobImportArgs::ShadingModes
_shadingModesImportArgs(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> shadingModeArgs
        = _Vector<std::vector<VtValue>>(userArgs, key);

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

    auto addExportRootPathPairFn = [&pathMap, stripNamespaces](const MDagPath& rootDagPath) {
        if (!rootDagPath.isValid())
            return;

        SdfPath rootSdfPath = UsdMayaUtil::MDagPathToUsdPath(rootDagPath, false, stripNamespaces);

        if (rootSdfPath.IsEmpty())
            return;

        SdfPath newRootSdfPath
            = rootSdfPath.ReplacePrefix(rootSdfPath.GetParentPath(), SdfPath::AbsoluteRootPath());

        pathMap[rootSdfPath] = newRootSdfPath;
    };

    bool includeEntireSelection = false;

    const std::vector<std::string> exportRoots = _Vector<std::string>(userArgs, key);
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
        = _Vector<std::string>(userArgs, UsdMayaJobExportArgsTokens->filterTypes);
    std::set<unsigned int> result;
    for (const std::string& s : vec) {
        _AddFilteredTypeName(s.c_str(), result);
    }
    return result;
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
bool _MergeJobContexts(bool isExport, const VtDictionary& userArgs, VtDictionary& allContextArgs)
{
    // List of all argument dictionaries found while exploring jobContexts
    std::vector<VtDictionary> contextArgs;

    bool canMergeContexts = true;

    // This first loop gathers all job context argument dictionaries found in the userArgs
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

    // Convenience map holding the jobContext that first introduces an argument to the final
    // dictionary. Allows printing meaningful error messages.
    std::map<std::string, std::string> argInitialSource;

    // Traverse argument dictionaries and look for merge conflicts while building the returned
    // allContextArgs.
    for (auto const& dict : contextArgs) {
        // We made sure the value exists in the above loop, so we can fetch without fear:
        const std::string& sourceName = VtDictionaryGet<std::vector<VtValue>>(dict, jcKey)
                                            .front()
                                            .UncheckedGet<std::string>();
        for (auto const& dictTuple : dict) {
            const std::string& k = dictTuple.first;
            const VtValue&     v = dictTuple.second;

            auto allContextIt = allContextArgs.find(k);
            if (allContextIt == allContextArgs.end()) {
                // First time we see this argument. Store and remember source.
                allContextArgs[k] = v;
                argInitialSource[k] = sourceName;
            } else {
                // We have already seen this argument from another jobContext. Look for conflicts:
                const VtValue& allContextValue = allContextIt->second;

                if (allContextValue.IsHolding<std::vector<VtValue>>()) {
                    if (v.IsHolding<std::vector<VtValue>>()) {
                        // We merge arrays:
                        std::vector<VtValue> mergedValues
                            = allContextValue.UncheckedGet<std::vector<VtValue>>();
                        for (const VtValue& element : v.UncheckedGet<std::vector<VtValue>>()) {
                            if (element.IsHolding<std::vector<VtValue>>()) {
                                // vector<vector<string>> is common for chaserArgs and shadingModes
                                auto findElement = [&element](const VtValue& a) {
                                    return MayaUsdUtils::compareValues(element, a)
                                        == MayaUsdUtils::DiffResult::Same;
                                };
                                if (std::find_if(
                                        mergedValues.begin(), mergedValues.end(), findElement)
                                    == mergedValues.end()) {
                                    mergedValues.push_back(element);
                                }
                            } else {
                                if (std::find(mergedValues.begin(), mergedValues.end(), element)
                                    == mergedValues.end()) {
                                    mergedValues.push_back(element);
                                }
                            }
                        }
                        allContextArgs[k] = mergedValues;
                    } else {
                        // We have both an array and a scalar under the same argument name.
                        TF_RUNTIME_ERROR(TfStringPrintf(
                            "Context '%s' and context '%s' do not agree on type of argument '%s'.",
                            sourceName.c_str(),
                            argInitialSource[k].c_str(),
                            k.c_str()));
                        canMergeContexts = false;
                    }
                } else {
                    // Scalar value already exists. Check for value conflicts:
                    if (allContextValue != v) {
                        TF_RUNTIME_ERROR(TfStringPrintf(
                            "Context '%s' and context '%s' do not agree on argument '%s'.",
                            sourceName.c_str(),
                            argInitialSource[k].c_str(),
                            k.c_str()));
                        canMergeContexts = false;
                    }
                }
            }
        }
    }
    return canMergeContexts;
}

} // namespace
UsdMayaJobExportArgs::UsdMayaJobExportArgs(
    const VtDictionary&             userArgs,
    const UsdMayaUtil::MDagPathSet& dagPaths,
    const std::vector<double>&      timeSamples)
    : compatibility(_Token(
        userArgs,
        UsdMayaJobExportArgsTokens->compatibility,
        UsdMayaJobExportArgsTokens->none,
        { UsdMayaJobExportArgsTokens->appleArKit }))
    , defaultMeshScheme(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->defaultMeshScheme,
          UsdGeomTokens->catmullClark,
          { UsdGeomTokens->loop, UsdGeomTokens->bilinear, UsdGeomTokens->none }))
    , defaultUSDFormat(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->defaultUSDFormat,
          UsdUsdcFileFormatTokens->Id,
          { UsdUsdaFileFormatTokens->Id }))
    , eulerFilter(_Boolean(userArgs, UsdMayaJobExportArgsTokens->eulerFilter))
    , excludeInvisible(_Boolean(userArgs, UsdMayaJobExportArgsTokens->renderableOnly))
    , exportCollectionBasedBindings(
          _Boolean(userArgs, UsdMayaJobExportArgsTokens->exportCollectionBasedBindings))
    , exportColorSets(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportColorSets))
    , exportDefaultCameras(_Boolean(userArgs, UsdMayaJobExportArgsTokens->defaultCameras))
    , exportDisplayColor(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportDisplayColor))
    , exportInstances(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportInstances))
    , exportMaterialCollections(
          _Boolean(userArgs, UsdMayaJobExportArgsTokens->exportMaterialCollections))
    , exportMeshUVs(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportUVs))
    , exportNurbsExplicitUV(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportUVs))
    , referenceObjectMode(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->referenceObjectMode,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->attributeOnly, UsdMayaJobExportArgsTokens->defaultToMesh }))
    , exportRefsAsInstanceable(
          _Boolean(userArgs, UsdMayaJobExportArgsTokens->exportRefsAsInstanceable))
    , exportSkels(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->exportSkels,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->auto_, UsdMayaJobExportArgsTokens->explicit_ }))
    , exportSkin(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->exportSkin,
          UsdMayaJobExportArgsTokens->none,
          { UsdMayaJobExportArgsTokens->auto_, UsdMayaJobExportArgsTokens->explicit_ }))
    , exportBlendShapes(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportBlendShapes))
    , exportVisibility(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportVisibility))
    , exportComponentTags(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportComponentTags))
    , file(_String(userArgs, UsdMayaJobExportArgsTokens->file))
    , ignoreWarnings(_Boolean(userArgs, UsdMayaJobExportArgsTokens->ignoreWarnings))
    , materialCollectionsPath(
          _AbsolutePath(userArgs, UsdMayaJobExportArgsTokens->materialCollectionsPath))
    , materialsScopeName(
          _GetMaterialsScopeName(_String(userArgs, UsdMayaJobExportArgsTokens->materialsScopeName)))
    , mergeTransformAndShape(_Boolean(userArgs, UsdMayaJobExportArgsTokens->mergeTransformAndShape))
    , normalizeNurbs(_Boolean(userArgs, UsdMayaJobExportArgsTokens->normalizeNurbs))
    , stripNamespaces(_Boolean(userArgs, UsdMayaJobExportArgsTokens->stripNamespaces))
    , parentScope(_AbsolutePath(userArgs, UsdMayaJobExportArgsTokens->parentScope))
    , renderLayerMode(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->renderLayerMode,
          UsdMayaJobExportArgsTokens->defaultLayer,
          { UsdMayaJobExportArgsTokens->currentLayer,
            UsdMayaJobExportArgsTokens->modelingVariant }))
    , rootKind(_String(userArgs, UsdMayaJobExportArgsTokens->kind))
    , shadingMode(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->shadingMode,
          UsdMayaShadingModeTokens->none,
          UsdMayaShadingModeRegistry::ListExporters()))
    , allMaterialConversions(_TokenSet(userArgs, UsdMayaJobExportArgsTokens->convertMaterialsTo))
    , verbose(_Boolean(userArgs, UsdMayaJobExportArgsTokens->verbose))
    , staticSingleSample(_Boolean(userArgs, UsdMayaJobExportArgsTokens->staticSingleSample))
    , geomSidedness(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->geomSidedness,
          UsdMayaJobExportArgsTokens->derived,
          { UsdMayaJobExportArgsTokens->single, UsdMayaJobExportArgsTokens->double_ }))
    , includeAPINames(_TokenSet(userArgs, UsdMayaJobExportArgsTokens->apiSchema))
    , jobContextNames(_TokenSet(userArgs, UsdMayaJobExportArgsTokens->jobContext))
    , chaserNames(_Vector<std::string>(userArgs, UsdMayaJobExportArgsTokens->chaser))
    , allChaserArgs(_ChaserArgs(userArgs, UsdMayaJobExportArgsTokens->chaserArgs))
    ,

    melPerFrameCallback(_String(userArgs, UsdMayaJobExportArgsTokens->melPerFrameCallback))
    , melPostCallback(_String(userArgs, UsdMayaJobExportArgsTokens->melPostCallback))
    , pythonPerFrameCallback(_String(userArgs, UsdMayaJobExportArgsTokens->pythonPerFrameCallback))
    , pythonPostCallback(_String(userArgs, UsdMayaJobExportArgsTokens->pythonPostCallback))
    , dagPaths(dagPaths)
    , timeSamples(timeSamples)
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
        << "exportDefaultCameras: " << TfStringify(exportArgs.exportDefaultCameras) << std::endl
        << "exportDisplayColor: " << TfStringify(exportArgs.exportDisplayColor) << std::endl
        << "exportInstances: " << TfStringify(exportArgs.exportInstances) << std::endl
        << "exportMaterialCollections: " << TfStringify(exportArgs.exportMaterialCollections)
        << std::endl
        << "exportMeshUVs: " << TfStringify(exportArgs.exportMeshUVs) << std::endl
        << "exportNurbsExplicitUV: " << TfStringify(exportArgs.exportNurbsExplicitUV) << std::endl
        << "referenceObjectMode: " << exportArgs.referenceObjectMode << std::endl
        << "exportRefsAsInstanceable: " << TfStringify(exportArgs.exportRefsAsInstanceable)
        << std::endl
        << "exportSkels: " << TfStringify(exportArgs.exportSkels) << std::endl
        << "exportSkin: " << TfStringify(exportArgs.exportSkin) << std::endl
        << "exportBlendShapes: " << TfStringify(exportArgs.exportBlendShapes) << std::endl
        << "exportVisibility: " << TfStringify(exportArgs.exportVisibility) << std::endl
        << "exportComponentTags: " << TfStringify(exportArgs.exportComponentTags) << std::endl
        << "file: " << exportArgs.file << std::endl
        << "ignoreWarnings: " << TfStringify(exportArgs.ignoreWarnings) << std::endl;
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
        << "parentScope: " << exportArgs.parentScope << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "allMaterialConversions: " << std::endl;
    for (const auto& conv : exportArgs.allMaterialConversions) {
        out << "    " << conv << std::endl;
    }

    out << "stripNamespaces: " << TfStringify(exportArgs.stripNamespaces) << std::endl
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

    out << "filteredTypeIds (" << exportArgs.filteredTypeIds.size() << ")" << std::endl;
    for (unsigned int id : exportArgs.filteredTypeIds) {
        out << "    " << id << ": " << MNodeClass(MTypeId(id)).typeName() << std::endl;
    }

    out << "chaserNames (" << exportArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : exportArgs.chaserNames) {
        out << "    " << chaserName << std::endl;
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
    const std::vector<double>&      timeSamples)
{
    VtDictionary allUserArgs = VtDictionaryOver(userArgs, GetDefaultDictionary());
    VtDictionary allContextArgs;

    if (_MergeJobContexts(true, userArgs, allContextArgs)) {
        allUserArgs = VtDictionaryOver(allContextArgs, allUserArgs);
    } else {
        MGlobal::displayWarning(
            "Errors while processing export contexts. Using base export options.");
    }

    return UsdMayaJobExportArgs(allUserArgs, dagPaths, timeSamples);
}

/* static */
MStatus UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
    const MString&       optionsString,
    VtDictionary*        toFill,
    std::vector<double>* timeSamples)
{
    if (!toFill)
        return MS::kFailure;

    VtDictionary& userArgs = *toFill;

    bool       exportAnimation = false;
    GfInterval timeInterval(1.0, 1.0);
    double     frameStride = 1.0;

    std::set<double> frameSamples;

    // Get the options
    if (optionsString.length() > 0) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for (int i = 0; i < (int)optionList.length(); ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);
            if (theOption.length() != 2) {
                // We allow an empty string to be passed to exportRoots. We must process it here.
                if (theOption.length() == 1
                    && theOption[0] == UsdMayaJobExportArgsTokens->exportRoots.GetText()) {
                    std::vector<VtValue> userArgVals;
                    userArgVals.push_back(VtValue(""));
                    userArgs[UsdMayaJobExportArgsTokens->exportRoots] = userArgVals;
                }
                continue;
            }

            std::string argName(theOption[0].asChar());
            if (argName == "animation") {
                exportAnimation = (theOption[1].asInt() != 0);
            } else if (argName == "startTime") {
                timeInterval.SetMin(theOption[1].asDouble());
            } else if (argName == "endTime") {
                timeInterval.SetMax(theOption[1].asDouble());
            } else if (argName == "frameStride") {
                frameStride = theOption[1].asDouble();
            } else if (argName == "filterTypes") {
                std::vector<VtValue> userArgVals;
                MStringArray         filteredTypes;
                theOption[1].split(',', filteredTypes);
                unsigned int nbTypes = filteredTypes.length();
                for (unsigned int idxType = 0; idxType < nbTypes; ++idxType) {
                    const std::string filteredType = filteredTypes[idxType].asChar();
                    userArgVals.emplace_back(filteredType);
                }
                userArgs[UsdMayaJobExportArgsTokens->filterTypes] = userArgVals;
            } else if (argName == "frameSample") {
                frameSamples.clear();
                MStringArray samplesStrings;
                theOption[1].split(' ', samplesStrings);
                unsigned int nbSams = samplesStrings.length();
                for (unsigned int sam = 0; sam < nbSams; ++sam) {
                    if (samplesStrings[sam].isDouble()) {
                        frameSamples.insert(samplesStrings[sam].asDouble());
                    }
                }
            } else if (argName == UsdMayaJobExportArgsTokens->exportRoots.GetText()) {
                MStringArray exportRootStrings;
                theOption[1].split(',', exportRootStrings);

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
                    TfToken shadingMode(theOption[1].asChar());
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
                userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                    argName, theOption[1].asChar(), UsdMayaJobExportArgs::GetGuideDictionary());
            }
        }
    }

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

    if (timeSamples)
        *timeSamples = UsdMayaWriteUtil::GetTimeSamples(timeInterval, frameSamples, frameStride);

    return MS::kSuccess;
}

/* static */
const VtDictionary& UsdMayaJobExportArgs::GetDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Base defaults.
        d[UsdMayaJobExportArgsTokens->chaser] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->chaserArgs] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->compatibility] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->defaultCameras] = false;
        d[UsdMayaJobExportArgsTokens->defaultMeshScheme] = UsdGeomTokens->catmullClark.GetString();
        d[UsdMayaJobExportArgsTokens->defaultUSDFormat] = UsdUsdcFileFormatTokens->Id.GetString();
        d[UsdMayaJobExportArgsTokens->eulerFilter] = false;
        d[UsdMayaJobExportArgsTokens->exportCollectionBasedBindings] = false;
        d[UsdMayaJobExportArgsTokens->exportColorSets] = true;
        d[UsdMayaJobExportArgsTokens->exportDisplayColor] = false;
        d[UsdMayaJobExportArgsTokens->exportInstances] = true;
        d[UsdMayaJobExportArgsTokens->exportMaterialCollections] = false;
        d[UsdMayaJobExportArgsTokens->referenceObjectMode]
            = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportRefsAsInstanceable] = false;
        d[UsdMayaJobExportArgsTokens->exportRoots] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->exportSkin] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportSkels] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportBlendShapes] = false;
        d[UsdMayaJobExportArgsTokens->exportUVs] = true;
        d[UsdMayaJobExportArgsTokens->exportVisibility] = true;
        d[UsdMayaJobExportArgsTokens->exportComponentTags] = true;
        d[UsdMayaJobExportArgsTokens->file] = std::string();
        d[UsdMayaJobExportArgsTokens->filterTypes] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->ignoreWarnings] = false;
        d[UsdMayaJobExportArgsTokens->kind] = std::string();
        d[UsdMayaJobExportArgsTokens->materialCollectionsPath] = std::string();
        d[UsdMayaJobExportArgsTokens->materialsScopeName]
            = UsdUtilsGetMaterialsScopeName().GetString();
        d[UsdMayaJobExportArgsTokens->melPerFrameCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->melPostCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->mergeTransformAndShape] = true;
        d[UsdMayaJobExportArgsTokens->normalizeNurbs] = false;
        d[UsdMayaJobExportArgsTokens->parentScope] = std::string();
        d[UsdMayaJobExportArgsTokens->pythonPerFrameCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->pythonPostCallback] = std::string();
        d[UsdMayaJobExportArgsTokens->renderableOnly] = false;
        d[UsdMayaJobExportArgsTokens->renderLayerMode]
            = UsdMayaJobExportArgsTokens->defaultLayer.GetString();
        d[UsdMayaJobExportArgsTokens->shadingMode]
            = UsdMayaShadingModeTokens->useRegistry.GetString();
        d[UsdMayaJobExportArgsTokens->convertMaterialsTo]
            = std::vector<VtValue> { VtValue(UsdImagingTokens->UsdPreviewSurface.GetString()) };
        d[UsdMayaJobExportArgsTokens->apiSchema] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->jobContext] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->stripNamespaces] = false;
        d[UsdMayaJobExportArgsTokens->verbose] = false;
        d[UsdMayaJobExportArgsTokens->staticSingleSample] = false;
        d[UsdMayaJobExportArgsTokens->geomSidedness]
            = UsdMayaJobExportArgsTokens->derived.GetString();

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
        const auto _string = VtValue(std::string());
        const auto _stringVector = VtValue(std::vector<VtValue>({ _string }));
        const auto _stringTriplet = VtValue(std::vector<VtValue>({ _string, _string, _string }));
        const auto _stringTripletVector = VtValue(std::vector<VtValue>({ _stringTriplet }));

        // Provide guide types for the parser:
        d[UsdMayaJobExportArgsTokens->chaser] = _stringVector;
        d[UsdMayaJobExportArgsTokens->chaserArgs] = _stringTripletVector;
        d[UsdMayaJobExportArgsTokens->compatibility] = _string;
        d[UsdMayaJobExportArgsTokens->defaultCameras] = _boolean;
        d[UsdMayaJobExportArgsTokens->defaultMeshScheme] = _string;
        d[UsdMayaJobExportArgsTokens->defaultUSDFormat] = _string;
        d[UsdMayaJobExportArgsTokens->eulerFilter] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportCollectionBasedBindings] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportColorSets] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportDisplayColor] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportInstances] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportMaterialCollections] = _boolean;
        d[UsdMayaJobExportArgsTokens->referenceObjectMode] = _string;
        d[UsdMayaJobExportArgsTokens->exportRefsAsInstanceable] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportRoots] = _stringVector;
        d[UsdMayaJobExportArgsTokens->exportSkin] = _string;
        d[UsdMayaJobExportArgsTokens->exportSkels] = _string;
        d[UsdMayaJobExportArgsTokens->exportBlendShapes] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportUVs] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportVisibility] = _boolean;
        d[UsdMayaJobExportArgsTokens->exportComponentTags] = _boolean;
        d[UsdMayaJobExportArgsTokens->file] = _string;
        d[UsdMayaJobExportArgsTokens->filterTypes] = _stringVector;
        d[UsdMayaJobExportArgsTokens->ignoreWarnings] = _boolean;
        d[UsdMayaJobExportArgsTokens->kind] = _string;
        d[UsdMayaJobExportArgsTokens->materialCollectionsPath] = _string;
        d[UsdMayaJobExportArgsTokens->materialsScopeName] = _string;
        d[UsdMayaJobExportArgsTokens->melPerFrameCallback] = _string;
        d[UsdMayaJobExportArgsTokens->melPostCallback] = _string;
        d[UsdMayaJobExportArgsTokens->mergeTransformAndShape] = _boolean;
        d[UsdMayaJobExportArgsTokens->normalizeNurbs] = _boolean;
        d[UsdMayaJobExportArgsTokens->parentScope] = _string;
        d[UsdMayaJobExportArgsTokens->pythonPerFrameCallback] = _string;
        d[UsdMayaJobExportArgsTokens->pythonPostCallback] = _string;
        d[UsdMayaJobExportArgsTokens->renderableOnly] = _boolean;
        d[UsdMayaJobExportArgsTokens->renderLayerMode] = _string;
        d[UsdMayaJobExportArgsTokens->shadingMode] = _string;
        d[UsdMayaJobExportArgsTokens->convertMaterialsTo] = _stringVector;
        d[UsdMayaJobExportArgsTokens->apiSchema] = _stringVector;
        d[UsdMayaJobExportArgsTokens->jobContext] = _stringVector;
        d[UsdMayaJobExportArgsTokens->stripNamespaces] = _boolean;
        d[UsdMayaJobExportArgsTokens->verbose] = _boolean;
        d[UsdMayaJobExportArgsTokens->staticSingleSample] = _boolean;
        d[UsdMayaJobExportArgsTokens->geomSidedness] = _string;
    });

    return d;
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

UsdMayaJobImportArgs::UsdMayaJobImportArgs(
    const VtDictionary& userArgs,
    const bool          importWithProxyShapes,
    const GfInterval&   timeInterval)
    : assemblyRep(_Token(
        userArgs,
        UsdMayaJobImportArgsTokens->assemblyRep,
        UsdMayaJobImportArgsTokens->Collapsed,
        { UsdMayaJobImportArgsTokens->Full,
          UsdMayaJobImportArgsTokens->Import,
          UsdMayaJobImportArgsTokens->Unloaded }))
    , excludePrimvarNames(_TokenSet(userArgs, UsdMayaJobImportArgsTokens->excludePrimvar))
    , includeAPINames(_TokenSet(userArgs, UsdMayaJobImportArgsTokens->apiSchema))
    , jobContextNames(_TokenSet(userArgs, UsdMayaJobImportArgsTokens->jobContext))
    , includeMetadataKeys(_TokenSet(userArgs, UsdMayaJobImportArgsTokens->metadata))
    , shadingModes(_shadingModesImportArgs(userArgs, UsdMayaJobImportArgsTokens->shadingMode))
    , preferredMaterial(_Token(
          userArgs,
          UsdMayaJobImportArgsTokens->preferredMaterial,
          UsdMayaPreferredMaterialTokens->none,
          UsdMayaPreferredMaterialTokens->allTokens))
    , importUSDZTexturesFilePath(UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(userArgs))
    , importUSDZTextures(_Boolean(userArgs, UsdMayaJobImportArgsTokens->importUSDZTextures))
    , importInstances(_Boolean(userArgs, UsdMayaJobImportArgsTokens->importInstances))
    , useAsAnimationCache(_Boolean(userArgs, UsdMayaJobImportArgsTokens->useAsAnimationCache))
    , importWithProxyShapes(importWithProxyShapes)
    , timeInterval(timeInterval)
    , chaserNames(_Vector<std::string>(userArgs, UsdMayaJobImportArgsTokens->chaser))
    , allChaserArgs(_ChaserArgs(userArgs, UsdMayaJobImportArgsTokens->chaserArgs))
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
    VtDictionary allUserArgs = VtDictionaryOver(userArgs, GetDefaultDictionary());
    VtDictionary allContextArgs;

    if (_MergeJobContexts(false, userArgs, allContextArgs)) {
        allUserArgs = VtDictionaryOver(allContextArgs, allUserArgs);
    } else {
        MGlobal::displayWarning(
            "Errors while processing import contexts. Using base import options.");
    }

    return UsdMayaJobImportArgs(allUserArgs, importWithProxyShapes, timeInterval);
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
        d[UsdMayaJobImportArgsTokens->useAsAnimationCache] = false;
        d[UsdMayaJobExportArgsTokens->chaser] = std::vector<VtValue>();
        d[UsdMayaJobExportArgsTokens->chaserArgs] = std::vector<VtValue>();

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
        const auto _string = VtValue(std::string());
        const auto _stringVector = VtValue(std::vector<VtValue>({ _string }));
        const auto _stringTuplet = VtValue(std::vector<VtValue>({ _string, _string, _string }));
        const auto _stringTriplet = VtValue(std::vector<VtValue>({ _string, _string, _string }));
        const auto _stringTupletVector = VtValue(std::vector<VtValue>({ _stringTuplet }));
        const auto _stringTripletVector = VtValue(std::vector<VtValue>({ _stringTriplet }));

        // Provide guide types for the parser:
        d[UsdMayaJobImportArgsTokens->assemblyRep] = _string;
        d[UsdMayaJobImportArgsTokens->apiSchema] = _stringVector;
        d[UsdMayaJobImportArgsTokens->excludePrimvar] = _stringVector;
        d[UsdMayaJobImportArgsTokens->jobContext] = _stringVector;
        d[UsdMayaJobImportArgsTokens->metadata] = _stringVector;
        d[UsdMayaJobImportArgsTokens->shadingMode] = _stringTupletVector;
        d[UsdMayaJobImportArgsTokens->preferredMaterial] = _string;
        d[UsdMayaJobImportArgsTokens->importInstances] = _boolean;
        d[UsdMayaJobImportArgsTokens->importUSDZTextures] = _boolean;
        d[UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath] = _string;
        d[UsdMayaJobImportArgsTokens->useAsAnimationCache] = _boolean;
        d[UsdMayaJobExportArgsTokens->chaser] = _stringVector;
        d[UsdMayaJobExportArgsTokens->chaserArgs] = _stringTripletVector;
    });

    return d;
}

const std::string UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(const VtDictionary& userArgs)
{
    if (!_Boolean(userArgs, UsdMayaJobImportArgsTokens->importUSDZTextures))
        return ""; // Not importing textures. File path stays empty.

    const std::string pathArg
        = _String(userArgs, UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath);
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
        << std::endl
        << "timeInterval: " << importArgs.timeInterval << std::endl
        << "useAsAnimationCache: " << TfStringify(importArgs.useAsAnimationCache) << std::endl
        << "importWithProxyShapes: " << TfStringify(importArgs.importWithProxyShapes) << std::endl;

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

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
