//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
//----------------------------------------------------------------------------------------------------------------------
/// \file   Metadata.h
/// \brief  This file contains the tokens for the USDMaya metadata.
/// \code
//----------------------------------------------------------------------------------------------------------------------
#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

struct Metadata{
  static const TfToken transformType;
  static const TfToken excludeFromProxyShape;
  static const TfToken importAsNative;
};

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
