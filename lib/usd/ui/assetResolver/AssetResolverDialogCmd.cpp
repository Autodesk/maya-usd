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
#include "AssetResolverDialogCmd.h"

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/variantSets.h>

#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsdUI/ui/PreferencesManagement.h>
#include <mayaUsdUI/ui/USDQtUtil.h>

#include <maya/MFnPlugin.h>

#include <AssetResolverExtensions/PathDialog/PathDialog.h>
#include <QtCore/QPointer>
#include <QtCore/QVariant>
#include <QtGui/QCursor>
#include <QtWidgets/QApplication>

#if ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY
#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>

#include <AdskAssetResolver/Notice.h>

#include <memory>
#endif

namespace MAYAUSD_NS_DEF {

const MString AssetResolverDialogCmd::name("assetResolverDialog");

namespace {

QPointer<Adsk::AssetResolverPathDialog> g_assetResolverDialog;

constexpr auto kTabFlag = "-tab";
constexpr auto kTabFlagLong = "-tabName";
constexpr auto kPathsTabName = "paths";
constexpr auto kSettingsTabName = "globalSettings";

constexpr auto kProxyShapeFlag = "-ps";
constexpr auto kProxyShapeFlagLong = "-proxyShape";

MString parseTextArg(const MArgParser& argData, const char* flag, const MString& defaultValue)
{
    MString value = defaultValue;
    if (argData.isFlagSet(flag))
        argData.getFlagArgument(flag, 0, value);
    return value;
}

#if ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY
// Reloads Maya-owned USD stages on Adsk resolver context-data changes.
// Adsk::SendContextDataChanged() walks pxr::UsdUtilsStageCache, which Maya
// does not populate (stages live in UsdStageMap), so without this listener
// an "Apply" leaves stages composed against the stale resolver context.
// Hooked to ArContextDataChangeCompleted so we run after every
// AdskResolverContext has refreshed its merged data, and inside
// SendContextDataChanged's PreventContextDataChangedNotification scope so
// our Reload() cannot trigger a re-entrant notification.
class ContextDataChangedListener : public PXR_NS::TfWeakBase
{
public:
    ContextDataChangedListener()
    {
        m_key = PXR_NS::TfNotice::Register(
            PXR_NS::TfCreateWeakPtr(this),
            &ContextDataChangedListener::onContextDataChangeCompleted);
    }

    ~ContextDataChangedListener() { PXR_NS::TfNotice::Revoke(m_key); }

    ContextDataChangedListener(const ContextDataChangedListener&) = delete;
    ContextDataChangedListener& operator=(const ContextDataChangedListener&) = delete;

private:
    void onContextDataChangeCompleted(const Adsk::ArContextDataChangeCompleted&)
    {
        // Same call the AE refresh button makes; ArNotice::ResolverChanged
        // has already been sent by AdskResolverContext::UpdateMergedData,
        // so Reload() recomposes against the new resolver state.
        for (const auto& weakStage : ufe::UsdStageMap::getInstance().allStages()) {
            if (PXR_NS::UsdStageRefPtr stage { weakStage }) {
                stage->Reload();
            }
        }
    }

