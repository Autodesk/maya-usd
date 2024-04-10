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

#ifndef MAYAUSD_TUTORIALS_IMPORT_EXPORT_PLUGIN_HELPERS_H
#define MAYAUSD_TUTORIALS_IMPORT_EXPORT_PLUGIN_HELPERS_H

#include <pxr/base/js/json.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>

#include <QLayout>
#include <QString>
#include <QWidget>

namespace Helpers {

// Simplify code by declaring some type vocabulary.
using Token = PXR_NS::TfToken;
using Dict = PXR_NS::VtDictionary;
using Json = PXR_NS::JsObject;

////////////////////////////////////////////////////////////////////////////
//
// Logging Helpers

// To help trace what is going on.
void logDebug(const char* msg);
void logDebug(const std::string& msg);

////////////////////////////////////////////////////////////////////////////
//
// Token Helpers

// Convert a token name into a Qt string.
QString tokenToQString(const Token& token);

// Convert a token name into a somehwat nice UI label.
// Adds a space before all uppercase letters and capitalizes the label.
QString tokenToLabel(const Token& token);

////////////////////////////////////////////////////////////////////////////
//
// JSON Helpers

// Convert a JSON-encoded text string into a USD dictionary.
Dict jsonToDictionary(const std::string& json);

// Convert a Maya option var containing a JSON-encoded text string into a USD dictionary.
Dict jsonOptionVarToDictionary(const char* optionVarName);

// Convert a USD JSON object into a text string saved into a Maya option var.
void jsonToOptionVar(const char* optionVarName, const Json& jsSettings);

// Convert a USD dictionary to a USD JSON object.
// Note: only data types supported by JSON will be converted.
Json dictionaryToJSON(const Dict& dict);

////////////////////////////////////////////////////////////////////////////
//
// UI Helpers

// Create a UI element based on the type of the named setting.
void createUIElement(QLayout* layout, const Dict& settings, const Token& settingName);

// Query a UI element data based on the type of the named setting and fill that setting.
void queryUIElement(QWidget* parent, Dict& settings, const Token& settingName);

// Plugin UI creation and filling function, receving a Qt row layout to fill.
using FillUIFunction = void (*)(QLayout* layout, const Dict& settings);

// Plugin UI query to retrieve the settings data when the UI is confirmed by the user.
using QueryUIFunction = void (*)(QWidget* container, Dict& settings);

// Save the plugin settings somewhere, for example in a Maya option var.
using SaveSettingsFunction = void (*)(const Dict& settings);

// Generic function to show a dialog with OK/cancel buttons.
//
// fillUI creates the individual UI elements and sets their values.
// queryUI reads the values from the UI
// saveSettings saves the values obtained from the UI
void showDialogUI(
    const std::string&   title,
    const std::string&   parentUIName,
    Dict&                settings,
    FillUIFunction       fillUI,
    QueryUIFunction      queryUI,
    SaveSettingsFunction saveSettings);

} // namespace Helpers

#endif
