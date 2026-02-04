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
#ifndef LAYERCONTENTSWIDGET_H
#define LAYERCONTENTSWIDGET_H

// Needs to come first when used with VS2017 and Qt5.
#include "pxr/usd/sdf/layer.h"

#include <QtWidgets/QTextEdit>
#include <QtWidgets/QtWidgets>

QT_BEGIN_NAMESPACE
class QSyntaxHighlighter;
QT_END_NAMESPACE

namespace UsdLayerEditor {

/**
 * @brief Widget used to display the contents of a layer. Owned by the LayerEditorWidget
 *
 */
class LayerContentsWidget : public QWidget
{
    Q_OBJECT

public:
    LayerContentsWidget(QWidget* in_parent);
    ~LayerContentsWidget() override;

    void setLayer(const PXR_NS::SdfLayerRefPtr);

    void clear();
    bool isEmpty() const { return _isEmpty; }

private:
    void createUI();

    QPointer<QTextEdit>          _layerContents;
    QPointer<QSyntaxHighlighter> _syntaxHighlighter;
    bool                         _isEmpty { true };
};

} // namespace UsdLayerEditor

#endif // LAYERCONTENTSWIDGET_H
