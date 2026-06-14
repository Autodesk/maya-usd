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
#ifndef WARNINGDIALOGS_H
#define WARNINGDIALOGS_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QMessageBox>

#include <functional>

/**
 * @brief Helpers to easily pop a properly formatted and sized message box
 *
 */
namespace UsdLayerEditor {

// create a confirmation dialog, with an optional bullet list of stuff like layer names
bool confirmDialog(
    QWidget*           parent,
    const QString&     title,
    const QString&     message,
    const QStringList* bulletList = nullptr,
    const QString*     okButtonText = nullptr,
    QMessageBox::Icon  icon = QMessageBox::Icon::NoIcon);

// create a dialog with a single OK button, with an optional bullet list
void warningDialog(
    QWidget*           parent,
    const QString&     title,
    const QString&     message,
    const QStringList* bulletList = nullptr,
    QMessageBox::Icon  icon = QMessageBox::Icon::NoIcon);

// Test seam: when a handler is installed, confirmDialog/warningDialog return its
// result instead of showing a real dialog. Pass nullptr to uninstall.
using ModalDialogTestHandler = std::function<bool(const QString& title, const QString& message)>;
ModalDialogTestHandler setModalDialogTestHandler(ModalDialogTestHandler handler);

} // namespace UsdLayerEditor

#endif // WARNINGDIALOGS_H
