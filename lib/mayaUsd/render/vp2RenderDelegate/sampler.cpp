//
// Copyright 2017 Pixar
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
#include "sampler.h"

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  The constructor takes a reference to a buffer source. The data is
            owned externally;

    The caller is responsible for ensuring the buffer is alive while Sample() is being called.

    \param buffer The buffer being sampled.
*/
HdVP2BufferSampler::HdVP2BufferSampler(HdVtBufferSource const& buffer)
    : _buffer(buffer)
{
}

/*! \brief  Sample the buffer at given element index

    Sample the buffer at element index \p index, and write the sample to
    \p value.Interpret \p value as having arity \p numComponents, each of
    type \p componentType.These parameters may not match the datatype
    declaration of the underlying buffer, in which case Sample returns
    false.Sample also returns false if \p index is out of bounds.

    For example, to sample data as GfVec3, \p dataType would be
    HdTupleType{ HdTypeFloatVec3, 1 }.

    \param index The element index to sample.
    \param value The memory to write the value to(only written on success).
    \param dataType The HdTupleType describing element values.

    \return True if the value was successfully sampled.
*/
bool HdVP2BufferSampler::Sample(int index, void* value, HdTupleType dataType) const
{
    // Sanity checks: index is within the bounds of buffer,
    // and the sample type and buffer type (defined by the dataType)
    // are the same.
    if (_buffer.GetNumElements() <= (size_t)index || _buffer.GetTupleType() != dataType) {
        return false;
    }

    // Calculate the element's byte offset in the array.
    size_t elemSize = HdDataSizeOfTupleType(dataType);
    size_t offset = elemSize * index;

    // Equivalent to:
    // *static_cast<ElementType*>(value) =
    //     static_cast<ElementType*>(_buffer.GetData())[index];
    memcpy(value, static_cast<const uint8_t*>(_buffer.GetData()) + offset, elemSize);

    return true;
}

template <typename T>
static void
_InterpolateImpl(void* out, void** samples, float* weights, size_t sampleCount, short numComponents)
{
    // This is an implementation of a general blend of samples:
    // out = sum_j { sample[j] * weights[j] }.
    // Since the vector length comes in as a parameter, and not part
    // of the type, the blend is implemented per component.
    for (short i = 0; i < numComponents; ++i) {
        static_cast<T*>(out)[i] = 0;
        for (size_t j = 0; j < sampleCount; ++j) {
            static_cast<T*>(out)[i] += static_cast<T*>(samples[j])[i] * weights[j];
        }
    }
}

/*! \brief  Utility function for derived classes: combine multiple samples with
            blend weights: \p out = sum_i { \p samples[i] * \p weights[i] }.

    \param out The memory to write the output to (only written on success).
    \param samples The array of sample pointers (length \p sampleCount).
    \param weights The array of sample weights (length \p sampleCount).
    \param sampleCount The number of samples to combine.
    \param dataType The HdTupleType describing element values.

    \return True if the samples were successfully combined.
*/
bool HdVP2PrimvarSampler::_Interpolate(
    void*       out,
    void**      samples,
    float*      weights,
    size_t      sampleCount,
    HdTupleType dataType)
{
    // Combine maps from component type tag to C++ type, and delegates to
    // the templated _InterpolateImpl.

    // Combine number of components in the underlying type and tuple arity.
    short numComponents = HdGetComponentCount(dataType.type) * dataType.count;

    HdType componentType = HdGetComponentType(dataType.type);

    switch (componentType) {
    case HdTypeBool:
        /* This function isn't meaningful on boolean types. */
        return false;
    case HdTypeInt8: _InterpolateImpl<char>(out, samples, weights, sampleCount, numComponents);
    case HdTypeInt16:
        _InterpolateImpl<short>(out, samples, weights, sampleCount, numComponents);
        return true;
    case HdTypeUInt16:
        _InterpolateImpl<unsigned short>(out, samples, weights, sampleCount, numComponents);
        return true;
    case HdTypeInt32:
        _InterpolateImpl<int>(out, samples, weights, sampleCount, numComponents);
        return true;
    case HdTypeUInt32:
        _InterpolateImpl<unsigned int>(out, samples, weights, sampleCount, numComponents);
        return true;
    case HdTypeFloat:
        _InterpolateImpl<float>(out, samples, weights, sampleCount, numComponents);
        return true;
    case HdTypeDouble:
        _InterpolateImpl<double>(out, samples, weights, sampleCount, numComponents);
        return true;
    default:
        TF_CODING_ERROR(
            "Unsupported type '%s' passed to _Interpolate", TfEnum::GetName(componentType).c_str());
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
