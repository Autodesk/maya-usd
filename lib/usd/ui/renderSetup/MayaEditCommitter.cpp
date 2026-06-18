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

#include "MayaEditCommitter.h"

#include <mayaUsd/undo/MayaUsdUndoBlock.h>

#include <usdUfe/undo/UsdUndoUtils.h>

#include <pxr/usd/sdf/changeBlock.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <QtCore/QString>
#include <QtCore/QTimer>

namespace {

// Maya does not support spaces in undo chunk names. Backslash and double-quote
// are escaped so the label is safe to embed in a MEL double-quoted string.
MString cleanChunkName(const std::string& label)
{
    QString name = QString::fromStdString(label);
    name.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    name.replace(QLatin1Char('"'), QLatin1String("\\\""));
    name.replace(QLatin1Char(' '), QLatin1Char('_'));
    return MString(("\"" + name.toStdString() + "\"").c_str());
}

void openUndoChunk(const std::string& label)
{
    if (label.empty()) {
        MGlobal::executeCommand("undoInfo -openChunk", false, false);
    } else {
        MGlobal::executeCommand(
            MString("undoInfo -openChunk -chunkName ") + cleanChunkName(label), false, false);
    }
}

struct UndoChunkGuard
{
    ~UndoChunkGuard() { MGlobal::executeCommand("undoInfo -closeChunk", false, false); }
};

} // namespace

namespace MayaUsdRenderSetup {

MayaEditCommitter::MayaEditCommitter(QObject* parent)
    : QObject(parent)
{
}

void MayaEditCommitter::setStages(const std::vector<Adsk::HostStage>& stages)
{
    m_stages.clear();
    m_stages.reserve(stages.size());
    for (const auto& hostStage : stages) {
        if (hostStage.stage) {
            m_stages.push_back(hostStage.stage);
        }
    }
}

void MayaEditCommitter::commit(const std::string& undoLabel, std::function<void()> doEdit)
{
    if (!doEdit || m_stages.empty()) {
        return;
    }

    ++m_inFlightCount;
    // Schedule the decrement before doEdit() so it fires even if doEdit() throws.
    // The one-event-loop-cycle delay keeps isLocalEditInFlight() true through the
    // USD-notice burst that follows the edit, so the notice bridge treats the
    // resulting refresh as self-originated and skips re-entrancy.
    QTimer::singleShot(0, this, [this]() {
        if (m_inFlightCount > 0) {
            --m_inFlightCount;
        }
    });

    UsdUfe::trackStagesEditTargets(m_stages);

    openUndoChunk(undoLabel);
    const UndoChunkGuard      undoChunkGuard;
    MayaUsd::MayaUsdUndoBlock block;
    {
        PXR_NS::SdfChangeBlock changeBlock;
        doEdit();
    }
}

} // namespace MayaUsdRenderSetup
