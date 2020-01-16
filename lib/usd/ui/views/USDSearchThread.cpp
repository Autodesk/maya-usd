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

#include "USDSearchThread.h"
#include "TreeModel.h"
#include "factories/TreeModelFactory.h"

MAYAUSD_NS_DEF {

USDSearchThread::USDSearchThread(const UsdStageRefPtr& stage, const std::string& searchFilter, QObject* parent)
		: QThread{ parent }
		, fStage{ stage }
		, fSearchFilter{ searchFilter }
{
}

std::unique_ptr<TreeModel> USDSearchThread::consumeResults()
{
	return std::move(fResults);
}

void USDSearchThread::run()
{
	fResults = TreeModelFactory::createFromSearch(fStage, fSearchFilter);
}

} // namespace MayaUsd
