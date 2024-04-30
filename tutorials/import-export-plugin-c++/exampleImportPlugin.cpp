//
// Copyright 2024 Autodesk
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

#include "helpers.h"

#include <mayaUsd/fileio/jobContextRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <QLabel>
#include <QLayout>

// Note: the macro use type names from the PXR namespace, so we need to bring
//       those type into the current namespace for them to work.
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Note: the import/export plugin registry API requires that the nice name for the
//       import be the same as for the export.
const char niceName[] = "Example C++ Import and Export Plugin";

const char importDescription[] = "This is an example of an import plugin written in C++";

// Import plugin settings. Saved on-disk using a Maya option variable.
const char importOptionVar[] = "CppExampleImportPluginOptionVar";

const TfToken importThisOrThat("exampleImportPluginThisOrThat");
const int     importThisOrThatDefault = 37;

// Retrieve the import plugin default settings.
VtDictionary getDefaultImportSettings()
{
    // Note: we are controlling the instances setting even though that
    //       setting already has a UI in the MayaUSD export dialog. When
    //       an existing setting is forced in this way, the UI in the main
    //       dialog is disabled.
    //
    //       We recommend *not* controlling existing settings. We support
    //       it in case your plugin has special needs that require forcing
    //       the setting to a specific value. For example to ensure it is
    //       within a special range or that it is always on or always off.
    static const VtDictionary defaultSettings({
        std::make_pair(UsdMayaJobImportArgsTokens->importInstances, VtValue(true)),
        std::make_pair(importThisOrThat, VtValue(importThisOrThatDefault)),
    });

    return defaultSettings;
}

// Load the import plugin settings from a Maya option var, if it exists.
VtDictionary loadImportSettings()
{
    const VtDictionary savedSettings = Helpers::jsonOptionVarToDictionary(importOptionVar);
    return VtDictionaryOver(savedSettings, getDefaultImportSettings());
}

// Save the import plugin settings in a Maya option var.
void saveImportSettings(const VtDictionary& settings)
{
    // Note: we only convert the settings the plugin is interested in because the settings
    //       we receive as input are the settings that were passed to the showDialogUI
    //       function below, which were only our settings.
    Helpers::Json jsSettings = Helpers::dictionaryToJSON(settings);
    Helpers::jsonToOptionVar(importOptionVar, jsSettings);
}

// Import plugin UI creation and filling function, receving a Qt container to fill.
// You can create a different layout for the container if you wish to replace the
// default row layout.
void fillImportUI(QLayout* layout, const VtDictionary& settings)
{
    layout->addWidget(new QLabel("<h2>These are the import-plugin settings.</h2>"));

    Helpers::createUIElement(layout, settings, UsdMayaJobImportArgsTokens->importInstances);
    Helpers::createUIElement(layout, settings, importThisOrThat);
}

// Import plugin UI query to retrieve the data when the UI is confirmed by the user.
void queryImportUI(QWidget* container, VtDictionary& settings)
{
    Helpers::queryUIElement(container, settings, UsdMayaJobImportArgsTokens->importInstances);
    Helpers::queryUIElement(container, settings, importThisOrThat);
}

} // namespace

REGISTER_IMPORT_JOB_CONTEXT_FCT(CppExampleImportExportPlugin, niceName, importDescription)
{
    return loadImportSettings();
}

// Note: parameters are:
// const TfToken& jobContext, const std::string& parentUIName, const VtDictionary& settings
REGISTER_IMPORT_JOB_CONTEXT_UI_FCT(CppExampleImportExportPlugin)
{
    VtDictionary forcedSettings = loadImportSettings();

    const std::string title = TfStringPrintf("Options for %s", jobContext.GetString().c_str());

    Helpers::showDialogUI(
        title, parentUIName, forcedSettings, fillImportUI, queryImportUI, saveImportSettings);

    return forcedSettings;
}
