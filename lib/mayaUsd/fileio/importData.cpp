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
#include "importData.h"

#include <type_traits>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// ImportData:
//------------------------------------------------------------------------------

constexpr const char* kRootPrimPath = "/";

ImportData::ImportData()
	: fLoadSet(UsdStage::InitialLoadSet::LoadAll)
	, fRootPrimPath(kRootPrimPath)
	, fPrimsInScopeCount(0)
	, fSwitchedVariantCount(0)
{
}

ImportData::ImportData(const std::string& f)
	: fLoadSet(UsdStage::InitialLoadSet::LoadAll)
	, fRootPrimPath(kRootPrimPath)
	, fFilename(f)
	, fPrimsInScopeCount(0)
	, fSwitchedVariantCount(0)	
{
}

/*static*/
ImportData& ImportData::instance()
{
	static ImportData sImportData;
	return sImportData;
}

/*static*/
const ImportData& ImportData::cinstance()
{
	return instance();
}

void ImportData::clearData()
{
	fLoadSet = UsdStage::InitialLoadSet::LoadAll;
	UsdStagePopulationMask tmpPopMask;
	fPopMask.swap(tmpPopMask);
	fRootVariants.clear();
	fPrimVariants.clear();
	fFilename.clear();
	fRootPrimPath = kRootPrimPath;
	fPrimsInScopeCount = 0;
	fSwitchedVariantCount =0;	
}

bool ImportData::empty() const
{
	// If we don't have a filename set then we are empty.
	return fFilename.empty();
}

const std::string& ImportData::filename() const
{
	return fFilename;
}

void ImportData::setFilename(const std::string& f)
{
	// If the input filename doesn't match what we have stored (empty or not) we
	// clear the data because it doesn't belong to the new file.
	if (fFilename != f)
		clearData();
	fFilename = f;
}

const std::string& ImportData::rootPrimPath() const
{
	return fRootPrimPath;
}

void ImportData::setRootPrimPath(const std::string& primPath)
{
	fRootPrimPath = primPath;
}

bool ImportData::hasPopulationMask() const
{
	return !fPopMask.IsEmpty();
}

const UsdStagePopulationMask& ImportData::stagePopulationMask() const
{
	return fPopMask;
}

void ImportData::setStagePopulationMask(const UsdStagePopulationMask& mask)
{
	fPopMask = mask;
}

void ImportData::setStagePopulationMask(UsdStagePopulationMask&& mask)
{
	fPopMask = std::move(mask);
}

UsdStage::InitialLoadSet ImportData::stageInitialLoadSet() const
{
	return fLoadSet;
}

void ImportData::setStageInitialLoadSet(UsdStage::InitialLoadSet loadSet)
{
	fLoadSet = loadSet;
}

bool ImportData::hasVariantSelections() const
{
	return !(fRootVariants.empty() && fPrimVariants.empty());
}

const SdfVariantSelectionMap& ImportData::rootVariantSelections() const
{
	return fRootVariants;
}

const ImportData::PrimVariantSelections& ImportData::primVariantSelections() const
{
	return fPrimVariants;
}

void ImportData::setRootVariantSelections(const SdfVariantSelectionMap& vars)
{
	fRootVariants = vars;
}

void ImportData::setRootVariantSelections(SdfVariantSelectionMap&& vars)
{
	fRootVariants = std::move(vars);
}

void ImportData::setPrimVariantSelections(const PrimVariantSelections& vars)
{
	fPrimVariants = vars;
}

void ImportData::setPrimVariantSelections(PrimVariantSelections&& vars)
{
	fPrimVariants = std::move(vars);
}

void ImportData::setPrimsInScopeCount(int count)
{
	fPrimsInScopeCount = count;
}

void ImportData::setSwitchedVariantCount(int count)
{
	fSwitchedVariantCount = count;
}

int ImportData::primsInScopeCount() const
{
	return fPrimsInScopeCount;
}

int ImportData::switchedVariantCount() const
{
	return fSwitchedVariantCount;
}

} // namespace MayaUsd
