//
// Copyright 2020 Autodesk
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
#include "warningDialogs.h"

#include "qtUtils.h"

#include <QtWidgets/QMessageBox>

namespace {

// returns a bullet-ed html text for a list of layers tree items
// used for dialog boxes, or an empty string if none
QString getLayerBulletList(const QStringList* in_list)
{
    QString bullet = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -";
    QString text = "";
    if (in_list != nullptr && !in_list->isEmpty()) {
        text += "<br><nobr>";
        for (const auto& item : *in_list)
            text += bullet + item + "<br>";
        text += "</nobr>";
    }
    return text;
}

} // namespace

namespace UsdLayerEditor {

bool confirmDialog_internal(
    bool               okCancel,
    const QString&     title,
    const QString&     message,
    const QStringList* bulletList,
    const QString*     okButtonText)
{
    QMessageBox msgBox;
    // there is no title bar text on mac, instead it's bold text
    if (IS_MAC_OS)
        msgBox.setText(title);
    else
        msgBox.setWindowTitle(title);

    QString text(message);
    text += getLayerBulletList(bulletList);
    msgBox.setInformativeText(text);

    if (!IS_MAC_OS) {
        auto margins = msgBox.layout()->contentsMargins();
        margins.setTop(0);
        msgBox.layout()->setContentsMargins(margins);
    }

    if (okCancel) {
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
    } else {
        msgBox.setStandardButtons(QMessageBox::Ok);
    }
    msgBox.setStyleSheet(QString("QLabel{min-width: %1px;}").arg(DPIScale(400)));

    if (okButtonText != nullptr)
        msgBox.button(QMessageBox::Ok)->setText(*okButtonText);

    return msgBox.exec() == QMessageBox::Ok;
}

bool confirmDialog(
    const QString&     title,
    const QString&     message,
    const QStringList* bulletList,
    const QString*     okButtonText)
{
    return confirmDialog_internal(true, title, message, bulletList, okButtonText);
}

// create a warning dialog, with an optional bullet list
void warningDialog(const QString& title, const QString& message, const QStringList* bulletList)
{
    confirmDialog_internal(false, title, message, bulletList, nullptr);
}

} // namespace UsdLayerEditor
