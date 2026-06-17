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

#ifndef MAYAUSD_UI_RENDERSETUP_MAYAEDITCOMMITTER_H
#define MAYAUSD_UI_RENDERSETUP_MAYAEDITCOMMITTER_H

#include <pxr/usd/usd/stage.h>

#include <QtCore/QObject>
#include <RenderSetup/HostStage.h>
#include <RenderSetup/IEditCommitter.h>

#include <functional>
#include <string>
#include <vector>

namespace MayaUsdRenderSetup {

//! MayaUSD implementation of Adsk::IEditCommitter for the Render Setup UI.
//! Captures USD edits through UsdUfe and flushes them onto the Maya undo stack
//! via MayaUsdUndoBlock.
class MayaEditCommitter
    : public QObject
    , public Adsk::IEditCommitter
{
    Q_OBJECT
public:
    explicit MayaEditCommitter(QObject* parent = nullptr);

    void setStages(const std::vector<Adsk::HostStage>& stages);

    void commit(const std::string& undoLabel, std::function<void()> doEdit) override;
    bool isLocalEditInFlight() const override { return m_inFlightCount > 0; }

private:
    std::vector<PXR_NS::UsdStageRefPtr> m_stages;
    int                                 m_inFlightCount { 0 };
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSD_UI_RENDERSETUP_MAYAEDITCOMMITTER_H
