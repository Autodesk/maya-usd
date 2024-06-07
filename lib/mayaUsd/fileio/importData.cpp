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

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(ImportData);

//------------------------------------------------------------------------------
// ImportData:
//------------------------------------------------------------------------------

constexpr const char* kRootPrimPath = "/";

ImportData::ImportData()
    : _loadSet(UsdStage::InitialLoadSet::LoadAll)
    , _rootPrimPath(kRootPrimPath)
    , _primsInScopeCount(0)
    , _switchedVariantCount(0)
    , _applyEulerFilter(false)
{
}

ImportData::ImportData(const std::string& f)
    : _loadSet(UsdStage::InitialLoadSet::LoadAll)
    , _rootPrimPath(kRootPrimPath)
    , _filename(f)
    , _primsInScopeCount(0)
    , _switchedVariantCount(0)
    , _applyEulerFilter(false)
{
}

/*static*/
ImportData& ImportData::instance()
{
    static ImportData sImportData;
    return sImportData;
}

/*static*/
const ImportData& ImportData::cinstance() { return instance(); }

void ImportData::clearData()
{
    _loadSet = UsdStage::InitialLoadSet::LoadAll;
    UsdStagePopulationMask tmpPopMask;
    _popMask.swap(tmpPopMask);
    _rootVariants.clear();
    _primVariants.clear();
    _filename.clear();
    _rootPrimPath = kRootPrimPath;
    _primsInScopeCount = 0;
    _switchedVariantCount = 0;
}

bool ImportData::empty() const
{
    // If we don't have a filename set then we are empty.
    return _filename.empty();
}

const std::string& ImportData::filename() const { return _filename; }

void ImportData::setFilename(const std::string& f)
{
    // If the input filename doesn't match what we have stored (empty or not) we
    // clear the data because it doesn't belong to the new file.
    if (_filename != f)
        clearData();
    _filename = f;
}

const std::string& ImportData::rootPrimPath() const { return _rootPrimPath; }

void ImportData::setRootPrimPath(const std::string& primPath) { _rootPrimPath = primPath; }

bool ImportData::hasPopulationMask() const { return !_popMask.IsEmpty(); }

void ImportData::setApplyEulerFilter(bool value) { _applyEulerFilter = value; }

bool ImportData::applyEulerFilter() const { return _applyEulerFilter; }

const UsdStagePopulationMask& ImportData::stagePopulationMask() const { return _popMask; }

void ImportData::setStagePopulationMask(const UsdStagePopulationMask& mask) { _popMask = mask; }

void ImportData::setStagePopulationMask(UsdStagePopulationMask&& mask)
{
    _popMask = std::move(mask);
}

UsdStage::InitialLoadSet ImportData::stageInitialLoadSet() const { return _loadSet; }

void ImportData::setStageInitialLoadSet(UsdStage::InitialLoadSet loadSet) { _loadSet = loadSet; }

bool ImportData::hasVariantSelections() const
{
    return !(_rootVariants.empty() && _primVariants.empty());
}

const SdfVariantSelectionMap& ImportData::rootVariantSelections() const { return _rootVariants; }

const ImportData::PrimVariantSelections& ImportData::primVariantSelections() const
{
    return _primVariants;
}

void ImportData::setRootVariantSelections(const SdfVariantSelectionMap& vars)
{
    _rootVariants = vars;
}

void ImportData::setRootVariantSelections(SdfVariantSelectionMap&& vars)
{
    _rootVariants = std::move(vars);
}

void ImportData::setPrimVariantSelections(const PrimVariantSelections& vars)
{
    _primVariants = vars;
}

void ImportData::setPrimVariantSelections(PrimVariantSelections&& vars)
{
    _primVariants = std::move(vars);
}

void ImportData::setPrimsInScopeCount(int count) { _primsInScopeCount = count; }

void ImportData::setSwitchedVariantCount(int count) { _switchedVariantCount = count; }

int ImportData::primsInScopeCount() const { return _primsInScopeCount; }

int ImportData::switchedVariantCount() const { return _switchedVariantCount; }

} // namespace MAYAUSD_NS_DEF
