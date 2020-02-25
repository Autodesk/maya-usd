//
// Copyright 2020 Autodesk
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

#include "USDQtUtil.h"

#include <maya/MQtUtil.h>

#include <QtGui/QPixmap>

MAYAUSD_NS_DEF {

USDQtUtil::~USDQtUtil()
{
}

int USDQtUtil::dpiScale(int size) const
{
	return MQtUtil::dpiScale(size);
}

float USDQtUtil::dpiScale(float size) const
{
	return MQtUtil::dpiScale(size);
}

QPixmap* USDQtUtil::createPixmap(const std::string& imageName) const
{
	return MQtUtil::createPixmap(imageName.c_str());
}

} // namespace MayaUsd
