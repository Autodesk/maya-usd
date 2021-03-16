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

#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
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

/// Extracts a bool at \p key from \p userArgs, or false if it can't extract.
static bool _Boolean(const VtDictionary& userArgs, const TfToken& key)
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
static std::string _String(const VtDictionary& userArgs, const TfToken& key)
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
static TfToken _Token(
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
static SdfPath _AbsolutePath(const VtDictionary& userArgs, const TfToken& key)
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
template <typename T>
static std::vector<T> _Vector(const VtDictionary& userArgs, const TfToken& key)
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
static TfToken::Set _TokenSet(const VtDictionary& userArgs, const TfToken& key)
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
static std::map<std::string, UsdMayaJobExportArgs::ChaserArgs>
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
static UsdMayaJobImportArgs::ShadingModes
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

static TfToken _GetMaterialsScopeName(const std::string& materialsScopeName)
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
    , exportReferenceObjects(_Boolean(userArgs, UsdMayaJobExportArgsTokens->exportReferenceObjects))
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
    , convertMaterialsTo(_Token(
          userArgs,
          UsdMayaJobExportArgsTokens->convertMaterialsTo,
          UsdImagingTokens->UsdPreviewSurface,
          UsdMayaShadingModeRegistry::ListMaterialConversions()))
    , verbose(_Boolean(userArgs, UsdMayaJobExportArgsTokens->verbose))
    , staticSingleSample(_Boolean(userArgs, UsdMayaJobExportArgsTokens->staticSingleSample))
    ,

    chaserNames(_Vector<std::string>(userArgs, UsdMayaJobExportArgsTokens->chaser))
    , allChaserArgs(_ChaserArgs(userArgs, UsdMayaJobExportArgsTokens->chaserArgs))
    ,

    melPerFrameCallback(_String(userArgs, UsdMayaJobExportArgsTokens->melPerFrameCallback))
    , melPostCallback(_String(userArgs, UsdMayaJobExportArgsTokens->melPostCallback))
    , pythonPerFrameCallback(_String(userArgs, UsdMayaJobExportArgsTokens->pythonPerFrameCallback))
    , pythonPostCallback(_String(userArgs, UsdMayaJobExportArgsTokens->pythonPostCallback))
    ,

    dagPaths(dagPaths)
    , timeSamples(timeSamples)
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
        << "exportRefsAsInstanceable: " << TfStringify(exportArgs.exportRefsAsInstanceable)
        << std::endl
        << "exportSkels: " << TfStringify(exportArgs.exportSkels) << std::endl
        << "exportSkin: " << TfStringify(exportArgs.exportSkin) << std::endl
        << "exportBlendShapes: " << TfStringify(exportArgs.exportBlendShapes) << std::endl
        << "exportVisibility: " << TfStringify(exportArgs.exportVisibility) << std::endl
        << "file: " << exportArgs.file << std::endl
        << "ignoreWarnings: " << TfStringify(exportArgs.ignoreWarnings) << std::endl
        << "materialCollectionsPath: " << exportArgs.materialCollectionsPath << std::endl
        << "materialsScopeName: " << exportArgs.materialsScopeName << std::endl
        << "mergeTransformAndShape: " << TfStringify(exportArgs.mergeTransformAndShape) << std::endl
        << "normalizeNurbs: " << TfStringify(exportArgs.normalizeNurbs) << std::endl
        << "parentScope: " << exportArgs.parentScope << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "convertMaterialsTo: " << exportArgs.convertMaterialsTo << std::endl
        << "stripNamespaces: " << TfStringify(exportArgs.stripNamespaces) << std::endl
        << "timeSamples: " << exportArgs.timeSamples.size() << " sample(s)" << std::endl
        << "staticSingleSample: " << TfStringify(exportArgs.staticSingleSample) << std::endl
        << "usdModelRootOverridePath: " << exportArgs.usdModelRootOverridePath << std::endl;

    out << "melPerFrameCallback: " << exportArgs.melPerFrameCallback << std::endl
        << "melPostCallback: " << exportArgs.melPostCallback << std::endl
        << "pythonPerFrameCallback: " << exportArgs.pythonPerFrameCallback << std::endl
        << "pythonPostCallback: " << exportArgs.pythonPostCallback << std::endl;

    out << "dagPaths (" << exportArgs.dagPaths.size() << ")" << std::endl;
    for (const MDagPath& dagPath : exportArgs.dagPaths) {
        out << "    " << dagPath.fullPathName().asChar() << std::endl;
    }

    out << "_filteredTypeIds (" << exportArgs.GetFilteredTypeIds().size() << ")" << std::endl;
    for (unsigned int id : exportArgs.GetFilteredTypeIds()) {
        out << "    " << id << ": " << MNodeClass(MTypeId(id)).className() << std::endl;
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

    return out;
}

/* static */
UsdMayaJobExportArgs UsdMayaJobExportArgs::CreateFromDictionary(
    const VtDictionary&             userArgs,
    const UsdMayaUtil::MDagPathSet& dagPaths,
    const std::vector<double>&      timeSamples)
{
    return UsdMayaJobExportArgs(
        VtDictionaryOver(userArgs, GetDefaultDictionary()), dagPaths, timeSamples);
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
        d[UsdMayaJobExportArgsTokens->exportReferenceObjects] = false;
        d[UsdMayaJobExportArgsTokens->exportRefsAsInstanceable] = false;
        d[UsdMayaJobExportArgsTokens->exportSkin] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportSkels] = UsdMayaJobExportArgsTokens->none.GetString();
        d[UsdMayaJobExportArgsTokens->exportBlendShapes] = false;
        d[UsdMayaJobExportArgsTokens->exportUVs] = true;
        d[UsdMayaJobExportArgsTokens->exportVisibility] = true;
        d[UsdMayaJobExportArgsTokens->file] = std::string();
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
            = UsdImagingTokens->UsdPreviewSurface.GetString();
        d[UsdMayaJobExportArgsTokens->stripNamespaces] = false;
        d[UsdMayaJobExportArgsTokens->verbose] = false;
        d[UsdMayaJobExportArgsTokens->staticSingleSample] = false;

        // plugInfo.json site defaults.
        // The defaults dict should be correctly-typed, so enable
        // coerceToWeakerOpinionType.
        const VtDictionary site
            = UsdMaya_RegistryHelper::GetComposedInfoDictionary(_usdExportInfoScope->allTokens);
        VtDictionaryOver(site, &d, /*coerceToWeakerOpinionType*/ true);
    });

    return d;
}

