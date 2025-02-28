//
// Copyright 2025 Autodesk
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
#ifndef ASDK_PLUGIN_DRAWUTILS_H
#define ASDK_PLUGIN_DRAWUTILS_H

#include "base/api.h"

#include <mayaUsd/base/api.h>

#include <maya/MFloatVectorArray.h>
#include <maya/MUintArray.h>

#include <iostream>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Base class used to generate geometry shapes
class MAYAUSD_PLUGIN_PUBLIC PrimitiveData
{
protected:
    PrimitiveData() { }

public:
    virtual ~PrimitiveData() { }
    MFloatVectorArray wirePositions;
    MUintArray        wireIndexing;
};

//! Generates geometry data for drawing a line
class MAYAUSD_PLUGIN_PUBLIC LinePrimitive : public PrimitiveData
{
public:
    LinePrimitive(float length = 1, unsigned int up = 2);
};

//! Generates geometry data that draw a line and a triangle at the end to form an arrow
class MAYAUSD_PLUGIN_PUBLIC ArrowPrimitive : public PrimitiveData
{
public:
    ArrowPrimitive(double offset[3], float length = 2);
};

//! Generates a circle given the radius.
class MAYAUSD_PLUGIN_PUBLIC DiskPrimitive : public PrimitiveData
{
public:
    //! Constructor to specify the Disk parameters
    //! \param[in] offset specifies the offset from the point of origin
    //! \param[in] radius specifies the radius of the circle
    //! \param[in] circleOnly When true, it only generates the data for a circle. When false, 
    //! is also generates lines connecting the circle to the center.
    //! \param[in] resolution specifies the number of edges generated when forming the circle.
    //! \note resolution also impacts the number of lines drawn connecting the circle to its center
    DiskPrimitive(
        double       offset[3],
        float        radius = 1,
        bool         circleOnly = false,
        unsigned int resolution = 20);
};

//! Generates a sphere given the radius.
//! \param[in] radius specifies the radius of the circle
//! \param[in] resolution specifies the complexity of the sphere geometry data
//! \param[in] scale specifies the scaling
//! \param[in] offset specifies the offset from the center of the sphere
//! \note It's best not to specify changing scaling. Maya by default handles scaling / transform
//! interactions which avoids re-generating the geometry data.
class MAYAUSD_PLUGIN_PUBLIC SpherePrimitive : public PrimitiveData
{
public:
    SpherePrimitive(float radius, unsigned int resolution, double scale[3], double offset[3]);
};

//! Generates a cylinder given the radius and the height
//! \param[in] radius specifies the radius of the cylinder
//! \param[in] height specified the height of the cylinder
//! \param[in] resolution specifies the complexity of the sphere geometry data
class MAYAUSD_PLUGIN_PUBLIC CylinderPrimitive : public PrimitiveData
{
public:
    CylinderPrimitive(float radius = 1.0f, float height = 1.0f, unsigned int resolution = 20);
};

//! Generates a cone and its penumbra given the cone / penumbra angles.
//! \param[in] height specifies the distance of the circles from the local origin
//! \param[in] coneAngle specifies the cone angle in degrees
//! \param[in] showPenumbra if true, a secondary circle is shown with the angle set in penumbraAngle
//! \param[in] penumbraAngle specifies the penumbra angle in degrees
//! \param[in] resolution specifies the complexity of the cone geometry data
class MAYAUSD_PLUGIN_PUBLIC ConePrimitive : public PrimitiveData
{
public:
    ConePrimitive(
        float        height = 1.0f,
        float        coneAngle = 75.0f,
        bool         showPenumbra = true,
        float        penumbraAngle = 75.0f,
        unsigned int resolution = 20);
};

//! Generates a quad shape.
//! \param[in] scale specifies the scale
class MAYAUSD_PLUGIN_PUBLIC QuadPrimitive : public PrimitiveData
{
public:
    QuadPrimitive(double scale[3]);
};

} // namespace MAYAUSD_NS_DEF

#endif // ASDK_PLUGIN_DRAWUTILS_H