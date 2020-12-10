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
#include "qtUtils.h"

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtWidgets/QtWidgets>

namespace UsdLayerEditor {

QIcon QtUtils::createIcon(const char* iconName) { return QIcon(iconName); }

QPixmap QtUtils::createPNGResPixmap(QString const& in_pixmapName, int width, int height)
{

    QString pixmapName(in_pixmapName);
    if (pixmapName.indexOf(".png") == -1) {
        pixmapName += ".png";
    }

    const QString resourcePrefix(":/");
    if (pixmapName.left(2) != resourcePrefix) {
        pixmapName = resourcePrefix + pixmapName;
    }
    return createPixmap(pixmapName, width, height);
}

QPixmap QtUtils::createPixmap(QString const& in_pixmapName, int width, int height)
{

    QPixmap pixmap(in_pixmapName);
    if (width != 0 && height != 0) {
        return pixmap.scaled(width, height);
    }
    return pixmap;
}

UsdLayerEditor::QtUtils* utils;

void QtUtils::initLayoutMargins(QLayout* layout, int margin)
{
    layout->setContentsMargins(margin, margin, margin, margin);
}

// returns the widget after setting it fixed-size
QWidget* QtUtils::fixedWidget(QWidget* widget)
{
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return widget;
}

} // namespace UsdLayerEditor
