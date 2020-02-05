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

#include <mayaUsd/ui/api.h>

#include <QtWidgets/QHeaderView>

MAYAUSD_NS_DEF {

/**
 * \brief Override the default Qt header view so we can draw a checkbox in
 * the header for the import column.
 */
class MAYAUSD_UI_PUBLIC TreeHeader : public QHeaderView
{
public:
	using ParentClass = QHeaderView;

	//! \brief Constructor
	TreeHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

	//! brief Destructor.
	virtual ~TreeHeader() = default;

protected:
	void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
	void mousePressEvent(QMouseEvent *e) override;

private:
	bool fChecked;
};

} // namespace MayaUsd
