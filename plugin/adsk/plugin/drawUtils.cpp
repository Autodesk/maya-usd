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
#include "drawUtils.h"

#include <cstring> // memcpy

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PIover2
#define M_PIover2 1.57079632679489661923
#endif

#ifndef M_PItimes2
#define M_PItimes2 6.28318530717958647692
#endif

#ifndef degToRad
#define degToRad(degrees) (degrees * M_PI / 180.0)
#endif

namespace MAYAUSD_NS_DEF {

LinePrimitive::LinePrimitive(float length /*= 1*/, unsigned int up /*= 2*/)
{
    static float l_vertices[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    switch (up) {
    case 0: l_vertices[3] = length * -1.0f; break;
    case 1: l_vertices[4] = length * -1.0f; break;
    case 2:
    default: l_vertices[5] = length * -1.0f; break;
    }

    static const unsigned int l_indices[] = { 0, 1 };

    wirePositions.setLength(2);
    for (unsigned int i = 0; i < 2; i++) {
        wirePositions[i] = MPoint(l_vertices[i * 3], l_vertices[i * 3 + 1], l_vertices[i * 3 + 2]);
    }

    wireIndexing.setLength(2);

    memcpy(&wireIndexing[0], &l_indices[0], sizeof(unsigned int) * 2);
}

ArrowPrimitive::ArrowPrimitive(double offset[3], float length /*= 2*/)
{
    MPoint offsetPoint(offset[0], offset[1], offset[2]);

    MPoint lineStart(0.0f, 0.0f, 0.0f);
    MPoint lineEnd(0.0f, 0.0f, length * -1.0f);
    MPoint Arrow1(0.0f, 0.0625f, length * -1.0f);
    MPoint Arrow2(0.0f, -0.0625f, length * -1.0f);
    MPoint Arrow3(0.0f, 0.0f, length * -1.25f);

    static const unsigned int l_indices[] = { 0, 1, 2, 3, 3, 4, 4, 2 };

    wirePositions.append(lineStart + offsetPoint);
    wirePositions.append(lineEnd + offsetPoint);
    wirePositions.append(Arrow1 + offsetPoint);
    wirePositions.append(Arrow2 + offsetPoint);
    wirePositions.append(Arrow3 + offsetPoint);

    wireIndexing.setLength(8);

    memcpy(&wireIndexing[0], &l_indices[0], sizeof(unsigned int) * 8);
}

DiskPrimitive::DiskPrimitive(
    double       offset[3],
    float        radius /*= 1*/,
    bool         circleOnly /*= false*/,
    unsigned int resolution /*= 20*/)
{
    if (circleOnly) {
        wirePositions.setLength(resolution);
        for (unsigned int i = 0; i < resolution; ++i) {
            const float d = M_PItimes2 * (float(i) / float(resolution));
            wirePositions[i] = MPoint(
                offset[0] + radius * cosf(d), offset[1] + radius * sinf(d), offset[2] + 0.0f);
        }

        for (unsigned int i = 0; i < wirePositions.length() - 1; ++i) {
            wireIndexing.append(i);
            wireIndexing.append(i + 1);
        }
        wireIndexing.append(wirePositions.length() - 1);
        wireIndexing.append(0);
    } else {
        wirePositions.setLength(resolution + 1);
        wirePositions[0] = MPoint(0.0, 0.0, 0.0);
        for (unsigned int i = 0; i < resolution; ++i) {
            const float d = M_PItimes2 * (float(i) / float(resolution));
            wirePositions[(i + 1)] = MPoint(
                offset[0] + radius * cosf(d), offset[1] + radius * sinf(d), offset[2] + 0.0f);
        }
        wireIndexing.setLength(resolution * 4);
        const unsigned int res2 = resolution * 2;
        for (unsigned int i = 0; i < resolution; ++i) {
            const unsigned int i2 = i * 2;
            wireIndexing[i2] = 0;
            wireIndexing[i2 + 1] = i + 1;
            wireIndexing[i2 + res2] = i + 1;
            wireIndexing[i2 + res2 + 1] = (i + 1) % resolution + 1;
        }
    }
}

QuadPrimitive::QuadPrimitive(double scale[3])
{
    static const float l_vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f,
    };

    static const unsigned int l_indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0, 0, 2, 1, 3,
    };

    wirePositions.setLength(4);
    for (unsigned int i = 0; i < 4; i++) {
        wirePositions[i] = MPoint(
            l_vertices[i * 3] * scale[0],
            l_vertices[i * 3 + 1] * scale[1],
            l_vertices[i * 3 + 2] * scale[2]);
    }
    wireIndexing.setLength(12);

    memcpy(&wireIndexing[0], &l_indices[0], sizeof(unsigned int) * 12);
}

