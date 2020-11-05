//
// Copyright 2019 Autodesk
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

#ifndef MAYAUSDUI_USD_IMPORT_DIALOG_H
#define MAYAUSDUI_USD_IMPORT_DIALOG_H

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/IUSDImportView.h>
#include <mayaUsdUI/ui/ItemDelegate.h>
#include <mayaUsdUI/ui/TreeModel.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/stage.h>

#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QDialog>

#include <memory>

namespace Ui {
class ImportDialog;
}

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class IMayaMQtUtil;

/**
 * \brief USD file import dialog.
 */
class MAYAUSD_UI_PUBLIC USDImportDialog
    : public QDialog
    , public IUSDImportView
{
    Q_OBJECT

public:
    /**
     * \brief Constructor.
     * \param filename Absolute file path of a USD file to import.
     * \param parent A reference to the parent widget of the dialog.
     */
    explicit USDImportDialog(
        const std::string&  filename,
        const ImportData*   importData,
        const IMayaMQtUtil& mayaQtUtil,
        QWidget*            parent = nullptr);

    //! Destructor.
    ~USDImportDialog();

    // IUSDImportView overrides
    const std::string&                filename() const override;
    const std::string&                rootPrimPath() const override;
    UsdStagePopulationMask            stagePopulationMask() const override;
    UsdStage::InitialLoadSet          stageInitialLoadSet() const override;
    ImportData::PrimVariantSelections primVariantSelections() const override;
    bool                              execute() override;

    int primsInScopeCount() const;
    int switchedVariantCount() const;

private Q_SLOTS:
    void onItemClicked(const QModelIndex&);
    void onResetFileTriggered();
    void onHierarchyViewHelpTriggered();
    void onCheckedStateChanged(int);
    void onModifiedVariantsChanged(int);

protected:
    // Reference to the Qt UI View of the dialog:
    std::unique_ptr<Ui::ImportDialog> fUI;

    // Reference to the Model holding the structure of the USD file hierarchy:
    std::unique_ptr<TreeModel> fTreeModel;
    // Reference to the Proxy Model used to sort and filter the USD file hierarchy:
    std::unique_ptr<QSortFilterProxyModel> fProxyModel;
    // Reference to the delegate we set on the tree view:
    std::unique_ptr<ItemDelegate> fItemDelegate;

    // Reference to the USD Stage holding the list of Prims which could be imported:
    UsdStageRefPtr fStage;

    // The filename for the USD stage we opened.
    std::string fFilename;

    // The root prim path.
    mutable std::string fRootPrimPath;
};

} // namespace MAYAUSD_NS_DEF

#endif
