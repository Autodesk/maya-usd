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
    ((NoneDescription, "No material data gets exported."))
);

namespace {
    std::pair<TfToken, TfToken> _GetOptions(const MString& niceName) {
        TfToken niceToken(niceName.asChar());

        TfToken shadingMode, convertMaterialsTo;
        if (niceToken == _tokens->NoneNiceName) {
            return std::make_pair(_tokens->NoneOption, convertMaterialsTo);
        }

        for (auto const& e: UsdMayaShadingModeRegistry::ListExporters()) {
            if (niceToken == UsdMayaShadingModeRegistry::GetExporterNiceName(e)) {
                shadingMode = e;
                break;
            }
        }
        if (shadingMode.IsEmpty()) {
            for (auto const& r: UsdMayaShadingModeRegistry::ListExportConversions()) {
                if (niceToken == UsdMayaShadingModeRegistry::GetExportConversionInfo(r).niceName) {
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
        // We have these default exporters which are always 1-2-3 in the options:
        appendToResult(UsdMayaShadingModeRegistry::GetExportConversionInfo(
                           UsdImagingTokens->UsdPreviewSurface)
                           .niceName.GetText());
        appendToResult(
            UsdMayaShadingModeRegistry::GetExporterNiceName(UsdMayaShadingModeTokens->displayColor)
                .c_str());
        appendToResult(_tokens->NoneNiceName.GetText());
        // Then we explore the registries:
        for (auto const& shadingMode : UsdMayaShadingModeRegistry::ListExporters()) {
            if (shadingMode != UsdMayaShadingModeTokens->useRegistry
                && shadingMode != UsdMayaShadingModeTokens->displayColor) {
                appendToResult(
                    UsdMayaShadingModeRegistry::GetExporterNiceName(shadingMode).c_str());
            }
        }
        for (auto const& materialConversion : UsdMayaShadingModeRegistry::ListExportConversions()) {
            appendToResult(
                UsdMayaShadingModeRegistry::GetExportConversionInfo(materialConversion)
                    .niceName.GetText());
        }
    } else if (argData.isFlagSet("import")) {
        // Always include the "none" shading mode.
        appendToResult(UsdMayaShadingModeTokens->none.GetText());
        for (const auto& e : UsdMayaShadingModeRegistry::ListImporters()) {
            appendToResult(e.GetText());
        }
    } else if (argData.isFlagSet("exportOptions")) {
        MString niceName;
        status = argData.getFlagArgument("exportOptions", 0, niceName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken shadingMode, materialConversion;
        std::tie(shadingMode, materialConversion) = _GetOptions(niceName);
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
    } else if (argData.isFlagSet("exportAnnotation")) {
        MString niceName;
        status = argData.getFlagArgument("exportAnnotation", 0, niceName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken shadingMode, materialConversion;
        std::tie(shadingMode, materialConversion) = _GetOptions(niceName);
        if (shadingMode.IsEmpty()) {
            return MS::kNotFound;
        } else if (materialConversion.IsEmpty()) {
            if (shadingMode == _tokens->NoneOption) {
                setResult(_tokens->NoneDescription.GetText());
            } else {
                setResult(UsdMayaShadingModeRegistry::GetExporterDescription(shadingMode).c_str());
            }
        } else {
            setResult(UsdMayaShadingModeRegistry::GetExportConversionInfo(materialConversion)
                          .description.GetText());
        }
    } else if (argData.isFlagSet("findExportName")) {
        MString optName;
        status = argData.getFlagArgument("findExportName", 0, optName);
        if (status != MS::kSuccess) {
            return status;
        }
        TfToken optToken(optName.asChar());
        if (optToken == _tokens->NoneOption) {
            setResult(_tokens->NoneNiceName.GetText());
            return MS::kSuccess;
        }
        for (auto const& r: UsdMayaShadingModeRegistry::ListExportConversions()) {
            if (r == optToken) {
                setResult(
                    UsdMayaShadingModeRegistry::GetExportConversionInfo(r).niceName.GetText());
                return MS::kSuccess;
            }
        }
        for (auto const& s: UsdMayaShadingModeRegistry::ListExporters()) {
            if (s == optToken && s != UsdMayaShadingModeTokens->useRegistry) {
                setResult(UsdMayaShadingModeRegistry::GetExporterNiceName(s).c_str());
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
    syntax.addFlag("-ea", "-exportAnnotation", MSyntax::kString);
    syntax.addFlag("-fen", "-findExportName", MSyntax::kString);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* MayaUSDListShadingModesCommand::creator() {
    return new MayaUSDListShadingModesCommand();
}

} // MAYAUSD_NS_DEF
