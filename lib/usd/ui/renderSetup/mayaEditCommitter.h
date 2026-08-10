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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYAEDITCOMMITTER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYAEDITCOMMITTER_H

#include <pxr/usd/usd/stage.h>

#include <AdskUsdRenderSetup/HostStage.h>
#include <AdskUsdRenderSetup/IEditCommitter.h>
#include <QtCore/QObject>

#include <functional>
#include <string>
#include <vector>

namespace MayaUsdRenderSetup {

//! MayaUSD implementation of AdskUsdRenderSetup::IEditCommitter for the Render Setup UI.
//! Captures USD edits through UsdUfe and flushes them onto the Maya undo stack
//! via MayaUsdUndoBlock.
class MayaEditCommitter
    : public QObject
    , public AdskUsdRenderSetup::IEditCommitter
{
    Q_OBJECT
public:
    explicit MayaEditCommitter(QObject* parent = nullptr);

    void setStages(const std::vector<AdskUsdRenderSetup::HostStage>& stages);

    void commit(const std::string& undoLabel, std::function<void()> doEdit) override;

private:
    std::vector<PXR_NS::UsdStageRefPtr> _stages;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYAEDITCOMMITTER_H
