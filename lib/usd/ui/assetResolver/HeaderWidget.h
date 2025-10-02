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
#include <qwidget.h>

namespace Adsk {
class HeaderWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle);

public:
    HeaderWidget(const QString& title = "", QWidget* parent = nullptr);
    ~HeaderWidget();

    QString title() const;
    void    setTitle(const QString& title);

    void paintEvent(QPaintEvent* event) override;

private:
    const std::unique_ptr<class HeaderWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(HeaderWidget);
};
} // namespace Adsk
