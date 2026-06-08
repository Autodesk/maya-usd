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
#include "utilQT.h"

#include <pxr/base/tf/stringUtils.h>

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtWidgets/QtWidgets>

namespace UsdLayerEditor {

void initializeQtUtils()
{
    if (nullptr == UsdLayerEditor::getQtUtils()) {
        UsdLayerEditor::setQtUtils(new QtUtils());
    }
}

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

bool QtUtils::lightTheme() const
{
    return QApplication::palette().color(QPalette::Window).lightnessF() > 0.5f;
}

QPixmap QtUtils::lightPixmap(const QPixmap& pixmap, float factor) const
{
    QImage image = pixmap.toImage();
    for (int i = 0; i < image.width(); ++i) {
        for (int j = 0; j < image.height(); ++j) {
            QColor color = QColor(image.pixel(i, j));
            int    r = std::min(255, static_cast<int>(static_cast<float>(color.red()) * factor));
            int    g = std::min(255, static_cast<int>(static_cast<float>(color.green()) * factor));
            int    b = std::min(255, static_cast<int>(static_cast<float>(color.blue()) * factor));
            image.setPixel(i, j, qRgb( r, g, b ));
        }
    }
    return QPixmap::fromImage(image);
}

QPixmap QtUtils::createPixmap(QString const& in_pixmapName, int width, int height)
{
    // Set appropriate post-fix for DPI scaling.
    std::string noExtName = in_pixmapName.toStdString();
    const std::string pngExt = ".png";
    size_t            extPos = noExtName.find(pngExt);
    if (extPos != std::string::npos) {
        noExtName.erase(extPos, pngExt.length());
    }
    
    QPixmap pixmap(getDPIPixmapName(QString::fromStdString(noExtName)));
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
    const auto scale = getQtUtils()->dpiScale();
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

QtUtils* utils = nullptr;

QtUtils* getQtUtils() { return utils; }

void setQtUtils(QtUtils* qtUtils) { utils = qtUtils; }

ValidTfIdentifierValidator::ValidTfIdentifierValidator(QObject* parent)
    : QValidator(parent)
{
}

QValidator::State ValidTfIdentifierValidator::validate(QString& input, int& /*pos*/) const
{
    std::string orig = input.toStdString();
    std::string valid = PXR_NS::TfMakeValidIdentifier(orig);
    if (input.isEmpty()) {
        return Intermediate; // Allow user to type
    }
    if (orig == valid && !valid.empty()) {
        return Acceptable;
    }
    return Invalid;
}

} // namespace UsdLayerEditor
