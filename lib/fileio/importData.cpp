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

#include "importData.h"

#include <type_traits>

MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// ImportData:
//------------------------------------------------------------------------------

constexpr const char* kRootPrimPath = "/";

ImportData::ImportData()
	: fLoadSet(UsdStage::InitialLoadSet::LoadAll)
	, fRootPrimPath(kRootPrimPath)
{
}

/*static*/
ImportData& ImportData::instance()
{
	static ImportData sImportData;
	return sImportData;
}

void ImportData::clearData()
{
	fLoadSet = UsdStage::InitialLoadSet::LoadAll;
	UsdStagePopulationMask tmpPopMask;
	fPopMask.swap(tmpPopMask);
	fRootVariants.clear();
	fPrimVariants.clear();
	fFilename.clear();
	fRootPrimPath.clear();
}

bool ImportData::empty() const
{
	// If we don't have a filename set then we are empty.
	return fFilename.empty();
}

std::string ImportData::filename() const
{
	return fFilename;
}

void ImportData::setFilename(const std::string& f)
{
	fFilename = f;
}

std::string ImportData::rootPrimPath() const
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
	static_assert(std::is_move_assignable<UsdStagePopulationMask>::value, "UsdStagePopulationMask is not move enabled");
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
	return !(fRootVariants.empty() || fPrimVariants.empty());
}

const ImportData::VariantSelections& ImportData::rootVariantSelections() const
{
	return fRootVariants;
}

const ImportData::PrimVariantSelections& ImportData::primVariantSelections() const
{
	return fPrimVariants;
}

void ImportData::setRootVariantSelections(const VariantSelections& vars)
{
	fRootVariants = vars;
}

void ImportData::setRootVariantSelections(VariantSelections&& vars)
{
	static_assert(std::is_move_assignable<VariantSelections>::value, "VariantSelections is not move enabled");
	fRootVariants = std::move(vars);
}

void ImportData::setPrimVariantSelections(const PrimVariantSelections& vars)
{
	fPrimVariants = vars;
}

void ImportData::setPrimVariantSelections(PrimVariantSelections&& vars)
{
	static_assert(std::is_move_assignable<PrimVariantSelections>::value, "PrimVariantSelections is not move enabled");
	fPrimVariants = std::move(vars);
}

} // namespace MayaUsd
