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
//

#include "PreferencesApplicationHost.h"

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>

PreferencesApplicationHost* PreferencesApplicationHost::s_instance = nullptr;

PreferencesApplicationHost::PreferencesApplicationHost(QObject* parent)
    : Adsk::ApplicationHost(parent)
{
    Adsk::ApplicationHost::injectInstance(this);
}

void PreferencesApplicationHost::CreateInstance(QObject* parent)
{
    if (!s_instance) {
        s_instance = new PreferencesApplicationHost(parent);
    }
}

float PreferencesApplicationHost::uiScale() const { return MQtUtil::dpiScale(1.0f); }

QIcon PreferencesApplicationHost::icon(const IconName& name) const
{
    switch (name) {
    case IconName::Add: return getIcon(":/UsdLayerEditor/addCreateGeneric");
    case IconName::AddFolder: return getIcon(":/AdskAssetResolver/add_folder");
    case IconName::OpenFile: return getIcon("fileOpen.png");
    case IconName::Delete: return getIcon("trash.png");
    case IconName::MoveUp: return getIcon(":/AdskAssetResolver/move_up");
    case IconName::MoveDown: return getIcon(":/AdskAssetResolver/move_down");
    default: return QIcon();
    }
}

QIcon PreferencesApplicationHost::getIcon(const char* iconName) const
{
    QIcon* icon = MQtUtil::createIcon(iconName);
    QIcon  copy;
    if (nullptr != icon) {
        copy = QIcon(*icon);
    }
    delete icon;
    return copy;
}

int PreferencesApplicationHost::pm(const PixelMetric& metric) const
{
    const float scale = uiScale();
    switch (metric) {
    case PixelMetric::TinyPadding: return static_cast<int>(2 * scale);
    case PixelMetric::ResizableActiveAreaSize: return static_cast<int>(8 * scale);
    case PixelMetric::ResizableContentMargin: return static_cast<int>(4 * scale);
    case PixelMetric::IconSize: return static_cast<int>(20 * scale);
    case PixelMetric::ItemHeight: return static_cast<int>(24 * scale);
    case PixelMetric::HeaderHeight: return static_cast<int>(28 * scale);
    default: return 0;
    }
};

QString PreferencesApplicationHost::getUSDDialogFileFilters() const
{
    MString filters = MGlobal::executePythonCommandStringResult(
        "from mayaUsdUtils import getUSDDialogFileFilters; getUSDDialogFileFilters(False)");
    return MQtUtil::toQString(filters);
}

MString createMStringFormatArg(const MString& arg, const QString& str)
{
    MString mstr(" "); // Default with just space so MString format works correctly
    if (!str.isEmpty()) {
        mstr += arg;   // The argument name
        mstr += " \""; // Surround the argument with quotes
        mstr += MQtUtil::toMString(str);
        mstr += "\"";
    }
    return mstr;
}

QString PreferencesApplicationHost::getOpenFileName(
    QWidget*       parent,
    const QString& caption,
    const QString& dir,
    const QString& filter) const
{
    const char* script = R"mel(
    global proc string assetResolver_GetOpenFileName()
    {
        string $result[] = `fileDialog2
            -fileMode 1
            ^1s ^2s ^3s`;
        if (0 == size($result))
            return "";
        else
            return $result[0];
    }
    assetResolver_GetOpenFileName();
    )mel";

    // Note: the three args are optional, so we only add them if they are not empty.
    MString commandString;
    MString strCaption = createMStringFormatArg("-caption", caption);
    MString strDir = createMStringFormatArg("-dir", dir);
    MString strFilter = createMStringFormatArg("-fileFilter", filter);
    commandString.format(script, strCaption, strDir, strFilter);

    MString filePath = MGlobal::executeCommandStringResult(commandString);
    return MQtUtil::toQString(filePath);
}

QString PreferencesApplicationHost::getExistingDirectory(
    QWidget*             parent,
    const QString&       caption,
    const QString&       dir,
    QFileDialog::Options options) const
{
    const int   fileMode = (options | QFileDialog::ShowDirsOnly) ? 3 : 2;
    const char* script = R"mel(
    global proc string assetResolver_GetExistingDirectory()
    {
        string $result[] = `fileDialog2
            -fileMode ^1s
            ^2s ^3s
            -okCaption "Select Folder"`;
        if (0 == size($result))
            return "";
        else
            return $result[0];
    }
    assetResolver_GetExistingDirectory();
    )mel";

    MString commandString;
    MString strFileMode;
    strFileMode += fileMode;
    MString strCaption = createMStringFormatArg("-caption", caption);
    MString strDir = createMStringFormatArg("-dir", dir);
    commandString.format(script, strFileMode, strCaption, strDir);

    MString filePath = MGlobal::executeCommandStringResult(commandString);
    return MQtUtil::toQString(filePath);
}
