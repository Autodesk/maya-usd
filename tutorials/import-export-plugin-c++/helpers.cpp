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

#include <mayaUsd/utils/jsonDict.h>

#include <pxr/base/js/converter.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QBoxLayout.h>
#include <QDialog.h>
#include <QDialogButtonBox.h>

// To help trace what is going on.
void logDebug(const char* msg) { MGlobal::displayInfo(msg); }
void logDebug(const std::string& msg) { logDebug(msg.c_str()); }

// Utility function to convert a token name into a Qt string.
QString tokenToQString(const PXR_NS::TfToken& token) { return QString(token.GetString().c_str()); }

// Utility function to convert a token name into a somehwat nice UI label.
// Adds a space before all uppercase letters and capitalizes the label.
QString tokenToLabel(const PXR_NS::TfToken& token)
{
    QString label;

    bool isFirst = true;
    for (const QChar c : tokenToQString(token)) {
        if (isFirst) {
            label += c.toUpper();
            isFirst = false;
        } else {
            if (c.isUpper()) {
                label += ' ';
            }
            label += c;
        }
    }

    return label;
}

// Convert a JSON-encoded text string into a USD VtDictionary.
PXR_NS::VtDictionary jsonToDictionary(const std::string& json)
{
    PXR_NS::JsValue jsValue = PXR_NS::JsParseString(json.c_str());

    // Note: we pass false to the template so that it uses int instead of int64.
    //       This is more compatible with the way MayaUSD encodes integers in
    //       its settings.
    PXR_NS::VtValue value
        = PXR_NS::JsValueTypeConverter<PXR_NS::VtValue, PXR_NS::VtDictionary, false>::Convert(
            jsValue);

    if (!value.IsHolding<PXR_NS::VtDictionary>())
        return {};

    return value.Get<PXR_NS::VtDictionary>();
}

// Convert a Maya option var containing a JSON-encoded text string into a USD VtDictionary.
PXR_NS::VtDictionary jsonOptionVarToDictionary(const char* optionVarName)
{
    if (!MGlobal::optionVarExists(optionVarName))
        return {};

    MString encodedValue = MGlobal::optionVarStringValue(optionVarName);
    return jsonToDictionary(encodedValue.asChar());
}

// Convert a USD JSON object into a text string saved into a Maya option var.
void jsonToOptionVar(const char* optionVarName, const PXR_NS::JsObject& jsSettings)
{
    const std::string encodedSettings = PXR_NS::JsWriteToString(jsSettings);
    MGlobal::setOptionVarValue(optionVarName, encodedSettings.c_str());
}

// Convert a USD dictionary to a USD JSON object.
// Note: only data types supported by JSON will be converted.
PXR_NS::JsObject dictionaryToJSON(const PXR_NS::VtDictionary& dict)
{
    return MayaUsd::VtDictionaryToJsValueConverter::convertToDictionary(dict);
}

// Generic function to show a dialog with OK/cancel buttons.
void showDialogUI(
    const std::string&    title,
    const std::string&    parentUIName,
    PXR_NS::VtDictionary& settings,
    FillUIFunction        fillUI,
    QueryUIFunction       queryUI,
    SaveSettingsFunction  saveSettings)
{
    try {
        QWidget* parentWidget = MQtUtil::findControl(parentUIName.c_str());
        while (!parentWidget->isWindow())
            parentWidget = parentWidget->parentWidget();

        auto window = new QDialog(parentWidget);
        window->setModal(true);
        window->setWindowTitle(title.c_str());

        auto windowLayout = new QVBoxLayout();
        window->setLayout(windowLayout);

        auto container = new QWidget();
        windowLayout->addWidget(container);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        windowLayout->addWidget(buttonBox);

        auto rowLayout = new QVBoxLayout();
        container->setLayout(rowLayout);
        fillUI(rowLayout, settings);

        auto accept = [&]() {
            queryUI(container, settings);
            saveSettings(settings);
            window->accept();
        };

        buttonBox->connect(buttonBox, &QDialogButtonBox::accepted, accept);
        buttonBox->connect(buttonBox, &QDialogButtonBox::rejected, window, &QDialog::reject);

        window->exec();
    } catch (const std::exception& ex) {
        logDebug(std::string("Error: ") + ex.what());
    }
}
