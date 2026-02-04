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
class Resizable : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QWidget* widget READ widget WRITE setWidget)
    Q_PROPERTY(int contentSize READ contentSize WRITE setContentSize)
    Q_PROPERTY(int minContentSize READ minContentSize WRITE setMinContentSize)
    Q_PROPERTY(int maxContentSize READ maxContentSize WRITE setMaxContentSize)

public:
    Resizable(
        QWidget* widget,
        QWidget* parent = nullptr,
        QString  persistentStorageGroup = QString(),
        QString  persistentStorageKey = QString(),
        int      defaultSize = -1);
    ~Resizable() override;

    QWidget* widget() const;
    void     setWidget(QWidget* w);

    int  contentSize() const;
    void setContentSize(int s);

    int  minContentSize() const;
    void setMinContentSize(int s);

    int  maxContentSize() const;
    void setMaxContentSize(int s);

private:
    const std::unique_ptr<class ResizablePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Resizable);
};
} // namespace Adsk
