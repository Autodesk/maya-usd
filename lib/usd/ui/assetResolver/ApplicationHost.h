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

#pragma once

#include <QtCore/QObject>
#include <QtGui/QIcon>

namespace Adsk {

class ApplicationHost : public QObject
{
    Q_OBJECT

public:
    ApplicationHost(QObject* parent = nullptr);
    ~ApplicationHost();

    virtual float uiScale() const;

    virtual int   dpiScale(int size) const;
    virtual float dpiScale(float size) const;

    enum class IconName
    {
        Add,
        AddFolder,
        OpenFile,
        Delete,
        MoveUp,
        MoveDown,
    };
    virtual QIcon icon(const IconName& name) const;

    static QIcon getIcon(const char* iconName);
    enum class PixelMetric
    {
        TinyPadding,             // 2 px at 1.0 scale
        ResizableActiveAreaSize, // 8 px at 1.0 scale
        ResizableContentMargin,  // 4 px at 1.0 scale
        ItemHeight,              // 24 px at 1.0 scale
        HeaderHeight,            // 32 px at 1.0 scale
    };
    /** Gets a requested pixel metric value. These are already scaled by the
     * uiScale factor. */
    virtual int pm(const PixelMetric& metric) const;

    enum class ThemeColors
    {
        ListBorder,
    };
    virtual QColor themeColor(const ThemeColors& color) const;

    virtual QWidget*
    wrapWithCollapseable(const QString& title, QWidget* content, bool open = true) const;

    virtual QVariant loadPersistentData(const QString& group, const QString& key) const;
    virtual void
    savePersistentData(const QString& group, const QString& key, const QVariant& value) const;

    static ApplicationHost& instance();

protected:
    static void injectInstance(ApplicationHost* host);

Q_SIGNALS:
    void uiScaleChanged(float scale);
    void iconsChanged();
    void themeColorsChanged();
};

} // namespace Adsk