    PXR_NS::TfNotice::Key m_key;
};

std::unique_ptr<ContextDataChangedListener> g_contextDataChangedListener;
#endif

} // namespace

/*static*/
MStatus AssetResolverDialogCmd::initialize(MFnPlugin& plugin)
{
#if ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY
    // Reload Maya stages on resolver context-data changes; see listener docstring.
    if (!g_contextDataChangedListener) {
        g_contextDataChangedListener = std::make_unique<ContextDataChangedListener>();
    }
#endif
    return plugin.registerCommand(
        name, AssetResolverDialogCmd::creator, AssetResolverDialogCmd::createSyntax);
}

/*static*/
MStatus AssetResolverDialogCmd::finalize(MFnPlugin& plugin)
{
    // Destroy the dialog synchronously here (rather than relying on deleteLater
    // via close()) so that its captured functors, which point into this
    // plugin's binary, cannot fire after the plugin is unloaded.
    if (g_assetResolverDialog) {
        delete g_assetResolverDialog.data();
        g_assetResolverDialog.clear();
    }
#if ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY
    // Revoke before plugin binary unload so a late notice can't dispatch into freed code.
    g_contextDataChangedListener.reset();
#endif
    return plugin.deregisterCommand(name);
}
void* AssetResolverDialogCmd::creator() { return new AssetResolverDialogCmd(); }

MStatus AssetResolverDialogCmd::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);

    if (st) {
        const MString tabName = parseTextArg(argData, kTabFlag, kPathsTabName);
        const MString proxyShapePath = parseTextArg(argData, kProxyShapeFlag, MString());

        if (!g_assetResolverDialog) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            g_assetResolverDialog = new Adsk::AssetResolverPathDialog(MQtUtil::mainWindow());
            // Intentionally do NOT set Qt::WA_DeleteOnClose. That attribute
            // schedules deletion via deleteLater(), which races with:
            //   * the user reinvoking this command before the deferred delete
            //     runs (g_assetResolverDialog is still non-null until ~QObject
            //     actually starts, so we'd re-show a dialog that is queued for
            //     destruction);
            //   * pending events / signals on child widgets firing during
            //     deferred destruction;
            //   * plugin unload happening before the deferred delete runs,
            //     leaving the captured functors below dangling.
            // Instead we keep the dialog alive across closes (Qt's default
            // closeEvent just hides it) and destroy it explicitly in
            // finalize().

            // Tell Maya to treat this as a Maya-managed window. This is
            // the same mechanism Maya uses internally to keep its own
            // dialogs from going behind the main window. This should not be combined
            // with other flags.
            g_assetResolverDialog->setWindowFlags(Qt::Window);
            g_assetResolverDialog->setProperty("saveWindowPref", QVariant::fromValue(true));

            g_assetResolverDialog->setGetStagesFunctor([]() {
                auto allStages = ufe::UsdStageMap::getInstance().allStages();
                std::vector<PXR_NS::UsdStageRefPtr> stages;
                stages.reserve(allStages.size());
                for (const auto& weakStage : allStages) {
                    if (PXR_NS::UsdStageRefPtr stage { weakStage }) {
                        stages.push_back(stage);
                    }
                }
                return stages;
            });

            g_assetResolverDialog->setSettingsAppliedFunctor(
                PreferencesManagement::SaveUsdPreferences);

            QApplication::restoreOverrideCursor();
        }

        // If a proxy shape was provided, resolve it to a stage and select it
        // in the dialog. If resolution fails, leave whatever the dialog
        // currently has selected untouched.
        if (proxyShapePath.length() > 0) {
            if (auto stage = UsdMayaUtil::GetStageByProxyName(proxyShapePath.asChar())) {
                g_assetResolverDialog->setCurrentStage(stage);
            }
        }

        g_assetResolverDialog->setCurrentTab(
            tabName == kSettingsTabName ? Adsk::AssetResolverPathDialog::Tab::GlobalSettings
                                        : Adsk::AssetResolverPathDialog::Tab::Paths);

        // If the dialog was previously minimized, restore it before showing.
        if (g_assetResolverDialog->isMinimized()) {
            g_assetResolverDialog->setWindowState(
                (g_assetResolverDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        }

        g_assetResolverDialog->show();
        g_assetResolverDialog->raise();
        g_assetResolverDialog->activateWindow();

        return MS::kSuccess;
    }

    return MS::kInvalidParameter;
}

MSyntax AssetResolverDialogCmd::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(true);
    syntax.enableEdit(false);
    syntax.addFlag(kTabFlag, kTabFlagLong, MSyntax::kString);
    syntax.addFlag(kProxyShapeFlag, kProxyShapeFlagLong, MSyntax::kString);

    syntax.setObjectType(MSyntax::kStringObjects, 0, 1);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF