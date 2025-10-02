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

#include <qapplication.h>
#include <qboxlayout.h>
#include <qgroupbox.h>

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
    return MQtUtil::dpiScale(1.0f);
    // return 1.0f;
}

QIcon ApplicationHost::icon(const IconName& name) const
{
    switch (name) {
    case IconName::Add: return getIcon(":/UsdLayerEditor/addCreateGeneric");
    case IconName::OpenFile: return getIcon(":/fileOpen");
    case IconName::Delete: return getIcon(":/trash");
    case IconName::MoveUp: return QIcon::fromTheme("go-up");
    case IconName::MoveDown: return QIcon::fromTheme("go-down");
    default: return QIcon();
    }
}

QIcon ApplicationHost::getIcon(const char* iconName)
{
    QIcon* icon = MQtUtil::createIcon(iconName);
    QIcon  copy;
    if (icon) {
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
    // in a real implementation, these would be scaled by uiScale()
    switch (metric) {
    case PixelMetric::TinyPadding: return 2 * MQtUtil::dpiScale(1); // Default implementation
    case PixelMetric::ResizableActiveAreaSize:
        return 8 * MQtUtil::dpiScale(1); // Default implementation
    case PixelMetric::ResizableContentMargin:
        return 4 * MQtUtil::dpiScale(1);                            // Default implementation
    case PixelMetric::ItemHeight: return 24 * MQtUtil::dpiScale(1); // Default implementation
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