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

#include <AdskUsdAssetResolverExtensions/ApplicationHost.h>
#include <QtCore/QPointer>

class AssetResolverApplicationHost : public Adsk::UsdAssetResolver::Extensions::ApplicationHost
{
public:
    static void CreateInstance(QObject* parent = nullptr);
    static void DestroyInstance();

    float uiScale() const override;

    QIcon icon(const IconName& name) const override;
    int   pm(const PixelMetric& metric) const override;

    QString getUsdFileFilters() const override;
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

    virtual void showHelp(QWidget* parent = nullptr) const override;

protected:
    AssetResolverApplicationHost(QObject* parent = nullptr);
    ~AssetResolverApplicationHost() override = default;

    QIcon getIcon(const char* iconName) const;

    // A QPointer because the resolver deletes this host if another one is injected.
    static QPointer<AssetResolverApplicationHost> s_instance;
};
