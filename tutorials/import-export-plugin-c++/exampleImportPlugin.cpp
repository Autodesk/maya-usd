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

#include <QCheckBox.h>
#include <QLabel.h>
#include <QLayout.h>

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

// Retrieve the import plugin default settings.
VtDictionary getDefaultImportSettings()
{
    static const VtDictionary defaultSettings({
        std::make_pair(UsdMayaJobImportArgsTokens->importInstances, VtValue(true)),
    });

    return defaultSettings;
}

// Load the import plugin settings from a Maya option var, if it exists.
VtDictionary loadImportSettings()
{
    return VtDictionaryOver(jsonOptionVarToDictionary(importOptionVar), getDefaultImportSettings());
}

// Save the import plugin settings in a Maya option var.
void saveImportSettings(const VtDictionary& settings)
{
    // Note: only convert the settings the plugin is interested in.
    JsObject jsSettings = dictionaryToJSON(settings);

    if (VtDictionaryIsHolding<bool>(settings, UsdMayaJobImportArgsTokens->importInstances)) {
        const bool importInstances
            = VtDictionaryGet<bool>(settings, UsdMayaJobImportArgsTokens->importInstances);
        jsSettings[UsdMayaJobImportArgsTokens->importInstances.GetString()]
            = JsValue(importInstances);
    }

    jsonToOptionVar(importOptionVar, jsSettings);
}

// Import plugin UI creation and filling function, receving a Qt container to fill.
// You can create a different layout for the container if you wish to replace the
// default row layout.
void fillImportUI(QLayout* layout, const VtDictionary& settings)
{
    layout->addWidget(new QLabel("<h2>These are the import-plugin settings.</h2>"));

    const bool importInstances = VtDictionaryGet<bool>(
        settings, UsdMayaJobImportArgsTokens->importInstances, Vt_DefaultHolder<bool>(true));
    auto importInstancesCheckBox
        = new QCheckBox(tokenToLabel(UsdMayaJobImportArgsTokens->importInstances));
    importInstancesCheckBox->setChecked(importInstances);
    importInstancesCheckBox->setObjectName(
        tokenToQString(UsdMayaJobImportArgsTokens->importInstances));

    layout->addWidget(importInstancesCheckBox);
}

// Import plugin UI query to retrieve the data when the UI is confirmed by the user.
void queryImportUI(QWidget* container, VtDictionary& settings)
{
    QCheckBox* importInstancesCheckBox = container->findChild<QCheckBox*>(
        tokenToQString(UsdMayaJobImportArgsTokens->importInstances));
    if (importInstancesCheckBox) {
        const bool importInstances = importInstancesCheckBox->isChecked();
        settings[UsdMayaJobImportArgsTokens->importInstances] = importInstances;
    }
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

    showDialogUI(
        title, parentUIName, forcedSettings, fillImportUI, queryImportUI, saveImportSettings);

    return forcedSettings;
}
