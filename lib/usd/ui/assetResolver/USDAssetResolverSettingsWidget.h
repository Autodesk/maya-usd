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

#include <QtWidgets/QWidget>

namespace Adsk {

class USDAssetResolverSettingsWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString mappingFilePath READ mappingFilePath WRITE setMappingFilePath NOTIFY
                   mappingFilePathChanged)
    Q_PROPERTY(bool includeProjectTokens READ includeProjectTokens WRITE setIncludeProjectTokens
                   NOTIFY includeProjectTokensChanged)
    Q_PROPERTY(bool userPathsFirst READ userPathsFirst WRITE setUserPathsFirst NOTIFY
                   userPathsFirstChanged)
    Q_PROPERTY(
        bool userPathsOnly READ userPathsOnly WRITE setUserPathsOnly NOTIFY userPathsOnlyChanged)
    Q_PROPERTY(QStringList extAndEnvPaths READ extAndEnvPaths WRITE setExtAndEnvPaths NOTIFY
                   extAndEnvPathsChanged)
    Q_PROPERTY(QStringList userPaths READ userPaths WRITE setUserPaths NOTIFY userPathsChanged)

public:
    USDAssetResolverSettingsWidget(QWidget* parent = nullptr);
    ~USDAssetResolverSettingsWidget();

    QString mappingFilePath() const;
    void    setMappingFilePath(const QString& path);

    bool includeProjectTokens() const;
    void setIncludeProjectTokens(bool include);

    bool userPathsFirst() const;
    void setUserPathsFirst(bool userPathsFirst);

    bool userPathsOnly() const;
    void setUserPathsOnly(bool userPathsOnly);

    QStringList extAndEnvPaths() const;
    void        setExtAndEnvPaths(const QStringList& paths);

    QStringList userPaths() const;
    void        setUserPaths(const QStringList& paths);

Q_SIGNALS:
    void mappingFilePathChanged(const QString& path);
    void includeProjectTokensChanged(bool include);
    void userPathsFirstChanged(bool userPathsFirst);
    void userPathsOnlyChanged(bool userPathsOnly);
    void extAndEnvPathsChanged(const QStringList& paths);
    void userPathsChanged(const QStringList& paths);
    void saveRequested();
    void closeRequested();

private:
    const std::unique_ptr<class USDAssetResolverSettingsWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(USDAssetResolverSettingsWidget)
};

} // namespace Adsk