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

#include "TreeHeader.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

MAYAUSD_NS_DEF {

TreeHeader::TreeHeader(Qt::Orientation orientation, QWidget *parent /*= nullptr*/)
	: ParentClass(orientation, parent)
	, fChecked(false)
{
	setSectionsMovable(false);
	setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void TreeHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
	// Draw the base class first (will draw frame + label).
	painter->save();
	ParentClass::paintSection(painter, rect, logicalIndex);
	painter->restore();

	// Only paint the checkbox if the model returned a valid QVariant (see TreeModel::headerData).
	QVariant headerCheckState = model()->headerData(logicalIndex, Qt::Horizontal, Qt::CheckStateRole);
	if (headerCheckState.isValid())
	{
		QStyleOptionHeader optHdr;
		initStyleOption(&optHdr);
		optHdr.rect = rect;
		QRect rcCheckBox = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &optHdr, this);
		rcCheckBox.translate(3, 0);

		QStyleOptionButton optBtn;
		optBtn.initFrom(this);
		optBtn.rect = rcCheckBox;
		optBtn.state = fChecked ? QStyle::State_On : QStyle::State_Off;
		style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &optBtn, painter);
	}
}

void TreeHeader::mousePressEvent(QMouseEvent *e)
{
	// Only toggle checkstate if mouse press occurred inside of checkbox.
	QStyleOptionHeader optHdr;
	initStyleOption(&optHdr);
	QRect rcCheckBox = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &optHdr, this);
	rcCheckBox.translate(3, 0);
	if (rcCheckBox.contains(e->pos()))
	{
		fChecked = !fChecked;
		int ind = visualIndexAt(e->pos().x());
		headerDataChanged(orientation(), ind, ind);
		this->update();
		return;
	}

	ParentClass::mousePressEvent(e);
}

} // namespace MayaUsd
