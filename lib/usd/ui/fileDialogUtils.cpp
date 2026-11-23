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
#include "fileDialogUtils.h"

#include <mayaUsd/utils/util.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

namespace MayaUsdUI {

namespace {

// Returns " <flag> "<escaped text>"", or nothing when the text is empty, so
// the optional fileDialog2 flags are only passed when there is a value.
MString optionalFlag(const char* flag, const QString& text)
{
    MString result;
    if (!text.isEmpty()) {
        result += " ";
        result += flag;
        result += " \"";
        // Escape the text (a translation, a path, a filter) so that a quote or a
        // backslash in it cannot end the MEL string early.
        result += UsdMayaUtil::EscapeForMel(MQtUtil::toMString(text));
        result += "\"";
    }
    return result;
}

} // namespace

QString getUsdFileFilters(bool includeCompressed)
{
    MString cmd("from mayaUsdUtils import getUSDDialogFileFilters; getUSDDialogFileFilters(");
    cmd += includeCompressed ? "True" : "False";
    cmd += ")";
    return MQtUtil::toQString(MGlobal::executePythonCommandStringResult(cmd));
}

QStringList getOpenFileNames(
    const QString& caption,
    const QString& acceptLabel,
    const QString& dir,
    const QString& filter,
    bool           allowMultiple)
{
    // fileMode 1 is a single existing file, 4 is several existing files.
    MString cmd("fileDialog2 -fileMode ");
    cmd += allowMultiple ? 4 : 1;
    cmd += optionalFlag("-caption", caption);
    cmd += optionalFlag("-okCaption", acceptLabel);
    cmd += optionalFlag("-dir", dir);
    cmd += optionalFlag("-fileFilter", filter);

    // fileDialog2 returns an empty array when the dialog is cancelled.
    MStringArray selected;
    if (!MGlobal::executeCommand(cmd, selected, false, false)) {
        return {};
    }

    QStringList files;
    for (unsigned int i = 0; i < selected.length(); ++i) {
        files.append(MQtUtil::toQString(selected[i]));
    }
    return files;
}

} // namespace MayaUsdUI
