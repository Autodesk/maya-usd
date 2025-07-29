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
#include "layerContentsWidget.h"

#include "stringResources.h"
#include "usdSyntaxHighlighter.h"

namespace UsdLayerEditor {

LayerContentsWidget::LayerContentsWidget(QWidget* in_parent)
    : QWidget(in_parent)
{
    createUI();
}

LayerContentsWidget::~LayerContentsWidget() { }

void LayerContentsWidget::createUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    _layerContents = new QTextEdit;
    _layerContents->setAcceptRichText(true);
    _layerContents->setFrameStyle(QFrame::NoFrame);
    _layerContents->setPlaceholderText(
        StringResources::getAsQString(StringResources::kDisplayLayerContentsEmpty));
    _layerContents->setReadOnly(true);

    // Apply USD syntax highlighting
    _syntaxHighlighter = new UsdSyntaxHighlighter(_layerContents->document());

    mainLayout->addWidget(_layerContents);
    setLayout(mainLayout);
}

void LayerContentsWidget::setLayer(const PXR_NS::SdfLayerRefPtr in_layer)
{
    if (_layerContents) {
        _layerContents->clear();
        if (nullptr != in_layer) {
            std::string layerText;
            // Note: potentially slow operation for large layers.
            //       We could consider doing this in a worker thread if needed.
            if (in_layer->ExportToString(&layerText)) {
                _layerContents->setPlainText(QString::fromStdString(layerText));
            }
        }
    }
}

} // namespace UsdLayerEditor
