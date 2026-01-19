//
// Copyright 2025 Autodesk
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

#include "ApplicationHost.h"

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>

namespace Adsk {

ApplicationHost* instancePtr = nullptr;

ApplicationHost::ApplicationHost(QObject* parent)
    : QObject(parent)
{
}

ApplicationHost::~ApplicationHost()
{
    if (instancePtr == this) {
        instancePtr = nullptr;
    }
}

ApplicationHost& ApplicationHost::instance()
{
    if (!instancePtr) {
        instancePtr = new ApplicationHost(qApp);
    }
    return *instancePtr;
}

void ApplicationHost::injectInstance(ApplicationHost* host)
{
    if (instancePtr) {
        delete instancePtr;
    }
    instancePtr = host;
}

float ApplicationHost::uiScale() const
{
    return dpiScale(1.0f); // Default implementation
}

int   ApplicationHost::dpiScale(int size) const { return MQtUtil::dpiScale(size); }
float ApplicationHost::dpiScale(float size) const { return MQtUtil::dpiScale(size); }

QIcon ApplicationHost::icon(const IconName& name) const
{
    switch (name) {
    case IconName::Add: return getIcon(":/UsdLayerEditor/addCreateGeneric");
    case IconName::AddFolder: return getIcon(":/assetResolver/add_folder.png");
    case IconName::OpenFile: return getIcon("fileOpen.png");
    case IconName::Delete: return getIcon("trash.png");
    case IconName::MoveUp: return getIcon(":/assetResolver/move_up.png");
    case IconName::MoveDown: return getIcon(":/assetResolver/move_down.png");
    default: return QIcon();
    }
}

QIcon ApplicationHost::getIcon(const char* iconName)
{
    QIcon* icon = MQtUtil::createIcon(iconName);
    QIcon  copy;
    if (nullptr != icon) {
        copy = QIcon(*icon);
    }
    delete icon;
    return copy;
}

QColor ApplicationHost::themeColor(const ThemeColors& color) const
{
    switch (color) {
    case ThemeColors::ListBorder: return QColor(Qt::GlobalColor::black); // Default implementation
    default: return QColor();
    }
}

int ApplicationHost::pm(const PixelMetric& metric) const
{
    switch (metric) {
    case PixelMetric::TinyPadding: return dpiScale(2);
    case PixelMetric::ResizableActiveAreaSize: return dpiScale(8);
    case PixelMetric::ResizableContentMargin: return dpiScale(4);
    case PixelMetric::ItemHeight: return dpiScale(24);
    case PixelMetric::HeaderHeight: return dpiScale(28);
    default: return 0;
    }
};

QWidget*
ApplicationHost::wrapWithCollapseable(const QString& title, QWidget* content, bool open) const
{
    Q_UNUSED(open);

    QGroupBox*   groupBox = new QGroupBox(title);
    QVBoxLayout* layout = new QVBoxLayout(groupBox);
    layout->addWidget(content);
    groupBox->setLayout(layout);
    return groupBox;
}

QVariant ApplicationHost::loadPersistentData(const QString& group, const QString& key) const
{
    Q_UNUSED(group);
    Q_UNUSED(key);
    return QVariant();
}

void ApplicationHost::savePersistentData(
    const QString&  group,
    const QString&  key,
    const QVariant& value) const
{
    Q_UNUSED(group);
    Q_UNUSED(key);
    Q_UNUSED(value);
    // Default implementation does nothing
}

QString ApplicationHost::getUSDDialogFileFilters() const
{
    // A hard-coded default implementation could be:
    // return tr("All USD Files (*.usd *.usda *.usdc);;All Files (*.*)");

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

QString ApplicationHost::getOpenFileName(
    QWidget*       parent,
    const QString& caption,
    const QString& dir,
    const QString& filter) const
{
    // A default implementation using QFileDialog
    // QString filePath = QFileDialog::getOpenFileName(parent, caption, dir, filter);
    // return filePath;

    // Maya specific implementation.
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

QString ApplicationHost::getExistingDirectory(
    QWidget*             parent,
    const QString&       caption,
    const QString&       dir,
    QFileDialog::Options options) const
{
    // A default implementation using QFileDialog
    // QString pickedDir = QFileDialog::getExistingDirectory(parent, caption, dir, options);
    // return pickedDir;

    // Maya specific implementation.
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

} // namespace Adsk
