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

#pragma once

#include <map>
#include <string>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <mayaUsd/base/api.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

/*!
 *  \brief Singleton class to hold USD UI import data.
*/
class MAYAUSD_CORE_PUBLIC ImportData
{
public:
	//! typedef std::map<std::string, SdfVariantSelectionMap>
	//! \brief Variant selections mapped for prims.
	//! Key = USD prim path, Value = Variant selections
	typedef std::map<SdfPath, SdfVariantSelectionMap> PrimVariantSelections;

	//! \return The import data singleton instance.
	static ImportData& instance();
	static const ImportData& cinstance();

	//! Constructor (allows creating ImportData on stack without singleton instance).
	ImportData();
	ImportData(const std::string& f);

	//@{
	//! No copy or move constructor/assignment.
	ImportData(const ImportData&) = delete;
	ImportData& operator=(const ImportData&) = delete;
	ImportData(ImportData&&) = delete;
	ImportData& operator=(ImportData&&) = delete;
	//@}

	//! Clears all the stored data.
	void clearData();

	//! \return Is this import data empty?
	bool empty() const;

	//! \return The filename associated with this import data.
	const std::string& filename() const;

	//! Set the filename associated with this import data.
	void setFilename(const std::string& f);

	//! \return The root prim path to use when importing.
	const std::string& rootPrimPath() const;

	//! Set the root prim path to use for import.
	void setRootPrimPath(const std::string& primPath);

	//! \return True if the USD population mask is not empty.
	bool hasPopulationMask() const;

	//! \return The USD population mask of the stage to use for import.
	const UsdStagePopulationMask& stagePopulationMask() const;

	//@{
	//! Set the USD population mask of the stage to use for import.
	//! Both 'copy' and 'move' operations are supported. To use the move version
	//! call it with std::move(x).
	void setStagePopulationMask(const UsdStagePopulationMask& mask);
	void setStagePopulationMask(UsdStagePopulationMask&& mask);
	//@}

	//! \return The USD initial load set of the stage to use for import.
	UsdStage::InitialLoadSet stageInitialLoadSet() const;

	//! Set the USD initial load set of the stage to use for import.
	void setStageInitialLoadSet(UsdStage::InitialLoadSet loadSet);

	//! \return True if the USD variant selections is not empty.
	bool hasVariantSelections() const;

	//! \return The USD variant selections (for the root prim) of the stage to use for import.
	const SdfVariantSelectionMap& rootVariantSelections() const;

	//! \return The USD variant selections (for individual prims) of the stage to use for import.
	const PrimVariantSelections& primVariantSelections() const;

	//! Set the USD variant selections (for the root prim) of the stage to use for import.
	//! Both 'copy' and 'move' operations are supported. To use the move version
	//! call it with std::move(x).
	void setRootVariantSelections(const SdfVariantSelectionMap& vars);
	void setRootVariantSelections(SdfVariantSelectionMap&& vars);

	//! Set the USD variant selections (for individual prims) of the stage to use for import.
	//! Both 'copy' and 'move' operations are supported. To use the move version
	//! call it with std::move(x).
	void setPrimVariantSelections(const PrimVariantSelections& vars);
	void setPrimVariantSelections(PrimVariantSelections&& vars);

	//@{
	//! Set and get the number of prims to be imported and the number of prims within that scope
	//! that had a variant changed from what is currently set in the USD file.
	//! These values are stored here as a way of communicating choices made between the various
	//! Import options UI, and are used for display purposes only.
	void setPrimsInScopeCount(int count);
	void setSwitchedVariantCount(int count);
	int primsInScopeCount() const;
	int switchedVariantCount() const;
	//@}

private:
	UsdStagePopulationMask		fPopMask;
	UsdStage::InitialLoadSet	fLoadSet;
	SdfVariantSelectionMap		fRootVariants;
	PrimVariantSelections		fPrimVariants;
	std::string					fRootPrimPath;
	std::string					fFilename;

	int							fPrimsInScopeCount;
	int							fSwitchedVariantCount;
};

} // namespace MayaUsd