void UsdMayaJobExportArgs::AddFilteredTypeName(const MString& typeName)
{
    MNodeClass   cls(typeName);
    unsigned int id = cls.typeId().id();
    if (id == 0) {
        TF_WARN("Given excluded node type '%s' does not exist; ignoring", typeName.asChar());
        return;
    }
    _filteredTypeIds.insert(id);
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
        _filteredTypeIds.insert(id);
    }
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
    , includeMetadataKeys(_TokenSet(userArgs, UsdMayaJobImportArgsTokens->metadata))
    , shadingModes(_shadingModesImportArgs(userArgs, UsdMayaJobImportArgsTokens->shadingMode))
    , preferredMaterial(_Token(
          userArgs,
          UsdMayaJobImportArgsTokens->preferredMaterial,
          UsdMayaPreferredMaterialTokens->none,
          UsdMayaPreferredMaterialTokens->allTokens))
    , importUSDZTexturesFilePath(UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(
          _String(userArgs, UsdMayaJobImportArgsTokens->importUSDZTexturesFilePath)))
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
    return UsdMayaJobImportArgs(
        VtDictionaryOver(userArgs, GetDefaultDictionary()), importWithProxyShapes, timeInterval);
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
        d[UsdMayaJobImportArgsTokens->metadata]
            = std::vector<VtValue>({ VtValue(SdfFieldKeys->Hidden.GetString()),
                                     VtValue(SdfFieldKeys->Instanceable.GetString()),
                                     VtValue(SdfFieldKeys->Kind.GetString()) });
        d[UsdMayaJobImportArgsTokens->shadingMode] = std::vector<VtValue> { VtValue(
            std::vector<VtValue> { VtValue(UsdMayaShadingModeTokens->useRegistry.GetString()),
                                   VtValue(UsdImagingTokens->UsdPreviewSurface.GetString()) }) };
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

    return d;
}

const std::string UsdMayaJobImportArgs::GetImportUSDZTexturesFilePath(const std::string& userArg)
{
    std::string importTexturesRootDirPath;
    if (userArg.size() == 0) { // NOTE: (yliangsiew) If the user gives an empty argument, we'll try
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
            bool bStat
                = UsdMayaUtilFileSystem::pathAppendPath(importTexturesRootDirPath, "sourceimages");
            if (!bStat) {
                TF_RUNTIME_ERROR(
                    "Unable to determine the texture directory for the Maya project: %s.",
                    currentMayaWorkspacePath.asChar());
                return "";
            }
            TF_WARN(
                "Because -importUSDZTexturesFilePath was not explicitly specified, textures "
                "will be imported to the workspace folder: %s.",
                currentMayaWorkspacePath.asChar());
        }
    } else {
        importTexturesRootDirPath.assign(userArg);
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
