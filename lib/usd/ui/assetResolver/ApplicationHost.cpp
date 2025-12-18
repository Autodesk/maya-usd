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

#include <maya/MQtUtil.h>

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

} // namespace Adsk