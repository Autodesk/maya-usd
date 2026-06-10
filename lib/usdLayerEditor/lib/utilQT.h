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
#ifndef USDLAYEREDITOR_LAYEREDITOR_QTUTILS_H
#define USDLAYEREDITOR_LAYEREDITOR_QTUTILS_H

class QPushButton;
class QSize;
class QString;

#include "layerEditorAPI.h"

#include <QtCore/QPointF>
#include <QtGui/QCursor>
#include <QtWidgets/QtWidgets>

namespace UsdLayerEditor {

LayerEditorAPI void initializeQtUtils();

/**
 * @brief QT helpers the layer editor needs to load bitmaps and handle DPI scaling
 *
 */
class LayerEditorAPI QtUtils
{
public:
    virtual double  dpiScale() { return _dpiScale; }
    virtual void    setDpiScale(double dpiScale) { _dpiScale = dpiScale; }
    virtual QIcon   createIcon(const char* iconName);
    virtual QPixmap createPixmap(QString const& pixmapName, int width = 0, int height = 0);
    virtual QPixmap createPNGResPixmap(QString const& pixmapName, int width = 0, int height = 0);

    // Detects a DCC application's light theme by looking at the window background color.
    virtual bool lightTheme() const;
    // Lighten a pixmap by a given factor.
    virtual QPixmap lightPixmap(const QPixmap& pixmap, float factor) const;
    
    // shortcut to setting the margins
    static void initLayoutMargins(QLayout* layout, int margin = 0);
    // returns the widget after setting it fixed-size
    static QWidget* fixedWidget(QWidget* widget);

    // tests if the mouse is the given rectangle
    bool static inline isMouseInRectangle(QWidget* widget, QRect const& rect)
    {
        auto globalPos = QCursor::pos();
        auto localPos = widget->mapFromGlobal(globalPos);
        return rect.contains(localPos);
    }

    // Prepares a push button with DPI-appropriate regular, hover and pressed png in the
    // autodesk human interface guideline style
    static void setupButtonWithHIGBitmaps(QPushButton* button, const QString& baseName);

    // Returns image_100.png" when you pass "image",
    // using the DPI setting and also returns always 100 on mac, because Qt doesn't
    // properly support high dpi with style sheets
    static QString getDPIPixmapName(QString baseName);

    static void disableHIGButton(QPushButton* button, bool disable = true);

private:
    double _dpiScale = 1.0;

};

/**
 * @brief Disable repaint updates for the given widget until the disabler is destroyed.
 *
 */
class LayerEditorAPI QtDisableRepaintUpdates
{
public:
    QtDisableRepaintUpdates(QWidget& widget);
    ~QtDisableRepaintUpdates();

private:
    QWidget& _widget;
};

/**
 * Used by the component-save widget to gate component-name entry to a USD-safe
 * identifier.
 */
class LayerEditorAPI ValidTfIdentifierValidator : public QValidator
{
public:
    explicit ValidTfIdentifierValidator(QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;
};

#ifdef Q_OS_DARWIN
const bool IS_MAC_OS = true;
#else
const bool IS_MAC_OS = false;
#endif

LayerEditorAPI QtUtils* getQtUtils();

LayerEditorAPI void setQtUtils(QtUtils* qtUtils);

template <class T> inline T DPIScale(T pixel)
{
    return static_cast<T>(pixel * getQtUtils()->dpiScale());
}

} // namespace UsdLayerEditor

#endif // LAYEREDITOR_QTUTILS_H
