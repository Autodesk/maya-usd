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

#include "../api.h"

#include <QtCore/QThread>

#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

class TreeModel;


/**
 * \brief Thread used to identify specific USD Prims within a provided USD Stage.
 */
class MAYAUSD_UI_PUBLIC USDSearchThread : public QThread
{
	Q_OBJECT

public:
	/**
	 * \brief Constructor.
	 * \param stage A reference to the USD Stage in which to perform the search.
	 * \param searchFilter The search filter against which to try and match USD Prims in the given USD Stage.
	 * \param parent A reference to the parent of the thread.
	 */
	explicit USDSearchThread(
			const UsdStageRefPtr& stage, const std::string& searchFilter, QObject* parent = nullptr);

	/**
	 * \brief Consume the TreeModel built from the results of the search performed within the USD Stage.
	 * \return The TreeModel built from the results of the search performed within the USD Stage.
	 */
	std::unique_ptr<TreeModel> consumeResults();

protected:
	/**
	 * \brief Perform the search in the Qt thread.
	 */
	void run() override;

protected:
	// Reference to the USD Stage within which to perform the search:
	UsdStageRefPtr fStage;
	// Search filter against which to try and match USD Prims in the USD Stage:
	std::string fSearchFilter;
	// Reference to the TreeModel built from the search performed within the USD Stage:
	std::unique_ptr<TreeModel> fResults;
};

} // namespace MayaUsd
