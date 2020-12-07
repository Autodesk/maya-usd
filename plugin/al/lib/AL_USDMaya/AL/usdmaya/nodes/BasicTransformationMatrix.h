//
// Copyright 2019 Animal Logic
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

#include "AL/usdmaya/Api.h"

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/scope.h>

#include <maya/MObjectHandle.h>
#include <maya/MPxTransformationMatrix.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

class Scope;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class provides a very basic transformation matrix that can store a USD Prim and a
/// Maya Node it's able to manipulate. The implementation is very basic, it's also used as an
/// interface for more sophisticated AL_USDMaya transformation matrix implementantations \ingroup
/// nodes
//----------------------------------------------------------------------------------------------------------------------
class BasicTransformationMatrix : public MPxTransformationMatrix
{
public:
    /// \brief  ctor
    AL_USDMAYA_PUBLIC
    BasicTransformationMatrix();

    /// \brief  ctor
    /// \param  prim the USD prim that this matrix should represent
    AL_USDMAYA_PUBLIC
    BasicTransformationMatrix(const UsdPrim& prim);

    /// \brief  dtor
    virtual ~BasicTransformationMatrix() { }

    /// \brief  set the prim that this transformation matrix will read/write to.
    /// \param  prim the prim
    /// \param  scopeNode the owning maya node
    AL_USDMAYA_PUBLIC
    virtual void setPrim(const UsdPrim& prim, Scope* scopeNode);

    /// \brief  sets the MObject for the transform
    /// \param  object the MObject for the custom transform node
    void setMObject(const MObject object) { m_transformNode = object; }

    /// \brief  Is this transform set to write back onto the USD prim, and is it currently possible?
    virtual bool pushToPrimAvailable() const { return false; }

    /// \brief  return the prim this transform matrix is attached to
    /// \return the prim this transform matrix is controlling
    inline const UsdPrim& prim() const { return m_prim; }

    virtual void initialiseToPrim(bool readFromPrim = true, Scope* node = 0) { }

    /// \brief  the type ID of the transformation matrix
    AL_USDMAYA_PUBLIC
    static const MTypeId kTypeId;

    /// \brief  create an instance of  this transformation matrix
    /// \return a new instance of this transformation matrix
    AL_USDMAYA_PUBLIC
    static MPxTransformationMatrix* creator();

protected:
    UsdPrim       m_prim;
    MObjectHandle m_transformNode;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
