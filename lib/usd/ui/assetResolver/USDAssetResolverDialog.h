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

#ifndef MAYAUSDUI_USD_ASSETRESOLVER_DIALOG_H
#define MAYAUSDUI_USD_ASSETRESOLVER_DIALOG_H

#include "ApplicationHost.h"
#include "USDAssetResolverSettingsWidget.h"

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/IUSDImportView.h>
#include <mayaUsdUI/ui/ItemDelegate.h>
#include <mayaUsdUI/ui/TreeModel.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>

#include <AdskAssetResolver/AdskAssetResolver.h>
#include <AdskAssetResolver/AssetResolverContextData.h>
#include <AdskAssetResolver/AssetResolverContextDataRegistry.h>
#include <AdskAssetResolver/AssetResolverContextExtension.h>
#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDialog>

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
class MAYAUSD_UI_PUBLIC USDAssetResolverDialog : public QDialog
{
    Q_OBJECT
public:
    USDAssetResolverDialog(QWidget* parent);
    ~USDAssetResolverDialog();
    bool execute();

protected:
    void OnMappingFileChanged(const QString& path);
    void OnIncludeProjectTokensChanged(bool include);
    void OnSaveRequested();
    void OnCloseRequested();
    void OnUserPathsChanged(const QStringList& paths);
    void OnUserPathsFirstChanged(bool userPathsFirst);
    void OnUserPathsOnlyChanged(bool userPathsOnly);

    std::vector<std::string>                                              userSearchPaths;
    std::optional<std::reference_wrapper<Adsk::AssetResolverContextData>> userDataExt;
    std::string                                                           mappingFilePath;
    bool                                                                  includeMayaProjectTokens;
    bool                                                                  userPathsFirst;
    bool                                                                  userPathsOnly;
};
} // namespace MAYAUSD_NS_DEF
#endif
