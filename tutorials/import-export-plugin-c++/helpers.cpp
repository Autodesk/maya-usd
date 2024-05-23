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

namespace Helpers {

PXR_NAMESPACE_USING_DIRECTIVE

////////////////////////////////////////////////////////////////////////////
//
// Logging Helpers

// To help trace what is going on.
void logDebug(const char* msg) { MGlobal::displayInfo(msg); }
void logDebug(const std::string& msg) { logDebug(msg.c_str()); }

////////////////////////////////////////////////////////////////////////////
//
// Token Helpers

// Utility function to convert a token name into a Qt string.
QString tokenToQString(const TfToken& token) { return QString(token.GetString().c_str()); }

// Utility function to convert a token name into a somehwat nice UI label.
// Adds a space before all uppercase letters and capitalizes the label.
QString tokenToLabel(const TfToken& token)
{
    QString label;

    bool isFirst = true;
    for (const QChar& c : tokenToQString(token)) {
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

} // namespace Helpers