SpherePrimitive::SpherePrimitive(
    float        radius,
    unsigned int resolution,
    double       scale[3],
    double       offset[3])
    : PrimitiveData()
{
    wirePositions.setLength((resolution * resolution + 2) * 2);
    wirePositions[0] = MPoint(0.0, -radius, 0.0);
    wirePositions[1] = MPoint(0.0, radius, 0.0);

    unsigned int vid = 2;
    for (unsigned int yy = 0; yy < resolution; ++yy) {
        const double dy = M_PI * float(yy) / float(resolution) - M_PIover2;
        const double y = sin(dy) * radius;
        const double pr = cos(dy) * radius;
        for (unsigned int xx = 0; xx < resolution; ++xx) {
            const double dx = M_PItimes2 * double(xx) / double(resolution);
            wirePositions[vid++] = MPoint(cos(dx) * pr, y, sin(dx) * pr);
        }
    }

    wireIndexing.setLength(
        resolution * resolution * 2 +       // for horizontal lines
        (resolution + 1) * resolution * 2); // for vertical lines
    // fill horizontal lines
    unsigned int id = 0;
    for (unsigned int yy = 0; yy < resolution; ++yy) {
        const unsigned int wy = 2 + yy * resolution;
        for (unsigned int xx = 0; xx < resolution; ++xx) {
            wireIndexing[id++] = wy + xx;
            wireIndexing[id++] = wy + (xx + 1) % resolution;
        }
    }

    for (unsigned int xx = 0; xx < resolution; ++xx) {
        const unsigned int xx2 = 2 + xx;
        wireIndexing[id++] = 0;
        wireIndexing[id++] = xx2;
        for (unsigned int yy = 0; yy < (resolution - 1); ++yy) {
            wireIndexing[id++] = xx2 + yy * resolution;
            wireIndexing[id++] = xx2 + (yy + 1) * resolution;
        }
        wireIndexing[id++] = xx2 + (resolution - 1) * resolution;
        wireIndexing[id++] = 1;
    }
}

CylinderPrimitive::CylinderPrimitive(float radius, float height, unsigned int resolution /*= 20*/)
{
    wirePositions.setLength(resolution * 2);
    const unsigned int indexDiff = resolution;
    for (unsigned int i = 0; i < resolution; ++i) {
        const float d = M_PItimes2 * (float(i) / float(resolution));
        const float x = cosf(d) * radius;
        const float z = sinf(d) * radius;
        wirePositions[i] = MPoint(height, x, z);
        wirePositions[i + indexDiff] = MPoint(-height, x, z);
    }
    wireIndexing.setLength(resolution * 6);
    const unsigned int res2 = resolution * 2;
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int i2 = i * 2;
        const unsigned int i1 = (i + 1) % resolution;
        wireIndexing[i2] = i;
        wireIndexing[i2 + 1] = i1;
        wireIndexing[i2 + res2] = i + resolution;
        wireIndexing[i2 + res2 + 1] = i1 + resolution;
        const unsigned int i2o = i2 + resolution * 4;
        wireIndexing[i2o] = i;
        wireIndexing[i2o + 1] = i + resolution;
    }
}

ConePrimitive::ConePrimitive(
    float        height /*= 1.0f*/,
    float        coneAngle /*= 75.0f*/,
    bool         showPenumbra /*= true*/,
    float        penumbraAngle /*= 75.0f*/,
    unsigned int resolution /*= 20*/)
{
    double circleOffset[3] = { 0.0, 0.0, -height };

    // Cone circle
    // Note: In USD, the cone angle is the angle from the center axis to the edge of the cone.
    // In Maya, the cone angle is the full angle of the cone.
    const float   coneAngleInRadians = degToRad(coneAngle / 2.0f);
    const float   coneRadius = height * tan(coneAngleInRadians);
    DiskPrimitive coneDisk(circleOffset, coneRadius, true);
    wirePositions = coneDisk.wirePositions;
    wireIndexing = coneDisk.wireIndexing;
    int lastVertexIndex = wirePositions.length();

    // Penumbra circle
    if (showPenumbra) {
        const float   penumbraAngleInRadians = degToRad(penumbraAngle / 2.0f);
        const float   penumbraRadius = height * tan(penumbraAngleInRadians);
        DiskPrimitive penumbraDisk(circleOffset, penumbraRadius, true);

        for (unsigned int i = 0; i < penumbraDisk.wirePositions.length(); i++) {
            wirePositions.append(penumbraDisk.wirePositions[i]);
        }
        for (unsigned int i = 0; i < penumbraDisk.wireIndexing.length(); i++) {
            wireIndexing.append(lastVertexIndex + penumbraDisk.wireIndexing[i]);
        }
    }

    // Arrow
    double         lineOffset[3] = { 0.0, 0.0, 0.0 };
    ArrowPrimitive arrow(lineOffset, 0.5f + height);

    lastVertexIndex = wirePositions.length();
    for (unsigned int i = 0; i < arrow.wirePositions.length(); i++) {
        wirePositions.append(arrow.wirePositions[i]);
    }
    for (unsigned int i = 0; i < arrow.wireIndexing.length(); i++) {
        wireIndexing.append(lastVertexIndex + arrow.wireIndexing[i]);
    }

    // Lines going from the cone tip to the cone circle
    const MPoint coneTip(0.0f, 0.0f, 0.0f);
    lastVertexIndex = wirePositions.length();
    unsigned int numberOfLines = 4;
    for (unsigned int i = 0; i < numberOfLines; ++i) {
        wirePositions.append(coneTip);
        wireIndexing.append(lastVertexIndex++);
        const float d = M_PItimes2 * (float(i) / (float)numberOfLines);
        wirePositions.append(MPoint(
            circleOffset[0] + coneRadius * cosf(d),
            circleOffset[1] + coneRadius * sinf(d),
            circleOffset[2] + 0.0f));
        wireIndexing.append(lastVertexIndex++);
    }
}

} // namespace MAYAUSD_NS_DEF