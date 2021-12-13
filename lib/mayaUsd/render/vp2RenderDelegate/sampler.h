//
// Copyright 2016 Pixar
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
// Modifications copyright (C) 2019 Autodesk
//
#ifndef HD_VP2_SAMPLER_H
#define HD_VP2_SAMPLER_H

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4h.h>
#include <pxr/base/gf/vec4i.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/vtBufferSource.h>
#include <pxr/pxr.h>

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  A utility class that helps map between C++ types and Hd type tags.
    \class  HdVP2TypeHelper
*/
class HdVP2TypeHelper
{
public:
    //! Define a type that can hold one sample of any primvar.
    using PrimvarTypeContainer = char[sizeof(GfMatrix4d)];

    /*! \brief  Return the HdTupleType corresponding to the given C++ type.
     */
    template <typename T> static HdTupleType GetTupleType();
};

// Define template specializations of HdVP2TypeHelper methods for
// all our supported types...
#define TYPE_HELPER(T, type)                                          \
    template <> inline HdTupleType HdVP2TypeHelper::GetTupleType<T>() \
    {                                                                 \
        return HdTupleType { type, 1 };                               \
    }

TYPE_HELPER(bool, HdTypeBool)
TYPE_HELPER(char, HdTypeInt8)
TYPE_HELPER(short, HdTypeInt16)
TYPE_HELPER(unsigned short, HdTypeUInt16)
TYPE_HELPER(int, HdTypeInt32)
TYPE_HELPER(GfVec2i, HdTypeInt32Vec2)
TYPE_HELPER(GfVec3i, HdTypeInt32Vec3)
TYPE_HELPER(GfVec4i, HdTypeInt32Vec4)
TYPE_HELPER(unsigned int, HdTypeUInt32)
TYPE_HELPER(float, HdTypeFloat)
TYPE_HELPER(GfVec2f, HdTypeFloatVec2)
TYPE_HELPER(GfVec3f, HdTypeFloatVec3)
TYPE_HELPER(GfVec4f, HdTypeFloatVec4)
TYPE_HELPER(double, HdTypeDouble)
TYPE_HELPER(GfVec2d, HdTypeDoubleVec2)
TYPE_HELPER(GfVec3d, HdTypeDoubleVec3)
TYPE_HELPER(GfVec4d, HdTypeDoubleVec4)
TYPE_HELPER(GfMatrix4f, HdTypeFloatMat4)
TYPE_HELPER(GfMatrix4d, HdTypeDoubleMat4)
TYPE_HELPER(GfVec4h, HdTypeHalfFloatVec4)
#undef TYPE_HELPER

/*! \brief  A utility class that knows how to sample an element from a type-tagged
            buffer (like HdVtBufferSource).
    \class HdVP2BufferSampler

    This class provides templated accessors to let the caller directly get the
    final sample type; it also does bounds checks and type checks.
*/
class HdVP2BufferSampler
{
public:
    HdVP2BufferSampler(HdVtBufferSource const& buffer);

    bool Sample(int index, void* value, HdTupleType dataType) const;

    // Convenient, templated frontend for Sample().
    template <typename T> bool Sample(int index, T* value) const
    {
        return Sample(index, static_cast<void*>(value), HdVP2TypeHelper::GetTupleType<T>());
    }

private:
    HdVtBufferSource const& _buffer; //!< Buffer source to sample
};

/*! \brief  An abstract base class that knows how to sample a primvar.
    \class  HdVP2PrimvarSampler

    An abstract base class that knows how to sample a primvar signal given
    a ray hit coordinate: an <element, u, v> tuple. It provides templated
    accessors, but derived classes are responsible for implementing appropriate
    sampling or interpolation modes.
*/
class HdVP2PrimvarSampler
{
public:
    //! Default constructor.
    HdVP2PrimvarSampler() = default;
    //! Default destructor.
    virtual ~HdVP2PrimvarSampler() = default;

    /*! \brief  Sample the primvar at element index and local basis coordinates.

        Sample the primvar at element index \p index and local basis coordinates
        \p u and \p v, writing the sample to \p value.  Interpret \p value as
        having arity \p numComponents, each of type \p componentType. These
        parameters may not match the datatype declaration of the underlying
        buffer.

        Derived classes are responsible for implementing sampling logic for
        their particular interpolation modes. Sample returns true if a value
        was successfully retrieved.

        \param element The element index to sample.
        \param u The u coordinate to sample.
        \param v The v coordinate to sample.
        \param value The memory to write the value to (only written on success).
        \param dataType The HdTupleType describing element values.

        \return True if the value was successfully sampled.
    */
    virtual bool
    Sample(unsigned int element, float u, float v, void* value, HdTupleType dataType) const = 0;

    //! Convenient, templated frontend for Sample().
    template <typename T> bool Sample(unsigned int element, float u, float v, T* value) const
    {
        return Sample(element, u, v, static_cast<void*>(value), HdVP2TypeHelper::GetTupleType<T>());
    }

protected:
    static bool _Interpolate(
        void*       out,
        void**      samples,
        float*      weights,
        size_t      sampleCount,
        HdTupleType dataType);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_SAMPLER_H
