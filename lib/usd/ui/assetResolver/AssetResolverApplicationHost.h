//
// Copyright 2026 Autodesk
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

#include <AssetResolverWidgets/ApplicationHost.h>

class AssetResolverApplicationHost : public Adsk::ApplicationHost
{
public:
    static void CreateInstance(QObject* parent = nullptr);

    float uiScale() const override;

    QIcon icon(const IconName& name) const override;
    int   pm(const PixelMetric& metric) const override;

    QString getUSDDialogFileFilters() const;
    QString getOpenFileName(
        QWidget*       parent = nullptr,
        const QString& caption = QString(),
        const QString& dir = QString(),
        const QString& filter = QString()) const override;

    QString getExistingDirectory(
        QWidget*             parent = nullptr,
        const QString&       caption = QString(),
        const QString&       dir = QString(),
        QFileDialog::Options options = QFileDialog::ShowDirsOnly) const override;

protected:
    AssetResolverApplicationHost(QObject* parent = nullptr);
    ~AssetResolverApplicationHost() override = default;

    QIcon getIcon(const char* iconName) const;

    static AssetResolverApplicationHost* s_instance;
};
