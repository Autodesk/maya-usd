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

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <QtWidgets/QDialog>

namespace Adsk {
class USDAssetResolverSettingsWidget;
}

namespace MAYAUSD_NS_DEF {

class UsdPreferenceOptions;

class MAYAUSD_UI_PUBLIC USDAssetResolverDialog : public QDialog
{
    Q_OBJECT
public:
    USDAssetResolverDialog(QWidget* parent);
    ~USDAssetResolverDialog();
    bool execute();

    /// Get the options from the dialog UI
    const UsdPreferenceOptions getOptions() const;

protected:
    /// Load the options into the dialog UI
    void loadOptions(const UsdPreferenceOptions& options);

    void OnSaveRequested();
    void OnCloseRequested();

    Adsk::USDAssetResolverSettingsWidget* settingsWidget { nullptr };
};
} // namespace MAYAUSD_NS_DEF
#endif
