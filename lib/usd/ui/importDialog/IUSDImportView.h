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

#ifndef MAYAUSDUI_I_USD_IMPORT_VIEW_H
#define MAYAUSDUI_I_USD_IMPORT_VIEW_H

#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

/**
 * \brief USD file import dialog.
 */
class MAYAUSD_UI_PUBLIC IUSDImportView
{
public:
    /**
     * \brief Destructor.
     */
    virtual ~IUSDImportView() = 0;

    //! \return The filename associated with this import view.
    virtual const std::string& filename() const = 0;

    //! \return The root prim of the stage to use for import.
    virtual const std::string& rootPrimPath() const = 0;

    //! \return The USD population mask of the stage to use for import.
    virtual UsdStagePopulationMask stagePopulationMask() const = 0;

    //! \return The USD initial load set of the stage to use for import.
    virtual UsdStage::InitialLoadSet stageInitialLoadSet() const = 0;

    //! \return The USD variant selections (that were modified) to use for import.
    virtual ImportData::PrimVariantSelections primVariantSelections() const = 0;

    /**
     * \brief Display the view.
     * \return True if the user applied the changes, false if they canceled.
     */
    virtual bool execute() = 0;
};

} // namespace MAYAUSD_NS_DEF

#endif
