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
#include "baseListShadingModesCommand.h"

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((NoneOption, "none"))
    ((NoneNiceName, "None"))
    ((NoneExportDescription, "No material data gets exported."))
    ((NoneImportDescription, 
        "Stop the search for materials. Can signal that no materials are to be"
        " imported when used alone."))
);

namespace {
    std::pair<TfToken, TfToken> _GetOptions(const MString& niceName, bool isExport) {
        TfToken niceToken(niceName.asChar());

        TfToken shadingMode, convertMaterialsTo;
        if (niceToken == _tokens->NoneNiceName) {
            return std::make_pair(_tokens->NoneOption, convertMaterialsTo);
        }

        for (auto const& e : (isExport ? UsdMayaShadingModeRegistry::ListExporters()
                                       : UsdMayaShadingModeRegistry::ListImporters())) {
            if (niceToken == (isExport ? UsdMayaShadingModeRegistry::GetExporterNiceName(e)
                                       : UsdMayaShadingModeRegistry::GetImporterNiceName(e))) {
                shadingMode = e;
                break;
            }
        }
        if (shadingMode.IsEmpty()) {
            for (auto const& r : UsdMayaShadingModeRegistry::ListMaterialConversions()) {
                auto const& info = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(r);
                if (niceToken == info.niceName
                    && (isExport ? info.hasExporter : info.hasImporter)) {
                    shadingMode = UsdMayaShadingModeTokens->useRegistry;
                    convertMaterialsTo = r;
                }
            }
        }
        return std::make_pair(shadingMode, convertMaterialsTo);
    }
}

