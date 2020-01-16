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

#include "../base/api.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>

#include <map>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

/*!
 *  \brief Singleton class to hold USD UI import data.
*/
class MAYAUSD_CORE_PUBLIC ImportData
{
public:
	//! typedef std::map<std::string, std::string>
	//! \brief Variant selections as a map of strings.
	//! Key = variant set name, Value = variant selection
	typedef std::map<std::string, std::string> VariantSelections;

	//! typedef std::map<std::string, VariantSelections>
	//! \brief Variant selections mapped for prims.
	//! Key = USD prim path, Value = Variant selections
	typedef std::map<SdfPath, VariantSelections> PrimVariantSelections;

	//! \return The import data singleton instance.
	static ImportData& instance();

	//! Constructor (allows creating ImportData on stack without singleton instance).
	ImportData();

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
	std::string filename() const;

	//! Set the filename associated with this import data.
	void setFilename(const std::string& f);

	//! \return The root prim path to use when importing.
	std::string rootPrimPath() const;

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
	const VariantSelections& rootVariantSelections() const;

	//! \return The USD variant selections (for individual prims) of the stage to use for import.
	const PrimVariantSelections& primVariantSelections() const;

	//! Set the USD variant selections (for the root prim) of the stage to use for import.
	//! Both 'copy' and 'move' operations are supported. To use the move version
	//! call it with std::move(x).
	void setRootVariantSelections(const VariantSelections& vars);
	void setRootVariantSelections(VariantSelections&& vars);

	//! Set the USD variant selections (for individual prims) of the stage to use for import.
	//! Both 'copy' and 'move' operations are supported. To use the move version
	//! call it with std::move(x).
	void setPrimVariantSelections(const PrimVariantSelections& vars);
	void setPrimVariantSelections(PrimVariantSelections&& vars);

private:
	UsdStagePopulationMask		fPopMask;
	UsdStage::InitialLoadSet	fLoadSet;
	VariantSelections			fRootVariants;
	PrimVariantSelections		fPrimVariants;
	std::string					fRootPrimPath;
	std::string					fFilename;
};

} // namespace MayaUsd
