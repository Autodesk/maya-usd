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

#ifndef MAYAUSDUI_UNDO_CHUNK_UTILS_H
#define MAYAUSDUI_UNDO_CHUNK_UTILS_H

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <QtCore/QLatin1String>
#include <QtCore/QString>

#include <string>

namespace MayaUsdUI {

//! Sanitizes a user-facing undo label for use as a Maya undo chunk name.
//! Maya's -chunkName flag does not accept spaces; backslash and double-quote
//! are escaped so the result is safe to embed in a MEL double-quoted string.
inline MString cleanChunkName(const std::string& label)
{
    QString name = QString::fromStdString(label);
    name.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    name.replace(QLatin1Char('"'), QLatin1String("\\\""));
    name.replace(QLatin1Char(' '), QLatin1Char('_'));
    return MString(("\"" + name.toStdString() + "\"").c_str());
}

//! RAII guard that opens a named Maya undo chunk on construction and closes it
//! on destruction, ensuring the chunk is always balanced even if an exception
//! is thrown inside the guarded scope.
struct UndoChunkGuard
{
    //! Opens the undo chunk. If \p label is empty, no chunk name is set.
    explicit UndoChunkGuard(const std::string& label)
    {
        if (label.empty()) {
            MGlobal::executeCommand("undoInfo -openChunk", false, false);
        } else {
            MGlobal::executeCommand(
                MString("undoInfo -openChunk -chunkName ") + cleanChunkName(label), false, false);
        }
    }

    ~UndoChunkGuard() { MGlobal::executeCommand("undoInfo -closeChunk", false, false); }

    UndoChunkGuard(const UndoChunkGuard&) = delete;
    UndoChunkGuard& operator=(const UndoChunkGuard&) = delete;
};

} // namespace MayaUsdUI

#endif // MAYAUSDUI_UNDO_CHUNK_UTILS_H