MStatus
MayaUSDListShadingModesCommand::doIt(const MArgList& args) {    
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    if (argData.isFlagSet("export")) {
        // We have these default exporters which are always 1-2 in the options:
        appendToResult(UsdMayaShadingModeRegistry::GetMaterialConversionInfo(
                           UsdImagingTokens->UsdPreviewSurface)
                           .niceName.GetText());
        appendToResult(_tokens->NoneNiceName.GetText());
        // Then we explore the registries:
        for (auto const& s : UsdMayaShadingModeRegistry::ListExporters()) {
            if (s != UsdMayaShadingModeTokens->useRegistry) {
                appendToResult(UsdMayaShadingModeRegistry::GetExporterNiceName(s).c_str());
            }
        }
        for (auto const& c : UsdMayaShadingModeRegistry::ListMaterialConversions()) {
            if (c != UsdImagingTokens->UsdPreviewSurface) {
                auto const& info = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(c);
                if (info.hasExporter) {
                    appendToResult(info.niceName.GetText());
                }
            }
        }
    } else if (argData.isFlagSet("import")) {
        // Default priorities for searching for materials is defined here:
        //    - Specialized importers using registry based import.
        //    - Specialized importers, non-registry based.
        //    - Universal importers (MaterialX? UsdPreviewSurface) using registry based import.
        //    - Display colors as last resort
        // This is to be used when importing via the dialog. Finer grained import is available at
        // the command level.
        //
        for (const auto c : UsdMayaShadingModeRegistry::ListMaterialConversions()) {
            if (c != UsdImagingTokens->UsdPreviewSurface) {
                auto const& info = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(c);
                if (info.hasImporter) {
                    appendToResult(info.niceName.GetText());
                }
            }
        }
        for (const auto s : UsdMayaShadingModeRegistry::ListImporters()) {
            if (s != UsdMayaShadingModeTokens->useRegistry
                && s != UsdMayaShadingModeTokens->displayColor) {
                appendToResult(UsdMayaShadingModeRegistry::GetImporterNiceName(s).c_str());
            }
        }
        appendToResult(UsdMayaShadingModeRegistry::GetMaterialConversionInfo(
                           UsdImagingTokens->UsdPreviewSurface)
                           .niceName.GetText());
        appendToResult(
            UsdMayaShadingModeRegistry::GetImporterNiceName(UsdMayaShadingModeTokens->displayColor)
                .c_str());
        appendToResult(_tokens->NoneNiceName.GetText());
    } else if (argData.isFlagSet("exportOptions")) {
        MString    niceName;
        status = argData.getFlagArgument("exportOptions", 0, niceName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken shadingMode, materialConversion;
        std::tie(shadingMode, materialConversion) = _GetOptions(niceName, true);
        if (shadingMode.IsEmpty()) {
            return MS::kNotFound;
        }
        MString options = "shadingMode=";
        options += shadingMode.GetText();
        if (!materialConversion.IsEmpty()) {
            options += ";convertMaterialsTo=";
            options += materialConversion.GetText();
        }
        setResult(options);
    } else if (argData.isFlagSet("importOptions")) {
        MString    niceName;
        status = argData.getFlagArgument("importOptions", 0, niceName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken shadingMode, materialConversion;
        std::tie(shadingMode, materialConversion) = _GetOptions(niceName, false);
        if (shadingMode.IsEmpty()) {
            return MS::kNotFound;
        }
        appendToResult(shadingMode.GetText());
        if (!materialConversion.IsEmpty()) {
            appendToResult(materialConversion.GetText());
        } else {
            appendToResult(UsdMayaShadingModeTokens->none.GetText());
        }
    } else if (argData.isFlagSet("exportAnnotation") || argData.isFlagSet("importAnnotation")) {
        const bool isExport = argData.isFlagSet("exportAnnotation");
        MString    niceName;
        status = argData.getFlagArgument(
            isExport ? "exportAnnotation" : "importAnnotation", 0, niceName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken shadingMode, materialConversion;
        std::tie(shadingMode, materialConversion) = _GetOptions(niceName, isExport);
        if (shadingMode.IsEmpty()) {
            return MS::kNotFound;
        } else if (materialConversion.IsEmpty()) {
            if (shadingMode == _tokens->NoneOption) {
                setResult(
                    isExport ? _tokens->NoneExportDescription.GetText()
                             : _tokens->NoneImportDescription.GetText());
            } else {
                setResult(
                    isExport
                        ? UsdMayaShadingModeRegistry::GetExporterDescription(shadingMode).c_str()
                        : UsdMayaShadingModeRegistry::GetImporterDescription(shadingMode).c_str());
            }
        } else {
            auto const& info
                = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(materialConversion);
            setResult(
                isExport ? info.exportDescription.GetText()
                         : info.importDescription.GetText());
        }
    } else if (argData.isFlagSet("findExportName") || argData.isFlagSet("findImportName")) {
        const bool isExport = argData.isFlagSet("findExportName");
        MString    optName;
        status
            = argData.getFlagArgument(isExport ? "findExportName" : "findImportName", 0, optName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken optToken(optName.asChar());
        if (optToken == _tokens->NoneOption) {
            setResult(_tokens->NoneNiceName.GetText());
            return MS::kSuccess;
        }
        auto const& info = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(optToken);
        if (isExport ? info.hasExporter : info.hasImporter) {
            setResult(info.niceName.GetText());
            return MS::kSuccess;
        }
        if (optToken != UsdMayaShadingModeTokens->useRegistry) {
            const std::string& niceName = isExport
                ? UsdMayaShadingModeRegistry::GetExporterNiceName(optToken)
                : UsdMayaShadingModeRegistry::GetImporterNiceName(optToken);
            if (!niceName.empty()) {
                setResult(niceName.c_str());
                return MS::kSuccess;
            }
        }
        setResult("");
    }

    return MS::kSuccess;
}

MSyntax
MayaUSDListShadingModesCommand::createSyntax() {
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);
    syntax.addFlag("-eo", "-exportOptions", MSyntax::kString);
    syntax.addFlag("-io", "-importOptions", MSyntax::kString);
    syntax.addFlag("-ea", "-exportAnnotation", MSyntax::kString);
    syntax.addFlag("-ia", "-importAnnotation", MSyntax::kString);
    syntax.addFlag("-fen", "-findExportName", MSyntax::kString);
    syntax.addFlag("-fin", "-findImportName", MSyntax::kString);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* MayaUSDListShadingModesCommand::creator() {
    return new MayaUSDListShadingModesCommand();
}

} // MAYAUSD_NS_DEF
