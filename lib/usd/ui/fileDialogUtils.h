//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSDUI_FILE_DIALOG_UTILS_H
#define MAYAUSDUI_FILE_DIALOG_UTILS_H

#include <mayaUsdUI/ui/api.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace MayaUsdUI {

//! \return The USD file dialog filters, in the ";;"-separated format that Maya's
//! file dialogs accept.
//! \param includeCompressed Whether to also offer .usdz files.
MAYAUSD_UI_PUBLIC QString getUsdFileFilters(bool includeCompressed = true);

//! Opens Maya's native file dialog to pick existing files. Empty arguments are
//! left to Maya's defaults.
//! \param caption       The dialog title.
//! \param acceptLabel   The label of the accept button.
//! \param dir           The starting directory.
//! \param filter        The ";;"-separated file filters.
//! \param allowMultiple Whether more than one file can be selected.
//! \return The chosen file paths, or empty when cancelled.
MAYAUSD_UI_PUBLIC QStringList getOpenFileNames(
    const QString& caption,
    const QString& acceptLabel,
    const QString& dir,
    const QString& filter,
    bool           allowMultiple);

} // namespace MayaUsdUI

#endif
