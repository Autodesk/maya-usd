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

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>

namespace Helpers {

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void createBoolUI(QLayout* layout, const Dict& settings, const Token& settingName)
{
    const bool value = VtDictionaryGet<bool>(settings, settingName, VtDefault = true);
    auto       checkBox = new QCheckBox(tokenToLabel(settingName));
    checkBox->setChecked(value);
    checkBox->setObjectName(tokenToQString(settingName));
    layout->addWidget(checkBox);
}

void createIntUI(QLayout* layout, const Dict& settings, const Token& settingName)
{
    layout->addWidget(new QLabel(tokenToLabel(settingName)));

    const int value = VtDictionaryGet<int>(settings, settingName, VtDefault = 0);
    auto      lineEdit = new QLineEdit(QString().setNum(value));
    lineEdit->setValidator(new QIntValidator());
    lineEdit->setObjectName(tokenToQString(settingName));
    layout->addWidget(lineEdit);
}

void createDoubleUI(QLayout* layout, const Dict& settings, const Token& settingName)
{
    layout->addWidget(new QLabel(tokenToLabel(settingName)));

    const double value = VtDictionaryGet<double>(settings, settingName, VtDefault = 0.);
    auto         lineEdit = new QLineEdit(QString().setNum(value));
    lineEdit->setValidator(new QDoubleValidator());
    lineEdit->setObjectName(tokenToQString(settingName));
    layout->addWidget(lineEdit);
}

} // namespace

// Create a UI element based on the type of the named setting.
void createUIElement(QLayout* layout, const Dict& settings, const Token& settingName)
{
    if (VtDictionaryIsHolding<bool>(settings, settingName))
        return createBoolUI(layout, settings, settingName);
    if (VtDictionaryIsHolding<int>(settings, settingName))
        return createIntUI(layout, settings, settingName);
    if (VtDictionaryIsHolding<double>(settings, settingName))
        return createDoubleUI(layout, settings, settingName);

    const std::string errMsg = TfStringPrintf(
        "Cannot create UI for unsupported type for setting %s", settingName.GetString().c_str());
    MGlobal::displayWarning(errMsg.c_str());
}

namespace {

void queryBoolUI(QWidget* parent, Dict& settings, const Token& settingName)
{
    QCheckBox* checkBox = parent->findChild<QCheckBox*>(tokenToQString(settingName));
    if (!checkBox)
        return;

    const bool value = checkBox->isChecked();
    settings[settingName] = value;
}

void queryIntUI(QWidget* parent, Dict& settings, const Token& settingName)
{
    QLineEdit* lineEdit = parent->findChild<QLineEdit*>(tokenToQString(settingName));
    if (!lineEdit)
        return;

    const double value = lineEdit->text().toDouble();
    settings[settingName] = value;
}

void queryDoubleUI(QWidget* parent, Dict& settings, const Token& settingName)
{
    QLineEdit* lineEdit = parent->findChild<QLineEdit*>(tokenToQString(settingName));
    if (!lineEdit)
        return;

    const double value = lineEdit->text().toDouble();
    settings[settingName] = value;
}

} // namespace

// Query a UI element data based on the type of the named setting and fill that setting.
void queryUIElement(QWidget* parent, Dict& settings, const Token& settingName)
{
    if (VtDictionaryIsHolding<bool>(settings, settingName))
        return queryBoolUI(parent, settings, settingName);
    if (VtDictionaryIsHolding<int>(settings, settingName))
        return queryIntUI(parent, settings, settingName);
    if (VtDictionaryIsHolding<double>(settings, settingName))
        return queryDoubleUI(parent, settings, settingName);

    const std::string errMsg = TfStringPrintf(
        "Cannot query UI for unsupported type for setting %s", settingName.GetString().c_str());
    MGlobal::displayWarning(errMsg.c_str());
}

// Generic function to show a dialog with OK/cancel buttons.
void showDialogUI(
    const std::string&   title,
    const std::string&   parentUIName,
    Dict&                settings,
    FillUIFunction       fillUI,
    QueryUIFunction      queryUI,
    SaveSettingsFunction saveSettings)
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

} // namespace Helpers
