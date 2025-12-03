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

#include <pxr/base/tf/stringUtils.h>

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

    // Note: createPNGResPixmap calls QtUtils::createPNGResPixmap, but that is
    //       *not* the function below but rather MayaQtUtils::createPixmap, since
    //       these functions are virtual.
    //
    //       The MayaQtUtils version calls MQtUtil::createPixmap which calls
    //       QmayaQtHelper::createPixmap which generates the scaled image name
    //       by adding the _150 or _200 suffix as necessary.
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

QString QtUtils::getDPIPixmapName(QString baseName)
{
#ifdef Q_OS_DARWIN
    return baseName + "_100.png";
#else
    const auto scale = utils->dpiScale();
    if (scale >= 2.0)
        return baseName + "_200.png";
    else if (scale >= 1.5)
        return baseName + "_150.png";
    return baseName + "_100.png";
#endif
}

void QtUtils::setupButtonWithHIGBitmaps(QPushButton* button, const QString& baseName)
{
    button->setFlat(true);

    // regular size: 16px, pressed:24px
    // therefore, border is 4
    int     padding = DPIScale(4);
    QString cssTemplate(R"CSS(
    QPushButton {
        padding : %1px;
        background-image: url(%2);
        background-position: center center;
        background-repeat: no-repeat;
        border: 0px;
        background-origin: content;
        }
    QPushButton::hover {
            background-image: url(%3);
        }
    QPushButton::pressed {
        background-image: url(%4);
        border: 0px;
        padding: 0px;
        background-origin: content;
        })CSS");

    QString css = cssTemplate.arg(padding)
                      .arg(getDPIPixmapName(baseName))
                      .arg(getDPIPixmapName(baseName + "_hover"))
                      .arg(getDPIPixmapName(baseName + "_pressed"));

    button->setStyleSheet(css);

    // overkill, but used to generate the grayed out version
    auto effect = new QGraphicsOpacityEffect(button);
    button->setGraphicsEffect(effect);
}

void QtUtils::disableHIGButton(QPushButton* button, bool disable)
{
    button->setDisabled(disable);
    auto effect = dynamic_cast<QGraphicsOpacityEffect*>(button->graphicsEffect());
    effect->setOpacity(disable ? 0.4 : 1.0);
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

QtDisableRepaintUpdates::QtDisableRepaintUpdates(QWidget& widget)
    : _widget(widget)
{
    widget.setUpdatesEnabled(false);
}

QtDisableRepaintUpdates::~QtDisableRepaintUpdates()
{
    try {
        // Note: re-enabling updates automatically triggers a repaint.
        _widget.setUpdatesEnabled(true);
    } catch (std::exception&) {
        // Don't let exceptions out of destructor.
    }
}

TfValidIdentifierValidator::TfValidIdentifierValidator(QObject* parent)
    : QValidator(parent)
{
}

QValidator::State TfValidIdentifierValidator::validate(QString& input, int& pos) const
{
    std::string orig = input.toStdString();
    std::string valid = pxr::TfMakeValidIdentifier(orig);
    if (input.isEmpty()) {
        return Intermediate; // Allow user to type
    }
    if (orig == valid && !valid.empty()) {
        return Acceptable;
    }
    return Invalid;
}

} // namespace UsdLayerEditor
